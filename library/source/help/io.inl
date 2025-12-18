/**************************
 * @file        io.inl
 * @version     6.0
 * @date        2023-09-24
 * @author      maks.angels@mail.ru
 * @copyright   © 2021–2025 Maksim Andreevich Leonov
 *
 * This file is part of MSAPI.
 * License: see LICENSE.md
 * Contributor terms: see CONTRIBUTING.md
 *
 * This software is licensed under the Polyform Noncommercial License 1.0.0.
 * You may use, copy, modify, and distribute it for noncommercial purposes only.
 *
 * For commercial use, please contact: maks.angels@mail.ru
 *
 * Required Notice: MSAPI, copyright © 2021–2025 Maksim Andreevich Leonov, maks.angels@mail.ru
 *
 * @brief Common functions for working with files and directories.
 */

#ifndef MSAPI_IO_H
#define MSAPI_IO_H

#include "../help/log.h"
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <string>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <unistd.h>

namespace MSAPI {

namespace IO {

/*---------------------------------------------------------------------------------
Declarations
---------------------------------------------------------------------------------*/

namespace FileDescriptor {

/**************************
 * @brief RAII wrapper for POSIX file descriptor.
 */
struct ExitGuard {
	int32_t value{ -1 };

	/**
	 * @brief Open POSIX file descriptor.
	 *
	 * @attention Check value member for success after calling.
	 *
	 * @tparam T Type of path.
	 *
	 * @param path Full path to file.
	 * @param flags File access flags.
	 * @param mode File access mode in octal format.
	 *
	 * @test Has unit tests.
	 */
	template <typename T>
		requires StringableView<T>
	FORCE_INLINE ExitGuard(const T path, const int32_t flags, const int32_t mode) noexcept
		: value{ open(CString(path), flags, mode) }
	{
	}

	/**************************
	 * @brief Default constructor.
	 *
	 * @test Has unit tests.
	 */
	FORCE_INLINE ExitGuard() noexcept = default;

	const ExitGuard& operator=(const ExitGuard&) = delete;
	ExitGuard(const ExitGuard&) = delete;

	/**
	 * @brief Exchange file descriptor ownership. It is expected that moved from object will be destroyed soon.
	 *
	 * @test Has unit tests.
	 */
	FORCE_INLINE const ExitGuard& operator=(ExitGuard&& other) noexcept;

	/**
	 * @brief Exchange file descriptor ownership. It is expected that moved from object will be destroyed soon.
	 *
	 * @test Has unit tests.
	 */
	FORCE_INLINE ExitGuard(ExitGuard&& other) noexcept;

	/**************************
	 * @brief Call Clear on destruction.
	 *
	 * @test Has unit tests.
	 */
	FORCE_INLINE ~ExitGuard();

	/**************************
	 * @brief Close file descriptor if it's valid and set value to -1.
	 *
	 * @test Has unit tests.
	 */
	FORCE_INLINE void Clear();
};

} // namespace FileDescriptor

namespace Directory {

/**************************
 * @brief RAII wrapper for POSIX directory.
 */
struct ExitGuard {
	DIR* value{};

	/**
	 * @brief Open directory.
	 *
	 * @attention Check value member for success after calling.
	 *
	 * @tparam T Type of path.
	 *
	 * @param path Full path to directory.
	 *
	 * @test Has unit tests.
	 */
	template <typename T>
		requires StringableView<T>
	FORCE_INLINE ExitGuard(const T path) noexcept
		: value{ opendir(CString(path)) }
	{
	}

	ExitGuard() noexcept = delete;
	ExitGuard(const ExitGuard&) = delete;
	ExitGuard(ExitGuard&& other) noexcept = delete;
	const ExitGuard& operator=(const ExitGuard&) = delete;
	const ExitGuard& operator=(ExitGuard&& other) noexcept = delete;

