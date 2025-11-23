/**************************
 * @file        httpClient.h
 * @version     6.0
 * @date        2024-05-02
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

#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include "../../../../library/source/protocol/http.h"
#include "../../../../library/source/server/server.h"
#include "../../../../library/source/test/actionsCounter.h"

/**************************
 * @brief HTTP client for MSAPI tests of HTTP protocol.
 */
class HTTPClient : public MSAPI::Server, public MSAPI::ActionsCounter, MSAPI::HTTP::IHandler {
private:
	std::optional<MSAPI::HTTP::Data> m_HTTPData;

public:
	HTTPClient();

	//* MSAPI::Server
	void HandleBuffer(MSAPI::RecvBufferInfo* recvBufferInfo) final;
	//* MSAPI::HTTP::IHandler
	void HandleHttp(int connection, const MSAPI::HTTP::Data& data) final;

	const std::optional<MSAPI::HTTP::Data>& GetHTTPData() const noexcept;
	void SendRequest(int id, const std::string& HTTP);
};

#endif //* HTTP_CLIENT_H