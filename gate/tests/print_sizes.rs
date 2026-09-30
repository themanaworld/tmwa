//! Prints the same size table as the C++ cross-check program in
//! gate-spike/cxx-crosscheck. Run with `cargo test --test print_sizes
//! -- --ignored --nocapture` (or capture stdout) and diff.
#![allow(clippy::all)]

use tmwa_gate::proto::*;

#[test]
#[ignore]
fn sizes() {
    for &id in ALL_PACKET_IDS {
        let (h, r) = wire_sizes(id).unwrap();
        if r > 0 {
            println!("{id:04x} h {h}");
            println!("{id:04x} r {r}");
        } else {
            println!("{id:04x} f {h}");
        }
    }
    for &(name, size) in STRUCT_SIZES {
        println!("s {name} {size}");
    }
}