	/**
	 * @brief Close directory on destruction if it's opened and print error if occurs.
	 *
	 * @test Has unit tests.
	 */
	FORCE_INLINE ~ExitGuard();
};

} // namespace Directory

/**************************
 * @brief Rename file.
 *
 * @attention Directories in path must exist.
 *
 * @tparam T Type of current name.
 * @tparam S Type of new name.
 *
 * @param currentName Full path of current name.
 * @param newName Full path of new name.
 *
 * @return True if rename was successful, false otherwise.
 *
 * @test Has unit tests.
 */
template <typename T, typename S>
	requires StringableView<T> && StringableView<S>
FORCE_INLINE [[nodiscard]] bool RenameFile(const T currentName, const S newName)
{
	if (rename(CString(currentName), CString(newName)) == 0) [[likely]] {
		LOG_DEBUG_NEW("File renaming from {} to {} is successful", currentName, newName);
		return true;
	}

	LOG_ERROR_NEW(
		"File renaming from {} to {} is failed. Error №{}: {}", currentName, newName, errno, std::strerror(errno));
	return false;
}

/**************************
 * @brief Check if file or directory exists by access function.
 *
 * @param path Full path.
 *
 * @tparam T Type of path.
 *
 * @return True if path exists, false otherwise and error is printed.
 *
 * @test Has unit tests.
 */
template <typename T>
	requires StringableView<T>
FORCE_INLINE [[nodiscard]] bool HasPath(const T path)
{
	if (access(CString(path), F_OK) == 0) [[likely]] {
		return true;
	}

	if (errno != ENOENT) [[unlikely]] {
		LOG_ERROR_NEW("Cannot access path: {}. Error №{}: {}", path, errno, std::strerror(errno));
	}

	return false;
}

/**************************
 * @brief Suggest flags for open() function. Default flags are O_WRONLY and O_CREAT.
 *
 * @param append If true, adds O_APPEND, and O_TRUNC otherwise.
 *
 * @return Suggested flags.
 *
 * @test Has unit tests.
 */
consteval int32_t SuggestFlags(const bool append);

/**************************
 * @brief Save binary data in file.
 *
 * @attention Directories in path must exist. If file descriptor is passed, it must be valid.
 *
 * @tparam Append If true, data will be appended to the file and overwritten otherwise, default is false.
 * @tparam Mode File access mode in octal format, default is 0644.
 * @tparam Multiple Call is part of multiple save operations, default is false.
 * @tparam T Type of object.
 * @tparam S Type of path or file descriptor.
 *
 * @param object Object for saving.
 * @param pathOrFd Full path to file or file descriptor.
 *
 * @return True if save was successful, false otherwise.
 *
 * @test Has unit tests.
 */
template <bool Append = false, int32_t Mode = 0644, bool Multiple = false, typename T, typename S>
	requires((std::is_pointer_v<std::remove_cvref_t<T>> || std::is_reference_v<T>)
		&& (std::is_same_v<S, int32_t> || StringableView<S>))
FORCE_INLINE [[nodiscard]] bool SaveBinary(T&& object, const S pathOrFd)
{
	FileDescriptor::ExitGuard fd{};
	int32_t file;

	if constexpr (StringableView<S>) {
		fd = FileDescriptor::ExitGuard{ pathOrFd, SuggestFlags(Append), Mode };
		if (fd.value == -1) [[unlikely]] {
			LOG_ERROR_NEW("Can't open file: {}. Error №{}: {}", pathOrFd, errno, std::strerror(errno));
			return false;
		}

		file = fd.value;
	}
	else {
		file = pathOrFd;

		if constexpr (!Multiple) {
			if constexpr (Append) {
				if (lseek(file, 0, SEEK_END) == -1) [[unlikely]] {
					LOG_ERROR_NEW(
						"Failed to seek to end of file: {}. Error №{}: {}", pathOrFd, errno, std::strerror(errno));
					return false;
				}
			}
			else {
				if (ftruncate(file, 0) == -1) [[unlikely]] {
					LOG_ERROR_NEW("Failed to truncate file: {}. Error №{}: {}", pathOrFd, errno, std::strerror(errno));
					return false;
				}

				if (lseek(file, 0, SEEK_SET) == -1) [[unlikely]] {
					LOG_ERROR_NEW(
						"Failed to seek to start of file: {}. Error №{}: {}", pathOrFd, errno, std::strerror(errno));
					return false;
				}
			}
		}
	}

	const void* data;
	constexpr auto size{ sizeof(std::remove_pointer_t<std::remove_reference_t<T>>) };
	if constexpr (std::is_pointer_v<std::remove_cvref_t<T>>) {
		data = reinterpret_cast<const void*>(object);
	}
	else if constexpr (std::is_reference_v<T>) {
		data = reinterpret_cast<const void*>(&object);
	}
	else {
		static_assert(sizeof(T) + 1 == 0, "Type of object to save must be pointer or reference");
	}

	const auto result{ write(file, data, size) };
	if (result == -1) [[unlikely]] {
		LOG_ERROR_NEW("Write failed for file: {}. Error №{}: {}", pathOrFd, errno, std::strerror(errno));
		return false;
	}

	if (UINT64(result) != size) [[unlikely]] {
		LOG_ERROR_NEW("Written size {} is not equal to object size {} for file: {}", result, size, pathOrFd);
		return false;
	}

	if constexpr (!Multiple) {
		if constexpr (Append) {
			LOG_DEBUG_NEW("Saved binary file in append mode: {}, size: {}", pathOrFd, size);
		}
		else {
			LOG_DEBUG_NEW("Saved binary file: {}, size: {}", pathOrFd, size);
		}
	}

	return true;
}

constexpr bool append = true;
constexpr bool overwrite = false;

constexpr bool multiple = true;
constexpr bool single = false;

/**************************
 * @brief Save array of binary data in file.
 *
 * @attention Directories in path must exist. If file descriptor is passed, it must be valid.
 *
 * @tparam Append If true, data will be appended to the file and overwritten otherwise, default is false.
 * @tparam Mode File access mode in octal format, default is 0644.
 * @tparam T Type of forward container.
 * @tparam S Type of path or file descriptor.
 *
 * @param objects Forward container of objects for saving.
 * @param pathOrFd Full path to file or file descriptor.
 *
 * @return True if saves were successful, false otherwise.
 *
 * @test Has unit tests.
 */
template <bool Append = false, int32_t Mode = 0644, typename T, typename S>
	requires(std::forward_iterator<typename T::iterator> && (std::is_same_v<S, int32_t> || StringableView<S>))
FORCE_INLINE [[nodiscard]] bool SaveBinaries(const T& objects, const S pathOrFd)
{
	FileDescriptor::ExitGuard fd{};
	int32_t file;

	if constexpr (StringableView<S>) {
		fd = FileDescriptor::ExitGuard{ pathOrFd, SuggestFlags(Append), Mode };
		if (fd.value == -1) [[unlikely]] {
			LOG_ERROR_NEW("Can't open file: {}. Error №{}: {}", pathOrFd, errno, std::strerror(errno));
			return false;
		}

		file = fd.value;
	}
	else {
		file = pathOrFd;

		if constexpr (Append) {
			if (lseek(file, 0, SEEK_END) == -1) [[unlikely]] {
				LOG_ERROR_NEW(
					"Failed to seek to end of file: {}. Error №{}: {}", pathOrFd, errno, std::strerror(errno));
				return false;
			}
		}
		else {
			if (ftruncate(file, 0) == -1) [[unlikely]] {
				LOG_ERROR_NEW("Failed to truncate file: {}. Error №{}: {}", pathOrFd, errno, std::strerror(errno));
				return false;
			}

			if (lseek(file, 0, SEEK_SET) == -1) [[unlikely]] {
				LOG_ERROR_NEW(
					"Failed to seek to start of file: {}. Error №{}: {}", pathOrFd, errno, std::strerror(errno));
				return false;
			}
		}
	}

	uint64_t savedItems{ 0 };
	for (const auto& item : objects) {
		if (SaveBinary<append, Mode, multiple>(item, file)) {
			++savedItems;
		}
	}

	if (savedItems != objects.size()) [[unlikely]] {
		LOG_WARNING_NEW(
			"Saved items {} is not equal to total items {} for file: {}.", savedItems, objects.size(), pathOrFd);
		return false;
	}

	LOG_DEBUG_NEW("Saved binary file {} with {} items", pathOrFd, savedItems);
	return true;
}

/**************************
 * @brief Suggest maximum size of primitive string representation for SavePrimitives function.
 *
 * @tparam T Type of primitive object.
 * @tparam PSM Provided maximum size of primitive string representation.
 *
 * @return Suggested maximum size of primitive string representation. For FP > 4 bytes and integer types > 8 bytes it is
 * maximum between provided PSM and 32.
 *
 * @test Has unit tests.
 */
template <typename T, uint64_t PSM>
	requires(std::is_integral_v<T> || std::is_floating_point_v<T>)
consteval uint64_t SuggestPsm()
{
	constexpr auto size = sizeof(T);

	if (std::is_integral_v<T>) {
		if (std::is_same_v<T, bool>) {
			return 4;
		}

		if (std::is_same_v<T, char>) {
			return 1;
		}

		if (std::is_signed_v<T>) {
			if (size == 1) {
				return 4;
			}
			if (size == 2) {
				return 6;
			}
			if (size == 4) {
				return 11;
			}
			if (size == 8) {
				return 20;
			}
		}

		if (size == 1) {
			return 3;
		}
		if (size == 2) {
			return 5;
		}
		if (size == 4) {
			return 10;
		}
		if (size == 8) {
			return 20;
		}
	}

	if (std::is_same_v<T, float>) {
		return 14;
	}

	return std::max(PSM, 32ul);
}

/**************************
 * @brief Save primitive type objects in file with specific separator.
 *
 * @attention Directories in path must exist. If file descriptor is passed, it must be valid.
 *
 * @tparam Append If true, data will be appended to the file with new line separator, and overwritten otherwise. Default
 * is false.
 * @tparam Mode File access mode in octal format, default is 0644.
 * @tparam Buffer Size of internal buffer, default is 512.
 * @tparam PSM Maximum size of primitive string representation, default and minimum is 32. Used for FP > 4 bytes and
 * integer types > 8 bytes, for another types this value is calculated automatically.
 * @tparam T Type of forward container.
 * @tparam TT Type of primitive object.
 * @tparam S Type of path or file descriptor.
 *
 * @param objects Forward container of primitive types for saving.
 * @param pathOrFd Full path to file or file descriptor.
 * @param separator Separator character.
 *
 * @return True if saves were successful or empty container, false otherwise.
 *
 * @test Has unit tests.
 */
template <bool Append = false, int32_t Mode = 0644, uint64_t Buffer = 512, uint64_t PSM = 32,
	template <typename> typename T, typename TT, typename S>
	requires(std::forward_iterator<typename T<TT>::iterator> && (std::is_same_v<S, int32_t> || StringableView<S>)
		&& (std::is_integral_v<TT> || std::is_floating_point_v<TT>))
[[nodiscard]] bool SavePrimitives(const T<TT>& objects, const S pathOrFd, const char separator)
{
	constexpr uint64_t suggestedPsm{ SuggestPsm<TT, PSM>() };
	static_assert(Buffer > suggestedPsm, "Buffer size must be greater than suggestedPsm");

	if (objects.empty()) {
		return true;
	}

	FileDescriptor::ExitGuard fd{};
	int32_t file;

	if constexpr (StringableView<S>) {
		fd = FileDescriptor::ExitGuard{ pathOrFd, SuggestFlags(Append), Mode };
		if (fd.value == -1) [[unlikely]] {
			LOG_ERROR_NEW("Can't open file: {}. Error №{}: {}", pathOrFd, errno, std::strerror(errno));
			return false;
		}

		file = fd.value;
	}
	else {
		file = pathOrFd;
	}

	if constexpr (Append) {
		const auto pos{ lseek(file, 0, SEEK_END) };

		if (pos == -1) [[unlikely]] {
			LOG_ERROR_NEW("Failed to seek to end of file: {}. Error №{}: {}", pathOrFd, errno, std::strerror(errno));
			return false;
		}

		if (pos > 0) {
			if (write(file, "\n", 1) != 1) [[unlikely]] {
				LOG_ERROR_NEW("Failed to write newline to {}. Error №{}: {}", pathOrFd, errno, std::strerror(errno));
				return false;
			}
		}
	}
	else if constexpr (std::is_same_v<S, int32_t>) {
		if (ftruncate(file, 0) == -1) [[unlikely]] {
			LOG_ERROR_NEW("Failed to truncate file: {}. Error №{}: {}", pathOrFd, errno, std::strerror(errno));
			return false;
		}

		if (lseek(file, 0, SEEK_SET) == -1) [[unlikely]] {
			LOG_ERROR_NEW("Failed to seek to start of file: {}. Error №{}: {}", pathOrFd, errno, std::strerror(errno));
			return false;
		}
	}

	auto begin{ objects.begin() };
	auto end{ objects.end() };

	char buffer[Buffer];
	uint64_t offset{};
	char* writtenEnd;

#define TMP_MSAPI_IO_SAVE_PRIMITIVES                                                                                   \
	if constexpr (std::is_integral_v<TT>) {                                                                            \
		writtenEnd = std::format_to(buffer + offset, "{}", *begin);                                                    \
	}                                                                                                                  \
	else if constexpr (std::is_floating_point_v<TT>) {                                                                 \
		if constexpr (std::is_same_v<TT, float>) {                                                                     \
			writtenEnd = std::format_to(buffer + offset, "{:.9f}", *begin);                                            \
		}                                                                                                              \
		else if constexpr (std::is_same_v<TT, double>) {                                                               \
			writtenEnd = std::format_to(buffer + offset, "{:.17f}", *begin);                                           \
		}                                                                                                              \
		else {                                                                                                         \
			writtenEnd = std::format_to(buffer + offset, "{:.21Lf}", *begin);                                          \
		}                                                                                                              \
	}                                                                                                                  \
	else {                                                                                                             \
		static_assert(sizeof(TT) + 1 == 0, "Type of object to save must be primitive");                                \
	}                                                                                                                  \
	offset += UINT64(writtenEnd - (buffer + offset));

	TMP_MSAPI_IO_SAVE_PRIMITIVES;

	while (++begin != end) {
		buffer[offset] = separator;
		++offset;

		TMP_MSAPI_IO_SAVE_PRIMITIVES;

		if (offset >= Buffer - suggestedPsm) {
#define TMP_MSAPI_IO_BUFFER_FLUSH                                                                                      \
	auto result{ write(file, buffer, offset) };                                                                        \
	if (result == -1) [[unlikely]] {                                                                                   \
		LOG_ERROR_NEW("Write failed for file: {}. Error №{}: {}", pathOrFd, errno, std::strerror(errno));              \
		return false;                                                                                                  \
	}                                                                                                                  \
                                                                                                                       \
	if (UINT64(result) != offset) [[unlikely]] {                                                                       \
		LOG_ERROR_NEW("Written size {} is not equal to buffer size {} for file: {}", result, offset, pathOrFd);        \
		return false;                                                                                                  \
	}

			TMP_MSAPI_IO_BUFFER_FLUSH;

			offset = 0;
		}
	}

	TMP_MSAPI_IO_BUFFER_FLUSH;
#undef TMP_MSAPI_IO_BUFFER_FLUSH
#undef TMP_MSAPI_IO_SAVE_PRIMITIVES

	LOG_DEBUG_NEW("Saved file {} with {} items", pathOrFd, objects.size());
	return true;
}

/**************************
 * @brief Save string in file.
 *
 * @attention Directories in path must exist.
 *
 * @tparam Append If true, data will be appended to the file with new line separator, and overwritten otherwise. Default
 * is false.
 * @tparam Mode File access mode in octal format, default is 0644.
 * @tparam T Type of path.
 *
 * @param str String for saving.
 * @param path Full path to file.
 *
 * @return True if save was successful, false otherwise.
 *
 * @test Has unit tests.
 */
template <bool Append = false, int32_t Mode = 0644, typename T>
	requires StringableView<T>
FORCE_INLINE [[nodiscard]] bool SaveStr(const std::string_view str, const T path)
{
	FileDescriptor::ExitGuard fd{ path, SuggestFlags(Append), Mode };

	if (fd.value == -1) [[unlikely]] {
		LOG_ERROR_NEW("Failed to open file: {}. Error №{}: {}", path, errno, std::strerror(errno));
		return false;
	}

	if constexpr (Append) {
		const auto pos{ lseek(fd.value, 0, SEEK_END) };
		if (pos == -1) [[unlikely]] {
			LOG_ERROR_NEW("Failed to seek to end of file: {}. Error №{}: {}", path, errno, std::strerror(errno));
			return false;
		}

		if (pos > 0) {
			if (write(fd.value, "\n", 1) != 1) [[unlikely]] {
				LOG_ERROR_NEW("Failed to write newline to {}. Error №{}: {}", path, errno, std::strerror(errno));
				return false;
			}
		}
	}

	const auto size{ str.size() };
	const auto result{ write(fd.value, str.data(), size) };
	if (result == -1) [[unlikely]] {
		LOG_ERROR_NEW("Failed to write in file: {}. Error №{}: {}", path, errno, std::strerror(errno));
		return false;
	}

	if (UINT64(result) != size) [[unlikely]] {
		LOG_ERROR_NEW("Written size {} is not equal to string size {} for file: {}", result, size, path);
		return false;
	}

	if constexpr (Append) {
		LOG_DEBUG_NEW("Saved str file in append mode: {} with size {}", path, size);
	}
	else {
		LOG_DEBUG_NEW("Saved str file: {} with size {}", path, size);
	}
	return true;
}

/**************************
 * @brief Read binary data from file.
 *
 * @param ptr Pointer to buffer.
 * @param path Full path to file.
 *
 * @return True if read was successful, false otherwise.
 *
 * @test Has unit tests.
 */
template <typename T, typename S>
	requires Pointable<T> && StringableView<S>
[[nodiscard]] bool ReadBinary(T&& object, const S path)
{
	if (!HasPath(path)) [[unlikely]] {
		LOG_ERROR_NEW("Can't find file to read data: {}", path);
		return false;
	}

	FileDescriptor::ExitGuard fd{ path, O_RDONLY, 0 };
	if (fd.value == -1) [[unlikely]] {
		LOG_ERROR_NEW("Can't open file to read data: {}. Error №{}: {}", path, errno, std::strerror(errno));
		return false;
	}

	constexpr auto size{ sizeof(std::remove_pointer_t<std::remove_reference_t<T>>) };
	const auto result{ read(fd.value, Pointer(object), size) };
	if (result == -1) [[unlikely]] {
		LOG_ERROR_NEW("Can't read data: {}. Error №{}: {}", path, errno, std::strerror(errno));
		return false;
	}

	if (UINT64(result) != size) [[unlikely]] {
		LOG_ERROR_NEW("Read size {} is not equal to object size {} for file: {}", result, size, path);
		return false;
	}

	return true;
}

/**************************
 * @brief Read array of binary data from file.
 *
 * @param container Container for reading data.
 * @param path Full path to file.
 *
 * @tparam T Type of forward container.
 * @tparam S Type of object.
 * @tparam N Type of path.
 *
 * @return True if read was successful, false otherwise.
 *
 * @test Has unit tests.
 */
template <template <typename> typename T, typename S, typename N>
	requires StringableView<N>
[[nodiscard]] bool ReadBinaries(T<S>& container, const N path)
{
	if (!HasPath(path)) [[unlikely]] {
		LOG_ERROR_NEW("Can't find file to read data: {}", path);
		return false;
	}

	FileDescriptor::ExitGuard fd{ path, O_RDONLY, 0 };
	if (fd.value == -1) [[unlikely]] {
		LOG_ERROR_NEW("Can't open file to read data: {}. Error №{}: {}", path, errno, std::strerror(errno));
		return false;
	}

	S item;
	while (true) {
		const auto result{ read(fd.value, &item, sizeof(S)) };
		if (result == -1) [[unlikely]] {
			LOG_ERROR_NEW("Can't read data: {}. Error №{}: {}", path, errno, std::strerror(errno));
			return {};
		}

		if (result == 0) {
			break;
		}

		if (UINT64(result) != sizeof(S)) [[unlikely]] {
			LOG_ERROR_NEW("Read size {} of object №{} is not equal to object size {} for file: {}.", result,
				container.size(), sizeof(S), path);
			return {};
		}

		container.emplace_back(std::move(item));
	}

	LOG_DEBUG_NEW("Read binary file: {} with {} items", path, container.size());
	return true;
}

/**************************
 * @brief Read string until end of the file.
 *
 * @param str String for reading.
 * @param path Full path to file.
 *
 * @tparam T Type of path.
 *
 * @return True if read was successful, false otherwise.
 *
 * @test Has unit tests.
 */
template <typename T>
	requires StringableView<T>
FORCE_INLINE [[nodiscard]] bool ReadStr(std::string& str, const T path)
{
	if (!HasPath(path)) [[unlikely]] {
		LOG_ERROR_NEW("Can't find file to read data: {}", path);
		return false;
	}

	FileDescriptor::ExitGuard fd{ path, O_RDONLY, 0 };
	if (fd.value == -1) [[unlikely]] {
		LOG_ERROR_NEW("Can't open file to read data: {}. Error №{}: {}", path, errno, std::strerror(errno));
		return false;
	}

	struct stat st { };
	if (fstat(fd.value, &st) != 0) [[unlikely]] {
		LOG_ERROR_NEW("Can't get file size for: {}. Error №{}: {}", path, errno, std::strerror(errno));
		return false;
	}

	if (st.st_size == 0) {
		str.clear();
		LOG_DEBUG_NEW("Read str file: {} with size 0", path);
		return true;
	}

	const auto size{ UINT64(st.st_size) };
	str.resize(size);
	uint64_t totalRead{ 0 };
	char* const buffer{ str.data() };

	while (totalRead < size) {
		const auto result{ read(fd.value, buffer + totalRead, size - totalRead) };
		if (result == 0) {
			break;
		}

		if (result == -1) [[unlikely]] {
			if (errno == EINTR) {
				LOG_DEBUG("Read interrupted by signal EINTR, continuing");
				continue;
			}

			LOG_ERROR_NEW("Can't read data: {}. Error №{}: {}", path, errno, std::strerror(errno));
			return false;
		}

		totalRead += UINT64(result);
	}

	if (totalRead != size) [[unlikely]] {
		str.resize(totalRead);
		LOG_WARNING_NEW("Read size {} is not equal to file size {} for file: {}", totalRead, size, path);
		return false;
	}

	LOG_DEBUG_NEW("Read str file: {} with size {}", path, str.size());
	return true;
}

/**************************
 * @brief Remove file or directory with all its content.
 *
 * @param path Full path.
 *
 * @tparam Buffer Size of internal buffer, default is 512.
 *
 * @return True if removing was successful, false otherwise.
 *
 * @test Has unit tests.
 */
template <uint64_t Buffer = 512> FORCE_INLINE [[nodiscard]] bool Remove(const std::string_view path)
{
	if (path.size() < 2) [[unlikely]] {
		LOG_WARNING_NEW("Invalid path to be removed: {}", path);
		return false;
	}

	if (path.size() + 1 >= Buffer) [[unlikely]] {
		LOG_WARNING_NEW("Path size {} exceeds internal buffer size {}: {}", path.size(), Buffer, path);
		return false;
	}

	char buffer[Buffer];
	const auto size{ path.size() };
	memcpy(buffer, path.data(), size + 1);

	auto remove{ [&buffer](uint64_t offset, auto&& self) -> bool {
		struct stat st { };
		if (lstat(buffer, &st) != 0) [[unlikely]] {
			LOG_ERROR_NEW(
				"Can't get info for path {} to be removed. Error №{}: {}", buffer, errno, std::strerror(errno));
			return false;
		}

		if (!S_ISDIR(st.st_mode)) {
			if (unlink(buffer) != 0) [[unlikely]] {
				LOG_ERROR_NEW("File {} is not removed. Error №{}: {}", buffer, errno, std::strerror(errno));
				return false;
			}

			return true;
		}

		Directory::ExitGuard guardDir{ buffer };
		if (guardDir.value == nullptr) [[unlikely]] {
			LOG_ERROR_NEW(
				"Error opening directory {} to be removed. Error №{}: {}", buffer, errno, std::strerror(errno));
			return false;
		}

		if (buffer[offset - 1] != '/') {
			buffer[offset] = '/';
			++offset;
		}

		bool result{ true };
		struct dirent* ent;
		while ((ent = readdir(guardDir.value)) != nullptr) {
			if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
				continue;
			}

			const auto childSize{ std::char_traits<char>::length(ent->d_name) };
			const auto newOffset{ offset + childSize };
			if (newOffset + 1 >= Buffer) [[unlikely]] {
				LOG_ERROR_NEW("Path size exceeds internal buffer size {}: {}", Buffer, buffer);
				result = false;
				break;
			}

			memcpy(buffer + offset, ent->d_name, childSize + 1);

			if (!self(newOffset, self)) [[unlikely]] {
				result = false;
				break;
			}

			memset(buffer + offset, 0, Buffer - offset - 1);
		}

		if (buffer[offset] != '\0') {
			buffer[offset] = '\0';
		}

		if (!result) {
			return false;
		}

		if (rmdir(buffer) != 0) [[unlikely]] {
			LOG_ERROR_NEW("Directory {} is not removed. Error №{}: {}", buffer, errno, std::strerror(errno));
			return false;
		}

		return true;
	} };

