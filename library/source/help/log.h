/**************************
 * @file        log.h
 * @version     6.0
 * @date        2023-09-24
 * @author      maks.angels@mail.ru
 * @copyright   © 2021–2026 Maksim Andreevich Leonov
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
 * Required Notice: MSAPI, copyright © 2021–2026 Maksim Andreevich Leonov, maks.angels@mail.ru
 */

#ifndef MSAPI_LOG_H
#define MSAPI_LOG_H

#if defined(__GNUC__) || defined(__clang__)
#define FORCE_INLINE inline __attribute__((always_inline))
#else
#define FORCE_INLINE inline
#endif

// #include "circleContainer.hpp"
#include "meta.hpp"
#include "time.h"
#include <format>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#define STRINGIZE(x) std::string(STRINGIZE2(x))
#define STRINGIZE2(x) #x
#define LINE_STRING STRINGIZE(__LINE__)
#define FILE_STRING STRINGIZE(__FILE__)

#ifdef NDEBUG
#define LOG_PLACE _F + "(" + LINE_STRING + "): "
#else
#define LOG_PLACE _S(gettid()) + " : " + _F + "(" + LINE_STRING + "): "
#endif

#define _F std::string(__func__)

#define RED_BEGIN "\033[0;31m"
#define YELLOW_BEGIN "\033[0;33m"
#define GREEN_BEGIN "\033[0;32m"
#define COLOR_END "\033[0m"

/*
	@todo
	https://stackoverflow.com/questions/78802843/build-error-caused-by-interaction-of-gcc-ub-sanitiser-and-std-format

	Unfortunately, that function cannot be used with GCC sanitizer null, undefined and bounds options.
	It sets checks even inside consteval function what makes them not consteval anymore.
	This code will be saved as convenient way for creating compilation-time strings, but not used for now.
*/
template <uint64_t... Size> consteval auto Concatenate(const char (&... strings)[Size])
{
	const auto size = (... + (Size - 1));
	std::array<char, size> result;

	auto it = result.begin();
	((it = std::copy_n(strings, Size - 1, it)), ...);

	return result;
}

