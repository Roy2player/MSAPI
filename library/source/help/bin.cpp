/**************************
 * @file        bin.cpp
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
 */

#include "bin.h"
#include <sys/stat.h>

namespace MSAPI {

namespace Bin {

bool Save(const void* object, const size_t size, const std::string& path)
{
	std::fstream stream(path, std::ios::binary | std::ios::out);
	if (!stream.is_open()) {
		LOG_ERROR("Can't open file: " + path);
		return false;
	}

	stream.write(static_cast<const char*>(object), static_cast<long int>(size));
	LOG_DEBUG("Saved bin file: " + path + ", size:" + _S(size));
	stream.close();
	return true;
}

bool SaveArray(const std::vector<double>& array, const char separator, const std::string& path)
{
	if (array.empty()) {
		return false;
	}

	std::fstream stream(path, std::ios::binary | std::ios::out);
	if (!stream.is_open()) {
		LOG_ERROR("Can't open file: " + path);
		return false;
	}

	stream << array[0];
	for (size_t index{ 1 }; index < array.size(); ++index) {
		stream << separator << array[index];
	}
	LOG_DEBUG("Saved array file: " + path);
	stream.close();
	return true;
}

bool Read(void* ptr, const size_t size, const std::string& path)
{
	if (!HasFile(path)) {
		LOG_ERROR("Can't find file to read data: " + path);
		return false;
	}

	std::fstream stream(path, std::ios::binary | std::ios::in);
	if (!stream.is_open()) {
		LOG_ERROR("Can't open file to read data: " + path);
		return false;
	}

	stream.read(static_cast<char*>(ptr), static_cast<long int>(size));
	stream.close();
	return true;
}

bool HasFile(const std::string& path)
{
	int file = open(path.c_str(), O_RDONLY, 0644);
	if (file != -1) {
		if (close(file) == -1) {
			LOG_ERROR("File " + path + " close fail. Error №" + _S(errno) + ": " + std::strerror(errno));
		}
		return true;
	}
	return false;
}

bool Rename(const std::string& currentName, const std::string& newName)
{
	if (rename(currentName.c_str(), newName.c_str()) == 0) {
		LOG_DEBUG("File " + currentName + " rename successes to: " + newName);
		return true;
	}

	LOG_ERROR("File " + currentName + " rename failed to: " + newName + ". Error №" + _S(errno) + ": "
		+ std::strerror(errno));
	return false;
}

bool Remove(const std::string& path)
{
	if (remove(path.c_str()) == 0) {
		LOG_DEBUG("File " + path + " removed");
		return true;
	}

	LOG_ERROR("File " + path + " didn't removed. Error №" + _S(errno) + ": " + std::strerror(errno));
	return false;
}

bool HasDir(const std::string& name)
{
	struct stat s;
	if (stat(name.c_str(), &s)) {
		return false;
	}
	return S_ISDIR(s.st_mode);
}

bool MakeDir(const std::string& name)
{
	if (system(std::string{ "mkdir -p " + name }.c_str()) == 0) {
		return true;
	}
	LOG_ERROR("Dir " + name + " is not created. Error №" + _S(errno) + ": " + std::strerror(errno));
	return false;
}

bool Copy(const std::string& from, const std::string& to)
{
	std::ifstream streamFrom(from, std::ios::binary);
	if (!streamFrom.is_open()) {
		LOG_ERROR("Can't open file to read data: " + from);
		return false;
	}
	std::ofstream streamTo(to, std::ios::binary);
	if (!streamTo.is_open()) {
		LOG_ERROR("Can't open file to save data: " + to);
		streamFrom.close();
		return false;
	}
	streamTo << streamFrom.rdbuf();
	streamTo.close();
	streamFrom.close();
	return true;
}

bool SaveStr(const std::string& str, const std::string& path)
{
	std::fstream stream(path, std::ios::binary | std::ios::out);
	if (!stream.is_open()) {
		LOG_ERROR("Can't open file: " + path);
		return false;
	}
	stream << str;
	LOG_DEBUG("Saved str file: " + path);
	stream.close();
	return true;
}

bool ReadStr(std::string& str, const std::string& path)
{
	if (!HasFile(path)) {
		LOG_ERROR("Can't find file to read data: " + path);
		return false;
	}
	std::fstream stream(path, std::ios::binary | std::ios::in);
	if (!stream.is_open()) {
		LOG_ERROR("Can't open file to read data: " + path);
		return false;
	}
	stream.seekg(0, stream.end);
	std::streampos length{ stream.tellg() };
	stream.seekg(0, stream.beg);
	str.resize(UINT64(length));
	stream.read(str.data(), length);
	stream.close();

	return true;
}

}; //* namespace Bin

}; //* namespace MSAPI