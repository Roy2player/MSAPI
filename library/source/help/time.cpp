/**************************
 * @file        time.cpp
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

#include "../test/test.h"
#include <cstring>
#include <iomanip>
#include <sstream>
#include <thread>

namespace MSAPI {

#define HOW_MUCH_DAYS_PER_MONTH(_month, isLeap) dayPerMonth[_month] + static_cast<uint8_t>((_month) == 1 ? isLeap : 0)

/*---------------------------------------------------------------------------------
Event
---------------------------------------------------------------------------------*/

Timer::Event::Event(std::function<void(int*)> callback, int* data)
{
	if (data == nullptr) {
		LOG_ERROR("Interrupted timer event creation due to data is nullptr, id: " + _S(m_uid));
		return;
	}

	m_callback = std::move(callback);
	std::get<2>(m_data) = data;

	m_sev.sigev_notify = SIGEV_THREAD;
	m_sev.sigev_notify_function = [](sigval_t v) {
		(*static_cast<std::function<void(int*)>*>(std::get<0>(*static_cast<std::tuple<void*, int*, int*>*>(
			v.sival_ptr))))(std::get<2>(*((static_cast<std::tuple<void*, int*, int*>*>(v.sival_ptr)))));

		Timer::Event* event{ reinterpret_cast<Timer::Event*>(
			std::get<1>(*((static_cast<std::tuple<void*, int*, int*>*>(v.sival_ptr))))) };
		if (!event->IsRepeat()) {
			event->Stop();
		}
	};

	std::get<0>(m_data) = &m_callback;
	std::get<1>(m_data) = reinterpret_cast<int*>(this);

	m_sev.sigev_value.sival_ptr = static_cast<void*>(&m_data);
}

Timer::Event::Event(Timer::Event::IHandler* handler)
	: m_handler{ handler }
{
	if (m_handler == nullptr) {
		LOG_ERROR("Interrupted timer event creation due to handler is nullptr, id: " + _S(m_uid));
		return;
	}

	m_sev.sigev_notify = SIGEV_THREAD;
	m_sev.sigev_notify_function = [](sigval_t v) {
		Timer::Event* event{ reinterpret_cast<Timer::Event*>(v.sival_ptr) };
		event->m_handler->HandleEvent(*event);
		if (!event->IsRepeat()) {
			event->Stop();
		}
	};

	m_sev.sigev_value.sival_ptr = this;
}

Timer::Event::~Event()
{
	if (m_running) {
		Stop();
	}
}

int64_t Timer::Event::GetId() const { return m_uid; }

#define TMP_MSAPI_TIMER_EVENT_STOP(ret)                                                                                \
	LOG_DEBUG("Stop timer event, id: " + _S(m_uid));                                                                   \
                                                                                                                       \
	if (timer_delete(m_id) != 0) {                                                                                     \
		LOG_ERROR("timer_delete, id " + _S(m_uid) + ". Error №" + _S(errno) + ": " + std::strerror(errno));            \
		return ret;                                                                                                    \
	}                                                                                                                  \
	m_running = false;

bool Timer::Event::Start(const time_t timeToCall, const time_t timeToRepeatCall, const bool instantCall)
{
	if (std::get<2>(m_data) != nullptr) {

#define TMP_MSAPI_TIMER_EVENT_START_BEGIN                                                                              \
	if (m_running) {                                                                                                   \
		TMP_MSAPI_TIMER_EVENT_STOP(false);                                                                             \
	}                                                                                                                  \
                                                                                                                       \
	m_timeToCall = timeToCall;                                                                                         \
	m_timeToRepeatCall = timeToRepeatCall;                                                                             \
	m_instantCall = instantCall;                                                                                       \
                                                                                                                       \
	/* First call */                                                                                                   \
	timespec t1;                                                                                                       \
	t1.tv_sec = m_timeToCall;                                                                                          \
	t1.tv_nsec = 0;                                                                                                    \
	m_its.it_value = t1;                                                                                               \
                                                                                                                       \
	/* Repeat calls */                                                                                                 \
	timespec t2;                                                                                                       \
	t2.tv_sec = m_timeToRepeatCall;                                                                                    \
	t2.tv_nsec = 0;                                                                                                    \
	m_its.it_interval = t2;

		TMP_MSAPI_TIMER_EVENT_START_BEGIN;

		if (m_instantCall) {
			m_callback(std::get<2>(m_data));
		}

#define TMP_MSAPI_TIMER_EVENT_START_END                                                                                \
	int res{ timer_create(CLOCK_REALTIME, &m_sev, &m_id) };                                                            \
	if (res != 0) {                                                                                                    \
		LOG_ERROR("timer_create, id: " + _S(m_uid) + ". Error №" + _S(errno) + ": " + std::strerror(errno));           \
		return false;                                                                                                  \
	}                                                                                                                  \
                                                                                                                       \
	res = timer_settime(m_id, 0, &m_its, nullptr);                                                                     \
	if (res != 0) {                                                                                                    \
		LOG_ERROR("timer_settime, id: " + _S(m_uid) + ". Error №" + _S(errno) + ": " + std::strerror(errno));          \
		return false;                                                                                                  \
	}                                                                                                                  \
                                                                                                                       \
	LOG_DEBUG("Start timer event, id: " + _S(m_uid) + ", time to call: " + _S(UINT64(m_timeToCall))                    \
		+ ", repeat: " + _S(m_timeToRepeatCall != 0) + ", time to repeat call: " + _S(UINT64(m_timeToRepeatCall))      \
		+ ", instant call: " + _S(m_instantCall));                                                                     \
                                                                                                                       \
	m_running = true;                                                                                                  \
	return true;

		TMP_MSAPI_TIMER_EVENT_START_END;
	}

	if (m_handler != nullptr) {
		TMP_MSAPI_TIMER_EVENT_START_BEGIN;

		if (m_instantCall) {
			m_handler->HandleEvent(*this);
		}

		TMP_MSAPI_TIMER_EVENT_START_END;
	}

	LOG_ERROR("Timer event starting is interrupted as it is created with error, id: " + _S(m_uid));
	return false;
}

void Timer::Event::Stop()
{
	if (!m_running) {
		return;
	}

	TMP_MSAPI_TIMER_EVENT_STOP();
}

#undef TMP_MSAPI_TIMER_EVENT_STOP

bool Timer::Event::IsRunning() const { return m_running; }

bool Timer::Event::IsRepeat() const { return m_timeToRepeatCall != 0; }

bool Timer::Event::IsInstantCall() const { return m_instantCall; }

/*---------------------------------------------------------------------------------
Duration
---------------------------------------------------------------------------------*/

Timer::Duration::Duration(const std::chrono::duration<int64_t, std::nano> duration)
	: m_nanoseconds{ duration.count() }
{
}

Timer::Duration::Duration(const int64_t nanoseconds)
	: m_nanoseconds{ nanoseconds }
{
}

Timer::Duration::Duration() { }

Timer::Duration Timer::Duration::Create(
	const int64_t days, const int64_t hours, const int64_t minutes, const int64_t seconds, const int32_t m_nanoseconds)
{
	return std::chrono::seconds(days * SECONDS_IN_DAY) + std::chrono::hours(hours) + std::chrono::minutes(minutes)
		+ std::chrono::seconds(seconds) + std::chrono::nanoseconds(m_nanoseconds);
}

bool Timer::Duration::Empty() const { return m_nanoseconds == 0; }

int64_t Timer::Duration::GetDays() const { return m_nanoseconds / 86400000000000; }

int64_t Timer::Duration::GetHours() const { return m_nanoseconds / 3600000000000; }

int64_t Timer::Duration::GetMinutes() const { return m_nanoseconds / 60000000000; }

int64_t Timer::Duration::GetSeconds() const { return m_nanoseconds / 1000000000; }

int64_t Timer::Duration::GetMilliseconds() const { return m_nanoseconds / 1000000; }

int64_t Timer::Duration::GetMicroseconds() const { return m_nanoseconds / 1000; }

int64_t Timer::Duration::GetNanoseconds() const { return m_nanoseconds; }

std::string Timer::Duration::ToString(const Timer::Duration::Type type) const
{
	static_assert(U(Timer::Duration::Type::Max) == 8, "Missed description for a new duration type enum");
	switch (type) {
	case Timer::Duration::Type::Undefined:
		LOG_ERROR("Undefined duration type, return nanoseconds");
		return std::to_string(m_nanoseconds) + " nanoseconds";
	case Timer::Duration::Type::Nanoseconds:
		return std::to_string(m_nanoseconds) + " nanoseconds";
	case Timer::Duration::Type::Microseconds:
		return std::format("{} microseconds", _S(static_cast<double>(m_nanoseconds) / 1000));
	case Timer::Duration::Type::Milliseconds:
		return std::format("{} milliseconds", _S(static_cast<double>(m_nanoseconds) / 1000000));
	case Timer::Duration::Type::Seconds:
		return std::format("{} seconds", _S(static_cast<double>(m_nanoseconds) / 1000000000));
	case Timer::Duration::Type::Minutes:
		return std::format("{} minutes", _S(static_cast<double>(m_nanoseconds) / 60000000000));
	case Timer::Duration::Type::Hours:
		return std::format("{} hours", _S(static_cast<double>(m_nanoseconds) / 3600000000000));
	case Timer::Duration::Type::Days:
		return std::format("{} days", _S(static_cast<double>(m_nanoseconds) / 86400000000000));
	case Timer::Duration::Type::Max:
		LOG_ERROR("Max duration type, return nanoseconds");
		return std::to_string(m_nanoseconds) + " nanoseconds";
	default:
		LOG_ERROR("Unknown duration type: " + _S(U(type)) + ", return nanoseconds");
		return std::to_string(m_nanoseconds) + " nanoseconds";
	}
}

