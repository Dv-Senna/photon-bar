#pragma once

#include <array>
#include <utility>


namespace photon::utils {
	template <typename T>
	constexpr auto makeArray(auto&&... datas) noexcept -> std::array<T, sizeof...(datas)> {
		return std::array<T, sizeof...(datas)> {std::forward<decltype(datas)> (datas)...};
	}
}
