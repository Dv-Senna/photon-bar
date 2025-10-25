#include "wayland/instance.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <expected>
#include <memory>
#include <tuple>

#include <EGL/egl.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wlr-layer-shell-unstable-v1/wlr-layer-shell-unstable-v1-protocol.h>

#include "utils/reflection.hpp"
#include "utils/semantic.hpp"
#include "utils/utils.hpp"


namespace photon::wayland {
	template <typename T>
	auto bindInterface(
		Instance::State& state,
		uint32_t name,
		uint32_t version
	) noexcept -> std::expected<void, Instance::CreateError>;

	template <>
	auto bindInterface<wl_compositor> (
		Instance::State& state,
		uint32_t name,
		uint32_t version
	) noexcept -> std::expected<void, Instance::CreateError> {
		state.compositor = photon::utils::Owned{static_cast<wl_compositor*> (wl_registry_bind(
			state.registry.get(), name, &wl_compositor_interface, version
		))};
		if (state.compositor == nullptr)
			return std::unexpected(Instance::CreateError::eCompositorBinding);
		return {};
	}

	template <>
	auto bindInterface<zwlr_layer_shell_v1> (
		Instance::State& state,
		uint32_t name,
		uint32_t version
	) noexcept -> std::expected<void, Instance::CreateError> {
		state.layerShell = photon::utils::Owned{static_cast<zwlr_layer_shell_v1*> (wl_registry_bind(
			state.registry.get(), name, &zwlr_layer_shell_v1_interface, version
		))};
		if (state.layerShell == nullptr)
			return std::unexpected(Instance::CreateError::eLayerShellBinding);
		return {};
	}


	static const wl_registry_listener registryListener {
		.global = [](
			void* data,
			[[maybe_unused]] wl_registry* registry,
			uint32_t name,
			const char* interface,
			uint32_t version
		) noexcept -> void {
			auto& state {*static_cast<Instance::State*> (data)};
			// if there was an error in previous instantiation, we exit right now
			if (!state.bindingResult.has_value())
				return;

			// list of the interfaces to bind
			using Interfaces = std::tuple<wl_compositor, zwlr_layer_shell_v1>;

			state.bindingResult = [&] <std::size_t I = 0uz> (this const auto& self) -> decltype(state.bindingResult) {
				using Interface = std::tuple_element_t<I, Interfaces>;
				if (interface == photon::utils::getTypeName<Interface> ())
					return bindInterface<Interface> (state, name, version);
				if constexpr (I < std::tuple_size_v<Interfaces> - 1)
					return self.template operator() <I + 1uz> ();
				return {};
			} ();
		},
		.global_remove = [](void*, wl_registry*, uint32_t) {}
	};

	Instance::~Instance() noexcept {
		if (m_state == nullptr)
			return;
		if (m_state->eglContext != nullptr)
			eglDestroyContext(m_state->eglDisplay.get(), m_state->eglContext.release());
		if (m_state->layerShell != nullptr)
			zwlr_layer_shell_v1_destroy(m_state->layerShell.release());
		if (m_state->compositor != nullptr)
			wl_compositor_destroy(m_state->compositor.release());
		if (m_state->registry != nullptr)
			wl_registry_destroy(m_state->registry.release());
		if (m_state->eglDisplay != nullptr)
			eglTerminate(m_state->eglDisplay.release());
		if (m_state->display != nullptr)
			wl_display_disconnect(m_state->display.release());
	}

	auto Instance::create() noexcept -> std::expected<Instance, CreateError> {
		static std::size_t instanceCount {0uz};
		assert(++instanceCount == 1 && "There can't be more than one instance of Wayland subsystem");
		Instance instance {};
		instance.m_state = std::make_unique<Instance::State> ();
		instance.m_state->display = photon::utils::Owned{wl_display_connect(nullptr)};
		if (instance.m_state->display == nullptr)
			return std::unexpected(CreateError::eDisplayCreation);

		instance.m_state->eglDisplay = photon::utils::Owned{eglGetDisplay(instance.m_state->display.get())};
		if (instance.m_state->eglDisplay == EGL_NO_DISPLAY)
			return std::unexpected(CreateError::eEGLDisplayGetting);
		if (eglInitialize(instance.m_state->eglDisplay.get(), nullptr, nullptr) == EGL_FALSE)
			return std::unexpected(CreateError::eEGLInitialisation);

		instance.m_state->registry = photon::utils::Owned{wl_display_get_registry(instance.m_state->display.get())};
		if (instance.m_state->registry == nullptr)
			return std::unexpected(CreateError::eRegistryCreation);

		if (wl_registry_add_listener(instance.m_state->registry.get(), &registryListener, instance.m_state.get()) != 0)
			return std::unexpected(CreateError::eRegistryAddListener);

		if (wl_display_dispatch(instance.m_state->display.get()) < 0)
			return std::unexpected(CreateError::eDisplayEventQueueDispatching);
		if (wl_display_roundtrip(instance.m_state->display.get()) < 0)
			return std::unexpected(CreateError::eDisplayEventQueueRoundtrip);

		if (!instance.m_state->bindingResult)
			return std::unexpected(instance.m_state->bindingResult.error());


		const auto eglConfigAttribs {photon::utils::makeArray<const EGLint> (
			EGL_ALPHA_SIZE, 8,
			EGL_RED_SIZE, 8,
			EGL_GREEN_SIZE, 8,
			EGL_BLUE_SIZE, 8,
			EGL_CONFORMANT, EGL_OPENGL_BIT,
			EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
			EGL_NONE
		)};
		EGLint configSize {};
		if (eglChooseConfig(
			instance.m_state->eglDisplay.get(), eglConfigAttribs.data(), nullptr, 0, &configSize
		) == EGL_FALSE)
			return std::unexpected(CreateError::eEGLConfiguration);

		std::vector<EGLConfig> eglConfigs {};
		eglConfigs.resize(static_cast<std::size_t> (configSize));
		if (eglChooseConfig(
			instance.m_state->eglDisplay.get(), eglConfigAttribs.data(), eglConfigs.data(), configSize, &configSize
		) == EGL_FALSE)
			return std::unexpected(CreateError::eEGLConfiguration);
		instance.m_state->eglConfig = eglConfigs[0];

		if (eglBindAPI(EGL_OPENGL_API) == EGL_FALSE)
			return std::unexpected(CreateError::eOpenGLBinding);

		const auto eglContextAttribs {photon::utils::makeArray<const EGLint> (
			EGL_CONTEXT_MAJOR_VERSION, 4,
			EGL_CONTEXT_MINOR_VERSION, 6,
			EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
		#ifdef NDEBUG
			EGL_CONTEXT_OPENGL_DEBUG, EGL_FALSE,
		#else
			EGL_CONTEXT_OPENGL_DEBUG, EGL_TRUE,
		#endif
			EGL_NONE
		)};
		instance.m_state->eglContext = photon::utils::Owned{eglCreateContext(
			instance.m_state->eglDisplay.get(),
			instance.m_state->eglConfig,
			EGL_NO_CONTEXT,
			eglContextAttribs.data()
		)};
		if (instance.m_state->eglContext == nullptr)
			return std::unexpected(CreateError::eEGLContextCreation);
		return instance;
	}
}