	if (!remove(size, remove)) [[unlikely]] {
		return false;
	}

	LOG_DEBUG_NEW("Path {} is removed successfully", path);
	return true;
}

/**************************
 * @brief Copy file.
 *
 * @attention Directories in path must exist.
 *
 * @param from Full path to source file.
 * @param to Full path to file to copy.
 *
 * @tparam T Type of source path.
 * @tparam S Type of destination path.
 *
 * @return True if copying was successful, false otherwise.
 *
 * @test Has unit tests.
 */
template <typename T, typename S>
	requires StringableView<T> && StringableView<S>
[[nodiscard]] bool CopyFile(const T from, const S to)
{
	FileDescriptor::ExitGuard guardFdFrom{ from, O_RDONLY, 0 };
	if (guardFdFrom.value == -1) [[unlikely]] {
		LOG_ERROR_NEW("Can't open file to read data: {}. Error №{}: {}", from, errno, std::strerror(errno));
		return false;
	}

	FileDescriptor::ExitGuard guardFdTo{ to, O_WRONLY | O_CREAT | O_TRUNC, 0644 };
	if (guardFdTo.value == -1) [[unlikely]] {
		LOG_ERROR_NEW("Can't open file to save data: {}. Error №{}: {}", to, errno, std::strerror(errno));
		return false;
	}

	struct stat st { };
	if (fstat(guardFdFrom.value, &st) != 0) [[unlikely]] {
		LOG_ERROR_NEW("Failed to get file size for {}. Error №{}: {}", from, errno, std::strerror(errno));
		return false;
	}

	if (st.st_size == 0) {
		LOG_DEBUG_NEW("Source file {} is empty, created empty file {}", from, to);
		if (write(guardFdTo.value, "", 0) == -1) [[unlikely]] {
			LOG_ERROR_NEW("Failed to create empty file {}. Error №{}: {}", to, errno, std::strerror(errno));
			return false;
		}

		return true;
	}

	if (S_ISREG(st.st_mode) == 0) [[unlikely]] {
		LOG_WARNING_NEW("Source file {} is not a regular file", from);
		return false;
	}

	off_t offset{ 0 };
	const off_t total{ st.st_size };
	while (offset < total) {
		const auto result{ sendfile(guardFdTo.value, guardFdFrom.value, &offset, UINT64(total - offset)) };
		if (result == 0) {
			break;
		}

		if (result == -1) [[unlikely]] {
			if (errno == EINTR) {
				LOG_DEBUG("Sendfile interrupted by signal EINTR, continuing");
				continue;
			}

			LOG_ERROR_NEW("Sendfile failed during file copy from {} to {}. "
						  "Error №{}: {}",
				from, to, errno, std::strerror(errno));
			return false;
		}
	}

	LOG_DEBUG_NEW("Copied file from {} to {} using sendfile, size {}", from, to, st.st_size);
	return true;
}

/**************************
 * @brief Create directory with all parent directories.
 *
 * @param path Full path to directory.
 *
 * @tparam T Type of path.
 * @tparam Mode Directory access mode in octal format, default is 0755.
 * @tparam Buffer Size of internal buffer, default is 512.
 *
 * @return True if directory was created, false otherwise.
 *
 * @test Has unit tests.
 */
template <typename T, int32_t Mode = 0755, uint64_t Buffer = 512>
	requires StringableView<T>
FORCE_INLINE [[nodiscard]] bool CreateDir(const T path)
{
	const char* const cpath{ CString(path) };
	if (cpath == nullptr || cpath[0] == '\0') [[unlikely]] {
		LOG_ERROR_NEW("Dir {} is not created. Empty path is provided", path);
		return false;
	}

	const auto size{ std::char_traits<char>::length(cpath) };
	if (size >= Buffer) [[unlikely]] {
		LOG_ERROR_NEW("Dir {} is not created. Path is too long {} >= {}", path, size, Buffer);
		return false;
	}

	auto ensureDir{ [](const char* dir) -> bool {
		struct stat st { };
		if (stat(dir, &st) == 0) {
			if (!S_ISDIR(st.st_mode)) [[unlikely]] {
				LOG_ERROR_NEW("Path {} exists and is not a directory", dir);
				return false;
			}

			return true;
		}

		if (errno != ENOENT) [[unlikely]] {
			LOG_ERROR_NEW("Dir {} is not created. Error №{}: {}", dir, errno, std::strerror(errno));
			return false;
		}

		if (mkdir(dir, Mode) != 0 && errno != EEXIST) [[unlikely]] {
			LOG_ERROR_NEW("Dir {} is not created. Error №{}: {}", dir, errno, std::strerror(errno));
			return false;
		}

		return true;
	} };

	char buffer[Buffer];
	memcpy(buffer, cpath, size + 1);

	for (uint64_t index{ buffer[0] == '/' ? UINT64(1) : UINT64(0) };;) {
		if (buffer[index] == '/') {
			buffer[index] = '\0';
			if (!ensureDir(buffer)) [[unlikely]] {
				return false;
			}
			buffer[index] = '/';
		}

		if (++index == size - 1) {
			if (!ensureDir(buffer)) [[unlikely]] {
				return false;
			}
			break;
		}
	}

	LOG_DEBUG_NEW("Dir {} is created successfully", path);
	return true;
}

/**************************
 * @brief Linux file types enumeration.
 */
enum class FileType : int16_t {
	Unknown = DT_UNKNOWN,
	Fifo = DT_FIFO,
	Char = DT_CHR,
	Directory = DT_DIR,
	Blk = DT_BLK,
	Regular = DT_REG,
	Lnk = DT_LNK,
	Sock = DT_SOCK
};

/**************************
 * @return Reinterpretation of FileType enum to string.
 *
 * @param type FileType enum value.
 *
 * @test Has unit tests.
 */
FORCE_INLINE [[nodiscard]] constexpr std::string_view EnumToString(const FileType type);

/**************************
 * @brief List directory content with specific type and append to provided container. "." and ".." are excluded from
 * results for tables.
 *
 * @attention Content sorting is filesystem dependent.
 *
 * @tparam FT Type of file to search.
 * @tparam T with strings and emplace_back method.
 * @tparam S Type of path.
 *
 * @param container Container to store results.
 * @param path Full path for parsing file names.
 *
 * @return True if read was successful, false otherwise.
 *
 * @test Has unit tests.
 *
 * @todo Add ability to provide not a container, but a callback to be called for each found item.
 */
template <FileType FT, template <typename> typename T, typename S>
	requires StringableView<S>
FORCE_INLINE [[nodiscard]] bool List(T<std::string>& container, const S path)
{
	Directory::ExitGuard guardDir{ path };
	if (guardDir.value == nullptr) [[unlikely]] {
		LOG_ERROR_NEW("Error opening directory: {}. Error №{}: {}", path, errno, std::strerror(errno));
		return false;
	}

	struct dirent* ent;
	while ((ent = readdir(guardDir.value)) != nullptr) {
		if constexpr (U(FT) == DT_DIR) {
			if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) [[unlikely]] {
				continue;
			}
		}

		if (ent->d_type != U(FT)) {
			continue;
		}

		container.emplace_back(std::string{ ent->d_name });
	}

