#pragma once

#include <array>
#include <concepts>
#include <cstddef>
#include <expected>
#include <filesystem>
#include <fstream>
#include <functional>
#include <type_traits>
#include <utility>
#include <vector>


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


	enum class FilesystemError {
		eSystem,
		eFile,
	};

	inline auto readBinaryFile(std::filesystem::path filePath) noexcept
		-> std::expected<std::vector<std::byte>, FilesystemError>
	{
		std::error_code errorCode {};
		const std::size_t fileSize {std::filesystem::file_size(filePath, errorCode)};
		if (errorCode)
			return std::unexpected(FilesystemError::eSystem);
		std::vector<std::byte> data (fileSize);
		std::ifstream file {filePath, std::ios::binary};
		if (!file)
			return std::unexpected(FilesystemError::eFile);
		file.read(reinterpret_cast<char*> (data.data()), data.size());
		return data;
	}
}
