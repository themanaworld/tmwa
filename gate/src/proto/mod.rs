//! Generated packet definitions plus hand-written wire types.

pub mod types;

#[allow(clippy::all, dead_code)]
mod imp {
    include!(concat!(env!("OUT_DIR"), "/proto.rs"));
}

pub use imp::*;
