/**************************
 * @file        time.h
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

#ifndef MSAPI_TIME_H
#define MSAPI_TIME_H

#if defined(__GNUC__) || defined(__clang__)
#define FORCE_INLINE inline __attribute__((always_inline))
#else
#define FORCE_INLINE inline
#endif

#include <atomic>
#include <chrono>
#include <ctime>
#include <functional>
#include <signal.h>
#include <string>
#include <sys/time.h>

namespace MSAPI {

//* Functions that use it must take into leap year
constexpr uint8_t dayPerMonth[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
constexpr uint32_t SECONDS_IN_YEAR = 31536000;
constexpr uint32_t SECONDS_IN_DAY = 86400;
constexpr uint32_t MILLISECONDS_IN_DAY = 86400000;
constexpr uint64_t MICROSECONDS_IN_DAY = 86400000000;
constexpr uint64_t NANOSECONDS_IN_DAY = 86400000000000;
constexpr uint16_t SECONDS_IN_HOUR = 3600;
constexpr uint8_t SECONDS_IN_MINUTE = 60;

/**************************
 * @brief Class to contain and manipulate with time. Can be used as a timer, and as a timestamp container. Negative
 * timestamps are not supported.
 *
 * @attention One timer always has same unique id.
 *
 * @attention The limit of the time is 2262-04-09 23:47:16.854775807. In web sources the result of 9223372035000000000
 * timestamp is 11 April, but not 9 April. This 2 days difference stable within decades.
 *
 * @todo It is a question what is faster: atomic increment or two-64-bit variables generation for unique id.
 */
class Timer {
public:
	/**************************
	 * @brief Class for planning a function call in future by timer. Callback and data or only handler can be provided
	 * in constructor.
	 *
	 * @test Has unit test.
	 */
	class Event {
	public:
		class IHandler {
		public:
			virtual ~IHandler() = default;

			virtual void HandleEvent(const Event& event) = 0;
		};

	private:
		std::function<void(int*)> m_callback;
		IHandler* m_handler;
		timer_t m_id{};
		//* Callback, this, parameter
		std::tuple<void*, int*, int*> m_data;
		bool m_running{};
		time_t m_timeToCall{};
		time_t m_timeToRepeatCall{};
		bool m_repeat{};
		bool m_instantCall{};
		sigevent m_sev{ 0, 0, 0, 0 };
		itimerspec m_its;
		int64_t m_uid{ m_eventsCounter.fetch_add(1, std::memory_order_relaxed) };

		static inline std::atomic<int64_t> m_eventsCounter{};

	public:
		/**************************
		 * @brief Construct a new Event object with unique identifier, interrupt if data is nullptr.
		 *
		 * @param callback Function to call.
		 * @param data Parameter for callback function.
		 */
		Event(std::function<void(int*)> callback, int* data);

		/**************************
		 * @brief Construct a new Event object with unique identifier, interrupt if handler is nullptr.
		 *
		 * @param handler Handler object which implements IHandler interface.
		 */
		Event(IHandler* handler);

		/**************************
		 * @brief Call Stop() function inside if event is running.
		 */
		~Event();

		/**************************
		 * @return Id of timer event.
		 */
		int64_t GetId() const;

		/**************************
		 * @brief Start event.
		 *
		 * @param timeToCall Time to call function in UTC+0.
		 * @param timeToRepeatCall Time to repeat call function.
		 * @param instantCall Call function immediately after start.
		 *
		 * @return True if event started, false if event was not started or data is nullptr.
		 */
		bool Start(time_t timeToCall, time_t timeToRepeatCall = 0, bool instantCall = false);

		/**************************
		 * @brief Stop event if it's running.
		 *
		 * @note Save event state as active if delete event function was failed.
		 */
		void Stop();

		/**************************
		 * @return Event running state.
		 */
		bool IsRunning() const;

		/**************************
		 * @return Event repeat state.
		 */
		bool IsRepeat() const;

		/**************************
		 * @return Event instant call state.
		 */
		bool IsInstantCall() const;
	};

	/**************************
	 * @brief Structure for containing date in format year-month-day.
	 */
	struct Date {
		const uint16_t year;
		const uint8_t month;
		const uint8_t day;

		/**************************
		 * @brief Construct a new Date object, empty constructor.
		 *
		 * @param year Date year.
		 * @param month Date month.
		 * @param day Date day.
		 */
		Date(uint16_t year, uint8_t month, uint8_t day);