Timer::Duration::operator std::string() { return ToString(); }

std::ostream& operator<<(std::ostream& out, const Timer::Duration duration) { return out << duration.ToString(); }

std::string operator+(const std::string& str, const Timer::Duration duration) { return str + duration.ToString(); }

Timer::Duration Timer::Duration::CreateDays(const int64_t days)
{
	return { std::chrono::seconds(days * SECONDS_IN_DAY) };
}

Timer::Duration Timer::Duration::CreateHours(const int64_t hours) { return { std::chrono::hours(hours) }; }

Timer::Duration Timer::Duration::CreateMinutes(const int64_t minutes) { return { std::chrono::minutes(minutes) }; }

Timer::Duration Timer::Duration::CreateSeconds(const int64_t seconds) { return { std::chrono::seconds(seconds) }; }

Timer::Duration Timer::Duration::CreateMilliseconds(const int64_t milliseconds)
{
	return { std::chrono::milliseconds(milliseconds) };
}

Timer::Duration Timer::Duration::CreateMicroseconds(const int64_t microseconds)
{
	return { std::chrono::microseconds(microseconds) };
}

Timer::Duration Timer::Duration::CreateNanoseconds(const int64_t nanoseconds)
{
	return std::chrono::nanoseconds(nanoseconds);
}

std::string_view Timer::Duration::EnumToString(const Type type)
{
	static_assert(U(Type::Max) == 8, "Missed description for a new duration type enum");

	switch (type) {
	case Type::Undefined:
		return "Undefined";
	case Type::Nanoseconds:
		return "Nanoseconds";
	case Type::Microseconds:
		return "Microseconds";
	case Type::Milliseconds:
		return "Milliseconds";
	case Type::Seconds:
		return "Seconds";
	case Type::Minutes:
		return "Minutes";
	case Type::Hours:
		return "Hours";
	case Type::Days:
		return "Days";
	case Type::Max:
		return "Max";
	default:
		LOG_ERROR("Unknown duration type: " + _S(U(type)));
		return "Unknown";
	}
}

bool operator<(const Timer::Duration first, const Timer::Duration second)
{
	return first.m_nanoseconds < second.m_nanoseconds;
}

bool operator<=(const Timer::Duration first, const Timer::Duration second) { return !(first > second); }

bool operator>(const Timer::Duration first, const Timer::Duration second)
{
	return first.m_nanoseconds > second.m_nanoseconds;
}

bool operator>=(const Timer::Duration first, const Timer::Duration second) { return !(first < second); }

bool operator==(const Timer::Duration first, const Timer::Duration second)
{
	return first.m_nanoseconds == second.m_nanoseconds;
}

bool operator!=(const Timer::Duration first, const Timer::Duration second) { return !(first == second); }

Timer::Duration operator-(const Timer::Duration first, const Timer::Duration second)
{
	return std::chrono::nanoseconds(first.m_nanoseconds - second.m_nanoseconds);
}

Timer::Duration operator+(const Timer::Duration first, const Timer::Duration second)
{
	return std::chrono::nanoseconds{ first.m_nanoseconds + second.m_nanoseconds };
}

/*---------------------------------------------------------------------------------
Date
---------------------------------------------------------------------------------*/

Timer::Date::Date(const uint16_t year, const uint8_t month, const uint8_t day)
	: year{ year }
	, month{ month }
	, day{ day }
{
}

std::string Timer::Date::ToString() const { return std::format("{:04}-{:02}-{:02}", year, month, day); }

Timer::Date::operator std::string() { return ToString(); }

std::ostream& operator<<(std::ostream& out, const Timer::Date& date) { return out << date.ToString(); }

std::string operator+(const std::string& str, const Timer::Date& date) { return str + date.ToString(); }

bool operator<(const Timer::Date& first, const Timer::Date& second)
{
	return first.year < second.year || (first.year == second.year && first.month < second.month)
		|| (first.year == second.year && first.month == second.month && first.day < second.day);
}

bool operator>(const Timer::Date& first, const Timer::Date& second)
{
	return first.year > second.year || (first.year == second.year && first.month > second.month)
		|| (first.year == second.year && first.month == second.month && first.day > second.day);
}

bool operator==(const Timer::Date& first, const Timer::Date& second)
{
	return first.year == second.year && first.month == second.month && first.day == second.day;
}

bool operator!=(const Timer::Date& first, const Timer::Date& second) { return !(first == second); }

bool operator>=(const Timer::Date& first, const Timer::Date& second) { return !(first < second); }

bool operator<=(const Timer::Date& first, const Timer::Date& second) { return !(first > second); }

/*---------------------------------------------------------------------------------
Timer
---------------------------------------------------------------------------------*/

const std::chrono::system_clock::time_point Timer::m_zero_point{ std::chrono::system_clock::from_time_t(INT64(0)) };

Timer::Timer()
	: m_point{ std::chrono::system_clock::now() }
{
}

Timer::Timer(const std::chrono::system_clock::time_point point)
	: m_point{ point }
{
}

Timer::Timer(const int64_t seconds, const int64_t nanoseconds)
	: m_point{ seconds == 0 && nanoseconds == 0 ? m_zero_point : TimespecToTimePoint(timespec{ seconds, nanoseconds }) }
{
}

bool Timer::Empty() const { return m_point == m_zero_point; }

void Timer::Reset() { m_point = std::chrono::system_clock::now(); }

double Timer::GetTimer() const
{
	return std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1>>>(
		std::chrono::system_clock::now() - m_point)
		.count();
}

std::chrono::time_point<std::chrono::high_resolution_clock> Timer::GetPoint() const { return m_point; }

std::string Timer::ToString() const
{
	int64_t timestamp{ std::chrono::time_point_cast<std::chrono::seconds>(m_point).time_since_epoch().count() };
	auto year{ static_cast<uint16_t>(timestamp / SECONDS_IN_YEAR) };
	timestamp -= INT64(year) * SECONDS_IN_YEAR;
	auto day{ static_cast<uint16_t>(timestamp / SECONDS_IN_DAY) };
	timestamp -= INT64(day) * SECONDS_IN_DAY;

	bool isLeap{ (1970 + year) % 4 == 0 };
	auto extraDays{ static_cast<uint16_t>(isLeap ? (year + 2) / 4 - 1 : (year + 2) / 4) };
	if (extraDays > day) {
		--year;
		isLeap = (1970 + year) % 4 == 0;
		day = static_cast<uint16_t>(day + 365 + static_cast<int32_t>(isLeap) - extraDays);
	}
	else {
		day -= extraDays;
	}

	auto hour{ static_cast<uint8_t>(timestamp / SECONDS_IN_HOUR) };
	timestamp -= SECONDS_IN_HOUR * hour;
	auto minute{ static_cast<uint8_t>(timestamp / SECONDS_IN_MINUTE) };
	auto second{ static_cast<uint8_t>(timestamp - static_cast<uint16_t>(minute) * SECONDS_IN_MINUTE) };

	uint8_t month{ 0 };
	for (uint8_t index{ 0 }; index < 12; ++index) {
		if (const auto daysPerMonth{ static_cast<uint16_t>(HOW_MUCH_DAYS_PER_MONTH(index, isLeap)) };
			day >= daysPerMonth) {

			++month;
			day -= daysPerMonth;
		}
		else {
			break;
		}
	}

	return std::format("{:04}-{:02}-{:02} {:02}:{:02}:{:02}.{:09}", year + 1970, ++month, ++day, hour, minute, second,
		std::chrono::time_point_cast<std::chrono::nanoseconds>(m_point).time_since_epoch().count() % 1000000000);
}

int64_t Timer::GetSeconds() const
{
	return std::chrono::time_point_cast<std::chrono::seconds>(m_point).time_since_epoch().count();
}

int64_t Timer::GetMilliseconds() const
{
	return std::chrono::time_point_cast<std::chrono::milliseconds>(m_point).time_since_epoch().count();
}

int64_t Timer::GetMicroseconds() const
{
	return std::chrono::time_point_cast<std::chrono::microseconds>(m_point).time_since_epoch().count();
}

int64_t Timer::GetNanoseconds() const
{
	return std::chrono::time_point_cast<std::chrono::nanoseconds>(m_point).time_since_epoch().count();
}

