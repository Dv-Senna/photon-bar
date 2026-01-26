#pragma once

#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>


namespace photon {
	auto convertUtf8ToCodepoint(std::u8string_view character) noexcept -> std::optional<char32_t>;
	auto convertCodepointToUtf8(char32_t codepoint) noexcept -> std::optional<std::u8string>;

	class Charset final {
		public:
			Charset(const Charset&) = delete;
			auto operator=(const Charset&) -> Charset& = delete;
			constexpr Charset(Charset&&) noexcept = default;
			constexpr auto operator=(Charset&&) noexcept -> Charset& = default;

			static auto create() noexcept -> Charset;
			static auto from(std::span<const char32_t> characters) noexcept -> std::optional<Charset>;

			auto clone() const noexcept -> std::optional<Charset>;

			inline auto has(char32_t character) const noexcept -> bool {
				return std::ranges::find(m_characters, character) != m_characters.end();
			}
			auto getCharacters() const noexcept -> const std::vector<char32_t>& {
				return m_characters;
			}

		private:
			constexpr Charset() noexcept = default;

			std::vector<char32_t> m_characters;
	};
}
