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
#include <unistd.h>
#include <vector>

namespace MSAPI {

namespace Bin {

/**************************
 * @brief Rename file or directory.
 *
 * @param currentName Full path of current name.
 * @param newName Full path of new name.
 *
 * @return True if rename was successful, false otherwise.
 */
bool Rename(const std::string& currentName, const std::string& newName);

/**************************
 * @brief Save array of double values in file with specific separator.
 *
 * @param array Array of double values.
 * @param separator Separator between values.
 * @param path Full path to file.
 *
 * @return True if save was successful, false if array is empty and any error occurred.
 */
bool SaveArray(const std::vector<double>& array, const char separator, const std::string& path);

/**************************
 * @brief Check if file exists.
 *
 * @param path Full path to file.
 *
 * @return True if file exists, false otherwise.
 */
bool HasFile(const std::string& path);

/**************************
 * @brief Save object in file.
 *
 * @param object Object for saving.
 * @param size Size of object.
 * @param path Full path to file.
 *
 * @return True if save was successful, false otherwise.
 *
 * @todo Make as template function.
 */
bool Save(const void* object, size_t size, const std::string& path);

/**************************
 * @brief Save string in file.
 *
 * @param str String for saving.
 * @param path Full path to file.
 *
 * @return True if save was successful, false otherwise.
 */
bool SaveStr(const std::string& str, const std::string& path);

/**************************
 * @brief Read object from file.
 *
 * @param ptr Pointer to object.
 * @param size Size of object.
 * @param path Full path to file.
 *
 * @return True if read was successful, false otherwise.
 *
 * @todo Make as template function.
 */
bool Read(void* ptr, size_t size, const std::string& path);

/**************************
 * @brief Read string from file, will be read any symbols until the end of the file.
 *
 * @param str String for reading.
 * @param path Full path to file.
 *
 * @return True if read was successful, false otherwise.
 */
bool ReadStr(std::string& str, const std::string& path);

/**************************
 * @brief Remove file.
 *
 * @param path Full path to file.
 *
 * @return True if remove was successful, false otherwise.
 */
bool Remove(const std::string& path);

/**************************
 * @brief Copy file.
 *
 * @param from Full path to file from copy.
 * @param to Full path to file to copy.
 *
 * @return True if copy was successful, false otherwise.
 */
bool Copy(const std::string& from, const std::string& to);

/**************************
 * @brief Check if directory exists.
 *
 * @param name Full path to directory.
 *
 * @return True if directory exists, false otherwise.
 */
bool HasDir(const std::string& name);

/**************************
 * @brief Make directory.
 *
 * @param name Full path to directory.
 *
 * @return True if directory was created, false otherwise.
 */
bool MakeDir(const std::string& name);

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
		LOG_ERROR("Unknown type: " + _S(U(type)));
		return "Unknown";
	}
}

/**************************
 * @brief Get list of files with specific type in specific path. '.' and '..' are excluded from results.
 *
 * @tparam FT Type of file to search.
 * @tparam Container with strings and emplace_back method.
 *
 * @param path Full path for parsing file names.
 *
 * @return Container with file names.
 */
template <FileType FT, typename Container> FORCE_INLINE Container ListFiles(const std::string path)
{
	DIR* dir{ opendir(path.c_str()) };
	if (dir == nullptr) [[unlikely]] {
		LOG_ERROR("Error opening directory: " + path + ". Error №" + _S(errno) + ": " + std::strerror(errno));
		return {};
	}

	Container files;
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

		files.emplace_back(std::move(str));
	}

	closedir(dir);
	return std::move(files);
}

}; //* namespace Bin

}; //* namespace MSAPI

#endif //* MSAPI_BIN_H