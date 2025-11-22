#pragma once

#include <expected>
#include <memory>

#include <wayland-client-protocol.h>
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
			};
			struct State {
				photon::utils::Owned<wl_registry*> registry;
				photon::utils::Owned<wl_display*> display;
				photon::utils::Owned<wl_compositor*> compositor;
				photon::utils::Owned<zwlr_layer_shell_v1*> layerShell;
				std::expected<void, CreateError> bindingResult;
			};

			Instance(const Instance&) = delete;
			auto operator=(const Instance&) -> Instance& = delete;
			auto operator=(Instance&&) -> Instance& = delete;

			constexpr Instance(Instance&&) noexcept = default;
			~Instance() noexcept;

			[[nodiscard]]
			static auto create() noexcept -> std::expected<Instance, CreateError>;

			inline auto getDisplay() const noexcept -> wl_display* {
				return m_state->display.get();
			}
			inline auto getCompositor() const noexcept -> wl_compositor* {
				return m_state->compositor.get();
			}
			inline auto getLayerShell() const noexcept -> zwlr_layer_shell_v1* {
				return m_state->layerShell.get();
			}

		private:
			constexpr Instance() noexcept = default;
			// must be on the heap to keep consistent address for C-callback
			// even after the created instance is returned from `Instance::create`
			std::unique_ptr<State> m_state;
	};
}
