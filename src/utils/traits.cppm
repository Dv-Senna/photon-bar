module;

#include <concepts>
#include <optional>
#include <type_traits>

export module photon.utils.traits;


namespace photon::utils {
	export template <typename T>
	concept error_wrapper = requires(T v) {
		*v;
		{!v} -> std::same_as<bool>;
	};

	export template <typename T>
	concept clone_traits = requires(const T cv) {
		requires std::same_as<decltype(cv.clone()), T>
			|| error_wrapper<decltype(cv.clone())>;
		requires noexcept(cv.clone());
	};

	export template <typename T>
	concept clonable = clone_traits<T> || std::copy_constructible<T>;

	export template <clonable T>
	constexpr auto clone(const T& value) noexcept {
		if constexpr (clone_traits<T>)
			return value.clone();
		else {
			if constexpr (std::is_nothrow_copy_constructible_v<T>)
				return T{value};
			else {
				try {
					return std::optional<T> {value};
				}
				catch (...) {
					return std::optional<T> {};
				}
			}
		}
	}
}
