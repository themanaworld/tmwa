//! Generated packet definitions plus hand-written wire types.

pub mod types;

#[allow(clippy::all, dead_code)]
mod imp {
    include!(concat!(env!("OUT_DIR"), "/proto.rs"));
}

pub use imp::*;

impl Opt0 {
    /// `@hide` GM flag; `Opt0::HIDE` in mmo/clif.t.hpp.
    pub const HIDE: u16 = 0x0040;
}
