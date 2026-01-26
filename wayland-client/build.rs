extern crate bindgen;
extern crate cc;

use std::path::PathBuf;


fn main() {
    let out_dir = PathBuf::from(String::from(std::env::var("OUT_DIR").unwrap()));

    let headers = [
        "/usr/include/wayland-client-core.h",
        "/usr/include/wayland-client-protocol.h",
    ];

    println!("cargo::rustc-link-lib=wayland-client");

    bindgen::builder()
        .headers(headers)
        .blocklist_item("FP_NAN")
        .blocklist_item("FP_INFINITE")
        .blocklist_item("FP_ZERO")
        .blocklist_item("FP_SUBNORMAL")
        .blocklist_item("FP_NORMAL")
        .generate().unwrap()
        .write_to_file(out_dir.join("wayland-client.rs")).unwrap();
}
