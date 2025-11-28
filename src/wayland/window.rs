extern crate wayland_client;
extern crate wayland_protocols_wlr;

use wayland_client::{protocol::{wl_compositor, wl_display, wl_registry, wl_surface}, ConnectError, Connection, Dispatch, DispatchError, EventQueue, Proxy, QueueHandle};
use wayland_protocols_wlr::layer_shell::v1::client::{zwlr_layer_shell_v1, zwlr_layer_surface_v1};

struct RegistryEventData {
    compositor: Option< wl_compositor::WlCompositor>,
    wlr_layer_shell: Option<zwlr_layer_shell_v1::ZwlrLayerShellV1>,
}

impl Dispatch<wl_compositor::WlCompositor, ()> for RegistryEventData {
    fn event(
        _state: &mut Self,
        _proxy: &wl_compositor::WlCompositor,
        _event: <wl_compositor::WlCompositor as Proxy>::Event,
        _data: &(),
        _conn: &Connection,
        _qhandle: &QueueHandle<Self>,
    ) {
        
    }
}

impl Dispatch<zwlr_layer_shell_v1::ZwlrLayerShellV1, ()> for RegistryEventData {
    fn event(
        _state: &mut Self,
        _proxy: &zwlr_layer_shell_v1::ZwlrLayerShellV1,
        _event: <zwlr_layer_shell_v1::ZwlrLayerShellV1 as Proxy>::Event,
        _data: &(),
        _conn: &Connection,
        _qhandle: &QueueHandle<Self>,
    ) {

    }
}

impl Dispatch<wl_registry::WlRegistry, ()> for RegistryEventData {
    fn event(
        state: &mut Self,
        registry: &wl_registry::WlRegistry,
        event: <wl_registry::WlRegistry as wayland_client::Proxy>::Event,
        _data: &(),
        _conn: &Connection,
        qhandle: &QueueHandle<Self>,
    ) {
        if let wl_registry::Event::Global{name, interface, version} = event {
            println!("[{}] {} : {}", name, interface, version);

            match interface.as_str() {
                "wl_compositor" => {
                    state.compositor = Some(registry.bind::<wl_compositor::WlCompositor, _, _> (
                        name, version, qhandle, ()
                    ));
                },
                "zwlr_layer_shell_v1" => {
                    state.wlr_layer_shell = Some(registry.bind::<zwlr_layer_shell_v1::ZwlrLayerShellV1, _, _> (
                        name, version, qhandle, ()
                    ))
                },
                _ => {}
            }
        }
    }
}


struct SurfaceEventData {

}

impl Dispatch<wl_surface::WlSurface, ()> for SurfaceEventData {
    fn event(
        state: &mut Self,
        proxy: &wl_surface::WlSurface,
        event: <wl_surface::WlSurface as Proxy>::Event,
        data: &(),
        conn: &Connection,
        qhandle: &QueueHandle<Self>,
    ) {
        
    }
}

impl Dispatch<zwlr_layer_surface_v1::ZwlrLayerSurfaceV1, ()> for SurfaceEventData {
    fn event(
        _state: &mut Self,
        layer_surface: &zwlr_layer_surface_v1::ZwlrLayerSurfaceV1,
        event: <zwlr_layer_surface_v1::ZwlrLayerSurfaceV1 as Proxy>::Event,
        _data: &(),
        _conn: &Connection,
        _qhandle: &QueueHandle<Self>,
    ) {
        if let zwlr_layer_surface_v1::Event::Configure {serial, width, height} = event {
            println!("configure wlr-layer-surface with size {}x{}", width, height);
            layer_surface.ack_configure(serial);
        }
    }
}


#[derive(Debug)]
pub enum WindowCreationError {
    Connection(ConnectError),
    Dispatch(DispatchError),
    Compositor,
    WlrLayerShell,
}

pub struct Window {
    connection : Connection,
    display: wl_display::WlDisplay,
    registry: wl_registry::WlRegistry,
    event_queue: EventQueue<RegistryEventData>,
    compositor: wl_compositor::WlCompositor,
    wlr_layer_shell: zwlr_layer_shell_v1::ZwlrLayerShellV1,
    surface: wl_surface::WlSurface,
    layer_surface: zwlr_layer_surface_v1::ZwlrLayerSurfaceV1,
}

impl Window {
    pub fn new() -> Result<Self, WindowCreationError> {
        let connection = match Connection::connect_to_env() {
            Ok(value) => value,
            Err(error) => return Err(WindowCreationError::Connection(error)),
        };
        let display = connection.display();

        let mut event_queue = connection.new_event_queue();
        let event_queue_handle = event_queue.handle();

        let mut registry_event_data = RegistryEventData{
            compositor: None,
            wlr_layer_shell: None,
        };
        let registry = display.get_registry(&event_queue_handle, ());
        if let Err(error) = event_queue.roundtrip(&mut registry_event_data) {
            return Err(WindowCreationError::Dispatch(error));
        }
        if let None = registry_event_data.compositor {
            return Err(WindowCreationError::Compositor);
        }
        if let None = registry_event_data.wlr_layer_shell {
            return Err(WindowCreationError::WlrLayerShell);
        }

        let mut surface_event_queue = connection.new_event_queue();
        let surface_event_queue_handle = surface_event_queue.handle();

        let surface = registry_event_data.compositor.as_ref().unwrap().create_surface(&surface_event_queue_handle, ());
        let layer_surface = registry_event_data.wlr_layer_shell.as_ref().unwrap().get_layer_surface(
            &surface,
            None,
            zwlr_layer_shell_v1::Layer::Bottom,
            "photon-bar".to_string(),
            &surface_event_queue_handle, ()
        );

        layer_surface.set_anchor(zwlr_layer_surface_v1::Anchor::Left
            | zwlr_layer_surface_v1::Anchor::Right
            | zwlr_layer_surface_v1::Anchor::Top
        );
        layer_surface.set_exclusive_edge(zwlr_layer_surface_v1::Anchor::Top);
        layer_surface.set_exclusive_zone(50);
        layer_surface.set_size(0, 50);

        let mut surface_event_data = SurfaceEventData{

        };
        surface.commit();
        if let Err(error) = surface_event_queue.roundtrip(&mut surface_event_data) {
            return Err(WindowCreationError::Dispatch(error));
        }

        return Ok(Self{
            connection: connection,
            display: display,
            registry: registry,
            event_queue: event_queue,
            compositor: registry_event_data.compositor.unwrap(),
            wlr_layer_shell: registry_event_data.wlr_layer_shell.unwrap(),
            surface: surface,
            layer_surface: layer_surface,
        });
    }


    pub fn present(self: &Self) {
        self.connection.flush().unwrap();
    }
}