#define CONCATENATE(name, ...)                                                                                         \
	static constexpr auto name##_tmp = Concatenate(__VA_ARGS__);                                                       \
	static constexpr auto name = std::string_view(name##_tmp.data(), name##_tmp.size())

#define BI(str, pattern, ...) std::format_to(std::back_inserter(str), pattern, __VA_ARGS__);

#define INT64(v) static_cast<int64_t>(v)
#define UINT64(v) static_cast<uint64_t>(v)

namespace MSAPI {
/*
template <typename T, int_fast32_t Size> class Logger {
private:
CircleContainer<T, Size>::Accessor m_accessor;
bool m_isRunning{ true };
int_fast64_t m_readsCounter{ 0 };
std::thread m_thread{ [this]() {
	std::cout << "logger's thread is running" << std::endl;

	while (m_isRunning) {
		auto newWrites{ Buffers::writesCounter.load(std::memory_order_acquire) - m_readsCounter };
		while (newWrites-- > 0) {
			auto& current{ m_accessor.GetCurrent() };

			// 	std::cout << m_readsCounter << " --> read from " << current.buffer << " : " << std::string_view{
			// static_cast<char*>(current.buffer), current.size } << std::endl;
			++m_readsCounter;
		}

		Buffers::writesCounter.wait(m_readsCounter); //! Can stuck here
	}

	// Write last portion of logs
	auto newWrites{ Buffers::writesCounter.load(std::memory_order_acquire) - m_readsCounter };
	while (newWrites-- > 0) {
		auto& current{ m_accessor.GetCurrent() };
		// std::cout << m_readsCounter << " --> read from " << current.buffer << " : " << std::string_view{
		// static_cast<char*>(current.buffer), current.size } << std::endl;
		// ++m_readsCounter; //! TMP FOR TESTING
	}
} };

public:
Logger(auto& container)
	: m_accessor{ container }
{
}

~Logger()
{
	m_isRunning = false;

	if (m_readsCounter < Buffers::writesCounter.load(std::memory_order_acquire)) {
		// std::cout << "Notify due to some logs to write" << std::endl;
		Buffers::writesCounter.notify_one();
	}
	else {
		// std::cout << "Notify with less value" << std::endl;
		Buffers::writesCounter.fetch_sub(1, std::memory_order_relaxed);
		Buffers::writesCounter.notify_one();
	}

	m_thread.join();
	// std::cout << "logger's thread is realized" << std::endl;
}
};
*/

/**************************
 * @brief For logging. Class is common for all calls inside a builded unit, disable syncronization between C and C++ I/O
 * buffers.
 *
 * @note If logging in file is enabled, logs will be saved in "parent path + logs/file name".
 * @note INFO - level for logging in production. Main work process information. Goal: Client understand what all are
 * going well and in problem situation a developer understand in which part of program was problem.
 * @note DEBUG - level for logging all useful information about program work process. Goal: Developer understand
 * whole picture.
 * @note PROTOCOL - level like debug, but for protocols. Goal: Developer understand why and how parts are chatting.
 * @note WARNING - messages for signal something happen, but not critical, program will work as well.
 * @note ERROR - messages about something really bad, what can has large impact to work process.
 * @note ERROR, WARNING, DEBUG and PROTOCOL levels included line number and function name for fast debugging.
 *
 * @attention First part of log should include main information, next part - all additional text.
 * @attention Avoid duplicating messages in different levels.
 */
class Log : Timer {
public:
	enum class Level : int16_t { Undefined, ERROR, WARNING, INFO, DEBUG, PROTOCOL, Max };

private:
	bool m_active{ false };
	bool m_separateDays{ false };
	bool m_toConsole;
	bool m_toFile;
	Level m_levelSave;
	std::ofstream m_ofstreamLog;
	std::string m_name;
	std::string m_path;

public:
	/**************************
	 * @brief Construct a new Log object.
	 *
	 * @note SetName() is required to start logging.
	 * @note Need set path for write separately.
	 *
	 * @param toConsole Write logs in console.
	 * @param toFile Write logs in file.
	 * @param levelSave Logging level, if print level is higher than this level, action will miss.
	 *
	 * @todo SetName() may not be as required if add name in constructor.
	 * @todo Change toFile flag in constructor to path to save, if not empty == flag is true.
	 * @todo Messages by email.
	 * @todo Messages in Telegram.
	 */
	Log(bool toConsole = false, bool toFile = false, Level levelSave = Level::PROTOCOL) noexcept;

	/**************************
	 * @brief Destroy the Log object
	 *
	 * @note Call Stop() inside
	 */
	~Log() noexcept;

	/**************************
	 * @brief Synchrony request to write str by particular level.
	 */
	void Print(std::string&& str, Level level) noexcept;

	// template <typename... Ts>
	// 	requires(sizeof...(Ts) > 0)
	// constexpr FORCE_INLINE void Print(const Level level, const std::string logPlace, const std::format_string<Ts...>
	// pattern, Ts&&... args)
	// {
	//! std::format_to(std::back_inserter(s), "Item: {}, Price: {}", item, price);
	//! Take buffer from circle container and write to it directly by back_inserter
	//! It will be great to support really huge buffers for tables and so on.
	// 	if (level > m_levelSave) {
	// 		return;
	// 	}
	// 	std::lock_guard<std::mutex> lock(coutMutex);
	// 	const auto time{ Timer().ToString() };
	// 	if (m_toFile && m_ofstreamLog.is_open()) {
	// 		m_ofstreamLog << std::format("# {} {} {} : {}.", time, GetStringLevel(level), m_name,
	// 			std::format(pattern, std::forward<Ts>(args)...))
	// 					  << std::endl;
	// 	}
	// 	if (m_toConsole) {
	// 		std::cout << std::format("# {} {} {} : {}.", time, GetStringLevel(level), m_name,
	// 			std::format(pattern, std::forward<Ts>(args)...))
	// 				  << std::endl;
	// 	}
	// }

	// constexpr FORCE_INLINE void Print(const Level level, const std::string logPlace, const std::string_view str)
	// {
	// 	if (level > m_levelSave) {
	// 		return;
	// 	}
	// 	std::lock_guard<std::mutex> lock(coutMutex);
	// 	const auto time{ Timer().ToString() };
	// 	if (m_toFile && m_ofstreamLog.is_open()) {
	// 		m_ofstreamLog << std::format("# {} {} {} : {}.", time, GetStringLevel(level), m_name, str) << std::endl;
	// 	}
	// 	if (m_toConsole) {
	// 		std::cout << std::format("# {} {} {} : {}.", time, GetStringLevel(level), m_name, str) << std::endl;
	// 	}
	// }

	/**************************
	 * @return Current level to save.
	 */
	Level GetLevelSave() const noexcept;

	/**************************
	 * @brief Change level to save
	 */
	void SetLevelSave(Level levelSave) noexcept;

	/**************************
	 * @return Current flag of save to file.
	 */
	bool GetToFile() const noexcept;

	/**************************
	 * @brief Change flag of save to file.
	 *
	 * @attention Set path is required.
	 */
	void SetToFile(bool toFile) noexcept;

	/**************************
	 * @return Current flag of print to console.
	 */
	bool GetToConsole() const noexcept;

	/**************************
	 * @return Change flag of print to console.
	 */
	void SetToConsole(bool toConsole) noexcept;

	/**************************
	 * @brief Set the Name of program which will be logged.
	 */
	void SetName(const std::string& name) noexcept;

	/**************************
	 * @brief Change flag of separate days mode.
	 *
	 * @note In this mode logger will create new log file every day at 00:00:00 UTC+0.
	 * @attention Logs between close old file and create new on will be lost.
	 */
	void SetSeparateDays(bool separate) noexcept;

	/**************************
	 * @return Current flag of separate days mode.
	 */
	bool GetSeparateDays() const noexcept;

	/**************************
	 * @return Current logging process state.
	 */
	bool IsActive() const noexcept;

	/**************************
	 * @brief Start logging.
	 *
	 * @note Interrupt if logging is active.
	 * @note Interrupt if name is empty.
	 */
	void Start() noexcept;

	/**************************
	 * @brief Interrupt logging.
	 *
	 * @note Close file to save logs if open.
	 * @note Stop timer to separate. Parameter will not change.
	 */
	void Stop() noexcept;

	/**************************
	 * @return Current parent path for logger data, if doesn't set will return empty string.
	 */
	const std::string& GetPath() const noexcept;

	/**************************
	 * @brief Set parent path for logger.
	 *
	 * @note If dir doesn't exist, it will be created.
	 *
	 * @attention '/' symbol at the end is required.
	 */
	void SetParentPath(const std::string& path) noexcept;

	/**************************
	 * @return String interpretation of Level enum.
	 */
	static std::string_view EnumToString(Level level) noexcept;

	// TODO: TG
	// TODO:https://ru.stackoverflow.com/questions/1349680/telegtam-api-%D0%9E%D1%82%D0%BF%D1%80%D0%B0%D0%B2%D0%B8%D1%82%D1%8C-%D1%82%D0%B5%D0%BA%D1%81%D1%82-%D0%9A%D0%B8%D1%80%D0%B8%D0%BB%D0%BB%D0%B8%D1%86%D1%83-%D1%87%D0%B5%D1%80%D0%B5%D0%B7-%D1%81-curl
	// TODO: https://core.telegram.org/bots/api

private:
	Timer::Event m_timerToSeparate{ { [](int* parameter) {
									   reinterpret_cast<Log*>(parameter)->Stop();
									   reinterpret_cast<Log*>(parameter)->Start();
								   } },
		reinterpret_cast<int*>(this) };

	/**************************
	 * @return String interpretation of Level enum with static width between '<' and '>' symbols.
	 */
	static std::string_view GetStringLevel(Level level) noexcept;
};

extern Log logger;

}; //* namespace MSAPI

#define LOG_ERROR_NEW(pattern, ...)                                                                                    \
	MSAPI::logger.Print(LOG_PLACE + std::format(pattern, __VA_ARGS__), MSAPI::Log::Level::ERROR)
#define LOG_WARNING_NEW(pattern, ...)                                                                                  \
	MSAPI::logger.Print(LOG_PLACE + std::format(pattern, __VA_ARGS__), MSAPI::Log::Level::WARNING)
#define LOG_INFO_NEW(pattern, ...)                                                                                     \
	MSAPI::logger.Print(LOG_PLACE + std::format(pattern, __VA_ARGS__), MSAPI::Log::Level::INFO)
#define LOG_DEBUG_NEW(pattern, ...)                                                                                    \
	MSAPI::logger.Print(LOG_PLACE + std::format(pattern, __VA_ARGS__), MSAPI::Log::Level::DEBUG)
#define LOG_PROTOCOL_NEW(pattern, ...)                                                                                 \
	MSAPI::logger.Print(LOG_PLACE + std::format(pattern, __VA_ARGS__), MSAPI::Log::Level::PROTOCOL)

#define LOG_ERROR(text) MSAPI::logger.Print(LOG_PLACE + text, MSAPI::Log::Level::ERROR)
#define LOG_WARNING(text) MSAPI::logger.Print(LOG_PLACE + text, MSAPI::Log::Level::WARNING)
#define LOG_INFO(text) MSAPI::logger.Print(LOG_PLACE + text, MSAPI::Log::Level::INFO)
#define LOG_DEBUG(text) MSAPI::logger.Print(LOG_PLACE + text, MSAPI::Log::Level::DEBUG)
#define LOG_PROTOCOL(text) MSAPI::logger.Print(LOG_PLACE + text, MSAPI::Log::Level::PROTOCOL)

#define U(x) static_cast<std::underlying_type_t<decltype(x)>>(x)

template <typename T> FORCE_INLINE std::string _S(const T x)
{
#define suppress
#pragma GCC diagnostic push
//* Because of std::to_string() has not declared for types less than int.
#pragma GCC diagnostic ignored "-Wsign-promo"
	if constexpr (MSAPI::is_integer_type<T>) {
		return std::to_string(x);
	}
	else if constexpr (std::is_enum_v<T>) {
		using S = std::underlying_type_t<T>;
		return std::to_string(static_cast<S>(x));
	}
	else if constexpr (MSAPI::is_integer_type_ptr<T> || MSAPI::is_integer_type_const_ptr<T>) {
		return std::to_string(*x);
	}
	else if constexpr (std::is_pointer_v<T> && std::is_enum_v<std::remove_pointer_t<T>>) {
		using S = std::underlying_type_t<std::remove_pointer_t<T>>;
		return std::to_string(static_cast<S>(*x));
	}
	else if constexpr (MSAPI::is_integer_type_optional<T>) {
		return x.has_value() ? std::to_string(x.value()) : "";
	}
	else if constexpr (MSAPI::is_integer_type_optional_ptr<T> || MSAPI::is_integer_type_optional_const_ptr<T>) {
		return x->has_value() ? std::to_string(x->value()) : "";
	}
	else if constexpr (std::is_same_v<T, float>) {
		static_assert(
			std::numeric_limits<float>::max_digits10 == 9, "std::numeric_limits<float>::max_digits10 is unexpected");
		return std::format("{:.9f}", x);
	}
	else if constexpr (std::is_same_v<T, double>) {
		static_assert(
			std::numeric_limits<double>::max_digits10 == 17, "std::numeric_limits<double>::max_digits10 is unexpected");
		return std::format("{:.17f}", x);
	}
	else if constexpr (std::is_same_v<T, long double>) {
		static_assert(std::numeric_limits<long double>::max_digits10 == 21,
			"std::numeric_limits<long double>::max_digits10 is unexpected");
		return std::format("{:.21Lf}", x);
	}
	else if constexpr (std::is_pointer_v<T> && std::is_same_v<std::remove_cv_t<std::remove_pointer_t<T>>, float>) {
		return std::format("{:.9f}", *x);
	}
	else if constexpr (std::is_pointer_v<T> && std::is_same_v<std::remove_cv_t<std::remove_pointer_t<T>>, double>) {
		return std::format("{:.17f}", *x);
	}
	else if constexpr (std::is_pointer_v<T>
		&& std::is_same_v<std::remove_cv_t<std::remove_pointer_t<T>>, long double>) {

		return std::format("{:.21Lf}", *x);
	}
	else if constexpr (MSAPI::is_optional_v<T> && std::is_same_v<MSAPI::remove_optional_t<T>, float>) {
		return x.has_value() ? std::format("{:.9f}", x.value()) : "";
	}
	else if constexpr (MSAPI::is_optional_v<T> && std::is_same_v<MSAPI::remove_optional_t<T>, double>) {
		return x.has_value() ? std::format("{:.17f}", x.value()) : "";
	}
	else if constexpr (MSAPI::is_optional_v<T> && std::is_same_v<MSAPI::remove_optional_t<T>, long double>) {
		return x.has_value() ? std::format("{:.21Lf}", x.value()) : "";
	}
	else if constexpr (std::is_pointer_v<T> && MSAPI::is_optional_v<std::remove_cv_t<std::remove_pointer_t<T>>>
		&& std::is_same_v<MSAPI::remove_optional_t<std::remove_cv_t<std::remove_pointer_t<T>>>, float>) {

		return x->has_value() ? std::format("{:.9f}", x->value()) : "";
	}
	else if constexpr (std::is_pointer_v<T> && MSAPI::is_optional_v<std::remove_cv_t<std::remove_pointer_t<T>>>
		&& std::is_same_v<MSAPI::remove_optional_t<std::remove_cv_t<std::remove_pointer_t<T>>>, double>) {

		return x->has_value() ? std::format("{:.17f}", x->value()) : "";
	}
	else if constexpr (std::is_pointer_v<T> && MSAPI::is_optional_v<std::remove_cv_t<std::remove_pointer_t<T>>>
		&& std::is_same_v<MSAPI::remove_optional_t<std::remove_cv_t<std::remove_pointer_t<T>>>, long double>) {

		return x->has_value() ? std::format("{:.21Lf}", x->value()) : "";
	}
	else if constexpr (std::is_same_v<T, bool>) {
		return x ? "true" : "false";
	}
	else if constexpr (std::is_pointer_v<T> && std::is_same_v<std::remove_cv_t<std::remove_pointer_t<T>>, bool>) {
		return *x ? "true" : "false";
	}
	else if constexpr (std::is_same_v<T, std::string>) {
		return x;
	}
	else if constexpr (std::is_pointer_v<T>
		&& std::is_same_v<std::remove_cv_t<std::remove_pointer_t<T>>, std::string>) {
		return *x;
	}
	else if constexpr (std::is_same_v<T, MSAPI::Timer> || std::is_same_v<T, MSAPI::Timer::Duration>) {
		return x.ToString();
	}
	else if constexpr ((std::is_pointer_v<T>
						   && std::is_same_v<std::remove_cv_t<std::remove_pointer_t<T>>, MSAPI::Timer>)
		|| (std::is_pointer_v<T>
			&& std::is_same_v<std::remove_cv_t<std::remove_pointer_t<T>>, MSAPI::Timer::Duration>)) {

		return x->ToString();
	}
	else {
		static_assert(sizeof(T) + 1 == 0, "Unsupported type");
	}
#pragma GCC diagnostic pop
#undef suppress
}

template <typename T>
	requires MSAPI::is_integer_type_optional<T>
struct std::formatter<T> {
	auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

	template <typename FormatContext> auto format(const T& opt, FormatContext& ctx)
	{
		return opt.has_value() ? format_to(ctx.out(), "{}", opt.value()) : format_to(ctx.out(), "");
	}
};

template <typename T>
	requires MSAPI::is_integer_type_optional_ptr<T> || MSAPI::is_integer_type_optional_const_ptr<T>
struct std::formatter<T> {
	auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

	template <typename FormatContext> auto format(const T opt, FormatContext& ctx)
	{
		return opt->has_value() ? format_to(ctx.out(), "{}", opt->value()) : format_to(ctx.out(), "");
	}
};

template <typename T>
	requires MSAPI::is_float_type_optional<T>
struct std::formatter<T> {
	auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

	template <typename FormatContext> auto format(const T& opt, FormatContext& ctx) { return _S(opt); }
};

template <typename T>
	requires MSAPI::is_float_type_optional_ptr<T> || MSAPI::is_float_type_optional_const_ptr<T>
struct std::formatter<T> {
	auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

	template <typename FormatContext> auto format(const T opt, FormatContext& ctx) { return _S(opt); }
};

#endif //* MSAPI_LOG_H