//! Hand-written wire types referenced by the generated packet code.

use crate::proto::Dir;

/// Error returned when decoding a packet or wire value fails.
#[derive(Debug, Clone, Copy, PartialEq, Eq, thiserror::Error)]
pub enum DecodeError {
    #[error("not enough data")]
    Short,
    #[error("wrong packet size")]
    Size,
    #[error("unknown packet id 0x{0:04x}")]
    UnknownId(u16),
}

/// A value with an exact wire representation.
///
/// Implementations write/read exactly `LEN` bytes. `buf` passed to
/// `wire_decode` may be longer; at least `LEN` bytes must be available.
pub trait Wire: Sized {
    const LEN: usize;
    fn wire_encode(&self, out: &mut [u8]);
    fn wire_decode(buf: &[u8]) -> Result<Self, DecodeError>;
}

macro_rules! int_wire {
    ($($t:ty),* $(,)?) => {$(
        impl Wire for $t {
            const LEN: usize = core::mem::size_of::<$t>();
            fn wire_encode(&self, out: &mut [u8]) {
                out[..Self::LEN].copy_from_slice(&self.to_le_bytes());
            }
            fn wire_decode(buf: &[u8]) -> Result<Self, DecodeError> {
                if buf.len() < Self::LEN {
                    return Err(DecodeError::Short);
                }
                Ok(Self::from_le_bytes(buf[..Self::LEN].try_into().unwrap()))
            }
        }
    )*}
}

int_wire!(u8, u16, u32, u64, i8, i16, i32, i64);

impl<T: Wire, const N: usize> Wire for [T; N] {
    const LEN: usize = N * T::LEN;
    fn wire_encode(&self, out: &mut [u8]) {
        debug_assert!(out.len() >= Self::LEN);
        for (i, e) in self.iter().enumerate() {
            e.wire_encode(&mut out[i * T::LEN..(i + 1) * T::LEN]);
        }
    }
    fn wire_decode(buf: &[u8]) -> Result<Self, DecodeError> {
        if buf.len() < Self::LEN {
            return Err(DecodeError::Short);
        }
        let mut v: Vec<T> = Vec::with_capacity(N);
        for i in 0..N {
            v.push(T::wire_decode(&buf[i * T::LEN..(i + 1) * T::LEN])?);
        }
        Ok(v.try_into().ok().expect("array length"))
    }
}

/// Fixed-length, NUL-padded wire string.
///
/// `VString<15>` and friends in C++ are `N + 1` bytes on the wire; this
/// type stores the whole wire buffer so decoding is lossless.
#[derive(Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash)]
#[repr(transparent)]
pub struct FixedStr<const N: usize>(pub [u8; N]);

impl<const N: usize> FixedStr<N> {
    /// The bytes up to the first NUL (the meaningful content).
    pub fn as_bytes(&self) -> &[u8] {
        let end = self.0.iter().position(|&b| b == 0).unwrap_or(N);
        &self.0[..end]
    }

    #[allow(clippy::wrong_self_convention)]
    pub fn to_string_lossy(&self) -> String {
        String::from_utf8_lossy(self.as_bytes()).into_owned()
    }

    /// Build from a string; fails when it does not fit (including the
    /// terminating NUL the C++ code reserves).
    pub fn try_from_str(s: &str) -> Result<Self, DecodeError> {
        if s.len() >= N {
            return Err(DecodeError::Size);
        }
        let mut a = [0u8; N];
        a[..s.len()].copy_from_slice(s.as_bytes());
        Ok(Self(a))
    }
}

impl<const N: usize> Default for FixedStr<N> {
    fn default() -> Self {
        Self([0; N])
    }
}

impl<const N: usize> core::fmt::Debug for FixedStr<N> {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        write!(f, "{:?}", self.as_bytes())
    }
}

impl<const N: usize> Wire for FixedStr<N> {
    const LEN: usize = N;
    fn wire_encode(&self, out: &mut [u8]) {
        out[..N].copy_from_slice(&self.0);
    }
    fn wire_decode(buf: &[u8]) -> Result<Self, DecodeError> {
        if buf.len() < N {
            return Err(DecodeError::Short);
        }
        Ok(Self(buf[..N].try_into().unwrap()))
    }
}

/// GM level; the wire width varies per packet (1, 2 or 4 bytes). Values
/// that do not fit are debug-asserted and truncated by the codec.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq, PartialOrd, Ord, Hash)]
#[repr(transparent)]
pub struct GmLevel(pub u32);

