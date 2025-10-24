#pragma once

#include <cstdint>
#include <expected>
#include <memory>
#include <optional>
#include <vector>

#include <wayland-client-protocol.h>
#include <wayland-client.h>
#include <xdg-shell/xdg-shell-client-protocol.h>

#include "utils/semantic.hpp"


namespace photon::wayland {
	class Instance final {
		public:
			enum class CreateError {
				eDisplayCreation,
				eRegistryCreation,
				eRegistryAddListener,
				eCompositorBinding,
				eWindowManagerBaseBinding,
				eWindowManagerBaseAddListener,
				eSharedMemoryBinding,
				eSharedMemoryAddListener,
				eDisplayEventQueueDispatching,
				eDisplayEventQueueRoundtrip,
				eSharedMemoryFormatNotSupported,
			};
			enum class UpdateError {
				eDisplayEventQueueRoundtrip,
			};
			struct State {
				photon::utils::Owned<wl_registry*> registry;
				photon::utils::Owned<wl_display*> display;
				photon::utils::Owned<wl_compositor*> compositor;
				photon::utils::Owned<xdg_wm_base*> windowManagerBase;
				photon::utils::Owned<wl_shm*> sharedMemory;
				std::expected<void, CreateError> bindingResult;
				std::vector<uint32_t> supportedFormats;
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
			inline auto getWindowManagerBase() const noexcept -> xdg_wm_base* {
				return m_state->windowManagerBase.get();
			}
			inline auto getSharedMemory() const noexcept -> wl_shm* {
				return m_state->sharedMemory.get();
			}

		private:
			constexpr Instance() noexcept = default;
			// must be on the heap to keep consistent address for C-callback
			// even after the created instance is returned from `Instance::create`
			std::unique_ptr<State> m_state;
	};
}
