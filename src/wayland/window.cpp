#include "wayland/window.hpp"

#include <EGL/eglplatform.h>
#include <cstdint>
#include <expected>
#include <map>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <EGL/egl.h>
#include <wayland-client-protocol.h>
#include <wayland-egl-core.h>
#include <wayland-egl.h>
#include <wlr-layer-shell-unstable-v1/wlr-layer-shell-unstable-v1-protocol.h>

#include "color.hpp"
#include "utils/semantic.hpp"
#include "utils/utils.hpp"


namespace photon::wayland {
	static const zwlr_layer_surface_v1_listener layerSurfaceListener {
		.configure = [](
			void* data,
			zwlr_layer_surface_v1* layerSurface,
			uint32_t serial,
			uint32_t width,
			uint32_t height
		) {
			auto eglWindow {static_cast<wl_egl_window*> (data)};
			wl_egl_window_resize(eglWindow, width, height, 0, 0);
			zwlr_layer_surface_v1_ack_configure(layerSurface, serial);
		},
		.closed = [](void*, [[maybe_unused]] zwlr_layer_surface_v1* layerSurface) noexcept -> void {}
	};

	enum class BufferCreateError {
		eNoXDGRuntimeDir,
		eAnonymousFileCreation,
		eAnonymousFileUnlinkage,
		eAnonymousFileTruncate,
		eAnonymousFileMapping,
		eSharedMemoryPoolCreation,
		eBufferCreation,
	};

	Window::~Window() noexcept {
		if (m_eglSurface != nullptr)
			eglDestroySurface(m_instance->getEGLDisplay(), m_eglSurface.release());
		if (m_eglWindow != nullptr)
			wl_egl_window_destroy(m_eglWindow.release());
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

		window.m_eglWindow = photon::utils::Owned{wl_egl_window_create(
			window.m_surface.get(),
			createInfos.width,
			createInfos.height
		)};
		if (window.m_eglWindow == nullptr)
			return std::unexpected(CreateError::eEGLWindowCreation);

		const auto eglSurfaceAttribs {photon::utils::makeArray<const EGLint> (
			EGL_GL_COLORSPACE, EGL_GL_COLORSPACE_LINEAR,
			EGL_RENDER_BUFFER, EGL_BACK_BUFFER,
			EGL_NONE
		)};
		window.m_eglSurface = photon::utils::Owned{eglCreateWindowSurface(
			window.m_instance->getEGLDisplay(),
			window.m_instance->getEGLConfig(),
			reinterpret_cast<EGLNativeWindowType> (window.m_eglWindow.get()),
			eglSurfaceAttribs.data()
		)};
		if (window.m_eglSurface == nullptr)
			return std::unexpected(CreateError::eEGLSurfaceCreation);

		if (eglMakeCurrent(
			window.m_instance->getEGLDisplay(),
			window.m_eglSurface.get(),
			window.m_eglSurface.get(),
			window.m_instance->getEGLContext()
		) == EGL_FALSE)
			return std::unexpected(CreateError::eEGLMakeCurrent);

		if (zwlr_layer_surface_v1_add_listener(
			window.m_layerSurface.get(),
			&layerSurfaceListener,
			window.m_eglWindow.get()
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
		zwlr_layer_surface_v1_set_anchor(window.m_layerSurface.get(), anchor->second);
		zwlr_layer_surface_v1_set_size(window.m_layerSurface.get(), createInfos.width, createInfos.height);

		wl_surface_commit(window.m_surface.get());
//		window.fill({.r = 0, .g = 0, .b = 0, .a = 255});
		return window;
	}

	auto Window::fill(photon::Color color) noexcept -> void {
	}
}
