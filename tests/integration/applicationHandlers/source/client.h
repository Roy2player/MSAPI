/**************************
 * @file        client.h
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

#ifndef APPLICATION_HANDLERS_CLIENT_H
#define APPLICATION_HANDLERS_CLIENT_H

#include "../../../../library/source/server/server.h"
#include "../../../../library/source/test/actionsCounter.h"

/**************************
 * @brief Client for MSAPI tests of application handlers based on standard protocol.
 */
class Client : public MSAPI::Server, public MSAPI::ActionsCounter {
private:
	int8_t m_parameter1{ 1 };
	int16_t m_parameter2{ 2 };
	int32_t m_parameter3{ 3 };
	int64_t m_parameter4{ 4 };
	uint8_t m_parameter5{ 5 };
	uint16_t m_parameter6{ 6 };
	uint32_t m_parameter7{ 7 };
	uint64_t m_parameter8{ 8 };
	float m_parameter9{ 9 };
	double m_parameter10{ 10 };
	double m_parameter11{ 11.0 };
	std::optional<int8_t> m_parameter12{ 12 };
	std::optional<int8_t> m_parameter13;
	std::optional<int16_t> m_parameter14{ 14 };
	std::optional<int16_t> m_parameter15;
	std::optional<int32_t> m_parameter16{ 16 };
	std::optional<int32_t> m_parameter17;
	std::optional<int64_t> m_parameter18{ 18 };
	std::optional<int64_t> m_parameter19;
	std::optional<uint8_t> m_parameter20{ 20 };
	std::optional<uint8_t> m_parameter21;
	std::optional<uint16_t> m_parameter22{ 22 };
	std::optional<uint16_t> m_parameter23;
	std::optional<uint32_t> m_parameter24{ 24 };
	std::optional<uint32_t> m_parameter25;
	std::optional<uint64_t> m_parameter26{ 26 };
	std::optional<uint64_t> m_parameter27;
	std::optional<float> m_parameter28{ 28 };
	std::optional<float> m_parameter29;
	std::optional<double> m_parameter30{ 30 };
	std::optional<double> m_parameter31;
	std::optional<double> m_parameter32{ 32 };
	std::optional<double> m_parameter33;
	std::string m_parameter34{ "34" };
	std::string m_parameter35;
	MSAPI::Timer m_parameter36{ MSAPI::Timer::Create(2024, 4, 10, 23, 8, 30) };
	MSAPI::Timer m_parameter37{ 0 };
	MSAPI::Timer::Duration m_parameter38{ MSAPI::Timer::Duration::Create(10, 20, 40, 45, 99987653) };
	MSAPI::Timer::Duration m_parameter39;
	bool m_parameter40{ false };
	MSAPI::Table<bool, bool, std::string, std::string, std::optional<double>> m_parameter41{ { 411, 412, 413, 414,
		415 } };
	MSAPI::Table<size_t, size_t> m_parameter42{ { 4121, 422 } };
	MSAPI::Table<std::string, MSAPI::Timer, MSAPI::Timer::Duration, MSAPI::Timer::Duration, MSAPI::Timer::Duration,
		standardSimpleTypes, std::string, MSAPI::Timer, MSAPI::Timer::Duration, bool, std::string,
		std::optional<double>, std::optional<double>>
		m_parameter43{ { 11111, 22222, 33333, 44444, 55555, 66666, 77777, 88888, 99999, 1010101010, 1111111111,
			1212121212, 1313131313, 1414141414, 1515151515, 1616161616, 1717171717, 1818181818, 1919191919, 2020202020,
			2121212121, 2222222222, 2323232323, 2424242424, 2525252525, 2626262626, 2727272727, 2828282828, 2929292929,
			3030303030, 3131313131, 3232323232, 3333333333 } };
	MSAPI::Table<int> m_parameter44{ { 1 } };

	MSAPI::ActionsCounter m_unhandledActions;

public:
	Client();

	//* MSAPI::Server
	void HandleBuffer(MSAPI::RecvBufferInfo* recvBufferInfo) final;
	//* MSAPI::Application
	void HandleRunRequest() final;
	void HandlePauseRequest() final;
	void HandleModifyRequest(const std::map<size_t, std::variant<standardTypes>>& parametersUpdate) final;
	void HandleHello(int connection) final;
	void HandleMetadata(int connection, std::string_view metadata) final;
	void HandleParameters(int connection, const std::map<size_t, std::variant<standardTypes>>& parameters) final;

	int8_t GetParameter1() const noexcept;
	int16_t GetParameter2() const noexcept;
	int32_t GetParameter3() const noexcept;
	int64_t GetParameter4() const noexcept;
	uint8_t GetParameter5() const noexcept;
	uint16_t GetParameter6() const noexcept;
	uint32_t GetParameter7() const noexcept;
	uint64_t GetParameter8() const noexcept;
	float GetParameter9() const noexcept;
	double GetParameter10() const noexcept;
	double GetParameter11() const noexcept;
	std::optional<int8_t> GetParameter12() const noexcept;
	std::optional<int8_t> GetParameter13() const noexcept;
	std::optional<int16_t> GetParameter14() const noexcept;
	std::optional<int16_t> GetParameter15() const noexcept;
	std::optional<int32_t> GetParameter16() const noexcept;
	std::optional<int32_t> GetParameter17() const noexcept;
	std::optional<int64_t> GetParameter18() const noexcept;
	std::optional<int64_t> GetParameter19() const noexcept;
	std::optional<uint8_t> GetParameter20() const noexcept;
	std::optional<uint8_t> GetParameter21() const noexcept;
	std::optional<uint16_t> GetParameter22() const noexcept;
	std::optional<uint16_t> GetParameter23() const noexcept;
	std::optional<uint32_t> GetParameter24() const noexcept;
	std::optional<uint32_t> GetParameter25() const noexcept;
	std::optional<uint64_t> GetParameter26() const noexcept;
	std::optional<uint64_t> GetParameter27() const noexcept;
	std::optional<float> GetParameter28() const noexcept;
	std::optional<float> GetParameter29() const noexcept;
	std::optional<double> GetParameter30() const noexcept;
	std::optional<double> GetParameter31() const noexcept;
	std::optional<double> GetParameter32() const noexcept;
	std::optional<double> GetParameter33() const noexcept;
	std::string GetParameter34() const noexcept;
	std::string GetParameter35() const noexcept;
	MSAPI::Timer GetParameter36() const noexcept;
	MSAPI::Timer GetParameter37() const noexcept;
	MSAPI::Timer::Duration GetParameter38() const noexcept;
	MSAPI::Timer::Duration GetParameter39() const noexcept;
	bool GetParameter40() const noexcept;
	const MSAPI::Table<bool, bool, std::string, std::string, std::optional<double>>& GetParameter41() const noexcept;
	const MSAPI::Table<uint64_t, uint64_t>& GetParameter42() const noexcept;
	const MSAPI::Table<std::string, MSAPI::Timer, MSAPI::Timer::Duration, MSAPI::Timer::Duration,
		MSAPI::Timer::Duration, standardSimpleTypes, std::string, MSAPI::Timer, MSAPI::Timer::Duration, bool,
		std::string, std::optional<double>, std::optional<double>>&
	GetParameter43() const noexcept;
	const MSAPI::Table<int32_t>& GetParameter44() const noexcept;

	const size_t& GetUnhandledActions() const noexcept;
	void WaitUnhandledActions(const MSAPI::Test& test, size_t delay, size_t expected);
};

#endif //* APPLICATION_HANDLERS_CLIENT_H