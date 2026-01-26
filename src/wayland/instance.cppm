module;

#include <cassert>
#include <cstdint>
#include <expected>
#include <memory>
#include <tuple>

#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wlr-layer-shell-unstable-v1/wlr-layer-shell-unstable-v1-protocol.h>

#include <flex/reflection/reflection.hpp>

export module photon.wayland.instance;

import photon.utils.semantic;


namespace photon::wayland {
	export class Instance final {
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
			~Instance() noexcept {
				if (m_state == nullptr)
					return;
				if (m_state->layerShell != nullptr)
					zwlr_layer_shell_v1_destroy(m_state->layerShell.release());
				if (m_state->compositor != nullptr)
					wl_compositor_destroy(m_state->compositor.release());
				if (m_state->registry != nullptr)
					wl_registry_destroy(m_state->registry.release());
				if (m_state->display != nullptr)
					wl_display_disconnect(m_state->display.release());
			}

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
				if (interface == flex::reflection::internals::getTypeName<Interface> ())
					return bindInterface<Interface> (state, name, version);
				if constexpr (I < std::tuple_size_v<Interfaces> - 1)
					return self.template operator() <I + 1uz> ();
				return {};
			} ();
		},
		.global_remove = [](void*, wl_registry*, uint32_t) {}
	};


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

		if (wl_registry_add_listener(instance.m_state->registry.get(), &registryListener, instance.m_state.get()) != 0)
			return std::unexpected(CreateError::eRegistryAddListener);

		if (wl_display_dispatch(instance.m_state->display.get()) < 0)
			return std::unexpected(CreateError::eDisplayEventQueueDispatching);
		if (wl_display_roundtrip(instance.m_state->display.get()) < 0)
			return std::unexpected(CreateError::eDisplayEventQueueRoundtrip);

		if (!instance.m_state->bindingResult)
			return std::unexpected(instance.m_state->bindingResult.error());
		return instance;
	}
}