		/**************************
		 * @example 2023-12-30
		 *
		 * @test Has unit test.
		 */
		std::string ToString() const;

		/**************************
		 * @brief Call ToString() inside.
		 */
		operator std::string();

		/**************************
		 * @brief Call ToString() inside.
		 */
		friend std::ostream& operator<<(std::ostream& out, const Date& date);

		/**************************
		 * @brief Call ToString() inside.
		 */
		friend std::string operator+(const std::string& str, const Date& date);

		/**************************
		 * @brief Compare by all fields.
		 *
		 * @test Has unit test.
		 */
		friend bool operator<(const Timer::Date& first, const Timer::Date& second);

		/**************************
		 * @brief Compare by all fields.
		 *
		 * @test Has unit test.
		 */
		friend bool operator>(const Timer::Date& first, const Timer::Date& second);

		/**************************
		 * @brief Compare by all fields.
		 *
		 * @test Has unit test.
		 */
		friend bool operator==(const Timer::Date& first, const Timer::Date& second);

		/**************************
		 * @brief Compare by all fields.
		 *
		 * @test Has unit test.
		 */
		friend bool operator!=(const Timer::Date& first, const Timer::Date& second);

		/**************************
		 * @brief Compare by all fields.
		 *
		 * @test Has unit test.
		 */
		friend bool operator>=(const Timer::Date& first, const Timer::Date& second);

		/**************************
		 * @brief Compare by all fields.
		 *
		 * @test Has unit test.
		 */
		friend bool operator<=(const Timer::Date& first, const Timer::Date& second);
	};

	/**************************
	 * @brief Class for containing particular time duration or difference between two times. Contains nanoseconds and
	 * can be negative.
	 */
	class Duration {
	public:
		enum class Type : int16_t {
			Undefined,
			Nanoseconds,
			Microseconds,
			Milliseconds,
			Seconds,
			Minutes,
			Hours,
			Days,
			Max
		};

	private:
		int64_t m_nanoseconds{ 0 };

	public:
		/**************************
		 * @brief Construct a new Duration object, empty constructor.
		 *
		 * @param duration Chrono duration in nanoseconds.
		 */
		Duration(std::chrono::duration<int64_t, std::nano> duration);

		/**************************
		 * @brief Construct a new Duration object, empty constructor.
		 *
		 * @param nanoseconds Duration in nanoseconds.
		 */
		explicit Duration(int64_t nanoseconds);

		/**************************
		 * @brief Construct a new empty Duration object, empty constructor.
		 *
		 * @test Has unit test.
		 */
		Duration();

		/**************************
		 * @return True if duration nanoseconds is 0.
		 *
		 * @test Has unit test.
		 */
		bool Empty() const;

		/**************************
		 * @return The total number of whole days in duration.
		 *
		 * @test Has unit test.
		 */
		int64_t GetDays() const;

		/**************************
		 * @return The total number of whole hours in duration.
		 *
		 * @test Has unit test.
		 */
		int64_t GetHours() const;

		/**************************
		 * @return The total number of whole minutes in duration.
		 *
		 * @test Has unit test.
		 */
		int64_t GetMinutes() const;

		/**************************
		 * @return The total number of whole seconds in duration.
		 *
		 * @test Has unit test.
		 */
		int64_t GetSeconds() const;

		/**************************
		 * @return The total number of whole milliseconds in duration.
		 *
		 * @test Has unit test.
		 */
		int64_t GetMilliseconds() const;

		/**************************
		 * @return The total number of whole microseconds in duration.
		 *
		 * @test Has unit test.
		 */
		int64_t GetMicroseconds() const;

		/**************************
		 * @return The total number of whole nanoseconds in duration.
		 *
		 * @test Has unit test.
		 */
		int64_t GetNanoseconds() const;

		/**************************
		 * @return String interpretation of duration in particular type, default is nanoseconds.
		 *
		 * @example 108 hours
		 *
		 * @test Has unit test.
		 */
		std::string ToString(Type type = Type::Nanoseconds) const;

		/**************************
		 * @brief Call ToString() inside.
		 */
		operator std::string();

		/**************************
		 * @brief Call ToString() inside.
		 */
		friend std::ostream& operator<<(std::ostream& out, Duration duration);

