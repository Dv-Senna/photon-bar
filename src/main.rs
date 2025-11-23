extern crate wayland_client;

fn main() {
    let display = unsafe {wayland_client::wl_display_connect(std::ptr::null())};
    if display == std::ptr::null_mut() {
        panic!();
    }

    let registry = unsafe {wayland_client::wl_display_get_registry(display)};
    if registry == std::ptr::null_mut() {
        panic!();
    }

    unsafe {wayland_client::wl_registry_destroy(registry)};
    unsafe {wayland_client::wl_display_disconnect(display)};
}
