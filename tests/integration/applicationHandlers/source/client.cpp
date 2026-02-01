/**************************
 * @file        client.cpp
 * @version     6.0
 * @date        2024-04-10
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

#include "client.h"

Client::Client()
{
	MSAPI::Application::RegisterParameter(1, { "Parameter 1 - int8_t", &m_parameter1 });
	MSAPI::Application::RegisterParameter(2, { "Parameter 2 - int16_t", &m_parameter2 });
	MSAPI::Application::RegisterParameter(3, { "Parameter 3 - int32_t", &m_parameter3 });
	MSAPI::Application::RegisterParameter(4, { "Parameter 4 - int64_t", &m_parameter4 });
	MSAPI::Application::RegisterParameter(5, { "Parameter 5 - uint8_t", &m_parameter5 });
	MSAPI::Application::RegisterParameter(6, { "Parameter 6 - uint16_t", &m_parameter6 });
	MSAPI::Application::RegisterParameter(7, { "Parameter 7 - uint32_t", &m_parameter7 });
	MSAPI::Application::RegisterParameter(8, { "Parameter 8 - uint64_t", &m_parameter8 });
	MSAPI::Application::RegisterParameter(9, { "Parameter 9 - float", &m_parameter9 });
	MSAPI::Application::RegisterParameter(10, { "Parameter 10 - double", &m_parameter10 });
	MSAPI::Application::RegisterParameter(11, { "Parameter 11 - double", &m_parameter11 });
	MSAPI::Application::RegisterParameter(12, { "Parameter 12 - optional<int8_t>", &m_parameter12 });
	MSAPI::Application::RegisterParameter(13, { "Parameter 13 - optional<int8_t>", &m_parameter13, {}, {}, true });
	MSAPI::Application::RegisterParameter(14, { "Parameter 14 - optional<int16_t>", &m_parameter14 });
	MSAPI::Application::RegisterParameter(15, { "Parameter 15 - optional<int16_t>", &m_parameter15, {}, {}, true });
	MSAPI::Application::RegisterParameter(16, { "Parameter 16 - optional<int32_t>", &m_parameter16 });
	MSAPI::Application::RegisterParameter(17, { "Parameter 17 - optional<int32_t>", &m_parameter17, {}, {}, true });
	MSAPI::Application::RegisterParameter(18, { "Parameter 18 - optional<int64_t>", &m_parameter18 });
	MSAPI::Application::RegisterParameter(19, { "Parameter 19 - optional<int64_t>", &m_parameter19, {}, {}, true });
	MSAPI::Application::RegisterParameter(20, { "Parameter 20 - optional<uint8_t>", &m_parameter20 });
	MSAPI::Application::RegisterParameter(21, { "Parameter 21 - optional<uint8_t>", &m_parameter21, {}, {}, true });
	MSAPI::Application::RegisterParameter(22, { "Parameter 22 - optional<uint16_t>", &m_parameter22 });
	MSAPI::Application::RegisterParameter(23, { "Parameter 23 - optional<uint16_t>", &m_parameter23, {}, {}, true });
	MSAPI::Application::RegisterParameter(24, { "Parameter 24 - optional<uint32_t>", &m_parameter24 });
	MSAPI::Application::RegisterParameter(25, { "Parameter 25 - optional<uint32_t>", &m_parameter25, {}, {}, true });
	MSAPI::Application::RegisterParameter(26, { "Parameter 26 - optional<uint64_t>", &m_parameter26 });
	MSAPI::Application::RegisterParameter(
		27, { "Parameter 27 - optional<uint64_t>", &m_parameter27, { 300 }, { 6000 } });
	MSAPI::Application::RegisterParameter(28, { "Parameter 28 - optional<float>", &m_parameter28 });
	MSAPI::Application::RegisterParameter(
		29, { "Parameter 29 - optional<float>", &m_parameter29, { -400.001 }, { 400.001 } });
	MSAPI::Application::RegisterParameter(30, { "Parameter 30 - optional<double>", &m_parameter30 });
	MSAPI::Application::RegisterParameter(31, { "Parameter 31 - optional<double>", &m_parameter31, {}, {}, true });
	MSAPI::Application::RegisterParameter(32, { "Parameter 32 - optional<double>", &m_parameter32 });
	MSAPI::Application::RegisterParameter(33, { "Parameter 33 - optional<double>", &m_parameter33, {}, {}, true });
	MSAPI::Application::RegisterParameter(34, { "Parameter 34 - string", &m_parameter34, true });
	MSAPI::Application::RegisterParameter(35, { "Parameter 35 - string", &m_parameter35 });
	MSAPI::Application::RegisterParameter(36, { "Parameter 36 - Timer", &m_parameter36, true });
	MSAPI::Application::RegisterParameter(37, { "Parameter 37 - Timer", &m_parameter37 });
	MSAPI::Application::RegisterParameter(
		38, { "Parameter 38 - Timer::Duration", &m_parameter38, MSAPI::Timer::Duration::Type::Seconds, {}, {}, true });
	MSAPI::Application::RegisterParameter(39,
		{ "Parameter 39 - Timer::Duration", &m_parameter39, MSAPI::Timer::Duration::Type::Seconds, {},
			{ MSAPI::Timer::Duration::CreateSeconds(60) } });
	MSAPI::Application::RegisterParameter(40, { "Parameter 40 - bool", &m_parameter40 });
	MSAPI::Application::RegisterParameter(41, { "Parameter 41 - Table", &m_parameter41, true });
	MSAPI::Application::RegisterParameter(42, { "Parameter 42 - Table", &m_parameter42 });
	MSAPI::Application::RegisterParameter(43, { "Parameter 43 - Table", &m_parameter43, true });
	MSAPI::Application::RegisterParameter(44, { "Parameter 44 - Table", &m_parameter44 });
}

void Client::HandleBuffer(MSAPI::RecvBufferInfo* recvBufferInfo)
{
	MSAPI::DataHeader header{ *recvBufferInfo->buffer };

	LOG_ERROR("Unexpected buffer received: " + header.ToString());
	m_unhandledActions.IncrementActionsNumber();
}

void Client::HandleRunRequest()
{
	if (MSAPI::Application::IsRunning()) {
		LOG_DEBUG("Already running, do nothing");
		MSAPI::ActionsCounter::IncrementActionsNumber();
		return;
	}
	if (MSAPI::Application::AreParametersValid()) {
		LOG_DEBUG("Parameters are valid, set state to Running");
		MSAPI::Application::SetState(MSAPI::Application::State::Running);
	}
	else {
		LOG_DEBUG("Parameters are invalid");
	}
	MSAPI::ActionsCounter::IncrementActionsNumber();
}

void Client::HandlePauseRequest()
{
	if (MSAPI::Application::IsRunning()) {
		LOG_DEBUG("Set state to Paused");
		MSAPI::Application::SetState(MSAPI::Application::State::Paused);
		MSAPI::ActionsCounter::IncrementActionsNumber();
		return;
	}

	LOG_DEBUG("Already paused, do nothing");
	MSAPI::ActionsCounter::IncrementActionsNumber();
}

void Client::HandleModifyRequest(const std::map<size_t, std::variant<standardTypes>>& parametersUpdate)
{
	MSAPI::Application::MergeParameters(parametersUpdate);
	if (!MSAPI::Application::AreParametersValid()) {
		HandlePauseRequest();
	}
	MSAPI::ActionsCounter::IncrementActionsNumber();
}

void Client::HandleHello(const int connection)
{
	LOG_ERROR("Unexpected hello received from connection: " + _S(connection));
	MSAPI::ActionsCounter::IncrementActionsNumber();
}

void Client::HandleMetadata(const int connection, [[maybe_unused]] const std::string_view metadata)
{
	LOG_ERROR("Unexpected metadata received from connection: " + _S(connection));
	MSAPI::ActionsCounter::IncrementActionsNumber();
}

void Client::HandleParameters(
	const int connection, [[maybe_unused]] const std::map<size_t, std::variant<standardTypes>>& parameters)
{
	LOG_ERROR("Unexpected parameters received from connection: " + _S(connection));
	MSAPI::ActionsCounter::IncrementActionsNumber();
}

int8_t Client::GetParameter1() const noexcept { return m_parameter1; }

int16_t Client::GetParameter2() const noexcept { return m_parameter2; }

int32_t Client::GetParameter3() const noexcept { return m_parameter3; }

int64_t Client::GetParameter4() const noexcept { return m_parameter4; }

uint8_t Client::GetParameter5() const noexcept { return m_parameter5; }

uint16_t Client::GetParameter6() const noexcept { return m_parameter6; }

uint32_t Client::GetParameter7() const noexcept { return m_parameter7; }

uint64_t Client::GetParameter8() const noexcept { return m_parameter8; }

float Client::GetParameter9() const noexcept { return m_parameter9; }

double Client::GetParameter10() const noexcept { return m_parameter10; }

double Client::GetParameter11() const noexcept { return m_parameter11; }

std::optional<int8_t> Client::GetParameter12() const noexcept { return m_parameter12; }

std::optional<int8_t> Client::GetParameter13() const noexcept { return m_parameter13; }

std::optional<int16_t> Client::GetParameter14() const noexcept { return m_parameter14; }

std::optional<int16_t> Client::GetParameter15() const noexcept { return m_parameter15; }

std::optional<int32_t> Client::GetParameter16() const noexcept { return m_parameter16; }

std::optional<int32_t> Client::GetParameter17() const noexcept { return m_parameter17; }

std::optional<int64_t> Client::GetParameter18() const noexcept { return m_parameter18; }

std::optional<int64_t> Client::GetParameter19() const noexcept { return m_parameter19; }

std::optional<uint8_t> Client::GetParameter20() const noexcept { return m_parameter20; }

std::optional<uint8_t> Client::GetParameter21() const noexcept { return m_parameter21; }

std::optional<uint16_t> Client::GetParameter22() const noexcept { return m_parameter22; }

std::optional<uint16_t> Client::GetParameter23() const noexcept { return m_parameter23; }

std::optional<uint32_t> Client::GetParameter24() const noexcept { return m_parameter24; }

std::optional<uint32_t> Client::GetParameter25() const noexcept { return m_parameter25; }

std::optional<uint64_t> Client::GetParameter26() const noexcept { return m_parameter26; }

std::optional<uint64_t> Client::GetParameter27() const noexcept { return m_parameter27; }

std::optional<float> Client::GetParameter28() const noexcept { return m_parameter28; }

std::optional<float> Client::GetParameter29() const noexcept { return m_parameter29; }

std::optional<double> Client::GetParameter30() const noexcept { return m_parameter30; }

std::optional<double> Client::GetParameter31() const noexcept { return m_parameter31; }

std::optional<double> Client::GetParameter32() const noexcept { return m_parameter32; }

std::optional<double> Client::GetParameter33() const noexcept { return m_parameter33; }

std::string Client::GetParameter34() const noexcept { return m_parameter34; }

std::string Client::GetParameter35() const noexcept { return m_parameter35; }

MSAPI::Timer Client::GetParameter36() const noexcept { return m_parameter36; }

MSAPI::Timer Client::GetParameter37() const noexcept { return m_parameter37; }

MSAPI::Timer::Duration Client::GetParameter38() const noexcept { return m_parameter38; }

MSAPI::Timer::Duration Client::GetParameter39() const noexcept { return m_parameter39; }

bool Client::GetParameter40() const noexcept { return m_parameter40; }

const MSAPI::Table<bool, bool, std::string, std::string, std::optional<double>>& Client::GetParameter41() const noexcept
{
	return m_parameter41;
}

const MSAPI::Table<uint64_t, uint64_t>& Client::GetParameter42() const noexcept { return m_parameter42; }

const MSAPI::Table<std::string, MSAPI::Timer, MSAPI::Timer::Duration, MSAPI::Timer::Duration, MSAPI::Timer::Duration,
	standardSimpleTypes, std::string, MSAPI::Timer, MSAPI::Timer::Duration, bool, std::string, std::optional<double>,
	std::optional<double>>&
Client::GetParameter43() const noexcept
{
	return m_parameter43;
}

const MSAPI::Table<int32_t>& Client::GetParameter44() const noexcept { return m_parameter44; }

const size_t& Client::GetUnhandledActions() const noexcept { return m_unhandledActions.GetActionsNumber(); }

void Client::WaitUnhandledActions(const MSAPI::Test& test, const size_t delay, const size_t expected)
{
	m_unhandledActions.WaitActionsNumber(test, delay, expected);
}