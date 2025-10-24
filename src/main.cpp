#include <cstdlib>
#include <print>
#include <utility>

#include "wayland/instance.hpp"


auto main(int, char**) -> int {
	auto instance {photon::wayland::Instance::create()};
	if (!instance)
		return std::println(stderr, "Can't create wayland instance : {}", std::to_underlying(instance.error())), EXIT_FAILURE;
	return EXIT_SUCCESS;
}