		/**************************
		 * @brief Call ToString() inside.
		 */
		friend std::string operator+(const std::string& str, Duration duration);

		/**************************
		 * @brief Compare by nanoseconds counter.
		 *
		 * @test Has unit test.
		 */
		friend bool operator<(Duration first, Duration second);

		/**************************
		 * @brief Compare by nanoseconds counter.
		 *
		 * @test Has unit test.
		 */
		friend bool operator<=(Duration first, Duration second);

		/**************************
		 * @brief Compare by nanoseconds counter.
		 *
		 * @test Has unit test.
		 */
		friend bool operator>(Duration first, Duration second);

		/**************************
		 * @brief Compare by nanoseconds counter.
		 *
		 * @test Has unit test.
		 */
		friend bool operator>=(Duration first, Duration second);

		/**************************
		 * @brief Compare by nanoseconds counter.
		 *
		 * @test Has unit test.
		 */
		friend bool operator==(Duration first, Duration second);

		/**************************
		 * @brief Compare by nanoseconds counter.
		 *
		 * @test Has unit test.
		 *
		 * @test Has unit test.
		 */
		friend bool operator!=(Duration first, Duration second);

		/**************************
		 * @brief Compare by nanoseconds counter.
		 *
		 * @test Has unit test.
		 */
		friend Duration operator-(Duration first, Duration second);

		/**************************
		 * @brief Compare by nanoseconds counter.
		 *
		 * @test Has unit test.
		 */
		friend Duration operator+(Duration first, Duration second);

		/**************************
		 * @brief Create particular duration.
		 *
		 * @attention To create empty duration use Duration{} constructor.
		 *
		 * @param days Duration days.
		 * @param hours Duration hours.
		 * @param minutes Duration minutes.
		 * @param seconds Duration seconds.
		 * @param nanoseconds Duration nanoseconds.
		 *
		 * @test Has unit test.
		 */
		static Duration Create(
			int64_t days = 0, int64_t hours = 0, int64_t minutes = 0, int64_t seconds = 0, int32_t nanoseconds = 0);

		/**************************
		 * @brief Create a days duration.
		 *
		 * @param days Duration days.
		 *
		 * @test Has unit test.
		 */
		static Duration CreateDays(int64_t days);

		/**************************
		 * @brief Create a hours duration.
		 *
		 * @param hours Duration hours.
		 *
		 * @test Has unit test.
		 */
		static Duration CreateHours(int64_t hours);

		/**************************
		 * @brief Create a minutes duration.
		 *
		 * @param minutes Duration minutes.
		 *
		 * @test Has unit test.
		 */
		static Duration CreateMinutes(int64_t minutes);

		/**************************
		 * @brief Create a seconds duration.
		 *
		 * @param seconds Duration seconds.
		 *
		 * @test Has unit test.
		 */
		static Duration CreateSeconds(int64_t seconds);

		/**************************
		 * @brief Create a milliseconds duration.
		 *
		 * @param milliseconds Duration milliseconds.
		 *
		 * @test Has unit test.
		 */
		static Duration CreateMilliseconds(int64_t milliseconds);

		/**************************
		 * @brief Create a microseconds duration.
		 *
		 * @param microseconds Duration microseconds.
		 *
		 * @test Has unit test.
		 */
		static Duration CreateMicroseconds(int64_t microseconds);

		/**************************
		 * @brief Create a nanoseconds duration.
		 *
		 * @param nanoseconds Duration nanoseconds.
		 *
		 * @test Has unit test.
		 */
		static Duration CreateNanoseconds(int64_t nanoseconds);

		/**************************
		 * @return String interpretation of Type enum.
		 */
		static std::string_view EnumToString(Type type);
	};

private:
	std::chrono::system_clock::time_point m_point;

	static const std::chrono::system_clock::time_point m_zero_point;

public:
	/**************************
	 * @brief Construct a new Timer object with current time point, empty constructor.
	 */
	Timer();

	/**************************
	 * @brief Construct a new Timer object with particular time point, empty constructor.
	 *
	 * @param point Set 0 for create empty timer, cannot be negative.
	 */
	Timer(std::chrono::system_clock::time_point point);

	/**************************
	 * @brief Construct a new Timer object with time point from seconds and nanos.
	 *
	 * @param seconds Set both params as 0 for creating empty timer, cannot be negative.
	 * @param nanos Is 0 as default.
	 */
	Timer(int64_t seconds, int64_t nanos = 0);

