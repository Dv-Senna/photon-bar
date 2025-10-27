#pragma once

#include <array>
#include <concepts>
#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>


namespace photon::utils {
	template <typename T>
	constexpr auto makeArray(auto&&... datas) noexcept -> std::array<T, sizeof...(datas)> {
		return std::array<T, sizeof...(datas)> {std::forward<decltype(datas)> (datas)...};
	}


	template <typename T>
	concept hashable = requires(const T cv, std::hash<T> hash) {
		{hash(cv)} -> std::same_as<std::size_t>;
	}
		&& std::copy_constructible<T>
		&& std::destructible<T>
		&& std::default_initializable<T>
		&& std::is_copy_assignable_v<T>
		&& std::swappable<T>;

	template <typename From, typename Of>
	concept forward_of = (std::is_const_v<Of> && std::same_as<Of, std::remove_reference_t<From>>)
		|| (!std::is_const_v<Of> && std::same_as<Of, std::remove_const_t<std::remove_reference_t<From>>>);

	template <typename T>
	using forward_type_t = decltype(std::forward<T> (std::declval<T> ()));
}
