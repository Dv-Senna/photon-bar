#include <cstdlib>
#include <print>
#include <utility>

#include "wayland/instance.hpp"
#include "wayland/window.hpp"


auto main(int, char**) -> int {
	auto instance {photon::wayland::Instance::create()};
	if (!instance)
		return std::println(stderr, "Can't create wayland instance : {}", std::to_underlying(instance.error())), EXIT_FAILURE;
	auto window {photon::wayland::Window::create({
		.instance = *instance,
		.title = "Hello World!",
		.width = 2560,
		.height = 50,
		.anchor = photon::wayland::Window::Anchor::eTop,
	})};
	if (!window)
		return std::println(stderr, "Can't create wayland window : {}", std::to_underlying(window.error())), EXIT_FAILURE;
	while (true) {
		if (wl_display_dispatch_pending(instance->getDisplay()) < 0)
			return std::println(stderr, "Can't dispatch pending wayland"), EXIT_FAILURE;
		window->fill({.r = 0, .g = 0, .b = 0, .a = 100});
		if (!window->present())
			return std::println(stderr, "Can't present wayland window"), EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
