/**************************
 * @file        manager.h
 * @version     6.0
 * @date        2024-04-10
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

#ifndef APPLICATION_HANDLERS_MANAGER_H
#define APPLICATION_HANDLERS_MANAGER_H

#include "../../../../library/source/server/server.h"
#include "../../../../library/source/test/actionsCounter.h"

/**************************
 * @brief Manager for MSAPI tests of application handlers based on standard protocol.
 */
class Manager : public MSAPI::Server, public MSAPI::ActionsCounter {
private:
	int m_clientConnection{ -1 };
	int m_outcomeConnection{ -1 };
	int m_activeConnection{ -1 };
	std::string m_metadata;
	std::map<size_t, std::variant<standardTypes>> m_parametersResponse;
	MSAPI::ActionsCounter m_unhandledActions;

	static constexpr const size_t helloFotHelloCipher{ 59837493028 };

public:
	//* MSAPI::Server
	void HandleBuffer(MSAPI::RecvBufferInfo* recvBufferInfo) final;
	//* MSAPI::Application
	void HandleRunRequest() final;
	void HandlePauseRequest() final;
	void HandleModifyRequest(const std::map<size_t, std::variant<standardTypes>>& parametersUpdate) final;
	void HandleHello(int connection) final;
	void HandleMetadata(int connection, std::string_view metadata) final;
	void HandleParameters(int connection, const std::map<size_t, std::variant<standardTypes>>& parameters) final;

	void UseOutcomeConnection();
	void UseClientConnection();
	void SendData(const MSAPI::StandardProtocol::Data& data);
	void SendActionRun();
	void SendActionPause();
	void SendActionDelete();
	void SendActionHello();
	void SendMetadataRequest();
	void SendParametersRequest();
	void SendMetadataResponse();
	void SendParametersResponse();
	std::string GetParameters() const;
	const std::string& GetMetadata() const noexcept;
	const std::map<size_t, std::variant<standardTypes>>& GetParametersResponse() const noexcept;
	void Stop();
	const size_t& GetUnhandledActions() const noexcept;
	void WaitUnhandledActions(const MSAPI::Test& test, size_t delay, size_t expected);
};

#endif //* APPLICATION_HANDLERS_MANAGER_H