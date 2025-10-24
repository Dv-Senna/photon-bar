#include "wayland/window.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <expected>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <tuple>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <wayland-client-protocol.h>
#include <xdg-shell/xdg-shell-client-protocol.h>

#include "color.hpp"
#include "utils/janitor.hpp"
#include "utils/semantic.hpp"


namespace photon::wayland {
	static const xdg_surface_listener xdgSurfaceListener {
		.configure = [](void*, xdg_surface* xdgSurface, uint32_t serial) noexcept -> void {
			xdg_surface_ack_configure(xdgSurface, serial);
		}
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

	static auto createAnonymousFile(std::string_view name, std::size_t size) noexcept
		-> std::expected<int, BufferCreateError>
	{
		using namespace std::string_view_literals;
		const auto postfix {"-liteway-wayland-XXXXXX"sv};
		const char* directoryPath {std::getenv("XDG_RUNTIME_DIR")};
		if (directoryPath == nullptr)
			return std::unexpected(BufferCreateError::eNoXDGRuntimeDir);
		auto path {std::array{std::string_view{directoryPath}, "/"sv, name, postfix}
			| std::views::join
			| std::ranges::to<std::string> ()
		};

		int fd {mkostemp(path.data(), O_CLOEXEC)};
		if (fd < 0)
			return std::unexpected(BufferCreateError::eAnonymousFileCreation);
		if (unlink(path.c_str()) != 0)
			return std::unexpected(BufferCreateError::eAnonymousFileUnlinkage);
		if (ftruncate(fd, static_cast<int32_t> (size)) != 0)
			return std::unexpected(BufferCreateError::eAnonymousFileTruncate);
		return fd;
	}

	static auto createBuffer(std::string_view name, wl_shm* sharedMemory, uint32_t width, uint32_t height) noexcept
		-> std::expected<
			std::tuple<photon::utils::Owned<wl_buffer*>, photon::utils::OwnedSpan<std::byte>>,
			BufferCreateError
	> {
		constexpr auto surfaceFormat {WL_SHM_FORMAT_ARGB8888};
		constexpr auto bytesPerPixel {4uz};

		const std::size_t stride {width * bytesPerPixel};
		const std::size_t size {height * stride};

		const std::expected fdWithError {createAnonymousFile(name, size)};
		if (!fdWithError)
			return std::unexpected(fdWithError.error());
		const int fd {*fdWithError};
		photon::utils::Janitor _ {[fd] noexcept {close(fd);}};

		auto bufferData {static_cast<std::byte*> (mmap(nullptr, size, PROT_WRITE, MAP_SHARED, fd, 0))};
		if (bufferData == MAP_FAILED)
			return std::unexpected(BufferCreateError::eAnonymousFileMapping);

		wl_shm_pool* pool {wl_shm_create_pool(sharedMemory, fd, static_cast<int32_t> (size))};
		if (pool == nullptr)
			return std::unexpected(BufferCreateError::eSharedMemoryPoolCreation);
		wl_buffer* buffer {wl_shm_pool_create_buffer(pool, 0,
			static_cast<int32_t> (width), static_cast<int32_t> (height),
			static_cast<int32_t> (stride), surfaceFormat
		)};
		wl_shm_pool_destroy(pool);
		if (buffer == nullptr)
			return std::unexpected(BufferCreateError::eBufferCreation);
		return std::make_tuple(
			photon::utils::Owned{std::move(buffer)},
			photon::utils::OwnedSpan{std::move(bufferData), size}
		);
	};

	Window::~Window() noexcept {

	}

	auto Window::create(const CreateInfos& createInfos) noexcept -> std::expected<Window, CreateError> {
		Window window {};
		window.m_instance = &createInfos.instance;

		window.m_surface = photon::utils::Owned{wl_compositor_create_surface(window.m_instance->getCompositor())};
		if (window.m_surface == nullptr)
			return std::unexpected(CreateError::eSurfaceCreation);

		window.m_xdgSurface = photon::utils::Owned{xdg_wm_base_get_xdg_surface(
			window.m_instance->getWindowManagerBase(),
			window.m_surface.get()
		)};
		if (window.m_xdgSurface == nullptr)
			return std::unexpected(CreateError::eXDGSurfaceCreation);
		if (xdg_surface_add_listener(window.m_xdgSurface.get(), &xdgSurfaceListener, nullptr) != 0)
			return std::unexpected(CreateError::eXDGSurfaceAddListener);

		window.m_toplevel = photon::utils::Owned{xdg_surface_get_toplevel(window.m_xdgSurface.get())};
		if (window.m_toplevel == nullptr)
			return std::unexpected(CreateError::eXDGTopLevelGetting);

		std::expected bufferWithError {createBuffer(
			createInfos.title,
			window.m_instance->getSharedMemory(),
			createInfos.width,
			createInfos.height
		)};
		if (!bufferWithError)
			return std::unexpected(CreateError::eBufferCreation);
		auto& [buffer, bufferData] {*bufferWithError};
		window.m_buffer = std::move(buffer);
		window.m_bufferData = std::move(bufferData);

		wl_surface_attach(window.m_surface.get(), window.m_buffer.get(), 0, 0);
		wl_surface_commit(window.m_surface.get());
		window.fill({.r = 0, .g = 0, .b = 0, .a = 255});
		return window;
	}

	auto Window::fill(photon::Color color) noexcept -> void {
		assert(m_bufferData.size() % 4uz == 0uz);
		assert(reinterpret_cast<std::uintptr_t> (m_bufferData.data()) % 4uz == 0uz);
		const std::span<uint32_t> bufferDataAsU32 {
			reinterpret_cast<uint32_t*> (m_bufferData.data()),
			m_bufferData.size() / 4uz
		};
		std::ranges::fill(bufferDataAsU32, color.into<photon::ARGBColor> ().into<uint32_t> ());
	}
}
