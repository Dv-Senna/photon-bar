#include "wayland/window.hpp"

#include <cstdint>
#include <expected>
#include <map>
#include <print>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wlr-layer-shell-unstable-v1/wlr-layer-shell-unstable-v1-protocol.h>

#include "color.hpp"
#include "utils/semantic.hpp"


namespace photon::wayland {
	static const zwlr_layer_surface_v1_listener layerSurfaceListener {
		.configure = [](
			void* data,
			zwlr_layer_surface_v1* layerSurface,
			uint32_t serial,
			uint32_t width,
			uint32_t height
		) {
			std::println("configure wlr surface, {}x{}", width, height);
			zwlr_layer_surface_v1_ack_configure(layerSurface, serial);
		},
		.closed = [](void*, [[maybe_unused]] zwlr_layer_surface_v1* layerSurface) noexcept -> void {}
	};

	Window::~Window() noexcept {
		if (m_layerSurface != nullptr)
			zwlr_layer_surface_v1_destroy(m_layerSurface.release());
		if (m_surface != nullptr)
			wl_surface_destroy(m_surface.release());
	}

	auto Window::create(const CreateInfos& createInfos) noexcept -> std::expected<Window, CreateError> {
		Window window {};
		window.m_instance = &createInfos.instance;

		window.m_surface = photon::utils::Owned{wl_compositor_create_surface(window.m_instance->getCompositor())};
		if (window.m_surface == nullptr)
			return std::unexpected(CreateError::eSurfaceCreation);

		window.m_layerSurface = photon::utils::Owned{zwlr_layer_shell_v1_get_layer_surface(
			window.m_instance->getLayerShell(),
			window.m_surface.get(),
			nullptr,
			ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM,
			"photon-bar"
		)};
		if (window.m_layerSurface == nullptr)
			return std::unexpected(CreateError::eLayerSurfaceCreation);

		if (zwlr_layer_surface_v1_add_listener(
			window.m_layerSurface.get(),
			&layerSurfaceListener,
			nullptr
		) != 0)
			return std::unexpected(CreateError::eLayerSurfaceAddListener);

		static const std::map<Window::Anchor, uint32_t> anchorsMap {
			{Anchor::eTop, ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP},
			{Anchor::eBottom, ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM},
			{Anchor::eLeft, ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT},
			{Anchor::eRight, ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT}
		};
		const auto anchor {anchorsMap.find(createInfos.anchor)};
		assert(anchor != anchorsMap.end());
		uint32_t anchorWithSide {anchor->second};
		uint32_t width {0};
		uint32_t height {0};
		if (createInfos.anchor == Anchor::eTop || createInfos.anchor == Anchor::eBottom) {
			anchorWithSide |= ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;
			height = createInfos.size;
		}
		else {
			anchorWithSide |= ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM;
			width = createInfos.size;
		}
		zwlr_layer_surface_v1_set_anchor(window.m_layerSurface.get(), anchorWithSide);
		zwlr_layer_surface_v1_set_exclusive_edge(window.m_layerSurface.get(), anchor->second);
		zwlr_layer_surface_v1_set_exclusive_zone(window.m_layerSurface.get(), createInfos.size);
		zwlr_layer_surface_v1_set_size(window.m_layerSurface.get(), width, height);

		wl_surface_commit(window.m_surface.get());
		wl_display_roundtrip(window.m_instance->getDisplay());

		window.fill({.r = 0, .g = 0, .b = 0, .a = 255});
		return window;
	}

	auto Window::fill(photon::Color color) noexcept -> void {

	}

	auto Window::present() noexcept -> std::expected<void, PresentError> {
		return {};
	}
}
