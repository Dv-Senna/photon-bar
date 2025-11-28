#![allow(dead_code)]

extern crate wayland_client;
extern crate wayland_protocols;
extern crate wayland_protocols_wlr;

mod wayland;

fn main() {
    let window = wayland::Window::new().unwrap();

    loop {
        window.present();
    }
}
