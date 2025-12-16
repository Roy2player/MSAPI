/**************************
 * @file        log.cpp
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

#include "log.h"
#include "bin.h"
#include <cstring>
#include <mutex>

std::mutex coutMutex;

namespace MSAPI {

Log logger;

Log::Log(const bool toConsole, const bool toFile, const Level levelSave) noexcept
	: m_toConsole(toConsole)
	, m_toFile(toFile)
	, m_levelSave(levelSave)
{
	std::ios_base::sync_with_stdio(false);
}

Log::~Log() noexcept { Stop(); }

void Log::SetParentPath(const std::string& path) noexcept
{
	if (m_path == path) {
		return;
	}
	Print(
		"Update parent path from " + (m_path.empty() ? "<empty>" : m_path) + " to " + (path.empty() ? "<empty>" : path),
		Level::INFO);
	m_path = path;
}

const std::string& Log::GetPath() const noexcept { return m_path; }

//! Print(std::format_to(Log.GetBuffer(), ..., Level::INFO));
void Log::Print(std::string&& str, const Level level) noexcept
{
	if (level > m_levelSave) {
		return;
	}
	std::lock_guard<std::mutex> lock(coutMutex);
	const auto time{ Timer().ToString() };
	if (m_toFile && m_ofstreamLog.is_open()) {
		m_ofstreamLog << "# " << time << " " << GetStringLevel(level) << " " + m_name + " : " << str << "."
					  << std::endl;
	}
	if (m_toConsole) {
		std::cout << "# " << time << " " << GetStringLevel(level) << " " + m_name + " : " << str << "." << std::endl;
	}
}

Log::Level Log::GetLevelSave() const noexcept { return m_levelSave; }

void Log::SetLevelSave(const Level levelSave) noexcept
{
	if (m_levelSave == levelSave) {
		return;
	}
	Print(std::format("Update logging level from {} to {}", EnumToString(m_levelSave), EnumToString(levelSave)),
		Level::INFO);
	m_levelSave = levelSave;
}

bool Log::GetToFile() const noexcept { return m_toFile; }

void Log::SetToFile(const bool toFile) noexcept
{
	if (m_toFile == toFile) {
		return;
	}
	Print(std::format("Update flag of write logs in file from {} to {}", _S(m_toFile), _S(toFile)), Level::INFO);
	m_toFile = toFile;
}

bool Log::GetToConsole() const noexcept { return m_toConsole; }

void Log::SetToConsole(const bool toConsole) noexcept
{
	if (m_toConsole == toConsole) {
		return;
	}
	Print(
		std::format("Update flag of write logs in console from {} to {}", _S(m_toConsole), _S(toConsole)), Level::INFO);
	m_toConsole = toConsole;
}

std::string_view Log::GetStringLevel(const Level level) noexcept
{
	static_assert(U(Level::Max) == 6, "Missed description for a new log level enum");

	switch (level) {
	case Level::Undefined:
		return "Undefined";
	case Level::ERROR:
		return "\033[0;31m<ERROR   >\033[0m";
	case Level::WARNING:
		return "\033[0;33m<WARNING >\033[0m";
	case Level::INFO:
		return "<INFO    >";
	case Level::DEBUG:
		return "<DEBUG   >";
	case Level::PROTOCOL:
		return "<PROTOCOL>";
	case Level::Max:
		return "";
	default:
		LOG_ERROR("Unknown logging level: " + _S(U(level)));
		return "<UNKNOWN >";
	}
}

std::string_view Log::EnumToString(const Level level) noexcept
{
	static_assert(U(Level::Max) == 6, "Missed description for a new log level enum");

	switch (level) {
	case Level::Undefined:
		return "Undefined";
	case Level::ERROR:
		return "Error";
	case Level::WARNING:
		return "Warning";
	case Level::INFO:
		return "Info";
	case Level::DEBUG:
		return "Debug";
	case Level::PROTOCOL:
		return "Protocol";
	case Level::Max:
		return "Max";
	default:
		LOG_ERROR("Unknown logging level: " + _S(U(level)));
		return "Unknown";
	}
}

void Log::SetName(const std::string& name) noexcept
{
	if (m_active) {
		Print("Update name during logging process from " + m_name + " to: " + name, Level::INFO);
		m_name = name;
		Stop();
		Start();
	}
	else {
		Print("Update name from " + m_name + " to: " + name, Level::INFO);
		m_name = name;
	}
}

void Log::SetSeparateDays(const bool separate) noexcept
{
	if (m_active) {
		Print("Update separate days mode during logging process from " + _S(m_separateDays) + " to: " + _S(separate),
			Level::INFO);
		m_separateDays = separate;
		if (m_separateDays && !m_timerToSeparate.IsRunning()) {
			m_timerToSeparate.Start(Timer::GetSecondsToTomorrow(), static_cast<time_t>(SECONDS_IN_DAY), false);
		}
	}
	else {
		Print("Update separate days mode from " + _S(m_separateDays) + " to: " + _S(separate), Level::INFO);
		m_separateDays = separate;
	}
}

bool Log::GetSeparateDays() const noexcept { return m_separateDays; }

bool Log::IsActive() const noexcept { return m_active; }

void Log::Start() noexcept
{
	if (m_active) {
		Print("Logger is already running", Level::DEBUG);
		return;
	}

	if (m_name.empty()) {
		Print("Logger to console switched but name didn't specify", Level::INFO);
		return;
	}

	const auto sessionId{ Timer().GetMilliseconds() };

	if (m_toFile) {
		if (!m_path.empty() && !m_name.empty()
			&& (Bin::HasDir((m_path + "logs/").data()) || Bin::CreateDir((m_path + "logs/").data()))) {
			std::string name{ "logs/" + m_name };
			auto pos{ name.find(" ") };
			while (true) {
				if (pos == std::string::npos) {
					break;
				}
				name[pos] = '-';
				pos = name.find(" ");
			}

			const std::string path{ m_path + name + "_" + _S(sessionId) + ".log" };
			std::ofstream file{ path, std::ios::app };
			if (!file.is_open()) {
				m_toFile = false;
				if (m_toConsole) {
					Print("File to writing logs does not open, path: " + path + ". Error №" + _S(errno) + ": "
							+ std::strerror(errno),
						Level::ERROR);
				}
			}
			else {
				m_ofstreamLog = std::move(file);
			}
		}
		else {
			Print("Logger to file doesn't switch, because name (" + _S(m_name.empty()) + ") or path ("
					+ _S(m_path.empty()) + ") didn't specify, or dir doesn't exist and was not create",
				Level::INFO);
		}
	}

	if (m_separateDays && !m_timerToSeparate.IsRunning()) {
		m_timerToSeparate.Start(Timer::GetSecondsToTomorrow(), static_cast<time_t>(SECONDS_IN_DAY), false);
	}
	m_active = true;
}

void Log::Stop() noexcept
{
	Print("Log is stopping", Level::DEBUG);
	if (!m_active) {
		Print("Log was already stopped", Level::DEBUG);
		return;
	}

	if (m_timerToSeparate.IsRunning()) {
		m_timerToSeparate.Stop();
	}

	m_active = false;
	if (m_toFile && m_ofstreamLog.is_open()) {
		m_ofstreamLog.close();
	}
	Print("Log was stopped", Level::DEBUG);
}

}; //* namespace MSAPI