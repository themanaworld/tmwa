//! WebSocket transport: binary frames carry the client byte stream.
//! Browser/game clients connect to `http.ws_path`, are upgraded, and
//! then run the ordinary client handler (login/char/map relay).

use std::net::{IpAddr, Ipv4Addr};
use std::sync::Arc;

use axum::extract::ws::{Message, WebSocket, WebSocketUpgrade};
use axum::extract::{ConnectInfo, State as AxState};
use axum::http::HeaderMap;
use axum::response::Response;
use futures_util::{SinkExt, Stream};

use crate::net::framing::PacketFramer;

/// Decrements the connection count on drop — panic-safe.
struct ConnGuard(Arc<super::state::State>);

impl ConnGuard {
    fn new(st: Arc<super::state::State>) -> Self {
        st.conn_inc();
        ConnGuard(st)
    }
}

impl Drop for ConnGuard {
    fn drop(&mut self) {
        self.0.conn_dec();
    }
}

/// Read half of the WebSocket as an AsyncRead of concatenated binary
/// payloads (frame boundaries are meaningless to the protocol).
struct WsRead {
    inner: futures_util::stream::SplitStream<WebSocket>,
    buf: bytes::BytesMut,
    closed: bool,
}

impl tokio::io::AsyncRead for WsRead {
    fn poll_read(
        mut self: std::pin::Pin<&mut Self>,
        cx: &mut std::task::Context<'_>,
        buf: &mut tokio::io::ReadBuf<'_>,
    ) -> std::task::Poll<std::io::Result<()>> {
        use bytes::Buf;
        loop {
            if !self.buf.is_empty() {
                let n = buf.remaining().min(self.buf.len());
                buf.put_slice(&self.buf[..n]);
                self.buf.advance(n);
                return std::task::Poll::Ready(Ok(()));
            }
            if self.closed {
                return std::task::Poll::Ready(Ok(()));
            }
            match futures_util::ready!(std::pin::Pin::new(&mut self.inner).poll_next(cx)) {
                Some(Ok(Message::Binary(d))) => self.buf.extend_from_slice(&d),
                Some(Ok(Message::Text(_))) => {
                    // protocol is binary-only; close on text frames
                    self.closed = true;
                    return std::task::Poll::Ready(Err(std::io::Error::new(
                        std::io::ErrorKind::InvalidData,
                        "text frame",
                    )));
                }
                Some(Ok(Message::Close(_))) | None => {
                    self.closed = true;
                }
                Some(Ok(_)) => continue,
                Some(Err(e)) => {
                    return std::task::Poll::Ready(Err(std::io::Error::other(e.to_string())));
                }
            }
        }
    }
}

pub async fn handle_ws(
    AxState(hs): AxState<Arc<super::http::HttpState>>,
    ConnectInfo(peer): ConnectInfo<std::net::SocketAddr>,
    headers: HeaderMap,
    ws: WebSocketUpgrade,
) -> Response {
    // real client IP behind a trusted proxy; IPv6 gets a stable
    // 240/4 pseudo address (see net::map_ip)
    let real = crate::net::forwarded_for(
        peer.ip(),
        headers.get("x-forwarded-for").and_then(|v| v.to_str().ok()),
        &hs.st.cfg.http.trusted_proxies,
    );
    if !matches!(real, IpAddr::V4(_)) {
        tracing::info!("ws client from {real} -> {}", crate::net::map_ip(real));
    }
    let ip: Ipv4Addr = crate::net::map_ip(real);

    // global connection cap across TCP + WS
    if hs.st.conn_count() >= hs.st.cfg.http.max_connections {
        return ws
            .protocols(["binary"])
            .max_message_size(64 * 1024)
            .max_frame_size(64 * 1024)
            .on_upgrade(move |mut s: WebSocket| async move {
                let _ = s
                    .send(Message::Close(Some(axum::extract::ws::CloseFrame {
                        code: 1013,
                        reason: "server busy".into(),
                    })))
                    .await;
            });
    }

    ws.protocols(["binary"])
        .max_message_size(64 * 1024)
        .max_frame_size(64 * 1024)
        .on_upgrade(move |sock| async move {
            let _guard = ConnGuard::new(hs.st.clone());
            let (mut sink, stream) = futures_util::StreamExt::split(sock);
            // One bounded queue per session: the client core pushes
            // whole packets (try_send, so a full queue drops the
            // packet like the TCP writer does), and this task turns
            // them into binary frames. The channel ending (session
            // over) makes the task send a clean Close(1000) rather
            // than an abrupt 1005.
            let (tx, mut rx) = tokio::sync::mpsc::channel::<Vec<u8>>(256);
            tokio::spawn(async move {
                while let Some(buf) = rx.recv().await {
                    if sink.send(Message::Binary(buf.into())).await.is_err() {
                        return;
                    }
                }
                let _ = sink
                    .send(Message::Close(Some(axum::extract::ws::CloseFrame {
                        code: 1000,
                        reason: "".into(),
                    })))
                    .await;
                let _ = sink.close().await;
            });
            let fr = PacketFramer::new(WsRead {
                inner: stream,
                buf: bytes::BytesMut::new(),
                closed: false,
            });
            // browsers can't open raw TCP sockets: WS clients keep
            // the gate-as-relay map stage
            let ip = u32::from_le_bytes(ip.octets());
            super::client::run_core(hs.st.clone(), fr, tx, ip, true).await;
        })
}
