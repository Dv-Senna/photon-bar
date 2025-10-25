#pragma once

#include <EGL/egl.h>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <string_view>

#include <wayland-client-protocol.h>
#include <wayland-egl.h>
#include <wlr-layer-shell-unstable-v1/wlr-layer-shell-unstable-v1-protocol.h>

#include "color.hpp"
#include "utils/semantic.hpp"
#include "wayland/instance.hpp"


namespace photon::wayland {
	class Window final {
		public:
			enum class CreateError {
				eSurfaceCreation,
				eBufferCreation,
				eLayerSurfaceCreation,
				eEGLWindowCreation,
				eLayerSurfaceAddListener,
				eEGLSurfaceCreation,
				eEGLMakeCurrent,
				eOpenGLFunctionsLoading,
			};
			enum class PresentError {
				eDisplayEventQueueRoundtrip,
				eBufferSwapping,
			};
			enum class Anchor {
				eTop,
				eBottom,
				eLeft,
				eRight,
			};
			struct CreateInfos {
				photon::wayland::Instance& instance;
				std::string_view title;
				uint32_t width;
				uint32_t height;
				Anchor anchor;
			};

			Window(const Window&) = delete;
			auto operator=(const Window&) -> Window& = delete;
			constexpr Window(Window&&) noexcept = default;
			constexpr auto operator=(Window&&) noexcept -> Window& = default;

			~Window() noexcept;

			static auto create(const CreateInfos& createInfos) noexcept -> std::expected<Window, CreateError>;

			auto fill(photon::Color color) noexcept -> void;
			auto present() noexcept -> std::expected<void, PresentError>;

		private:
			constexpr Window() noexcept = default;

			photon::wayland::Instance* m_instance;
			photon::utils::Owned<wl_surface*> m_surface;
			photon::utils::Owned<zwlr_layer_surface_v1*> m_layerSurface;
			photon::utils::Owned<wl_egl_window*> m_eglWindow;
			photon::utils::Owned<EGLSurface> m_eglSurface;
	};
}
