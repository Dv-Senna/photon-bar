#pragma once

#include <cassert>
#include <cstddef>
#include <span>
#include <type_traits>
#include <utility>


namespace photon::utils {
	template <typename T>
	requires std::is_pointer_v<T>
	class Owned final {
		public:
			constexpr Owned() noexcept = default;
			constexpr ~Owned() noexcept = default;
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
				m_ptr {std::move(ptr)}
			{
				ptr = nullptr;
			}

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


	template <typename T>
	class OwnedSpan final {
		public:
			constexpr OwnedSpan() noexcept = default;
			constexpr ~OwnedSpan() noexcept = default;
			OwnedSpan(const OwnedSpan&) = delete;
			auto operator=(const OwnedSpan&) -> OwnedSpan& = delete;

			constexpr OwnedSpan(OwnedSpan&& other) noexcept :
				m_span {other.m_span}
			{
				other.m_span = {};
			}
			constexpr auto operator=(OwnedSpan&& other) noexcept -> OwnedSpan& {
				m_span = other.m_span;
				other.m_span = {};
				return *this;
			}

			explicit constexpr OwnedSpan(T*&& ptr, std::size_t size) noexcept :
				m_span {std::move(ptr), size}
			{
				ptr = nullptr;
			}

			constexpr auto operator==(std::nullptr_t) const noexcept -> bool {
				return m_span == nullptr;
			}

			constexpr auto size() const noexcept -> std::size_t {
				return m_span.size();
			}
			constexpr auto begin() const noexcept {
				return m_span.begin();
			}
			constexpr auto end() const noexcept {
				return m_span.end();
			}
			constexpr auto data() const noexcept -> T* {
				return m_span.data();
			}

			constexpr auto get() const noexcept -> std::span<T> {
				return m_span;
			}
			constexpr auto release() noexcept -> std::span<T> {
				auto span {m_span};
				m_span = nullptr;
				return span;
			}

		private:
			std::span<T> m_span;
	};
}
