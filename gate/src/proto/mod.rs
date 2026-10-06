//! Generated packet definitions plus hand-written wire types.

pub mod types;

#[allow(clippy::all, dead_code)]
mod imp {
    include!(concat!(env!("OUT_DIR"), "/proto.rs"));
}

pub use imp::*;

/// Encode helper: `enc(|v| p.encode(v))`.
pub fn enc(f: impl FnOnce(&mut Vec<u8>)) -> Vec<u8> {
    let mut v = Vec::new();
    f(&mut v);
    v
}

impl Opt0 {
    /// `@hide` GM flag; `Opt0::HIDE` in mmo/clif.t.hpp.
    pub const HIDE: u16 = 0x0040;
}
