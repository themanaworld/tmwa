//! PacketFramer tests: split/coalesced input, 0x8000, unknown ids.

use std::io;
use std::pin::Pin;
use std::task::{Context, Poll};
use tmwa_gate::net::framing::{FrameError, PacketFramer};
use tmwa_gate::proto::types::*;
use tmwa_gate::proto::*;

/// AsyncRead that yields at most `chunk` bytes per poll, to exercise
/// packets arriving split over several reads.
struct ChunkReader<'a> {
    data: &'a [u8],
    chunk: usize,
}

impl<'a> ChunkReader<'a> {
    fn new(data: &'a [u8], chunk: usize) -> Self {
        ChunkReader { data, chunk }
    }
}

impl tokio::io::AsyncRead for ChunkReader<'_> {
    fn poll_read(
        mut self: Pin<&mut Self>,
        _cx: &mut Context<'_>,
        buf: &mut tokio::io::ReadBuf<'_>,
    ) -> Poll<io::Result<()>> {
        let n = self.data.len().min(self.chunk).min(buf.remaining());
        buf.put_slice(&self.data[..n]);
        self.data = &self.data[n..];
        Poll::Ready(Ok(()))
    }
}

fn login_packet() -> Vec<u8> {
    let mut p = P0064::default();
    p.account_name = FixedStr::<24>::try_from_str("u").unwrap();
    let mut v = Vec::new();
    p.encode(&mut v);
    v
}

#[tokio::test]
async fn split_and_coalesced() {
    let p1 = login_packet();
    let mut p2 = Vec::new();
    P007D::default().encode(&mut p2);
    let mut wire = Vec::new();
    wire.extend_from_slice(&p1);
    wire.extend_from_slice(&p2);

    // split reads: 3-byte chunks
    let mut f = PacketFramer::new(ChunkReader::new(&wire, 3));
    let a = f.next().await.unwrap().unwrap();
    assert_eq!(a.id, 0x0064);
    assert_eq!(a.bytes, p1);
    let b = f.next().await.unwrap().unwrap();
    assert_eq!(b.id, 0x007d);
    assert!(f.next().await.unwrap().is_none());

    // coalesced: everything in one buffer
    let mut f = PacketFramer::new(ChunkReader::new(&wire, 65536));
    let a = f.next().await.unwrap().unwrap();
    assert_eq!(a.id, 0x0064);
    let b = f.next().await.unwrap().unwrap();
    assert_eq!(b.id, 0x007d);
}

#[tokio::test]
async fn variable_length_packet() {
    let mut p = P0063::default();
    p.repeat.push(P0063Repeat { c: 1 });
    p.repeat.push(P0063Repeat { c: 2 });
    p.repeat.push(P0063Repeat { c: 3 });
    let mut v = Vec::new();
    p.encode(&mut v);
    let mut f = PacketFramer::new(ChunkReader::new(&v, 2));
    let got = f.next().await.unwrap().unwrap();
    assert_eq!(got.id, 0x0063);
    assert_eq!(got.bytes, v);
}

#[tokio::test]
async fn special_0x8000() {
    // tmwa emits 0x8000 as [0x00, 0x80, 0x04, 0x00]
    let wire = [0x00u8, 0x80, 0x04, 0x00];
    let mut f = PacketFramer::new(ChunkReader::new(&wire, 2));
    let p = f.next().await.unwrap().unwrap();
    assert_eq!(p.id, 0x8000);
    assert_eq!(p.bytes.len(), 4);
}

#[tokio::test]
async fn unknown_id_is_error() {
    let wire = [0x01u8, 0xde, 0x00, 0x00];
    let mut f = PacketFramer::new(ChunkReader::new(&wire, 4));
    match f.next().await {
        Err(FrameError::UnknownId(0xde01)) => {}
        other => panic!("expected UnknownId, got {other:?}"),
    }
}

#[tokio::test]
async fn skewed_length() {
    // 0x794e: u32 at offset 4 holds total - 8
    let mut wire = vec![0x4eu8, 0x79, 0, 0, 2, 0, 0, 0];
    wire.extend_from_slice(b"hi");
    let mut f = PacketFramer::new(ChunkReader::new(&wire, 5));
    let p = f.next().await.unwrap().unwrap();
    assert_eq!(p.id, 0x794e);
    assert_eq!(p.bytes.len(), 10);
}

#[tokio::test]
async fn truncated_errors() {
    let wire = [0x64u8, 0x00, 0x01];
    let mut f = PacketFramer::new(ChunkReader::new(&wire, 64));
    match f.next().await {
        Err(FrameError::Truncated) => {}
        other => panic!("expected Truncated, got {other:?}"),
    }
}
