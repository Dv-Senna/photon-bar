#pragma once

#include <cstdint>
#include <expected>
#include <memory>
#include <vector>

#include <EGL/egl.h>
#include <wayland-client-protocol.h>
#include <wayland-egl.h>
#include <wlr-layer-shell-unstable-v1/wlr-layer-shell-unstable-v1-protocol.h>

#include "utils/semantic.hpp"


namespace photon::wayland {
	class Instance final {
		public:
			enum class CreateError {
				eDisplayCreation,
				eRegistryCreation,
				eRegistryAddListener,
				eLayerShellBinding,
				eCompositorBinding,
				eDisplayEventQueueDispatching,
				eDisplayEventQueueRoundtrip,
				eEGLDisplayGetting,
				eEGLInitialisation,
				eEGLConfiguration,
				eOpenGLBinding,
				eEGLContextCreation,
			};
			enum class UpdateError {
				eDisplayEventQueueRoundtrip,
			};
			struct State {
				photon::utils::Owned<wl_registry*> registry;
				photon::utils::Owned<wl_display*> display;
				photon::utils::Owned<wl_compositor*> compositor;
				photon::utils::Owned<zwlr_layer_shell_v1*> layerShell;
				photon::utils::Owned<EGLDisplay> eglDisplay;
				photon::utils::Owned<EGLContext> eglContext;
				EGLConfig eglConfig;
				std::expected<void, CreateError> bindingResult;
			};

			Instance(const Instance&) = delete;
			auto operator=(const Instance&) -> Instance& = delete;
			auto operator=(Instance&&) -> Instance& = delete;

			constexpr Instance(Instance&&) noexcept = default;
			~Instance() noexcept;

			[[nodiscard]]
			static auto create() noexcept -> std::expected<Instance, CreateError>;

			auto update() noexcept -> std::expected<void, UpdateError>;

			inline auto getCompositor() const noexcept -> wl_compositor* {
				return m_state->compositor.get();
			}
			inline auto getLayerShell() const noexcept -> zwlr_layer_shell_v1* {
				return m_state->layerShell.get();
			}
			inline auto getEGLContext() const noexcept -> EGLContext {
				return m_state->eglContext.get();
			}
			inline auto getEGLDisplay() const noexcept -> EGLDisplay {
				return m_state->eglDisplay.get();
			}
			inline auto getEGLConfig() const noexcept -> EGLConfig {
				return m_state->eglConfig;
			}

		private:
			constexpr Instance() noexcept = default;
			// must be on the heap to keep consistent address for C-callback
			// even after the created instance is returned from `Instance::create`
			std::unique_ptr<State> m_state;
	};
}
