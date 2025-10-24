#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <string_view>

#include <wayland-client-protocol.h>
#include <xdg-shell/xdg-shell-client-protocol.h>

#include "color.hpp"
#include "utils/semantic.hpp"
#include "wayland/instance.hpp"


namespace photon::wayland {
	class Window final {
		public:
			enum class CreateError {
				eSurfaceCreation,
				eXDGSurfaceCreation,
				eXDGSurfaceAddListener,
				eXDGTopLevelGetting,
				eBufferCreation,
			};
			struct CreateInfos {
				photon::wayland::Instance& instance;
				std::string_view title;
				uint32_t width;
				uint32_t height;
			};

			Window(const Window&) = delete;
			auto operator=(const Window&) -> Window& = delete;
			constexpr Window(Window&&) noexcept = default;
			constexpr auto operator=(Window&&) noexcept -> Window& = default;

			~Window() noexcept;

			static auto create(const CreateInfos& createInfos) noexcept -> std::expected<Window, CreateError>;

			auto fill(photon::Color color) noexcept -> void;

		private:
			constexpr Window() noexcept = default;

			photon::wayland::Instance* m_instance;
			photon::utils::Owned<wl_surface*> m_surface;
			photon::utils::Owned<xdg_surface*> m_xdgSurface;
			photon::utils::Owned<xdg_toplevel*> m_toplevel;
			photon::utils::Owned<wl_buffer*> m_buffer;
			photon::utils::OwnedSpan<std::byte> m_bufferData;
	};
}