Timer::operator std::string() { return ToString(); }

std::ostream& operator<<(std::ostream& out, const Timer timer) { return out << timer.ToString(); }

std::string operator+(const std::string& str, const Timer timer) { return str + timer.ToString(); }

bool operator<(const Timer first, const Timer second) { return first.m_point < second.m_point; }

bool operator<=(const Timer first, const Timer second) { return !(first > second); }

bool operator>(const Timer first, const Timer second) { return first.m_point > second.m_point; }

bool operator>=(const Timer first, const Timer second) { return !(first < second); }

bool operator==(const Timer first, const Timer second) { return first.m_point == second.m_point; }

bool operator!=(const Timer first, const Timer second) { return !(first == second); }

Timer::Duration operator-(const Timer first, const Timer second) { return first.GetPoint() - second.GetPoint(); }

Timer operator+(const Timer timer, const Timer::Duration duration)
{
	return timer.GetPoint() + std::chrono::nanoseconds(duration.GetNanoseconds());
}

Timer operator-(const Timer timer, const Timer::Duration duration)
{
	return timer.GetPoint() - std::chrono::nanoseconds(duration.GetNanoseconds());
}

Timer::Date Timer::ToDate() const
{
	int64_t timestamp{ std::chrono::time_point_cast<std::chrono::seconds>(m_point).time_since_epoch().count() };
	auto year{ static_cast<uint16_t>(timestamp / SECONDS_IN_YEAR) };
	timestamp -= INT64(year) * SECONDS_IN_YEAR;
	auto day{ static_cast<uint16_t>(timestamp / SECONDS_IN_DAY) };

	bool isLeap{ (1970 + year) % 4 == 0 };
	auto extraDays{ static_cast<uint16_t>(isLeap ? (year + 2) / 4 - 1 : (year + 2) / 4) };
	if (extraDays > day) {
		--year;
		isLeap = (1970 + year) % 4 == 0;
		day = static_cast<uint16_t>(day + 365 + static_cast<int32_t>(isLeap) - extraDays);
	}
	else {
		day -= extraDays;
	}

	uint8_t month{ 0 };
	for (uint8_t index{ 0 }; index < 12; ++index) {
		if (const auto daysPerMonth{ static_cast<uint16_t>(HOW_MUCH_DAYS_PER_MONTH(index, isLeap)) };
			day >= daysPerMonth) {

			++month;
			day -= daysPerMonth;
		}
		else {
			break;
		}
	}

	return { static_cast<uint16_t>(year + 1970), ++month, static_cast<uint8_t>(++day) };
}

uint8_t Timer::HowMuchDaysInMonth(const uint8_t month, const bool isLeap)
{
	if (month > 12 || month <= 0) {
		return 0;
	}

	return static_cast<uint8_t>(dayPerMonth[month - 1] + (month == 2 ? isLeap : 0));
}

uint16_t Timer::HowMuchDaysFromStartOfYearTillMonth(const uint8_t month, const bool isLeap)
{
	if (month > 12 || month < 1) {
		return 0;
	}

	uint16_t sum{ 0 };
	for (uint8_t index{ 0 }; index < month - 1; ++index) {
		sum += static_cast<uint16_t>(HOW_MUCH_DAYS_PER_MONTH(index, isLeap));
	}

	return sum;
}

Timer Timer::Create(uint16_t year, const uint8_t month, const uint8_t day, const uint8_t hour, const uint8_t minute,
	const uint8_t second, const uint32_t nanosecond)
{
	if (year < 1970 || month == 0 || month > 12 || day == 0 || day > HowMuchDaysInMonth(month, year) || hour > 23
		|| minute > 60 || second > 59 || nanosecond > 999999999) {

		LOG_WARNING("Unexpected date will be empty: " + _S(year) + "-" + _S(month) + "-" + _S(day) + " " + _S(hour)
			+ ":" + _S(minute) + ":" + _S(second) + "." + _S(nanosecond));
		return Timer{ 0 };
	}

	year -= 1970;
	const bool isLeap{ (year + 2) % 4 == 0 };
	return { INT64(year) * SECONDS_IN_YEAR
			+ INT64((day - 1 /* first day is 0 days left from start of new month */
				  + HowMuchDaysFromStartOfYearTillMonth(month, isLeap)
				  + (isLeap ? (year + 2) / 4 - 1 : (year + 2) / 4)))
				* SECONDS_IN_DAY
			+ INT64(hour) * SECONDS_IN_HOUR + INT64(minute) * SECONDS_IN_MINUTE + INT64(second),
		nanosecond };
}

std::chrono::nanoseconds Timer::TimespecToDuration(const timespec ts)
{
	return std::chrono::duration_cast<std::chrono::nanoseconds>(
		std::chrono::seconds{ ts.tv_sec } + std::chrono::nanoseconds{ ts.tv_nsec });
}

timespec Timer::DurationToTimespec(std::chrono::nanoseconds duration)
{
	const auto seconds{ std::chrono::duration_cast<std::chrono::seconds>(duration) };
	duration -= seconds;

	return timespec{ seconds.count(), duration.count() };
}

std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds> Timer::TimespecToTimePoint(
	const timespec ts)
{
	return std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds>{
		std::chrono::duration_cast<std::chrono::system_clock::duration>(TimespecToDuration(ts))
	};
}

timespec Timer::TimePointToTimespec(
	const std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds> tp)
{
	const auto seconds{ std::chrono::time_point_cast<std::chrono::seconds>(tp) };
	const auto nanoseconds{ std::chrono::time_point_cast<std::chrono::nanoseconds>(tp)
		- std::chrono::time_point_cast<std::chrono::nanoseconds>(seconds) };

	return { seconds.time_since_epoch().count(), nanoseconds.count() };
}

std::chrono::microseconds Timer::TimevalToDuration(timeval tv)
{
	return std::chrono::duration_cast<std::chrono::microseconds>(
		std::chrono::seconds{ tv.tv_sec } + std::chrono::microseconds{ tv.tv_usec });
}

Timer Timer::GetToday()
{
	const auto todayDate{ Timer{}.ToDate() };
	return Timer::Create(todayDate.year, todayDate.month, todayDate.day);
}

uint32_t Timer::GetSecondsToTomorrow()
{
	return static_cast<uint32_t>(SECONDS_IN_DAY + Timer::GetToday().GetSeconds() - Timer{}.GetSeconds());
}

uint32_t Timer::GetMillisecondsToTomorrow()
{
	return static_cast<uint32_t>(MILLISECONDS_IN_DAY + Timer::GetToday().GetMilliseconds() - Timer{}.GetMilliseconds());
}

uint64_t Timer::GetMicrosecondsToTomorrow()
{
	return MICROSECONDS_IN_DAY + UINT64(Timer::GetToday().GetMilliseconds()) - UINT64(Timer{}.GetMilliseconds());
}

uint64_t Timer::GetNanosecondsToTomorrow()
{
	return NANOSECONDS_IN_DAY + UINT64(Timer::GetToday().GetNanoseconds()) - UINT64(Timer{}.GetNanoseconds());
}