	return true;
}

/*---------------------------------------------------------------------------------
Definitions
---------------------------------------------------------------------------------*/

namespace FileDescriptor {

FORCE_INLINE const ExitGuard& ExitGuard::operator=(ExitGuard&& other) noexcept
{
	std::swap(value, other.value);
	return *this;
}

FORCE_INLINE ExitGuard::ExitGuard(ExitGuard&& other) noexcept { std::swap(value, other.value); }

FORCE_INLINE ExitGuard::~ExitGuard() { Clear(); }

FORCE_INLINE void ExitGuard::Clear()
{
	if (value != -1) {
		if (close(value) == -1) [[unlikely]] {
			LOG_ERROR_NEW("File descriptor close fail. Error №{}: {}", errno, std::strerror(errno));
		}
		value = -1;
	}
}

}; //* namespace FileDescriptor

namespace Directory {

FORCE_INLINE ExitGuard::~ExitGuard()
{
	if (value != nullptr) {
		if (closedir(value) != 0) [[unlikely]] {
			LOG_ERROR_NEW("Failed to close directory. Error №{}: {}", errno, std::strerror(errno));
		}
		value = nullptr;
	}
}

} //* namespace Directory

static_assert(IO::append, "Append global is true");
static_assert(!IO::overwrite, "Overwrite global is false");

