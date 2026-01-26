module;

#include <cstdint>

export module photon.color;


namespace photon {
	export struct Color final {
		uint8_t r;
		uint8_t g;
		uint8_t b;
		uint8_t a;

		template <typename T>
		constexpr auto into() const noexcept -> T;
	};

	export class ARGBColor final {
		friend Color;
		public:
			constexpr ~ARGBColor() noexcept = default;
			constexpr ARGBColor(const ARGBColor&) noexcept = default;
			constexpr auto operator=(const ARGBColor&) noexcept -> ARGBColor& = default;
			constexpr ARGBColor(ARGBColor&&) noexcept = default;
			constexpr auto operator=(ARGBColor&&) noexcept -> ARGBColor& = default;

			template <typename T>
			constexpr auto into() const noexcept -> T;

		private:
			ARGBColor() = delete;
			constexpr ARGBColor(Color color) noexcept : m_color {color} {}
			Color m_color;
	};

	export template <>
	constexpr auto ARGBColor::into<uint32_t> () const noexcept -> uint32_t {
		return static_cast<uint32_t> (m_color.a) << 24uz
			| static_cast<uint32_t> (m_color.r) << 16uz
			| static_cast<uint32_t> (m_color.g) << 8uz
			| static_cast<uint32_t> (m_color.b);
	}

	export class RGBAColor final {
		friend Color;
		public:
			constexpr ~RGBAColor() noexcept = default;
			constexpr RGBAColor(const RGBAColor&) noexcept = default;
			constexpr auto operator=(const RGBAColor&) noexcept -> RGBAColor& = default;
			constexpr RGBAColor(RGBAColor&&) noexcept = default;
			constexpr auto operator=(RGBAColor&&) noexcept -> RGBAColor& = default;

			template <typename T>
			constexpr auto into() const noexcept -> T;

		private:
			RGBAColor() = delete;
			constexpr RGBAColor(Color color) noexcept : m_color {color} {}
			Color m_color;
	};

	export template <>
	constexpr auto RGBAColor::into<uint32_t> () const noexcept -> uint32_t {
		return static_cast<uint32_t> (m_color.r) << 24uz
			| static_cast<uint32_t> (m_color.g) << 16uz
			| static_cast<uint32_t> (m_color.b) << 8uz
			| static_cast<uint32_t> (m_color.a);
	}

	export template <>
	constexpr auto Color::into<ARGBColor> () const noexcept -> ARGBColor {
		return ARGBColor{*this};
	}
	export template <>
	constexpr auto Color::into<RGBAColor> () const noexcept -> RGBAColor {
		return RGBAColor{*this};
	}
}
