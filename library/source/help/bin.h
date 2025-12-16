/**************************
 * @file        bin.h
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
 *
 * @todo It is possible to save primitives in text file with separator, but reading them is not implemented yet as it is
 * not used. But that can be done in future with techniques from JSON parser.
 */

#ifndef MSAPI_BIN_H
#define MSAPI_BIN_H

#include "../help/log.h"
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <list>
#include <string>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace MSAPI {

namespace Bin {

/**************************
 * @brief RAII wrapper for POSIX file descriptor.
 */
struct FileDescriptor {
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
	 */
	template <typename T>
		requires StringableView<T>
	FORCE_INLINE FileDescriptor(const T path, int32_t flags, int32_t mode) noexcept
		: value{ open(CString(path), flags, mode) }
	{
	}

	FORCE_INLINE FileDescriptor() noexcept = default;

	const FileDescriptor& operator=(const FileDescriptor&) = delete;
	FileDescriptor(const FileDescriptor&) = delete;

	/**
	 * @brief Exchange file descriptor ownership. It is expected that moved from object will be destroyed soon.
	 */
	FORCE_INLINE const FileDescriptor& operator=(FileDescriptor&& other) noexcept
	{
		std::swap(value, other.value);
		return *this;
	}

	/**
	 * @brief Exchange file descriptor ownership. It is expected that moved from object will be destroyed soon.
	 */
	FORCE_INLINE FileDescriptor(FileDescriptor&& other) noexcept { std::swap(value, other.value); }

	/**************************
	 * @brief Call Clear on destruction.
	 */
	FORCE_INLINE ~FileDescriptor() noexcept { Clear(); }

