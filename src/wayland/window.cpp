#include "wayland/window.hpp"

#include <cstdint>
#include <expected>
#include <map>
#include <print>
#include <unordered_map>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <glad/glad.h>
#include <EGL/egl.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-egl-core.h>
#include <wayland-egl.h>
#include <wlr-layer-shell-unstable-v1/wlr-layer-shell-unstable-v1-protocol.h>

#include "color.hpp"
#include "utils/semantic.hpp"
#include "utils/utils.hpp"


namespace photon::wayland {
#ifndef NDEBUG
	static auto debugMessengerCallback(
		GLenum source,
		GLenum type,
		GLuint id,
		GLenum severity,
		GLsizei length,
		const GLchar* message,
		const void*
	) -> void APIENTRY {
		static const std::unordered_map<GLenum, std::string_view> SOURCE_MAP {
			{GL_DEBUG_SOURCE_API, "api"},
			{GL_DEBUG_SOURCE_APPLICATION, "application"},
			{GL_DEBUG_SOURCE_SHADER_COMPILER, "shader compiler"},
			{GL_DEBUG_SOURCE_THIRD_PARTY, "third party"},
			{GL_DEBUG_SOURCE_WINDOW_SYSTEM, "window system"},
			{GL_DEBUG_SOURCE_OTHER, "other"}
		};
		static const std::unordered_map<GLenum, std::string_view> TYPE_MAP {
			{GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR, "deprecated behavior"},
			{GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR, "undefined behavior"},
			{GL_DEBUG_TYPE_ERROR, "error"},
			{GL_DEBUG_TYPE_MARKER, "marker"},
			{GL_DEBUG_TYPE_PERFORMANCE, "performance"},
			{GL_DEBUG_TYPE_POP_GROUP, "pop group"},
			{GL_DEBUG_TYPE_PUSH_GROUP, "push group"},
			{GL_DEBUG_TYPE_PORTABILITY, "portability"},
			{GL_DEBUG_TYPE_OTHER, "other"}
		};
		static const std::unordered_map<GLenum, std::string_view> SEVERITY_MAP {
			{GL_DEBUG_SEVERITY_NOTIFICATION, "verbose"},
			{GL_DEBUG_SEVERITY_LOW, "info"},
			{GL_DEBUG_SEVERITY_MEDIUM, "warning"},
			{GL_DEBUG_SEVERITY_HIGH, "error"}
		};

		std::println("{} > OpenGL (id={}) : from {}, type {} : {}",
			SEVERITY_MAP.find(severity)->second,
			id, SOURCE_MAP.find(source)->second,
			TYPE_MAP.find(type)->second,
			std::string_view{message, static_cast<std::size_t> (length)}
		);
	}
#endif

	static const zwlr_layer_surface_v1_listener layerSurfaceListener {
		.configure = [](
			void* data,
			zwlr_layer_surface_v1* layerSurface,
			uint32_t serial,
			uint32_t width,
			uint32_t height
		) {
			std::println("configure wlr surface, {}x{}", width, height);
			auto eglWindow {static_cast<wl_egl_window*> (data)};
			wl_egl_window_resize(eglWindow, width, height, 0, 0);
			zwlr_layer_surface_v1_ack_configure(layerSurface, serial);
		},
		.closed = [](void*, [[maybe_unused]] zwlr_layer_surface_v1* layerSurface) noexcept -> void {}
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
			100, 100
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

		if (gladLoadGLLoader(reinterpret_cast<GLADloadproc> (eglGetProcAddress)) == 0)
			return std::unexpected(CreateError::eOpenGLFunctionsLoading);

	#ifndef NDEBUG
		glEnable(GL_DEBUG_OUTPUT);
		glDebugMessageCallback(&debugMessengerCallback, nullptr);
	#endif

		glViewport(0, 0, width, height);

		window.fill({.r = 0, .g = 0, .b = 0, .a = 255});
		return window;
	}

	auto Window::fill(photon::Color color) noexcept -> void {
		glClearColor(color.r / 255.f, color.g / 255.f, color.b / 255.f, color.a / 255.f);
		glClear(GL_COLOR_BUFFER_BIT);
	}

	auto Window::present() noexcept -> std::expected<void, PresentError> {
		if (eglSwapBuffers(m_instance->getEGLDisplay(), m_eglSurface.get()) == EGL_FALSE)
			return std::unexpected(PresentError::eBufferSwapping);
		return {};
	}
}
