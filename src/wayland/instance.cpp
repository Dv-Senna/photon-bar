#include "wayland/instance.hpp"

#include <cassert>
#include <cstdint>
#include <expected>
#include <memory>
#include <tuple>

#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <xdg-shell/xdg-shell-client-protocol.h>

#include "utils/reflection.hpp"
#include "utils/semantic.hpp"


namespace photon::wayland {
	static const xdg_wm_base_listener windowManagerBaseListener {
		.ping = [](void*, xdg_wm_base* windowManagerBase, uint32_t serial) noexcept -> void {
			xdg_wm_base_pong(windowManagerBase, serial);
		}
	};
	static const wl_shm_listener sharedMemoryListener {
		.format = [](void* data, [[maybe_unused]] wl_shm* sharedMemory, uint32_t format) {
			auto& state {*static_cast<Instance::State*> (data)};
			state.supportedFormats.push_back(format);
		}
	};

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
	auto bindInterface<xdg_wm_base> (
		Instance::State& state,
		uint32_t name,
		uint32_t version
	) noexcept -> std::expected<void, Instance::CreateError> {
		state.windowManagerBase = photon::utils::Owned{static_cast<xdg_wm_base*> (wl_registry_bind(
			state.registry.get(), name, &xdg_wm_base_interface, version
		))};
		if (state.windowManagerBase == nullptr)
			return std::unexpected(Instance::CreateError::eWindowManagerBaseBinding);

		if (xdg_wm_base_add_listener(state.windowManagerBase.get(), &windowManagerBaseListener, &state) != 0)
			return std::unexpected(Instance::CreateError::eWindowManagerBaseAddListener);
		return {};
	}

	template <>
	auto bindInterface<wl_shm> (
		Instance::State& state,
		uint32_t name,
		uint32_t version
	) noexcept -> std::expected<void, Instance::CreateError> {
		state.sharedMemory = photon::utils::Owned{static_cast<wl_shm*> (wl_registry_bind(
			state.registry.get(), name, &wl_shm_interface, version
		))};
		if (state.sharedMemory == nullptr)
			return std::unexpected(Instance::CreateError::eSharedMemoryBinding);

		if (wl_shm_add_listener(state.sharedMemory.get(), &sharedMemoryListener, &state) != 0)
			return std::unexpected(Instance::CreateError::eSharedMemoryAddListener);
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
			using Interfaces = std::tuple<wl_compositor, xdg_wm_base, wl_shm>;

			state.bindingResult = [&] <std::size_t I = 0uz> (this const auto& self) -> decltype(state.bindingResult) {
				using Interface = std::tuple_element_t<I, Interfaces>;
				if (interface != photon::utils::getTypeName<Interface> ())
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
		if (m_state->sharedMemory != nullptr)
			wl_shm_destroy(m_state->sharedMemory.release());
		if (m_state->windowManagerBase != nullptr)
			xdg_wm_base_destroy(m_state->windowManagerBase.release());
		if (m_state->compositor != nullptr)
			wl_compositor_destroy(m_state->compositor.release());
		if (m_state->registry != nullptr)
			wl_registry_destroy(m_state->registry.release());
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

		instance.m_state->registry = photon::utils::Owned{wl_display_get_registry(instance.m_state->display.get())};
		if (instance.m_state->registry == nullptr)
			return std::unexpected(CreateError::eRegistryCreation);

		if (wl_registry_add_listener(instance.m_state->registry.get(), &registryListener, nullptr) != 0)
			return std::unexpected(CreateError::eRegistryAddListener);
		return instance;
	}
}
