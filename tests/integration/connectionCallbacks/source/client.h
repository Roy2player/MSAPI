/**************************
 * @file        client.h
 * @version     6.0
 * @date        2024-12-03
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

#ifndef CONNECTION_CALLBACKS_CLIENT_H
#define CONNECTION_CALLBACKS_CLIENT_H

#include "../../../../library/source/server/server.h"
#include "../../../../library/source/test/actionsCounter.h"

/**************************
 * @brief Client for MSAPI tests of connection callbacks based on standard protocol.
 */
class Client : public MSAPI::Server, public MSAPI::ActionsCounter {
private:
MSAPI::ActionsCounter m_openConnectionActions;
MSAPI::ActionsCounter m_closeConnectionActions;
int32_t m_lastOpenConnectionIp{ 0 };
int16_t m_lastOpenConnectionPort{ 0 };
bool m_lastOpenConnectionNeedReconnection{ false };
int32_t m_lastCloseConnectionIp{ 0 };
int16_t m_lastCloseConnectionPort{ 0 };

public:
Client();

//* MSAPI::Server
void HandleBuffer(MSAPI::RecvBufferInfo* recvBufferInfo) final;
//* MSAPI::Application
void HandleOpenConnectionRequest(int32_t ip, int16_t port, bool needReconnection) final;
void HandleCloseConnectionRequest(int32_t ip, int16_t port) final;

const size_t& GetOpenConnectionActions() const noexcept;
void WaitOpenConnectionActions(const MSAPI::Test& test, size_t delay, size_t expected);
const size_t& GetCloseConnectionActions() const noexcept;
void WaitCloseConnectionActions(const MSAPI::Test& test, size_t delay, size_t expected);
int32_t GetLastOpenConnectionIp() const noexcept;
int16_t GetLastOpenConnectionPort() const noexcept;
bool GetLastOpenConnectionNeedReconnection() const noexcept;
int32_t GetLastCloseConnectionIp() const noexcept;
int16_t GetLastCloseConnectionPort() const noexcept;
};

#endif //* CONNECTION_CALLBACKS_CLIENT_H
