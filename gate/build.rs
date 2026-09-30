fn main() {
    let out_dir = std::env::var("OUT_DIR").expect("OUT_DIR not set");
    let out = std::path::Path::new(&out_dir).join("proto.rs");
    let status = std::process::Command::new("python3")
        .arg("../tools/protocol.py")
        .arg("--rust")
        .arg(&out)
        .status()
        .expect("failed to run tools/protocol.py (is python3 installed?)");
    assert!(status.success(), "tools/protocol.py --rust failed");
    println!("cargo:rerun-if-changed=../tools/protocol.py");
    println!("cargo:rerun-if-changed=../src/mmo/consts.hpp");
    println!("cargo:rerun-if-changed=../src/mmo/enums.hpp");
}
