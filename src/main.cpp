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
		.width = 16*70,
		.height = 9*70,
		.anchor = photon::wayland::Window::Anchor::eTop,
	})};
	if (!window)
		return std::println(stderr, "Can't create wayland window : {}", std::to_underlying(window.error())), EXIT_FAILURE;
	while (true) {
		if (!instance->update())
			return std::println(stderr, "Can't update wayland instance"), EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