/// Seconds since the epoch; sent as 4 or 8 bytes depending on the packet.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq, PartialOrd, Ord, Hash)]
#[repr(transparent)]
pub struct TimeT(pub i64);

/// Milliseconds since the map server started.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq, PartialOrd, Ord, Hash)]
#[repr(transparent)]
pub struct TickT(pub u32);

/// A duration in milliseconds; sent as 2 or 4 bytes.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq, PartialOrd, Ord, Hash)]
#[repr(transparent)]
pub struct IntervalT(pub u32);

/// Inventory index shifted by 2, sent as the raw u16.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq, PartialOrd, Ord, Hash)]
#[repr(transparent)]
pub struct IOff2(pub u16);

/// Storage index shifted by 1, sent as the raw u16.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq, PartialOrd, Ord, Hash)]
#[repr(transparent)]
pub struct SOff1(pub u16);

/// IPv4 address as 4 raw octets, like `IP4Address` in C++.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq, PartialOrd, Ord, Hash)]
#[repr(transparent)]
pub struct Ip4Address(pub [u8; 4]);

impl Wire for Ip4Address {
    const LEN: usize = 4;
    fn wire_encode(&self, out: &mut [u8]) {
        out[..4].copy_from_slice(&self.0);
    }
    fn wire_decode(buf: &[u8]) -> Result<Self, DecodeError> {
        if buf.len() < 4 {
            return Err(DecodeError::Short);
        }
        Ok(Self(buf[..4].try_into().unwrap()))
    }
}

/// 10-bit x/y plus a 4-bit direction, packed into 3 bytes
/// (`NetPosition1` in src/mmo/clif.t.hpp).
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct Position1 {
    pub x: u16,
    pub y: u16,
    pub dir: Dir,
}

impl Wire for Position1 {
    const LEN: usize = 3;
    fn wire_encode(&self, out: &mut [u8]) {
        let x = self.x;
        let y = self.y;
        let d = self.dir.0;
        out[0] = (x >> 2) as u8;
        out[1] = ((x << 6) as u8) | (((y >> 4) & 0x3f) as u8);
        out[2] = ((y << 4) as u8) | d;
    }
    fn wire_decode(buf: &[u8]) -> Result<Self, DecodeError> {
        if buf.len() < 3 {
            return Err(DecodeError::Short);
        }
        let p = buf;
        let x = ((p[0] as u16) & (0x3ff >> 2)) << 2 | (p[1] as u16) >> (8 - 2);
        let y = ((p[1] as u16) & (0x3ff >> 4)) << 4 | (p[2] as u16) >> (8 - 4);
        Ok(Position1 {
            x,
            y,
            dir: Dir(p[2] & 0x0f),
        })
    }
}

/// Two 10-bit positions (from/to), packed into 5 bytes
/// (`NetPosition2` in src/mmo/clif.t.hpp).
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct Position2 {
    pub x0: u16,
    pub y0: u16,
    pub x1: u16,
    pub y1: u16,
}

impl Wire for Position2 {
    const LEN: usize = 5;
    fn wire_encode(&self, out: &mut [u8]) {
        let x0 = self.x0;
        let y0 = self.y0;
        let x1 = self.x1;
        let y1 = self.y1;
        out[0] = (x0 >> 2) as u8;
        out[1] = ((x0 << 6) as u8) | (((y0 >> 4) & 0x3f) as u8);
        out[2] = ((y0 << 4) as u8) | (((x1 >> 6) & 0x0f) as u8);
        out[3] = ((x1 << 2) as u8) | (((y1 >> 8) & 0x03) as u8);
        out[4] = y1 as u8;
    }
    fn wire_decode(buf: &[u8]) -> Result<Self, DecodeError> {
        if buf.len() < 5 {
            return Err(DecodeError::Short);
        }
        let p = buf;
        Ok(Position2 {
            x0: ((p[0] as u16) & (0x3ff >> 2)) << 2 | (p[1] as u16) >> (8 - 2),
            y0: ((p[1] as u16) & (0x3ff >> 4)) << 4 | (p[2] as u16) >> (8 - 4),
            x1: ((p[2] as u16) & (0x3ff >> 6)) << 6 | (p[3] as u16) >> (8 - 6),
            y1: ((p[3] as u16) & (0x3ff >> 8)) << 8 | (p[4] as u16),
        })
    }
}
