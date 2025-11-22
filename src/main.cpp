#include <cstdlib>
#include <print>
#include <thread>

#include <flex/enums/enums.hpp>

#include "wayland/instance.hpp"
#include "wayland/window.hpp"


auto main(int, char**) -> int {
	auto instance {photon::wayland::Instance::create()};
	if (!instance) {
		std::println(stderr, "Can't create wayland instance : {}", flex::toString(instance.error()).value_or("?"));
		return EXIT_FAILURE;
	}
	auto window {photon::wayland::Window::create({
		.instance = *instance,
		.title = "Hello World!",
		.size = 30,
		.anchor = photon::wayland::Window::Anchor::eTop,
	})};
	if (!window) {
		std::println(stderr, "Can't create wayland window : {}", flex::toString(window.error()).value_or("?"));
		return EXIT_FAILURE;
	}

	bool running {true};
	while (running) {
		if (wl_display_dispatch_pending(instance->getDisplay()) < 0)
			return std::println(stderr, "Can't dispatch pending wayland"), EXIT_FAILURE;
		window->fill({.r = 0, .g = 0, .b = 0, .a = 100});

		if (!window->present())
			return std::println(stderr, "Can't present wayland window"), EXIT_FAILURE;

		using namespace std::chrono_literals;
		std::this_thread::sleep_for(1s);
		running = false;
	}
	return EXIT_SUCCESS;
}
