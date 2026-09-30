//! WebSocket transport: binary frames carry the client byte stream.
//! Browser/game clients connect to `http.ws_path`, are upgraded, and
//! then run the ordinary client handler (login/char/map relay).

use std::net::{IpAddr, Ipv4Addr};
use std::sync::Arc;

use axum::extract::ws::{Message, WebSocket, WebSocketUpgrade};
use axum::extract::{ConnectInfo, State as AxState};
use axum::http::HeaderMap;
use axum::response::Response;
use futures_util::{Sink, Stream};

/// Adapt an axum WebSocket to AsyncRead+AsyncWrite carrying binary
/// payloads (frame boundaries are meaningless to the protocol).
struct WsStream {
    inner: WebSocket,
    buf: bytes::BytesMut,
    closed: bool,
}

impl tokio::io::AsyncRead for WsStream {
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

impl tokio::io::AsyncWrite for WsStream {
    fn poll_write(
        mut self: std::pin::Pin<&mut Self>,
        cx: &mut std::task::Context<'_>,
        buf: &[u8],
    ) -> std::task::Poll<std::io::Result<usize>> {
        match std::pin::Pin::new(&mut self.inner).poll_ready(cx) {
            std::task::Poll::Ready(Ok(())) => {
                match std::pin::Pin::new(&mut self.inner)
                    .start_send(Message::Binary(buf.to_vec().into()))
                {
                    Ok(()) => std::task::Poll::Ready(Ok(buf.len())),
                    Err(e) => std::task::Poll::Ready(Err(std::io::Error::other(e.to_string()))),
                }
            }
            std::task::Poll::Ready(Err(e)) => {
                std::task::Poll::Ready(Err(std::io::Error::other(e.to_string())))
            }
            std::task::Poll::Pending => std::task::Poll::Pending,
        }
    }
    fn poll_flush(
        mut self: std::pin::Pin<&mut Self>,
        cx: &mut std::task::Context<'_>,
    ) -> std::task::Poll<std::io::Result<()>> {
        std::pin::Pin::new(&mut self.inner)
            .poll_flush(cx)
            .map_err(|e| std::io::Error::other(e.to_string()))
    }
    fn poll_shutdown(
        mut self: std::pin::Pin<&mut Self>,
        cx: &mut std::task::Context<'_>,
    ) -> std::task::Poll<std::io::Result<()>> {
        std::pin::Pin::new(&mut self.inner)
            .poll_close(cx)
            .map_err(|e| std::io::Error::other(e.to_string()))
    }
}

pub async fn handle_ws(
    AxState(hs): AxState<Arc<super::http::HttpState>>,
    ConnectInfo(peer): ConnectInfo<std::net::SocketAddr>,
    headers: HeaderMap,
    ws: WebSocketUpgrade,
) -> Response {
    // real client IP behind a trusted proxy
    let ip: Ipv4Addr = {
        let mut ip = match peer.ip() {
            IpAddr::V4(v) => v,
            IpAddr::V6(v6) => v6.to_ipv4().unwrap_or(Ipv4Addr::LOCALHOST),
        };
        if hs
            .st
            .cfg
            .http
            .trusted_proxies
            .iter()
            .any(|p| p.parse::<IpAddr>().map(|t| t == peer.ip()).unwrap_or(false))
        {
            if let Some(xff) = headers.get("x-forwarded-for") {
                if let Ok(s) = xff.to_str() {
                    if let Some(first) = s.split(',').next() {
                        if let Ok(IpAddr::V4(v)) = first.trim().parse() {
                            ip = v;
                        }
                    }
                }
            }
        }
        ip
    };

    // global connection cap across TCP + WS
    if hs.st.conn_count() >= hs.st.cfg.http.max_connections {
        return ws
            .protocols(["binary"])
            .on_upgrade(move |mut s: WebSocket| async move {
                let _ = s
                    .send(Message::Close(Some(axum::extract::ws::CloseFrame {
                        code: 1013,
                        reason: "server busy".into(),
                    })))
                    .await;
            });
    }

    ws.protocols(["binary"]).on_upgrade(move |sock| async move {
        hs.st.conn_inc();
        let stream = WsStream {
            inner: sock,
            buf: bytes::BytesMut::new(),
            closed: false,
        };
        super::client::run(hs.st.clone(), stream, ip).await;
        hs.st.conn_dec();
    })
}