	/**************************
	 * @brief Refresh time point to current timestamp.
	 */
	void Reset();

	/**************************
	 * @brief Get difference between current time point and timer's time point.
	 *
	 * @return Seconds and 6 sign after dot (microseconds), like 1.234567.
	 */
	double GetTimer() const;

	/**************************
	 * @return Chrono time point.
	 */
	std::chrono::system_clock::time_point GetPoint() const;

	/**************************
	 * @return True if point is zero.
	 */
	bool Empty() const;

	/**************************
	 * @return The total number of whole seconds in timer.
	 */
	int64_t GetSeconds() const;

	/**************************
	 * @return The total number of whole milliseconds in timer.
	 */
	int64_t GetMilliseconds() const;

	/**************************
	 * @return The total number of whole microseconds in timer.
	 */
	int64_t GetMicroseconds() const;

	/**************************
	 * @return The total number of whole nanoseconds in timer.
	 */
	int64_t GetNanoseconds() const;

	/**************************
	 * @return Timer converted to Date format.
	 *
	 * @test Has unit test.
	 */
	Date ToDate() const;

	/**************************
	 * @example 2023-11-20 21:36:03.492368859
	 *
	 * @test Has unit test.
	 */
	std::string ToString() const;

	/**************************
	 * @brief Call ToString() inside.
	 */
	operator std::string();

	/**************************
	 * @brief Call ToString() inside.
	 */
	friend std::ostream& operator<<(std::ostream& out, Timer timer);

	/**************************
	 * @brief Call ToString() inside.
	 */
	friend std::string operator+(const std::string& str, Timer timer);

	/**************************
	 * @brief Compare by time point.
	 *
	 * @test Has unit test.
	 */
	friend bool operator<(Timer first, Timer second);

	/**************************
	 * @brief Compare by time point.
	 *
	 * @test Has unit test.
	 */
	friend bool operator<=(Timer first, Timer second);

	/**************************
	 * @brief Compare by time point.
	 *
	 * @test Has unit test.
	 */
	friend bool operator>(Timer first, Timer second);

	/**************************
	 * @brief Compare by time point.
	 *
	 * @test Has unit test.
	 */
	friend bool operator>=(Timer first, Timer second);

	/**************************
	 * @brief Compare by time point.
	 *
	 * @test Has unit test.
	 */
	friend bool operator==(Timer first, Timer second);

	/**************************
	 * @brief Compare by time point.
	 *
	 * @test Has unit test.
	 */
	friend bool operator!=(Timer first, Timer second);

	/**************************
	 * @return Difference between time points.
	 *
	 * @test Has unit test.
	 */
	friend Duration operator-(Timer first, Timer second);

	/**************************
	 * @return Sum of time point and duration.
	 *
	 * @test Has unit test.
	 */
	friend Timer operator+(Timer timer, Duration duration);

	/**************************
	 * @return Timer with subtracted duration.
	 *
	 * @test Has unit test.
	 */
	friend Timer operator-(Timer timer, Duration duration);

	/**************************
	 * @attention If month is wrong, return 0.
	 *
	 * @param month Moth from 1 to 12
	 * @param isLeap True if year is leap.
	 *
	 * @return Number of days in month in particular year, take into leap year.
	 */
	static uint8_t HowMuchDaysInMonth(uint8_t month, bool isLeap);

	/**************************
	 * @attention If month is wrong, return 0.
	 *
	 * @param month Moth from 1 to 12
	 * @param isLeap True if year is leap.
	 *
	 * @return Number of days from start of the year to month include, take into leap year.
	 */
	static uint16_t HowMuchDaysFromStartOfYearTillMonth(uint8_t month, const bool isLeap);

	/**************************
	 * @brief Create particular timer.
	 *
	 * @attention To create empty timer use Timer{ 0 } constructor.
	 *
	 * @param year Year, if less than 1970, return empty timer.
	 * @param month Month, if less than 1 or more than 12, return empty timer.
	 * @param day Day, if less than 1 or more than output of HowMuchDaysInMonth(), return empty timer.
	 * @param hour Hour, if more than 24, return empty timer.
	 * @param minute Minute, if more than 60, return empty timer.
	 * @param second Second, if more than 60, return empty timer.
	 * @param nanosecond Nanosecond, if more than 999999999, return empty timer.
	 *
	 * @test Has unit test.
	 */
	static Timer Create(uint16_t year = 1970, uint8_t month = 1, uint8_t day = 1, uint8_t hour = 0, uint8_t minute = 0,
		uint8_t second = 0, uint32_t nanosecond = 0);

