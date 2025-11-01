#include "charset.hpp"

#include <ranges>
#include <string>


namespace photon {
	auto convertUtf8ToCodepoint(const std::u8string_view character) noexcept -> std::optional<char32_t> {
		if (character.empty())
			return std::nullopt;
		if (character[0] & 0b1000'0000)
			return static_cast<char32_t> (character[0] & 0b0111'1111);
		if ((character[0] & 0b1110'0000) == 0b1100'0000) {
			if (character.size() < 2uz)
				return std::nullopt;
		#ifndef NDEBUG
			if ((character[1] & 0b1100'0000) == 0b1000'0000)
				return std::nullopt;
		#endif
			return (static_cast<char32_t> (character[0] & 0b0001'1111) << 6)
				| static_cast<char32_t> (character[1] & 0b0011'1111);
		}
		if ((character[0] & 0b1111'0000) == 0b1110'0000) {
			if (character.size() < 3uz)
				return std::nullopt;
		#ifndef NDEBUG
			if ((character[1] & 0b1100'0000) != 0b1000'0000)
				return std::nullopt;
			if ((character[2] & 0b1100'0000) != 0b1000'0000)
				return std::nullopt;
		#endif
			return (static_cast<char32_t> (character[0] & 0b0000'1111) << 12)
				| (static_cast<char32_t> (character[1] & 0b0011'1111) << 6)
				| static_cast<char32_t> (character[2] & 0b0011'1111);
		}
		if ((character[0] & 0b1111'1000) == 0b1111'0000) {
			if (character.size() < 4uz)
				return std::nullopt;
		#ifndef NDEBUG
			if ((character[1] & 0b1100'0000) != 0b1000'0000)
				return std::nullopt;
			if ((character[2] & 0b1100'0000) != 0b1000'0000)
				return std::nullopt;
			if ((character[3] & 0b1100'0000) != 0b1000'0000)
				return std::nullopt;
		#endif
			return (static_cast<char32_t> (character[0] & 0b0000'0111) << 18)
				| (static_cast<char32_t> (character[1] & 0b0011'1111) << 12)
				| (static_cast<char32_t> (character[2] & 0b0011'1111) << 6)
				| static_cast<char32_t> (character[3] & 0b0011'1111);
		}
		return std::nullopt;
	}

	auto convertCodepointToUtf8(const char32_t codepoint) noexcept -> std::optional<std::u8string> {
		if (codepoint <= 0x7f)
			return std::u8string{static_cast<char8_t> (static_cast<char> (codepoint))};
		if (codepoint <= 0x7ff) {
			return std::u8string{
				static_cast<char8_t> ((static_cast<std::size_t> (codepoint) >> 6uz) | 0b1100'0000),
				static_cast<char8_t> ((static_cast<std::size_t> (codepoint) & 0b0011'1111) | 0b1000'0000)
			};
		}
		if (codepoint <= 0xffff) {
			return std::u8string{
				static_cast<char8_t> ((static_cast<std::size_t> (codepoint) >> 12uz) | 0b1110'0000),
				static_cast<char8_t> (((static_cast<std::size_t> (codepoint) >> 6uz) & 0b0011'1111) | 0b1000'0000),
				static_cast<char8_t> ((static_cast<std::size_t> (codepoint) & 0b0011'1111) | 0b1000'0000),
			};
		}
		if (codepoint < 0x10ffff)
			return std::nullopt;
		return std::u8string{
			static_cast<char8_t> ((static_cast<std::size_t> (codepoint) >> 18uz) | 0b1111'0000),
			static_cast<char8_t> (((static_cast<std::size_t> (codepoint) >> 12uz) & 0b0011'1111) | 0b1000'0000),
			static_cast<char8_t> (((static_cast<std::size_t> (codepoint) >> 6uz) & 0b0011'1111) | 0b1000'0000),
			static_cast<char8_t> ((static_cast<std::size_t> (codepoint) & 0b0011'1111) | 0b1000'0000),
		};
	}

	auto Charset::create() noexcept -> Charset {
		return Charset{};
	}

	auto Charset::from(std::span<const char32_t> characters) noexcept -> std::optional<Charset> {
		Charset charset {};
		try {
			charset.m_characters = std::vector<char32_t> (std::from_range, characters);
		}
		catch (...) {
			return std::nullopt;
		}
		return charset;
	}

	auto Charset::clone() const noexcept -> std::optional<Charset> {
		try {
			Charset charset {};
			charset.m_characters = m_characters;
			return charset;
		}
		catch (...) {
			return std::nullopt;
		}
	}
}
