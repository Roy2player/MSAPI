/**************************
 * @file        helper.cpp
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

#include "helper.h"
#include <arpa/inet.h>
#include <cmath>
#include <cstring>
#include <netdb.h>
#include <regex>

namespace MSAPI {

namespace Helper {

bool ValidateEmail(const std::string_view email)
{
	return std::regex_match(email.begin(), email.end(),
		std::regex{ R"(^[a-zA-Z0-9]+([.-]?[a-zA-Z0-9]+)*@[a-zA-Z0-9]+([.-]?[a-zA-Z0-9]+)*\.[a-zA-Z]{2,}$)" });
}

bool ValidateIpv4(const char* ip)
{
	sockaddr_in sa;
	return inet_pton(AF_INET, ip, &sa.sin_addr) == 1;
}

bool ValidateIpv6(const char* ip)
{
	sockaddr_in6 sa;
	return inet_pton(AF_INET6, ip, &sa.sin6_addr.s6_addr) == 1;
}

void GetExecutableDir(std::string& path)
{
	const auto result{ readlink("/proc/self/exe", path.data(), path.size()) };
	if (result != -1) [[likely]] {
		path.resize(UINT64(result));

		const auto lastSlash{ path.find_last_of('/') };
		if (lastSlash != std::string::npos) {
			path = path.substr(0, lastSlash + 1);
		}
		else {
			path.clear();
		}

		return;
	}

	LOG_ERROR("Cannot read link of self process. Error №" + _S(errno) + ": " + std::strerror(errno));
}

std::string DomainToIp(const char* domain)
{
	hostent* ptr{ gethostbyname(domain) };
	if (ptr == nullptr) {
		LOG_ERROR("gethostbyname returned nullptr");
		return "";
	}

	std::string destIp;
	destIp.resize(INET6_ADDRSTRLEN);

	inet_ntop(ptr->h_addrtype, ptr->h_addr, destIp.data(), INET6_ADDRSTRLEN);
	return destIp;
}

void Separating(std::vector<std::string>& vector, const std::string& str, const char symbol)
{
	std::string property;
	for (size_t index{ 0 }; index < str.size(); ++index) {
		if (str[index] == symbol && !property.empty()) {
			vector.push_back(property);
			property.erase();
			continue;
		}
		property += str[index];
	}
	if (!property.empty()) {
		vector.push_back(property);
	}
}

void Separating(std::set<std::string>& vector, const std::string& str, const char symbol)
{
	std::string property;
	for (size_t index{ 0 }; index < str.size(); ++index) {
		if (str[index] == symbol && !property.empty()) {
			vector.insert(property);
			property.erase();
			continue;
		}
		property += str[index];
	}
	if (!property.empty()) {
		vector.insert(property);
	}
}

std::optional<int> HexStrToDecimal(const std::string& str)
{
	if (str.empty()) {
		return {};
	}

	int out{ 0 };
	int index{ static_cast<int>(str.size()) - 1 };
	for (const auto& symbol : str) {
		int multiplier = static_cast<int>(std::pow(16, index));
		switch (std::tolower(symbol)) {
		case '0':
			break;
		case '1':
			out += multiplier;
			break;
		case '2':
			out += 2 * multiplier;
			break;
		case '3':
			out += 3 * multiplier;
			break;
		case '4':
			out += 4 * multiplier;
			break;
		case '5':
			out += 5 * multiplier;
			break;
		case '6':
			out += 6 * multiplier;
			break;
		case '7':
			out += 7 * multiplier;
			break;
		case '8':
			out += 8 * multiplier;
			break;
		case '9':
			out += 9 * multiplier;
			break;
		case 'a':
		case 'A':
			out += 10 * multiplier;
			break;
		case 'b':
		case 'B':
			out += 11 * multiplier;
			break;
		case 'c':
		case 'C':
			out += 12 * multiplier;
			break;
		case 'd':
		case 'D':
			out += 13 * multiplier;
			break;
		case 'e':
		case 'E':
			out += 14 * multiplier;
			break;
		case 'f':
		case 'F':
			out += 15 * multiplier;
			break;
		default:
			return {};
			break;
		}
		--index;
	}
	return out;
}

double ToDouble(const int64_t units, const int32_t nano)
{
	return static_cast<double>(units) + static_cast<double>(nano) / 1000000000;
}

int32_t Round(const int32_t nano, double tick)
{
	double tickPow{ 0 };
	while (FloatGreater(tick, floor(tick))) {
		tick *= 10;
		++tickPow;
	}
	int32_t baseDegree{ 1000000000 / static_cast<int32_t>(std::pow(10, tickPow + 1)) };
	return static_cast<int32_t>(round(static_cast<double>(nano / baseDegree) / 10)) * baseDegree * 10;
}

double Round(double price, double tick)
{
	double save{ price };
	int tickPow{ 0 };

	while (FloatGreater(tick, floor(tick))) {
		tick *= 10;
		++tickPow;
	}

	const auto multiplier{ std::pow(10, tickPow) };
	price *= multiplier;
	price = round(price);
	price /= multiplier;

	size_t index{ 0 };
	while (FloatGreater(price, save)) {
		price *= multiplier;
		++index;
		price = price - static_cast<double>(index);
		price = round(price);
		price /= multiplier;
	}

	return price;
}

double RoundUp(double value, const size_t accuracy)
{
	const auto multiplier{ pow(10, static_cast<double>(accuracy)) };
	value *= multiplier;
	return ceil(value) / multiplier;
}

double RoundDown(double value, const size_t accuracy)
{
	const auto multiplier{ pow(10, static_cast<double>(accuracy)) };
	value *= multiplier;
	return floor(value) / multiplier;
}

int WhereIsPoint(const double x1, const double y1, const double x2, const double y2, const double x3, const double y3)
{
	if (FloatEqual(x1, x2) || FloatEqual(y1, y2)) {
		return -2;
	}

	const int temp{ CompareFloats((x3 - x1) / (x2 - x1), (y3 - y1) / (y2 - y1)) };
	if (CompareFloats(y1, y2) == temp) {
		return 1;
	}
	return temp == 0 ? 0 : -1;
}

std::string GetStringIp(const sockaddr_in addr)
{
	std::string ip;
	ip.resize(INET_ADDRSTRLEN);
	inet_ntop(AF_INET, &addr.sin_addr, ip.data(), INET_ADDRSTRLEN);
	ip.resize(strlen(ip.c_str()));
	return ip;
}

}; //* namespace Helper

}; //* namespace MSAPI