	/**************************
	 * @brief Close file descriptor if it's valid.
	 */
	FORCE_INLINE void Clear() noexcept
	{
		if (value != -1) {
			if (close(value) == -1) {
				LOG_ERROR_NEW("File descriptor close fail. Error №{}: {}", errno, std::strerror(errno));
			}
			value = -1;
		}
	}
};

/**************************
 * @brief Rename file.
 *
 * @attention Directorys in path must exist.
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
FORCE_INLINE bool RenameFile(const T currentName, const S newName)
{
	if (rename(CString(currentName), CString(newName)) == 0) {
		LOG_DEBUG_NEW("File renaming from {} to {} is successful", currentName, newName);
		return true;
	}

	LOG_ERROR_NEW(
		"File renaming from {} to {} is failed. Error №{}: {}", currentName, newName, errno, std::strerror(errno));
	return false;
}

/**************************
 * @brief Check if file exists by access function.
 *
 * @param path Full path to file.
 *
 * @tparam T Type of path.
 *
 * @return True if file exists, false otherwise.
 *
 * @test Has unit tests.
 */
template <typename T>
	requires StringableView<T>
FORCE_INLINE bool HasFile(const T path)
{
	return access(CString(path), F_OK) == 0;
}

template <typename T>
	requires StringableView<T>
FORCE_INLINE bool HasPath(const T path)
{
	return access(CString(path), F_OK) == 0;
}

constexpr bool append = true;
constexpr bool overwrite = false;

constexpr bool enableLog = true;
constexpr bool disableLog = false;

/**************************
 * @brief Save binary data in file.
 *
 * @attention Directorys in path must exist. If file descriptor is passed, it must be valid.
 *
 * @tparam Append If true, data will be appended to the file with new line separator, and overwritten otherwise. Default
 * is false.
 * @tparam Mode File access mode in octal format, default is 0644.
 * @tparam L Log result of save operation, default is true.
 * @tparam T Type of object.
 * @tparam S Type of path or file descriptor.
 *
 * @param object Object for saving.
 * @param pathOrFd Full path to file or file descriptor.
 *
 * @return True if save was successful, false otherwise.
 */
template <bool Append = false, int32_t Mode = 0644, bool L = true, typename T, typename S>
	requires((std::is_pointer_v<std::remove_cvref_t<T>> || std::is_reference_v<T>)
		&& (std::is_same_v<S, int32_t> || StringableView<S>))
FORCE_INLINE bool SaveBinary(T&& object, const S pathOrFd)
{
	FileDescriptor cleaner{};
	int32_t file;

	if constexpr (StringableView<S>) {
		int32_t flags{ O_WRONLY | O_CREAT };
		if constexpr (Append) {
			flags |= O_APPEND;
		}
		else {
			flags |= O_TRUNC;
		}

		cleaner = FileDescriptor{ pathOrFd, flags, Mode };
		if (cleaner.value == -1) {
			LOG_ERROR_NEW("Can't open file: {}. Error №{}: {}", pathOrFd, errno, std::strerror(errno));
			return false;
		}

		file = cleaner.value;
	}
	else {
		file = pathOrFd;

		if constexpr (Append) {
			if (lseek(file, 0, SEEK_END) == -1) {
				LOG_ERROR_NEW(
					"Failed to seek to end of file: {}. Error №{}: {}", pathOrFd, errno, std::strerror(errno));
				return false;
			}
		}
		else {
			if (ftruncate(file, 0) == -1) {
				LOG_ERROR_NEW("Failed to truncate file: {}. Error №{}: {}", pathOrFd, errno, std::strerror(errno));
				return false;
			}

			if (lseek(file, 0, SEEK_SET) == -1) {
				LOG_ERROR_NEW(
					"Failed to seek to start of file: {}. Error №{}: {}", pathOrFd, errno, std::strerror(errno));
				return false;
			}
		}
	}

	const void* data;
	uint64_t size;
	if constexpr (std::is_pointer_v<std::remove_cvref_t<T>>) {
		data = reinterpret_cast<const void*>(object);
		size = sizeof(std::remove_pointer_t<std::remove_reference_t<T>>);
	}
	else if constexpr (std::is_reference_v<T>) {
		data = reinterpret_cast<const void*>(&object);
		size = sizeof(std::remove_reference_t<T>);
	}
	else {
		static_assert(sizeof(T) + 1 == 0, "Type of object to save must be pointer or reference");
	}

	const auto result{ write(file, data, size) };
	if (result == -1) {
		LOG_ERROR_NEW("Write failed for file: {}. Error №{}: {}", pathOrFd, errno, std::strerror(errno));
		return false;
	}

	if (UINT64(result) != size) {
		LOG_ERROR_NEW("Written size {} is not equal to object size {} for file: {}", result, size, pathOrFd);
		return false;
	}

	if constexpr (L) {
		if constexpr (Append) {
			LOG_DEBUG_NEW("Saved bin file in append mode: {}, size: {}", pathOrFd, size);
		}
		else {
			LOG_DEBUG_NEW("Saved bin file: {}, size: {}", pathOrFd, size);
		}
	}

	return true;
}

/**************************
 * @brief Save array of binary data in file.
 *
 * @attention Directorys in path must exist. If file descriptor is passed, it must be valid.
 *
 * @tparam Append If true, data will be appended to the file with new line separator, and overwritten otherwise. Default
 * is false.
 * @tparam Mode File access mode in octal format, default is 0644.
 * @tparam T Type of forward container.
 * @tparam S Type of path or file descriptor.
 *
 * @param objects Forward container of objects for saving.
 * @param pathOrFd Full path to file or file descriptor.
 *
 * @return True if saves were successful, false otherwise.
 */
template <bool Append = false, int32_t Mode = 0644, typename T, typename S>
	requires(std::forward_iterator<typename T::iterator> && (std::is_same_v<S, int32_t> || StringableView<S>))
FORCE_INLINE bool SaveBinaries(const T& objects, const S pathOrFd)
{
	FileDescriptor cleaner{};
	int32_t file;

	if constexpr (StringableView<S>) {
		int32_t flags{ O_WRONLY | O_CREAT };
		if constexpr (Append) {
			flags |= O_APPEND;
		}
		else {
			flags |= O_TRUNC;
		}

		cleaner = FileDescriptor{ pathOrFd, flags, Mode };
		if (cleaner.value == -1) {
			LOG_ERROR_NEW("Can't open file: {}. Error №{}: {}", pathOrFd, errno, std::strerror(errno));
			return false;
		}

		file = cleaner.value;
	}
	else {
		file = pathOrFd;
	}

	uint64_t savedItems{ 0 };
	for (const auto& item : objects) {
		if (SaveBinary<append, Mode, disableLog>(item, file)) {
			++savedItems;
		}
	}

	if (savedItems != objects.size()) {
		LOG_WARNING_NEW(
			"Saved items {} is not equal to total items {} for file: {}.", savedItems, objects.size(), pathOrFd);
		return false;
	}

	LOG_DEBUG_NEW("Saved bin file {} with {} items", pathOrFd, savedItems);
	return true;
}

/**************************
 * @brief Save primitive type objects in file with specific separator.
 *
 * @attention Directorys in path must exist. If file descriptor is passed, it must be valid.
 *
 * @tparam Append If true, data will be appended to the file with new line separator, and overwritten otherwise. Default
 * is false.
 * @tparam Mode File access mode in octal format, default is 0644.
 * @tparam T Type of forward container.
 * @tparam S Type of path or file descriptor.
 *
 * @param objects Forward container of primitive types for saving.
 * @param separator Separator character.
 * @param pathOrFd Full path to file or file descriptor.
 *
 * @return True if saves were successful, false otherwise.
 */
template <bool Append = false, int32_t Mode = 0644, template <typename> typename T, typename TT, typename S>
	requires(std::forward_iterator<typename T<TT>::iterator> && (std::is_same_v<S, int32_t> || StringableView<S>)
		&& (is_integer_type<TT> || is_float_type<TT>))
bool SavePrimitives(const T<TT>& objects, const char separator, const S pathOrFd)
{
	if (objects.empty()) {
		return true;
	}

	FileDescriptor cleaner{};
	int32_t file;

	if constexpr (StringableView<S>) {
		int32_t flags{ O_WRONLY | O_CREAT };
		if constexpr (Append) {
			flags |= O_APPEND;
		}
		else {
			flags |= O_TRUNC;
		}

		cleaner = FileDescriptor{ pathOrFd, flags, Mode };
		if (cleaner.value == -1) {
			LOG_ERROR_NEW("Can't open file: {}. Error №{}: {}", pathOrFd, errno, std::strerror(errno));
			return false;
		}

		file = cleaner.value;
	}
	else {
		file = pathOrFd;
	}

	if constexpr (Append) {
		const auto pos{ lseek(file, 0, SEEK_END) };

		if (pos == -1) {
			LOG_ERROR_NEW("Failed to seek to end of file: {}. Error №{}: {}", pathOrFd, errno, std::strerror(errno));
			return false;
		}

		if (pos > 0) {
			if (write(file, "\n", 1) != 1) {
				LOG_ERROR_NEW("Failed to write newline to {}. Error №{}: {}", pathOrFd, errno, std::strerror(errno));
				return false;
			}
		}
	}
	else if constexpr (std::is_same_v<S, int32_t>) {
		if (ftruncate(file, 0) == -1) {
			LOG_ERROR_NEW("Failed to truncate file: {}. Error №{}: {}", pathOrFd, errno, std::strerror(errno));
			return false;
		}

		if (lseek(file, 0, SEEK_SET) == -1) {
			LOG_ERROR_NEW("Failed to seek to start of file: {}. Error №{}: {}", pathOrFd, errno, std::strerror(errno));
			return false;
		}
	}

	auto begin = objects.begin();
	auto end = objects.end();

	char buffer[512];
	uint64_t offset{ 0 };
	char* writtenEnd;

#define TMP_MSAPI_BIN_SAVE_PRIMITIVES                                                                                  \
	if constexpr (is_integer_type<TT>) {                                                                               \
		writtenEnd = std::format_to(buffer + offset, "{}", *begin);                                                    \
	}                                                                                                                  \
	else if constexpr (is_float_type<TT>) {                                                                            \
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
	offset += UINT64(writtenEnd - (buffer + offset));

	TMP_MSAPI_BIN_SAVE_PRIMITIVES;

	while (++begin != end) {
		buffer[offset] = separator;
		++offset;

		TMP_MSAPI_BIN_SAVE_PRIMITIVES;

		// Assume 32 bytes is the largest possible string interpretation. That is not the real truth, as double can
		// contain huge number of digits, long double event more, but for practical usage, currently, it is enough.
		if (offset >= 480) {
#define TMP_MSAPI_BIN_BUFFER_FLUSH                                                                                     \
	auto result{ write(file, buffer, offset) };                                                                        \
	if (result == -1) {                                                                                                \
		LOG_ERROR_NEW("Write failed for file: {}. Error №{}: {}", pathOrFd, errno, std::strerror(errno));              \
		return false;                                                                                                  \
	}                                                                                                                  \
                                                                                                                       \
	if (UINT64(result) != offset) {                                                                                    \
		LOG_ERROR_NEW("Written size {} is not equal to buffer size {} for file: {}", result, offset, pathOrFd);        \
		return false;                                                                                                  \
	}

			TMP_MSAPI_BIN_BUFFER_FLUSH;

			offset = 0;
		}
	}

	TMP_MSAPI_BIN_BUFFER_FLUSH;
#undef TMP_MSAPI_BIN_BUFFER_FLUSH
#undef TMP_MSAPI_BIN_SAVE_PRIMITIVES

	LOG_DEBUG_NEW("Saved file {} with {} items", pathOrFd, objects.size());
	return true;
}

/**************************
 * @brief Save string in file.
 *
 * @attention Directorys in path must exist.
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
FORCE_INLINE bool SaveStr(const std::string_view str, const T path)
{
	FileDescriptor fd;
	int32_t flags{ O_WRONLY | O_CREAT };
	if constexpr (Append) {
		flags |= O_APPEND;
	}
	else {
		flags |= O_TRUNC;
	}

	fd = FileDescriptor{ path, flags, Mode };

	if (fd.value == -1) {
		LOG_ERROR_NEW("Failed to open file: {}. Error №{}: {}", path, errno, std::strerror(errno));
		return false;
	}

	if constexpr (Append) {
		const auto pos{ lseek(fd.value, 0, SEEK_END) };
		if (pos == -1) {
			LOG_ERROR_NEW("Failed to seek to end of file: {}. Error №{}: {}", path, errno, std::strerror(errno));
			return false;
		}

		if (pos > 0) {
			if (write(fd.value, "\n", 1) != 1) {
				LOG_ERROR_NEW("Failed to write newline to {}. Error №{}: {}", path, errno, std::strerror(errno));
				return false;
			}
		}
	}

	const auto size{ str.size() };
	const auto result{ write(fd.value, str.data(), size) };
	if (result == -1) {
		LOG_ERROR_NEW("Failed to write in file: {}. Error №{}: {}", path, errno, std::strerror(errno));
		return false;
	}

	if (UINT64(result) != size) {
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
 * @param size Size to be read.
 * @param path Full path to file.
 *
 * @return True if read was successful, false otherwise.
 *
 * @test Has unit tests.
 */
template <typename T, typename S>
	requires Pointable<T> && StringableView<S>
bool ReadBinary(T&& object, const S path)
{
	if (!HasFile(path)) {
		LOG_ERROR_NEW("Can't find file to read data: {}", path);
		return false;
	}

	FileDescriptor cleaner{ path, O_RDONLY, 0 };
	if (cleaner.value == -1) {
		LOG_ERROR_NEW("Can't open file to read data: {}. Error №{}: {}", path, errno, std::strerror(errno));
		return false;
	}

	constexpr auto size{ sizeof(std::remove_pointer_t<std::remove_reference_t<T>>) };
	const auto result{ read(cleaner.value, Pointer(object), size) };
	if (result == -1) {
		LOG_ERROR_NEW("Can't read data: {}. Error №{}: {}", path, errno, std::strerror(errno));
		return false;
	}

	if (UINT64(result) != size) {
		LOG_ERROR_NEW("Read size {} is not equal to object size {} for file: {}", result, size, path);
		return false;
	}

	return true;
}

template <template <typename> typename T, typename S, typename N>
	requires StringableView<N>
bool ReadBinaries(T<S>& container, const N path)
{
	if (!HasFile(path)) {
		LOG_ERROR_NEW("Can't find file to read data: {}", path);
		return false;
	}

	FileDescriptor cleaner{ path, O_RDONLY, 0 };
	if (cleaner.value == -1) {
		LOG_ERROR_NEW("Can't open file to read data: {}. Error №{}: {}", path, errno, std::strerror(errno));
		return false;
	}

	S item;
	while (true) {
		const auto result{ read(cleaner.value, &item, sizeof(S)) };
		if (result == -1) {
			LOG_ERROR_NEW("Can't read data: {}. Error №{}: {}", path, errno, std::strerror(errno));
			return {};
		}

		if (result == 0) {
			break;
		}

		if (UINT64(result) != sizeof(S)) {
			LOG_ERROR_NEW("Read size {} of object №{} is not equal to object size {} for file: {}.", result,
				container.size(), sizeof(S), path);
			return {};
		}

		container.emplace_back(std::move(item));
	}

	LOG_DEBUG_NEW("Read bin file: {} with {} items", path, container.size());
	return true;
}

/**************************
 * @brief Read string from file, will be read any symbols until the end of the file.
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
FORCE_INLINE bool ReadStr(std::string& str, const T path)
{
	if (!HasFile(path)) {
		LOG_ERROR_NEW("Can't find file to read data: {}", path);
		return false;
	}
	std::fstream stream(CString(path), std::ios::binary | std::ios::in);
	if (!stream.is_open()) {
		LOG_ERROR_NEW("Can't open file to read data: {}", path);
		return false;
	}
	stream.seekg(0, stream.end);
	std::streampos length{ stream.tellg() };
	stream.seekg(0, stream.beg);
	char* ptr{ static_cast<char*>(malloc(UINT64(length) + 1)) };
	stream.read(ptr, length);
	stream.close();
	str = std::move(std::string{ ptr, static_cast<std::size_t>(length) });
	free(ptr);

	LOG_DEBUG_NEW("Read str file: {} with size {}", path, str.size());

	return true;
}

/**************************
 * @brief Remove file or directory.
 *
 * @param path Full path.
 *
 * @return True if removing was successful, false otherwise.
 *
 * @test Has unit tests.
 *
 * @todo Remove use of system function.
 */
FORCE_INLINE bool Remove(const std::string_view path)
{
	if (path.size() < 2) {
		LOG_WARNING_NEW("Invalid path to be removed: {}", path);
		return false;
	}

	if (path.size() >= 506) {
		LOG_WARNING_NEW("Path to be removed is too long (>=506): {}", path);
		return false;
	}

	char name[512] = "rm -r ";
	std::copy(path.begin(), path.end(), name + 6);
	name[6 + path.size()] = '\0';
	if (system(name) == 0) {
		LOG_DEBUG_NEW("Path {} is removed successfully", path);
		return true;
	}

	LOG_ERROR_NEW("Path {} is not removed. Error №{}: {}", path, errno, std::strerror(errno));
	return false;
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
 *
 *
 */
template <typename T, typename S>
	requires StringableView<T> && StringableView<S>
bool CopyFile(const T from, const S to)
{
	FileDescriptor fromCleaner{ from, O_RDONLY, 0 };
	if (fromCleaner.value == -1) {
		LOG_ERROR_NEW("Can't open file to read data: {}. Error №{}: {}", from, errno, std::strerror(errno));
		return false;
	}

	FileDescriptor toCleaner{ to, O_WRONLY | O_CREAT | O_TRUNC, 0644 };
	if (toCleaner.value == -1) {
		LOG_ERROR_NEW("Can't open file to save data: {}. Error №{}: {}", to, errno, std::strerror(errno));
		return false;
	}

	struct stat st { };
	if (fstat(fromCleaner.value, &st) != 0) {
		LOG_ERROR_NEW("Failed to get file size for {}. Error №{}: {}", from, errno, std::strerror(errno));
		return false;
	}

	if (st.st_size == 0) {
		LOG_DEBUG_NEW("Source file {} is empty, created empty file {}", from, to);
		if (write(toCleaner.value, "", 0) == -1) {
			LOG_ERROR_NEW("Failed to create empty file {}. Error №{}: {}", to, errno, std::strerror(errno));
			return false;
		}

		return true;
	}

	if (S_ISREG(st.st_mode) == 0) {
		LOG_WARNING_NEW("Source file {} is not a regular file", from);
		return false;
	}

	off_t offset{ 0 };
	const off_t total{ st.st_size };
	while (offset < total) {
		const auto toSend{ UINT64(total - offset) };
		const auto sent{ sendfile(toCleaner.value, fromCleaner.value, &offset, toSend) };
		if (sent == 0) {
			break;
		}
		if (sent == -1) {
			if (errno == EINTR) {
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
 * @brief Check if directory exists.
 *
 * @param path Full path to directory.
 *
 * @tparam T Type of path.
 *
 * @return True if directory exists, false otherwise.
 *
 * @test Has unit tests.
 */
template <typename T>
	requires StringableView<T>
FORCE_INLINE bool HasDir(const T path)
{
	struct stat s;
	if (stat(CString<T>(path), &s)) {
		return false;
	}
	return S_ISDIR(s.st_mode);
}

/**************************
 * @brief Create directory.
 *
 * @param path Full path to directory.
 *
 * @return True if directory was created, false otherwise.
 *
 * @test Has unit tests.
 *
 * @todo Remove use of system function.
 */
template <typename T>
	requires StringableView<T>
FORCE_INLINE bool CreateDir(const T path)
{
	if (system(std::format("mkdir -p {}", path).c_str()) == 0) {
		return true;
	}

	LOG_ERROR_NEW("Dir {} is not created. Error №{}: {}", path, errno, std::strerror(errno));
	return false;
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
 * @return Reinterpretation of Type enum to string.
 */
FORCE_INLINE std::string_view EnumToString(const FileType type)
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

/**************************
 * @brief List directory content with specific type and append to provided container. '.' and '..' are excluded from
 * results for tables.
 *
 * @attention Contend sorting is filesystem dependent.
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
FORCE_INLINE bool List(T<std::string>& container, const S path)
{
	DIR* dir{ opendir(CString(path)) };
	if (dir == nullptr) [[unlikely]] {
		LOG_ERROR_NEW("Error opening directory: {}. Error №{}: {}", path, errno, std::strerror(errno));
		return false;
	}

	struct dirent* ent;
	while ((ent = readdir(dir)) != nullptr) {
		std::string str{ ent->d_name };
		if constexpr (U(FT) == DT_DIR) {
			if (str == "." || str == "..") {
				continue;
			}
		}

		if (ent->d_type != U(FT)) {
			continue;
		}

		container.emplace_back(std::move(str));
	}

	closedir(dir);
	return true;
}

}; //* namespace Bin

}; //* namespace MSAPI

#endif //* MSAPI_BIN_H