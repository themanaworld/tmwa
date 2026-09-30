//! Async packet framer. Works over any `AsyncRead`, so the same code can
//! frame TCP and WebSocket byte streams.
//!
//! Framing rules come from the generated `packet_len` table. An unknown
//! packet id is an error; the caller is expected to drop the connection.

use crate::proto::{self, PacketLen};
use tokio::io::{AsyncRead, AsyncReadExt};

/// One complete packet: the wire bytes including the 2-byte id.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Packet {
    pub id: u16,
    pub bytes: Vec<u8>,
}

#[derive(Debug, thiserror::Error)]
pub enum FrameError {
    #[error("unknown packet id 0x{0:04x}")]
    UnknownId(u16),
    #[error("io: {0}")]
    Io(#[from] std::io::Error),
    #[error("connection closed mid-packet")]
    Truncated,
}

/// Stateful framer; feed it an `AsyncRead` and pull complete packets out.
pub struct PacketFramer<R> {
    reader: R,
    buf: Vec<u8>,
}

impl<R: AsyncRead + Unpin> PacketFramer<R> {
    pub fn new(reader: R) -> Self {
        PacketFramer {
            reader,
            buf: Vec::with_capacity(8192),
        }
    }

    /// Read access to the wrapped reader (e.g. to check the WebSocket
    /// close code the transport recorded).
    pub fn reader(&self) -> &R {
        &self.reader
    }

    /// Mutable access for the same purpose.
    pub fn reader_mut(&mut self) -> &mut R {
        &mut self.reader
    }

    /// Buffered bytes not yet consumed (for tests).
    #[allow(dead_code)]
    pub fn buffered(&self) -> usize {
        self.buf.len()
    }

    /// Returns the next complete packet, or `Ok(None)` on a clean EOF
    /// with nothing buffered.
    pub async fn next(&mut self) -> Result<Option<Packet>, FrameError> {
        loop {
            if let Some(p) = self.try_take()? {
                return Ok(Some(p));
            }
            let mut tmp = [0u8; 8192];
            let n = self.reader.read(&mut tmp).await?;
            if n == 0 {
                if self.buf.is_empty() {
                    return Ok(None);
                }
                return Err(FrameError::Truncated);
            }
            self.buf.extend_from_slice(&tmp[..n]);
        }
    }

    fn try_take(&mut self) -> Result<Option<Packet>, FrameError> {
        if self.buf.len() < 2 {
            return Ok(None);
        }
        let id = u16::from_le_bytes([self.buf[0], self.buf[1]]);
        let need = match proto::packet_len(id) {
            Some(PacketLen::Fixed(n)) => n,
            Some(PacketLen::Variable { offset, skew, wide }) => {
                let want = offset + if wide { 4 } else { 2 };
                if self.buf.len() < want {
                    return Ok(None);
                }
                let raw = if wide {
                    u32::from_le_bytes(self.buf[offset..offset + 4].try_into().unwrap())
                } else {
                    u16::from_le_bytes(self.buf[offset..offset + 2].try_into().unwrap()) as u32
                };
                raw as usize + skew
            }
            None => return Err(FrameError::UnknownId(id)),
        };
        if self.buf.len() < need {
            return Ok(None);
        }
        let bytes: Vec<u8> = self.buf.drain(..need).collect();
        Ok(Some(Packet { id, bytes }))
    }
}