static_assert(IO::multiple, "Multiple global is true");
static_assert(!IO::single, "Single global is false");

consteval int32_t SuggestFlags(const bool append)
{
	int32_t flags{ O_WRONLY | O_CREAT };

	if (append) {
		flags |= O_APPEND;
	}
	else {
		flags |= O_TRUNC;
	}

	return flags;
}

static_assert(SuggestFlags(true) == (O_WRONLY | O_CREAT | O_APPEND), "SuggestFlags true failed");
static_assert(SuggestFlags(false) == (O_WRONLY | O_CREAT | O_TRUNC), "SuggestFlags false failed");

static_assert(SuggestPsm<int8_t, 32>() == 4, "PSM for int8");
static_assert(SuggestPsm<uint8_t, 32>() == 3, "PSM for uint8");
static_assert(SuggestPsm<int16_t, 32>() == 6, "PSM for int16");
static_assert(SuggestPsm<uint16_t, 32>() == 5, "PSM for uint16");
static_assert(SuggestPsm<int32_t, 32>() == 11, "PSM for int32");
static_assert(SuggestPsm<uint32_t, 32>() == 10, "PSM for uint32");
static_assert(SuggestPsm<int64_t, 32>() == 20, "PSM for int64");
static_assert(SuggestPsm<uint64_t, 32>() == 20, "PSM for uint64");
static_assert(SuggestPsm<float, 32>() == 14, "PSM for float");
static_assert(SuggestPsm<double, 31>() == 32, "PSM for double, less than minimum");
static_assert(SuggestPsm<double, 33>() == 33, "PSM for double, greater than minimum");
static_assert(SuggestPsm<long double, 31>() == 32, "PSM for long double, less than minimum");
static_assert(SuggestPsm<long double, 33>() == 33, "PSM for long double, greater than minimum");
static_assert(SuggestPsm<bool, 32>() == 4, "PSM for bool");
static_assert(SuggestPsm<char, 32>() == 1, "PSM for char");

