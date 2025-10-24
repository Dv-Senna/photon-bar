#pragma once

#include <cassert>
#include <cstddef>
#include <type_traits>
#include <utility>


namespace photon::utils {
	template <typename T>
	requires std::is_pointer_v<T>
	class Owned final {
		public:
			constexpr Owned() noexcept = default;
			constexpr ~Owned() noexcept {m_ptr = nullptr;}
			Owned(const Owned&) = delete;
			auto operator=(const Owned&) -> Owned& = delete;

			constexpr Owned(Owned&& other) noexcept :
				m_ptr {other.m_ptr}
			{
				other.m_ptr = nullptr;
			}
			constexpr auto operator=(Owned&& other) noexcept -> Owned& {
				m_ptr = other.m_ptr;
				other.m_ptr = nullptr;
				return *this;
			}

			explicit constexpr Owned(T&& ptr) noexcept :
				m_ptr {ptr}
			{}

			constexpr auto operator==(std::nullptr_t) const noexcept -> bool {
				return m_ptr == nullptr;
			}

			constexpr auto operator*() const noexcept -> std::add_lvalue_reference_t<std::remove_pointer_t<T>> {
				assert(m_ptr != nullptr);
				return *m_ptr;
			}
			constexpr auto operator->() const noexcept -> T {
				assert(m_ptr != nullptr);
				return m_ptr;
			}
			constexpr auto operator()(auto&&... args) -> std::invoke_result_t<
				std::add_lvalue_reference_t<std::remove_pointer_t<T>>,
				decltype(std::forward<decltype(args)> (args))...
			> {
				assert(m_ptr != nullptr);
				return (*m_ptr)(std::forward<decltype(args)> (args)...);
			}
			explicit constexpr operator bool() const noexcept {
				return !!m_ptr;
			}
			explicit constexpr operator T() const noexcept {
				return m_ptr;
			}
			constexpr auto get() const noexcept -> T {
				return m_ptr;
			}
			constexpr auto release() noexcept -> T {
				auto ptr {m_ptr};
				m_ptr = nullptr;
				return ptr;
			}

		private:
			T m_ptr {nullptr};
	};
}
