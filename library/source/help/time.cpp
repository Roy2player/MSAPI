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


#undef HOW_MUCH_DAYS_PER_MONTH

}; //* namespace MSAPI