FORCE_INLINE [[nodiscard]] constexpr std::string_view EnumToString(const FileType type)
{
	static_assert(U(FileType::Unknown) == 0 && U(FileType::Fifo) == 1 && U(FileType::Char) == 2
			&& U(FileType::Directory) == 4 && U(FileType::Blk) == 6 && U(FileType::Regular) == 8
			&& U(FileType::Lnk) == 10 && U(FileType::Sock) == 12,
		"FileType enum values have been changed, update EnumToString");

	// Must generate a jump table when the case labels are not dense, but short, and fill empty with default case.
	switch (type) {
	case FileType::Unknown:
		return "Unknown";
	case FileType::Fifo:
		return "Fifo";
	case FileType::Char:
		return "Char";
	case FileType::Directory:
		return "Directory";
	case FileType::Blk:
		return "Blk";
	case FileType::Regular:
		return "Regular";
	case FileType::Lnk:
		return "Lnk";
	case FileType::Sock:
		return "Sock";
	default:
		LOG_ERROR_NEW("Unknown type: {}", U(type));
		return "Unknown";
	}
}

static_assert(EnumToString(FileType::Unknown) == "Unknown", "EnumToString Unknown failed");
static_assert(EnumToString(FileType::Fifo) == "Fifo", "EnumToString Fifo failed");
static_assert(EnumToString(FileType::Char) == "Char", "EnumToString Char failed");
static_assert(EnumToString(FileType::Directory) == "Directory", "EnumToString Directory failed");
static_assert(EnumToString(FileType::Blk) == "Blk", "EnumToString Blk failed");
static_assert(EnumToString(FileType::Regular) == "Regular", "EnumToString Regular failed");
static_assert(EnumToString(FileType::Lnk) == "Lnk", "EnumToString Lnk failed");
static_assert(EnumToString(FileType::Sock) == "Sock", "EnumToString Sock failed");

}; //* namespace IO

}; //* namespace MSAPI

#endif //* MSAPI_IO_H