	/**************************
	 * @brief Create a Timer from a string in "YYYY.MM.DD" format, where dot can be any non-digit separator.
	 *
	 * @param dateStr String to parse.
	 *
	 * @return Timer object or empty timer if parsing fails.
	 *
	 * @todo Tests.
	 */
	static FORCE_INLINE Timer Create(std::string_view dateStr)
	{
		int32_t year{ 0 };
		int32_t month{ 0 };
		int32_t day{ 0 };
		size_t index{ 0 };
		size_t size{ dateStr.size() };

		// Parse year
		while (index < size && std::isdigit(dateStr[index])) {
			year = year * 10 + dateStr[index++] - '0';
		}
		// Skip separator
		while (index < size && !std::isdigit(dateStr[index]))
			++index;

		// Parse month
		while (index < size && std::isdigit(dateStr[index])) {
			month = month * 10 + dateStr[index++] - '0';
		}
		// Skip separator
		while (index < size && !std::isdigit(dateStr[index]))
			++index;

		// Parse day
		while (index < size && std::isdigit(dateStr[index])) {
			day = day * 10 + dateStr[index++] - '0';
		}

		return Create(static_cast<uint16_t>(year), static_cast<uint8_t>(month), static_cast<uint8_t>(day));
	}

	/**************************
	 * @return Today timer, with time 00:00:00 UTC+0.
	 */
	static Timer GetToday();

	/**************************
	 * @return Number of seconds to tomorrow UTC+0.
	 */
	static uint32_t GetSecondsToTomorrow();

	/**************************
	 * @return Number of milliseconds to tomorrow UTC+0.
	 */
	static uint32_t GetMillisecondsToTomorrow();

	/**************************
	 * @return Number of microseconds to tomorrow UTC+0.
	 */
	static uint64_t GetMicrosecondsToTomorrow();

	/**************************
	 * @return Number of nanoseconds to tomorrow UTC+0.
	 */
	static uint64_t GetNanosecondsToTomorrow();

	/**************************
	 * @return Chrono nanoseconds converted from timespec.
	 */
	static std::chrono::nanoseconds TimespecToDuration(timespec ts);

	/**************************
	 * @return Timespec converted from chrono nanoseconds.
	 */
	static timespec DurationToTimespec(std::chrono::nanoseconds duration);

	/**************************
	 * @return Chrono time point converted from timespec.
	 */
	static std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds> TimespecToTimePoint(
		timespec ts);

	/**************************
	 * @return Timespec converted from chrono time point.
	 */
	static timespec TimePointToTimespec(
		std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds> tp);

	/**************************
	 * @return Chrono microseconds converted from timeval.
	 */
	static std::chrono::microseconds TimevalToDuration(timeval tv);
};

} //* namespace MSAPI

namespace std {

template <> struct hash<MSAPI::Timer::Date> {
	constexpr size_t operator()(const MSAPI::Timer::Date d) const noexcept
	{
		uint64_t packed{ (static_cast<uint64_t>(d.year) << 8) | (static_cast<uint64_t>(d.month) << 8)
			| static_cast<uint64_t>(d.day) };

		packed ^= packed >> 30;
		packed *= 0xbf58476d1ce4e5b9ULL;
		packed ^= packed >> 27;
		packed *= 0x94d049bb133111ebULL;
		packed ^= packed >> 31;

		return packed;
	}
};

template <typename T, typename S>
	requires(std::is_same_v<T, MSAPI::Timer::Date> && (std::is_integral_v<S> || std::is_floating_point_v<S>))
	|| (std::is_same_v<S, MSAPI::Timer::Date> && (std::is_integral_v<T> || std::is_floating_point_v<T>))
struct hash<std::pair<T, S>> {
	constexpr size_t operator()(const std::pair<T, S> p) const noexcept
	{
		size_t seed{ std::hash<T>()(p.first) };
		const size_t valueHash{ std::hash<S>()(p.second) };
		seed ^= valueHash + 0x9e3779b9U + (seed << 6) + (seed >> 2);
		return seed;
	}
};

} //* namespace std

#endif //* MSAPI_TIME_H