bool Timer::UNITTEST()
{
	LOG_INFO_UNITTEST("MSAPI Timer");
	MSAPI::Test t;

	RETURN_IF_FALSE(t.Assert(HowMuchDaysInMonth(1, 2022 % 4 == 0), 31, "Days in month 1, 2022"));
	RETURN_IF_FALSE(t.Assert(HowMuchDaysInMonth(2, 2022 % 4 == 0), 28, "Days in month 2, 2022"));
	RETURN_IF_FALSE(t.Assert(HowMuchDaysInMonth(3, 2022 % 4 == 0), 31, "Days in month 3, 2022"));
	RETURN_IF_FALSE(t.Assert(HowMuchDaysInMonth(1, 2024 % 4 == 0), 31, "Days in month 1, 2024"));
	RETURN_IF_FALSE(t.Assert(HowMuchDaysInMonth(2, 2024 % 4 == 0), 29, "Days in month 2, 2024"));
	RETURN_IF_FALSE(t.Assert(HowMuchDaysInMonth(3, 2024 % 4 == 0), 31, "Days in month 3, 2024"));

	{
		MSAPI::Timer timer{ std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>{
			std::chrono::seconds{ 1734727947 } } };
		RETURN_IF_FALSE(t.Assert(timer.ToString(), "2024-12-20 20:52:27.000000000", "Timer to string №1"));
	}
	{
		MSAPI::Timer timer{ std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>{
			std::chrono::seconds{ 85945150 } } };
		RETURN_IF_FALSE(t.Assert(timer.ToString(), "1972-09-21 17:39:10.000000000", "Timer to string №2"));
	}

	{
		Timer timer;
		Timer timer2;
		RETURN_IF_FALSE(t.Assert(timer < timer2, true, "Timer not greater or equal"));
		const auto timer3{ timer };
		RETURN_IF_FALSE(t.Assert(timer3, timer, "Timer copy equals"));
	}

	{
		RETURN_IF_FALSE(
			t.Assert(Timer::Create(2022).ToString(), "2022-01-01 00:00:00.000000000", "Timer::Create(2022) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2022).ToDate().ToString(), "2022-01-01", "Timer::Create(2022) to date"));
		RETURN_IF_FALSE(t.Assert(
			Timer::Create(2022, 2).ToString(), "2022-02-01 00:00:00.000000000", "Timer::Create(2022,2) to string"));
		RETURN_IF_FALSE(
			t.Assert(Timer::Create(2022, 2).ToDate().ToString(), "2022-02-01", "Timer::Create(2022,2) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2022, 1, 2).ToString(), "2022-01-02 00:00:00.000000000",
			"Timer::Create(2022,1,2) to string"));
		RETURN_IF_FALSE(
			t.Assert(Timer::Create(2022, 1, 2).ToDate().ToString(), "2022-01-02", "Timer::Create(2022,1,2) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2022, 1, 1, 1).ToString(), "2022-01-01 01:00:00.000000000",
			"Timer::Create(2022,1,1,1) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2022, 1, 1, 0, 1).ToString(), "2022-01-01 00:01:00.000000000",
			"Timer::Create(2022,1,1,0,1) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2022, 1, 1, 0, 0, 1).ToString(), "2022-01-01 00:00:01.000000000",
			"Timer::Create(2022,1,1,0,0,1) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2022, 1, 1, 0, 0, 0, 123456789).ToString(),
			"2022-01-01 00:00:00.123456789", "Timer::Create(2022,1,1,0,0,0,123456789) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2022, 1, 1, 0, 0, 0, 1).ToString(), "2022-01-01 00:00:00.000000001",
			"Timer::Create(2022,1,1,0,0,0,1) to string"));
		RETURN_IF_FALSE(
			t.Assert(Timer::Create(1970).ToString(), "1970-01-01 00:00:00.000000000", "Timer::Create(1970) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(1970).ToDate().ToString(), "1970-01-01", "Timer::Create(1970) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(1971, 9, 21, 17, 39, 10, 1234).ToString(),
			"1971-09-21 17:39:10.000001234", "Timer::Create(1971,9,21,17,39,10,1234) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(1971, 9, 21, 17, 39, 10, 1234).ToDate().ToString(), "1971-09-21",
			"Timer::Create(1971,9,21,17,39,10,1234) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(1972, 9, 21, 17, 39, 10, 1234).ToString(),
			"1972-09-21 17:39:10.000001234", "Timer::Create(1972,9,21,17,39,10,1234) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(1972, 9, 21, 17, 39, 10, 1234).ToDate().ToString(), "1972-09-21",
			"Timer::Create(1972,9,21,17,39,10,1234) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(1973, 9, 21, 17, 39, 10, 1234).ToString(),
			"1973-09-21 17:39:10.000001234", "Timer::Create(1973,9,21,17,39,10,1234) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(1973, 9, 21, 17, 39, 10, 1234).ToDate().ToString(), "1973-09-21",
			"Timer::Create(1973,9,21,17,39,10,1234) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(1974, 9, 21, 17, 39, 10, 1234).ToString(),
			"1974-09-21 17:39:10.000001234", "Timer::Create(1974,9,21,17,39,10,1234) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(1974, 9, 21, 17, 39, 10, 1234).ToDate().ToString(), "1974-09-21",
			"Timer::Create(1974,9,21,17,39,10,1234) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(1975, 9, 21, 17, 39, 10, 1234).ToString(),
			"1975-09-21 17:39:10.000001234", "Timer::Create(1975,9,21,17,39,10,1234) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(1975, 9, 21, 17, 39, 10, 1234).ToDate().ToString(), "1975-09-21",
			"Timer::Create(1975,9,21,17,39,10,1234) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(1976, 9, 21, 17, 39, 10, 1234).ToString(),
			"1976-09-21 17:39:10.000001234", "Timer::Create(1976,9,21,17,39,10,1234) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(1976, 9, 21, 17, 39, 10, 1234).ToDate().ToString(), "1976-09-21",
			"Timer::Create(1976,9,21,17,39,10,1234) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(1977, 9, 21, 17, 39, 10, 1234).ToString(),
			"1977-09-21 17:39:10.000001234", "Timer::Create(1977,9,21,17,39,10,1234) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(1977, 9, 21, 17, 39, 10, 1234).ToDate().ToString(), "1977-09-21",
			"Timer::Create(1977,9,21,17,39,10,1234) to date"));
		RETURN_IF_FALSE(
			t.Assert(Timer::Create(1978).ToString(), "1978-01-01 00:00:00.000000000", "Timer::Create(1978) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(1978).ToDate().ToString(), "1978-01-01", "Timer::Create(1978) to date"));
		RETURN_IF_FALSE(
			t.Assert(Timer::Create(1979).ToString(), "1979-01-01 00:00:00.000000000", "Timer::Create(1979) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(1979).ToDate().ToString(), "1979-01-01", "Timer::Create(1979) to date"));
		RETURN_IF_FALSE(
			t.Assert(Timer::Create(1980).ToString(), "1980-01-01 00:00:00.000000000", "Timer::Create(1980) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(1980).ToDate().ToString(), "1980-01-01", "Timer::Create(1980) to date"));
		RETURN_IF_FALSE(
			t.Assert(Timer::Create(1990).ToString(), "1990-01-01 00:00:00.000000000", "Timer::Create(1990) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(1990).ToDate().ToString(), "1990-01-01", "Timer::Create(1990) to date"));
		RETURN_IF_FALSE(
			t.Assert(Timer::Create(2000).ToString(), "2000-01-01 00:00:00.000000000", "Timer::Create(2000) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2000).ToDate().ToString(), "2000-01-01", "Timer::Create(2000) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2024, 12, 20, 0, 1).ToDate().ToString(), "2024-12-20",
			"Timer::Create(2024,12,20,0,1) to date"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2024, 9, 21, 17, 39, 10, 1234).ToString(),
			"2024-09-21 17:39:10.000001234", "MSAPI::Timer::Create(2024,9,21,17,39,10,1234) to string"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2024, 9, 21, 17, 39, 10, 1234).ToString(),
			"2024-09-21 17:39:10.000001234", "MSAPI::Timer::Create(2024,9,21,17,39,10,1234) to string (repeat)"));
		RETURN_IF_FALSE(t.Assert(MSAPI::Timer::Create(2224, 9, 21, 17, 39, 10, 1234).ToString(),
			"2224-09-21 17:39:10.000001234", "MSAPI::Timer::Create(2224,9,21,17,39,10,1234) to string"));

		RETURN_IF_FALSE(
			t.Assert(Timer::Create(2052).ToString(), "2052-01-01 00:00:00.000000000", "Timer::Create(2052) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2052).ToDate().ToString(), "2052-01-01", "Timer::Create(2052) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2052, 2, 29, 0, 0, 0, 1).ToString(), "2052-02-29 00:00:00.000000001",
			"Timer::Create(2052,2,29,0,0,0,1) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2052, 2, 29, 0, 0, 0, 1).ToDate().ToString(), "2052-02-29",
			"Timer::Create(2052,2,29,0,0,0,1) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2052, 3, 1, 0, 0, 0, 1).ToString(), "2052-03-01 00:00:00.000000001",
			"Timer::Create(2052,3,1,0,0,0,1) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2052, 3, 1, 0, 0, 0, 1).ToDate().ToString(), "2052-03-01",
			"Timer::Create(2052,3,1,0,0,0,1) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2052, 7, 20, 20, 20, 20, 1).ToString(), "2052-07-20 20:20:20.000000001",
			"Timer::Create(2052,7,20,20,20,20,1) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2052, 7, 20, 20, 20, 20, 1).ToDate().ToString(), "2052-07-20",
			"Timer::Create(2052,7,20,20,20,20,1) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2052, 12, 31, 20, 20, 20, 1).ToString(), "2052-12-31 20:20:20.000000001",
			"Timer::Create(2052,12,31,20,20,20,1) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2052, 12, 31, 20, 20, 20, 1).ToDate().ToString(), "2052-12-31",
			"Timer::Create(2052,12,31,20,20,20,1) to date"));

		RETURN_IF_FALSE(
			t.Assert(Timer::Create(2053).ToString(), "2053-01-01 00:00:00.000000000", "Timer::Create(2053) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2053).ToDate().ToString(), "2053-01-01", "Timer::Create(2053) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2053, 2, 28, 0, 0, 0, 1).ToString(), "2053-02-28 00:00:00.000000001",
			"Timer::Create(2053,2,28,0,0,0,1) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2053, 2, 28, 0, 0, 0, 1).ToDate().ToString(), "2053-02-28",
			"Timer::Create(2053,2,28,0,0,0,1) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2053, 3, 1, 0, 0, 0, 1).ToString(), "2053-03-01 00:00:00.000000001",
			"Timer::Create(2053,3,1,0,0,0,1) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2053, 3, 1, 0, 0, 0, 1).ToDate().ToString(), "2053-03-01",
			"Timer::Create(2053,3,1,0,0,0,1) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2053, 7, 20, 20, 20, 20, 1).ToString(), "2053-07-20 20:20:20.000000001",
			"Timer::Create(2053,7,20,20,20,20,1) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2053, 7, 20, 20, 20, 20, 1).ToDate().ToString(), "2053-07-20",
			"Timer::Create(2053,7,20,20,20,20,1) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2053, 12, 31, 20, 20, 20, 1).ToString(), "2053-12-31 20:20:20.000000001",
			"Timer::Create(2053,12,31,20,20,20,1) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2053, 12, 31, 20, 20, 20, 1).ToDate().ToString(), "2053-12-31",
			"Timer::Create(2053,12,31,20,20,20,1) to date"));

		RETURN_IF_FALSE(
			t.Assert(Timer::Create(2054).ToString(), "2054-01-01 00:00:00.000000000", "Timer::Create(2054) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2054).ToDate().ToString(), "2054-01-01", "Timer::Create(2054) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2054, 2, 28, 0, 0, 0, 1).ToString(), "2054-02-28 00:00:00.000000001",
			"Timer::Create(2054,2,28,0,0,0,1) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2054, 2, 28, 0, 0, 0, 1).ToDate().ToString(), "2054-02-28",
			"Timer::Create(2054,2,28,0,0,0,1) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2054, 3, 1, 0, 0, 0, 1).ToString(), "2054-03-01 00:00:00.000000001",
			"Timer::Create(2054,3,1,0,0,0,1) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2054, 3, 1, 0, 0, 0, 1).ToDate().ToString(), "2054-03-01",
			"Timer::Create(2054,3,1,0,0,0,1) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2054, 7, 20, 20, 20, 20, 1).ToString(), "2054-07-20 20:20:20.000000001",
			"Timer::Create(2054,7,20,20,20,20,1) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2054, 7, 20, 20, 20, 20, 1).ToDate().ToString(), "2054-07-20",
			"Timer::Create(2054,7,20,20,20,20,1) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2054, 12, 31, 20, 20, 20, 1).ToString(), "2054-12-31 20:20:20.000000001",
			"Timer::Create(2054,12,31,20,20,20,1) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2054, 12, 31, 20, 20, 20, 1).ToDate().ToString(), "2054-12-31",
			"Timer::Create(2054,12,31,20,20,20,1) to date"));

		RETURN_IF_FALSE(
			t.Assert(Timer::Create(2055).ToString(), "2055-01-01 00:00:00.000000000", "Timer::Create(2055) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2055).ToDate().ToString(), "2055-01-01", "Timer::Create(2055) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2055, 2, 28, 0, 0, 0, 1).ToString(), "2055-02-28 00:00:00.000000001",
			"Timer::Create(2055,2,28,0,0,0,1) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2055, 2, 28, 0, 0, 0, 1).ToDate().ToString(), "2055-02-28",
			"Timer::Create(2055,2,28,0,0,0,1) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2055, 3, 1, 0, 0, 0, 1).ToString(), "2055-03-01 00:00:00.000000001",
			"Timer::Create(2055,3,1,0,0,0,1) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2055, 3, 1, 0, 0, 0, 1).ToDate().ToString(), "2055-03-01",
			"Timer::Create(2055,3,1,0,0,0,1) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2055, 7, 20, 20, 20, 20, 1).ToString(), "2055-07-20 20:20:20.000000001",
			"Timer::Create(2055,7,20,20,20,20,1) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2055, 7, 20, 20, 20, 20, 1).ToDate().ToString(), "2055-07-20",
			"Timer::Create(2055,7,20,20,20,20,1) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2055, 12, 31, 20, 20, 20, 1).ToString(), "2055-12-31 20:20:20.000000001",
			"Timer::Create(2055,12,31,20,20,20,1) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2055, 12, 31, 20, 20, 20, 1).ToDate().ToString(), "2055-12-31",
			"Timer::Create(2055,12,31,20,20,20,1) to date"));

		RETURN_IF_FALSE(
			t.Assert(Timer::Create(2156).ToString(), "2156-01-01 00:00:00.000000000", "Timer::Create(2156) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2156).ToDate().ToString(), "2156-01-01", "Timer::Create(2156) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2156, 2, 29, 0, 0, 0, 1).ToString(), "2156-02-29 00:00:00.000000001",
			"Timer::Create(2156,2,29,0,0,0,1) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2156, 2, 29, 0, 0, 0, 1).ToDate().ToString(), "2156-02-29",
			"Timer::Create(2156,2,29,0,0,0,1) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2156, 3, 1, 0, 0, 0, 1).ToString(), "2156-03-01 00:00:00.000000001",
			"Timer::Create(2156,3,1,0,0,0,1) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2156, 3, 1, 0, 0, 0, 1).ToDate().ToString(), "2156-03-01",
			"Timer::Create(2156,3,1,0,0,0,1) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2156, 7, 20, 20, 20, 20, 1).ToString(), "2156-07-20 20:20:20.000000001",
			"Timer::Create(2156,7,20,20,20,20,1) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2156, 7, 20, 20, 20, 20, 1).ToDate().ToString(), "2156-07-20",
			"Timer::Create(2156,7,20,20,20,20,1) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2156, 12, 31, 20, 20, 20, 1).ToString(), "2156-12-31 20:20:20.000000001",
			"Timer::Create(2156,12,31,20,20,20,1) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2156, 12, 31, 20, 20, 20, 1).ToDate().ToString(), "2156-12-31",
			"Timer::Create(2156,12,31,20,20,20,1) to date"));

		RETURN_IF_FALSE(
			t.Assert(Timer::Create(2157).ToString(), "2157-01-01 00:00:00.000000000", "Timer::Create(2157) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2157).ToDate().ToString(), "2157-01-01", "Timer::Create(2157) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2157, 2, 28, 0, 0, 0, 1).ToString(), "2157-02-28 00:00:00.000000001",
			"Timer::Create(2157,2,28,0,0,0,1) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2157, 2, 28, 0, 0, 0, 1).ToDate().ToString(), "2157-02-28",
			"Timer::Create(2157,2,28,0,0,0,1) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2157, 3, 1, 0, 0, 0, 1).ToString(), "2157-03-01 00:00:00.000000001",
			"Timer::Create(2157,3,1,0,0,0,1) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2157, 3, 1, 0, 0, 0, 1).ToDate().ToString(), "2157-03-01",
			"Timer::Create(2157,3,1,0,0,0,1) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2157, 7, 20, 20, 20, 20, 1).ToString(), "2157-07-20 20:20:20.000000001",
			"Timer::Create(2157,7,20,20,20,20,1) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2157, 7, 20, 20, 20, 20, 1).ToDate().ToString(), "2157-07-20",
			"Timer::Create(2157,7,20,20,20,20,1) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2157, 12, 31, 20, 20, 20, 1).ToString(), "2157-12-31 20:20:20.000000001",
			"Timer::Create(2157,12,31,20,20,20,1) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2157, 12, 31, 20, 20, 20, 1).ToDate().ToString(), "2157-12-31",
			"Timer::Create(2157,12,31,20,20,20,1) to date"));

		RETURN_IF_FALSE(
			t.Assert(Timer::Create(2158).ToString(), "2158-01-01 00:00:00.000000000", "Timer::Create(2158) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2158).ToDate().ToString(), "2158-01-01", "Timer::Create(2158) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2158, 2, 28, 0, 0, 0, 1).ToString(), "2158-02-28 00:00:00.000000001",
			"Timer::Create(2158,2,28,0,0,0,1) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2158, 2, 28, 0, 0, 0, 1).ToDate().ToString(), "2158-02-28",
			"Timer::Create(2158,2,28,0,0,0,1) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2158, 3, 1, 0, 0, 0, 1).ToString(), "2158-03-01 00:00:00.000000001",
			"Timer::Create(2158,3,1,0,0,0,1) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2158, 3, 1, 0, 0, 0, 1).ToDate().ToString(), "2158-03-01",
			"Timer::Create(2158,3,1,0,0,0,1) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2158, 7, 20, 20, 20, 20, 1).ToString(), "2158-07-20 20:20:20.000000001",
			"Timer::Create(2158,7,20,20,20,20,1) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2158, 7, 20, 20, 20, 20, 1).ToDate().ToString(), "2158-07-20",
			"Timer::Create(2158,7,20,20,20,20,1) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2158, 12, 31, 20, 20, 20, 1).ToString(), "2158-12-31 20:20:20.000000001",
			"Timer::Create(2158,12,31,20,20,20,1) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2158, 12, 31, 20, 20, 20, 1).ToDate().ToString(), "2158-12-31",
			"Timer::Create(2158,12,31,20,20,20,1) to date"));

		RETURN_IF_FALSE(
			t.Assert(Timer::Create(2159).ToString(), "2159-01-01 00:00:00.000000000", "Timer::Create(2159) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2159).ToDate().ToString(), "2159-01-01", "Timer::Create(2159) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2159, 2, 28, 0, 0, 0, 1).ToString(), "2159-02-28 00:00:00.000000001",
			"Timer::Create(2159,2,28,0,0,0,1) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2159, 2, 28, 0, 0, 0, 1).ToDate().ToString(), "2159-02-28",
			"Timer::Create(2159,2,28,0,0,0,1) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2159, 3, 1, 0, 0, 0, 1).ToString(), "2159-03-01 00:00:00.000000001",
			"Timer::Create(2159,3,1,0,0,0,1) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2159, 3, 1, 0, 0, 0, 1).ToDate().ToString(), "2159-03-01",
			"Timer::Create(2159,3,1,0,0,0,1) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2159, 7, 20, 20, 20, 20, 1).ToString(), "2159-07-20 20:20:20.000000001",
			"Timer::Create(2159,7,20,20,20,20,1) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2159, 7, 20, 20, 20, 20, 1).ToDate().ToString(), "2159-07-20",
			"Timer::Create(2159,7,20,20,20,20,1) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2159, 12, 31, 20, 20, 20, 1).ToString(), "2159-12-31 20:20:20.000000001",
			"Timer::Create(2159,12,31,20,20,20,1) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2159, 12, 31, 20, 20, 20, 1).ToDate().ToString(), "2159-12-31",
			"Timer::Create(2159,12,31,20,20,20,1) to date"));

		RETURN_IF_FALSE(
			t.Assert(Timer::Create(2252).ToString(), "2252-01-01 00:00:00.000000000", "Timer::Create(2252) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2252).ToDate().ToString(), "2252-01-01", "Timer::Create(2252) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2252, 2, 29, 0, 0, 0, 1).ToString(), "2252-02-29 00:00:00.000000001",
			"Timer::Create(2252,2,29,0,0,0,1) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2252, 2, 29, 0, 0, 0, 1).ToDate().ToString(), "2252-02-29",
			"Timer::Create(2252,2,29,0,0,0,1) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2252, 3, 1, 0, 0, 0, 1).ToString(), "2252-03-01 00:00:00.000000001",
			"Timer::Create(2252,3,1,0,0,0,1) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2252, 3, 1, 0, 0, 0, 1).ToDate().ToString(), "2252-03-01",
			"Timer::Create(2252,3,1,0,0,0,1) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2252, 7, 20, 20, 20, 20, 1).ToString(), "2252-07-20 20:20:20.000000001",
			"Timer::Create(2252,7,20,20,20,20,1) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2252, 7, 20, 20, 20, 20, 1).ToDate().ToString(), "2252-07-20",
			"Timer::Create(2252,7,20,20,20,20,1) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2252, 12, 31, 20, 20, 20, 1).ToString(), "2252-12-31 20:20:20.000000001",
			"Timer::Create(2252,12,31,20,20,20,1) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2252, 12, 31, 20, 20, 20, 1).ToDate().ToString(), "2252-12-31",
			"Timer::Create(2252,12,31,20,20,20,1) to date"));

		RETURN_IF_FALSE(
			t.Assert(Timer::Create(2253).ToString(), "2253-01-01 00:00:00.000000000", "Timer::Create(2253) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2253).ToDate().ToString(), "2253-01-01", "Timer::Create(2253) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2253, 2, 28, 0, 0, 0, 1).ToString(), "2253-02-28 00:00:00.000000001",
			"Timer::Create(2253,2,28,0,0,0,1) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2253, 2, 28, 0, 0, 0, 1).ToDate().ToString(), "2253-02-28",
			"Timer::Create(2253,2,28,0,0,0,1) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2253, 3, 1, 0, 0, 0, 1).ToString(), "2253-03-01 00:00:00.000000001",
			"Timer::Create(2253,3,1,0,0,0,1) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2253, 3, 1, 0, 0, 0, 1).ToDate().ToString(), "2253-03-01",
			"Timer::Create(2253,3,1,0,0,0,1) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2253, 7, 20, 20, 20, 20, 1).ToString(), "2253-07-20 20:20:20.000000001",
			"Timer::Create(2253,7,20,20,20,20,1) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2253, 7, 20, 20, 20, 20, 1).ToDate().ToString(), "2253-07-20",
			"Timer::Create(2253,7,20,20,20,20,1) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2253, 12, 31, 20, 20, 20, 1).ToString(), "2253-12-31 20:20:20.000000001",
			"Timer::Create(2253,12,31,20,20,20,1) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2253, 12, 31, 20, 20, 20, 1).ToDate().ToString(), "2253-12-31",
			"Timer::Create(2253,12,31,20,20,20,1) to date"));

		RETURN_IF_FALSE(
			t.Assert(Timer::Create(2254).ToString(), "2254-01-01 00:00:00.000000000", "Timer::Create(2254) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2254).ToDate().ToString(), "2254-01-01", "Timer::Create(2254) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2254, 2, 28, 0, 0, 0, 1).ToString(), "2254-02-28 00:00:00.000000001",
			"Timer::Create(2254,2,28,0,0,0,1) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2254, 2, 28, 0, 0, 0, 1).ToDate().ToString(), "2254-02-28",
			"Timer::Create(2254,2,28,0,0,0,1) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2254, 3, 1, 0, 0, 0, 1).ToString(), "2254-03-01 00:00:00.000000001",
			"Timer::Create(2254,3,1,0,0,0,1) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2254, 3, 1, 0, 0, 0, 1).ToDate().ToString(), "2254-03-01",
			"Timer::Create(2254,3,1,0,0,0,1) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2254, 7, 20, 20, 20, 20, 1).ToString(), "2254-07-20 20:20:20.000000001",
			"Timer::Create(2254,7,20,20,20,20,1) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2254, 7, 20, 20, 20, 20, 1).ToDate().ToString(), "2254-07-20",
			"Timer::Create(2254,7,20,20,20,20,1) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2254, 12, 31, 20, 20, 20, 1).ToString(), "2254-12-31 20:20:20.000000001",
			"Timer::Create(2254,12,31,20,20,20,1) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2254, 12, 31, 20, 20, 20, 1).ToDate().ToString(), "2254-12-31",
			"Timer::Create(2254,12,31,20,20,20,1) to date"));

		RETURN_IF_FALSE(
			t.Assert(Timer::Create(2255).ToString(), "2255-01-01 00:00:00.000000000", "Timer::Create(2255) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2255).ToDate().ToString(), "2255-01-01", "Timer::Create(2255) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2255, 2, 28, 0, 0, 0, 1).ToString(), "2255-02-28 00:00:00.000000001",
			"Timer::Create(2255,2,28,0,0,0,1) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2255, 2, 28, 0, 0, 0, 1).ToDate().ToString(), "2255-02-28",
			"Timer::Create(2255,2,28,0,0,0,1) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2255, 3, 1, 0, 0, 0, 1).ToString(), "2255-03-01 00:00:00.000000001",
			"Timer::Create(2255,3,1,0,0,0,1) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2255, 3, 1, 0, 0, 0, 1).ToDate().ToString(), "2255-03-01",
			"Timer::Create(2255,3,1,0,0,0,1) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2255, 7, 20, 20, 20, 20, 1).ToString(), "2255-07-20 20:20:20.000000001",
			"Timer::Create(2255,7,20,20,20,20,1) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2255, 7, 20, 20, 20, 20, 1).ToDate().ToString(), "2255-07-20",
			"Timer::Create(2255,7,20,20,20,20,1) to date"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2255, 12, 31, 20, 20, 20, 1).ToString(), "2255-12-31 20:20:20.000000001",
			"Timer::Create(2255,12,31,20,20,20,1) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2255, 12, 31, 20, 20, 20, 1).ToDate().ToString(), "2255-12-31",
			"Timer::Create(2255,12,31,20,20,20,1) to date"));

		RETURN_IF_FALSE(t.Assert(Timer::Create(2262, 4, 9, 23, 47, 15, 999999999).ToString(),
			"2262-04-09 23:47:15.999999999", "Timer::Create(2262,4,9,23,47,15,999999999) to string"));
		RETURN_IF_FALSE(t.Assert(Timer::Create(2262, 4, 9, 23, 47, 15, 999999999).ToDate().ToString(), "2262-04-09",
			"Timer::Create(2262,4,9,23,47,15,999999999) to date"));
		RETURN_IF_FALSE(t.Assert(Timer{ 9223372035, 999999999 }.ToString(), "2262-04-09 23:47:15.999999999",
			"Timer{9223372035,999999999} to string"));
	}

	{
		const MSAPI::Timer::Date first{ 2024, 9, 21 }, second{ 2024, 9, 22 };
		RETURN_IF_FALSE(t.Assert(first < second, true, "Date < operator: 2024-09-21 < 2024-09-22"));
		RETURN_IF_FALSE(t.Assert(first <= second, true, "Date <= operator: 2024-09-21 <= 2024-09-22"));
		RETURN_IF_FALSE(t.Assert(first > second, false, "Date > operator: 2024-09-21 > 2024-09-22"));
		RETURN_IF_FALSE(t.Assert(first >= second, false, "Date >= operator: 2024-09-21 >= 2024-09-22"));
		RETURN_IF_FALSE(t.Assert(first == second, false, "Date == operator: 2024-09-21 == 2024-09-22"));
		RETURN_IF_FALSE(t.Assert(first != second, true, "Date != operator: 2024-09-21 != 2024-09-22"));
	}
	{
		const MSAPI::Timer::Date first{ 2024, 9, 22 }, second{ 2024, 10, 22 };
		RETURN_IF_FALSE(t.Assert(first < second, true, "Date < operator: 2024-09-22 < 2024-10-22"));
		RETURN_IF_FALSE(t.Assert(first <= second, true, "Date <= operator: 2024-09-22 <= 2024-10-22"));
		RETURN_IF_FALSE(t.Assert(first > second, false, "Date > operator: 2024-09-22 > 2024-10-22"));
		RETURN_IF_FALSE(t.Assert(first >= second, false, "Date >= operator: 2024-09-22 >= 2024-10-22"));
		RETURN_IF_FALSE(t.Assert(first == second, false, "Date == operator: 2024-09-22 == 2024-10-22"));
		RETURN_IF_FALSE(t.Assert(first != second, true, "Date != operator: 2024-09-22 != 2024-10-22"));
	}
	{
		const MSAPI::Timer::Date first{ 2024, 9, 22 }, second{ 2025, 9, 22 };
		RETURN_IF_FALSE(t.Assert(first < second, true, "Date < operator: 2024-09-22 < 2025-09-22"));
		RETURN_IF_FALSE(t.Assert(first <= second, true, "Date <= operator: 2024-09-22 <= 2025-09-22"));
		RETURN_IF_FALSE(t.Assert(first > second, false, "Date > operator: 2024-09-22 > 2025-09-22"));
		RETURN_IF_FALSE(t.Assert(first >= second, false, "Date >= operator: 2024-09-22 >= 2025-09-22"));
		RETURN_IF_FALSE(t.Assert(first == second, false, "Date == operator: 2024-09-22 == 2025-09-22"));
		RETURN_IF_FALSE(t.Assert(first != second, true, "Date != operator: 2024-09-22 != 2025-09-22"));
	}
	{
		const MSAPI::Timer::Date first{ 2024, 9, 21 }, second{ 2024, 9, 21 };
		RETURN_IF_FALSE(t.Assert(first < second, false, "Date < operator: 2024-09-21 < 2024-09-21"));
		RETURN_IF_FALSE(t.Assert(first <= second, true, "Date <= operator: 2024-09-21 <= 2024-09-21"));
		RETURN_IF_FALSE(t.Assert(first > second, false, "Date > operator: 2024-09-21 > 2024-09-21"));
		RETURN_IF_FALSE(t.Assert(first >= second, true, "Date >= operator: 2024-09-21 >= 2024-09-21"));
		RETURN_IF_FALSE(t.Assert(first == second, true, "Date == operator: 2024-09-21 == 2024-09-21"));
		RETURN_IF_FALSE(t.Assert(first != second, false, "Date != operator: 2024-09-21 != 2024-09-21"));
	}

	{
		auto timer{ Timer::Create(2000) };
		const auto timerMoreNanosecond{ Timer::Create(2000, 1, 1, 0, 0, 0, 1) };
		const auto timerMoreMicrosecond{ Timer::Create(2000, 1, 1, 0, 0, 0, 1000) };
		const auto timerMoreMillisecond{ Timer::Create(2000, 1, 1, 0, 0, 0, 1000000) };
		const auto timerMoreSecond{ Timer::Create(2000, 1, 1, 0, 0, 1) };
		const auto timerMoreMinute{ Timer::Create(2000, 1, 1, 0, 1) };
		const auto timerMoreHour{ Timer::Create(2000, 1, 1, 1) };
		const auto timerMoreDay{ Timer::Create(2000, 1, 2) };

		RETURN_IF_FALSE(t.Assert(
			timerMoreNanosecond - timer, Timer::Duration::CreateNanoseconds(1), "timerMoreNanosecond - timer"));
		RETURN_IF_FALSE(t.Assert(
			timerMoreMicrosecond - timer, Timer::Duration::CreateMicroseconds(1), "timerMoreMicrosecond - timer"));
		RETURN_IF_FALSE(t.Assert(
			timerMoreMillisecond - timer, Timer::Duration::CreateMilliseconds(1), "timerMoreMillisecond - timer"));
		RETURN_IF_FALSE(
			t.Assert(timerMoreSecond - timer, Timer::Duration::CreateSeconds(1), "timerMoreSecond - timer"));
		RETURN_IF_FALSE(
			t.Assert(timerMoreMinute - timer, Timer::Duration::CreateMinutes(1), "timerMoreMinute - timer"));
		RETURN_IF_FALSE(t.Assert(timerMoreHour - timer, Timer::Duration::CreateHours(1), "timerMoreHour - timer"));
		RETURN_IF_FALSE(t.Assert(timerMoreDay - timer, Timer::Duration::CreateDays(1), "timerMoreDay - timer"));

		RETURN_IF_FALSE(
			t.Assert((timer = timer + Timer::Duration::CreateDays(1)), Timer::Create(2000, 1, 2), "timer + 1 day"));
		RETURN_IF_FALSE(t.Assert(
			(timer = timer + Timer::Duration::CreateHours(1)), Timer::Create(2000, 1, 2, 1), "timer + 1 hour"));
		RETURN_IF_FALSE(t.Assert(
			(timer = timer + Timer::Duration::CreateMinutes(1)), Timer::Create(2000, 1, 2, 1, 1), "timer + 1 minute"));
		RETURN_IF_FALSE(t.Assert((timer = timer + Timer::Duration::CreateSeconds(1)),
			Timer::Create(2000, 1, 2, 1, 1, 1), "timer + 1 second"));
		RETURN_IF_FALSE(t.Assert((timer = timer + Timer::Duration::CreateMilliseconds(1)),
			Timer::Create(2000, 1, 2, 1, 1, 1, 1000000), "timer + 1 ms"));
		RETURN_IF_FALSE(t.Assert((timer = timer + Timer::Duration::CreateMicroseconds(1)),
			Timer::Create(2000, 1, 2, 1, 1, 1, 1001000), "timer + 1 us"));
		RETURN_IF_FALSE(t.Assert((timer = timer + Timer::Duration::CreateNanoseconds(1)),
			Timer::Create(2000, 1, 2, 1, 1, 1, 1001001), "timer + 1 ns"));

		RETURN_IF_FALSE(t.Assert((timer = timer - Timer::Duration::CreateDays(1)),
			Timer::Create(2000, 1, 1, 1, 1, 1, 1001001), "timer - 1 day"));
		RETURN_IF_FALSE(t.Assert((timer = timer - Timer::Duration::CreateHours(1)),
			Timer::Create(2000, 1, 1, 0, 1, 1, 1001001), "timer - 1 h"));
		RETURN_IF_FALSE(t.Assert((timer = timer - Timer::Duration::CreateMinutes(1)),
			Timer::Create(2000, 1, 1, 0, 0, 1, 1001001), "timer - 1 min"));
		RETURN_IF_FALSE(t.Assert((timer = timer - Timer::Duration::CreateSeconds(1)),
			Timer::Create(2000, 1, 1, 0, 0, 0, 1001001), "timer - 1 s"));
		RETURN_IF_FALSE(t.Assert((timer = timer - Timer::Duration::CreateMilliseconds(1)),
			Timer::Create(2000, 1, 1, 0, 0, 0, 1001), "timer - 1 ms"));
		RETURN_IF_FALSE(t.Assert((timer = timer - Timer::Duration::CreateMicroseconds(1)),
			Timer::Create(2000, 1, 1, 0, 0, 0, 1), "timer - 1 us"));
		RETURN_IF_FALSE(
			t.Assert((timer = timer - Timer::Duration::CreateNanoseconds(1)), Timer::Create(2000), "timer - 1 ns"));

		RETURN_IF_FALSE(t.Assert(Timer::Duration::Create(1, 1, 1, 1, 1001001),
			Timer::Duration::CreateDays(1) + Timer::Duration::CreateHours(1) + Timer::Duration::CreateMinutes(1)
				+ Timer::Duration::CreateSeconds(1) + Timer::Duration::CreateMilliseconds(1)
				+ Timer::Duration::CreateMicroseconds(1) + Timer::Duration::CreateNanoseconds(1),
			"Duration create sum"));
		RETURN_IF_FALSE(t.Assert(Timer::Duration::Create(0, 22, 58, 58, 998998999),
			Timer::Duration::CreateDays(1) - Timer::Duration::CreateHours(1) - Timer::Duration::CreateMinutes(1)
				- Timer::Duration::CreateSeconds(1) - Timer::Duration::CreateMilliseconds(1)
				- Timer::Duration::CreateMicroseconds(1) - Timer::Duration::CreateNanoseconds(1),
			"Duration create diff"));

		RETURN_IF_FALSE(t.Assert(Timer::Create(2000) + Timer::Duration::Create(1, 1, 1, 1, 1001001),
			Timer::Create(2000, 1, 2, 1, 1, 1, 1001001), "Timer + Duration create sum"));

		RETURN_IF_FALSE(t.Assert(Duration{}.Empty(), true, "Duration empty"));
	}

	{
		const auto duration{ Duration::CreateHours(23) };
		RETURN_IF_FALSE(t.Assert(duration.GetDays(), 0, "Get days 0"));
		RETURN_IF_FALSE(t.Assert(duration.GetHours(), 23, "Get hours 23"));
		RETURN_IF_FALSE(t.Assert(duration.GetMinutes(), 1380, "Get minutes 1380"));
		RETURN_IF_FALSE(t.Assert(duration.GetSeconds(), 82800, "Get seconds 82800"));
		RETURN_IF_FALSE(t.Assert(duration.GetMilliseconds(), 82800000, "Get ms 82800000"));
		RETURN_IF_FALSE(t.Assert(duration.GetMicroseconds(), 82800000000, "Get us 82800000000"));
		RETURN_IF_FALSE(t.Assert(duration.GetNanoseconds(), 82800000000000, "Get ns 82800000000000"));
	}

	{
		const MSAPI::Timer::Duration first{ MSAPI::Timer::Duration::CreateNanoseconds(1) },
			second{ MSAPI::Timer::Duration{ 2 } };
		RETURN_IF_FALSE(t.Assert(first < second, true, "Duration < operator: 1 < 2"));
		RETURN_IF_FALSE(t.Assert(first <= second, true, "Duration <= operator: 1 <= 2"));
		RETURN_IF_FALSE(t.Assert(first > second, false, "Duration > operator: 1 > 2"));
		RETURN_IF_FALSE(t.Assert(first >= second, false, "Duration >= operator: 1 >= 2"));
		RETURN_IF_FALSE(t.Assert(first == second, false, "Duration == operator: 1 == 2"));
		RETURN_IF_FALSE(t.Assert(first != second, true, "Duration != operator: 1 != 2"));
	}
	{
		const MSAPI::Timer::Duration first{ MSAPI::Timer::Duration::CreateNanoseconds(1) },
			second{ MSAPI::Timer::Duration{ 1 } };
		RETURN_IF_FALSE(t.Assert(first < second, false, "Duration < operator: 1 < 1"));
		RETURN_IF_FALSE(t.Assert(first <= second, true, "Duration <= operator: 1 <= 1"));
		RETURN_IF_FALSE(t.Assert(first > second, false, "Duration > operator: 1 > 1"));
		RETURN_IF_FALSE(t.Assert(first >= second, true, "Duration >= operator: 1 >= 1"));
		RETURN_IF_FALSE(t.Assert(first == second, true, "Duration == operator: 1 == 1"));
		RETURN_IF_FALSE(t.Assert(first != second, false, "Duration != operator: 1 != 1"));
	}
	{
		const MSAPI::Timer first{ MSAPI::Timer::Create(1970, 1, 1, 0, 0, 0, 1) },
			second{ MSAPI::Timer::Create(1970, 1, 1, 0, 0, 0, 2) };
		RETURN_IF_FALSE(t.Assert(first < second, true, "Timer < operator: 1 < 2"));
		RETURN_IF_FALSE(t.Assert(first <= second, true, "Timer <= operator: 1 <= 2"));
		RETURN_IF_FALSE(t.Assert(first > second, false, "Timer > operator: 1 > 2"));
		RETURN_IF_FALSE(t.Assert(first >= second, false, "Timer >= operator: 1 >= 2"));
		RETURN_IF_FALSE(t.Assert(first == second, false, "Timer == operator: 1 == 2"));
		RETURN_IF_FALSE(t.Assert(first != second, true, "Timer != operator: 1 != 2"));
	}
	{
		const MSAPI::Timer first{ MSAPI::Timer::Create(1970, 1, 1, 0, 0, 0, 1) },
			second{ MSAPI::Timer::Create(1970, 1, 1, 0, 0, 0, 1) };
		RETURN_IF_FALSE(t.Assert(first < second, false, "Timer < operator: 1 < 1"));
		RETURN_IF_FALSE(t.Assert(first <= second, true, "Timer <= operator: 1 <= 1"));
		RETURN_IF_FALSE(t.Assert(first > second, false, "Timer > operator: 1 > 1"));
		RETURN_IF_FALSE(t.Assert(first >= second, true, "Timer >= operator: 1 >= 1"));
		RETURN_IF_FALSE(t.Assert(first == second, true, "Timer == operator: 1 == 1"));
		RETURN_IF_FALSE(t.Assert(first != second, false, "Timer != operator: 1 != 1"));
	}

	{
		size_t one{ 0 };
		size_t two{ 0 };
		size_t three{ 0 };
		size_t four{ 0 };

		Timer::Event eventOne([](int* parameter) { ++*parameter; }, reinterpret_cast<int*>(&one));
		Timer::Event eventTwo([](int* parameter) { ++*parameter; }, reinterpret_cast<int*>(&two));
		Timer::Event eventThree([](int* parameter) { ++*parameter; }, reinterpret_cast<int*>(&three));
		Timer::Event eventFour([](int* parameter) { ++*parameter; }, reinterpret_cast<int*>(&four));

		eventOne.Start(1);
		eventTwo.Start(1, 0, true);
		eventThree.Start(1, 1, false);
		eventFour.Start(1, 6, false);

		std::this_thread::sleep_for(std::chrono::seconds(4));

		RETURN_IF_FALSE(t.Assert(one, 1, "(1) event one == 1"));
		RETURN_IF_FALSE(t.Assert(!eventOne.IsRunning(), true, "(1) event one is stopped"));
		RETURN_IF_FALSE(t.Assert(two, 2, "(1) event two == 2"));
		RETURN_IF_FALSE(t.Assert(!eventTwo.IsRunning(), true, "(1) event two is stopped"));
		RETURN_IF_FALSE(t.Assert(three > 2, true, "(1) event three > 2"));
		RETURN_IF_FALSE(t.Assert(eventThree.IsRunning(), true, "(1) event three is running"));
		RETURN_IF_FALSE(t.Assert(four, 1, "(1) event four == 1"));
		RETURN_IF_FALSE(t.Assert(eventFour.IsRunning(), true, "(1) event four is running"));

		eventThree.Stop();
		RETURN_IF_FALSE(t.Assert(!eventThree.IsRunning(), true, "(1) event three is stopped"));

		std::this_thread::sleep_for(std::chrono::seconds(4));

		RETURN_IF_FALSE(t.Assert(four, 2, "(1) event four == 2"));
		RETURN_IF_FALSE(t.Assert(eventFour.IsRunning(), true, "(1) event four is running after sleep"));

		eventFour.Stop();
		RETURN_IF_FALSE(t.Assert(!eventFour.IsRunning(), true, "(1) event four is stopped"));
	}

	{
		struct EventHandler : public Event::IHandler {
			int64_t value{};

			void HandleEvent([[maybe_unused]] Event const& event) override { ++value; }
		};

		EventHandler one;
		EventHandler two;
		EventHandler three;
		EventHandler four;

		Timer::Event eventOne(&one);
		Timer::Event eventTwo(&two);
		Timer::Event eventThree(&three);
		Timer::Event eventFour(&four);

		eventOne.Start(1);
		eventTwo.Start(1, 0, true);
		eventThree.Start(1, 1, false);
		eventFour.Start(1, 6, false);

		std::this_thread::sleep_for(std::chrono::seconds(4));

		RETURN_IF_FALSE(t.Assert(one.value, 1, "(2) event one == 1"));
		RETURN_IF_FALSE(t.Assert(!eventOne.IsRunning(), true, "(2) event one is stopped"));
		RETURN_IF_FALSE(t.Assert(two.value, 2, "(2) event two == 2"));
		RETURN_IF_FALSE(t.Assert(!eventTwo.IsRunning(), true, "(2) event two is stopped"));
		RETURN_IF_FALSE(t.Assert(three.value > 2, true, "(2) event three > 2"));
		RETURN_IF_FALSE(t.Assert(eventThree.IsRunning(), true, "(2) event three is running"));
		RETURN_IF_FALSE(t.Assert(four.value, 1, "(2) event four == 1"));
		RETURN_IF_FALSE(t.Assert(eventFour.IsRunning(), true, "(2) event four is running"));

		eventThree.Stop();
		RETURN_IF_FALSE(t.Assert(!eventThree.IsRunning(), true, "(2) event three is stopped"));

		std::this_thread::sleep_for(std::chrono::seconds(4));

		RETURN_IF_FALSE(t.Assert(four.value, 2, "(2) event four == 2"));
		RETURN_IF_FALSE(t.Assert(eventFour.IsRunning(), true, "(2) event four is running after sleep"));

		eventFour.Stop();
		RETURN_IF_FALSE(t.Assert(!eventFour.IsRunning(), true, "(2) event four is stopped"));
	}

	return true;
}

#undef HOW_MUCH_DAYS_PER_MONTH

}; //* namespace MSAPI