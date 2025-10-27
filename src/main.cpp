#include <chrono>
#include <cstdlib>
#include <print>
#include <string>
#include <thread>
#include <utility>

#include "wayland/instance.hpp"
#include "wayland/window.hpp"
#include "event.hpp"


auto main(int, char**) -> int {
	enum class EventType {
		eSayHello,
		eSayGoodbye,
	};
	photon::EventQueue<EventType,
		photon::Event<EventType::eSayHello, std::string>,
		photon::Event<EventType::eSayGoodbye, uint32_t>
	> eventQueue {"eventQueue"};

	std::jthread _ {[&] noexcept {
		struct EventVisitor {
			bool running;
			auto handle(photon::Event<EventType::eSayHello, std::string> event) noexcept -> void {
				std::println("Hello {}!", event.value);
			}
			auto handle(photon::Event<EventType::eSayGoodbye, uint32_t> event) noexcept -> void {
				std::println("Goodbye with code {}", event.value);
				running = false;
			}
		};
		EventVisitor visitor {
			.running = true,
		};
		while (visitor.running) {
			eventQueue.waitOnEvent(visitor);
		}
	}};

	using namespace std::string_literals;
	eventQueue.push<EventType::eSayHello> ("Albert"s);

	auto instance {photon::wayland::Instance::create()};
	if (!instance)
		return std::println(stderr, "Can't create wayland instance : {}", std::to_underlying(instance.error())), EXIT_FAILURE;
	auto window {photon::wayland::Window::create({
		.instance = *instance,
		.title = "Hello World!",
		.size = 30,
		.anchor = photon::wayland::Window::Anchor::eTop,
	})};
	if (!window)
		return std::println(stderr, "Can't create wayland window : {}", std::to_underlying(window.error())), EXIT_FAILURE;
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

	eventQueue.push<EventType::eSayGoodbye> (12u);
	return EXIT_SUCCESS;
}
