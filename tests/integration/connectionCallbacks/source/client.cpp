/**************************
 * @file        client.cpp
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

#include "client.h"

Client::Client() { MSAPI::Application::SetName("ConnectionCallbacksClient"); }

void Client::HandleBuffer(MSAPI::RecvBufferInfo* recvBufferInfo)
{
MSAPI::DataHeader header{ *recvBufferInfo->buffer };
LOG_ERROR("Unexpected buffer received: " + header.ToString());
}

void Client::HandleOpenConnectionRequest(const int32_t ip, const int16_t port, const bool needReconnection)
{
LOG_DEBUG(
"Open connection request received: ip=" + _S(ip) + ", port=" + _S(port) + ", needReconnection=" + _S(needReconnection));
m_lastOpenConnectionIp = ip;
m_lastOpenConnectionPort = port;
m_lastOpenConnectionNeedReconnection = needReconnection;
m_openConnectionActions.IncrementActionsNumber();
MSAPI::ActionsCounter::IncrementActionsNumber();
}

void Client::HandleCloseConnectionRequest(const int32_t ip, const int16_t port)
{
LOG_DEBUG("Close connection request received: ip=" + _S(ip) + ", port=" + _S(port));
m_lastCloseConnectionIp = ip;
m_lastCloseConnectionPort = port;
m_closeConnectionActions.IncrementActionsNumber();
MSAPI::ActionsCounter::IncrementActionsNumber();
}

const size_t& Client::GetOpenConnectionActions() const noexcept { return m_openConnectionActions.GetActionsNumber(); }

void Client::WaitOpenConnectionActions(const MSAPI::Test& test, const size_t delay, const size_t expected)
{
m_openConnectionActions.WaitActionsNumber(test, delay, expected);
}

const size_t& Client::GetCloseConnectionActions() const noexcept
{
return m_closeConnectionActions.GetActionsNumber();
}

void Client::WaitCloseConnectionActions(const MSAPI::Test& test, const size_t delay, const size_t expected)
{
m_closeConnectionActions.WaitActionsNumber(test, delay, expected);
}

int32_t Client::GetLastOpenConnectionIp() const noexcept { return m_lastOpenConnectionIp; }

int16_t Client::GetLastOpenConnectionPort() const noexcept { return m_lastOpenConnectionPort; }

bool Client::GetLastOpenConnectionNeedReconnection() const noexcept { return m_lastOpenConnectionNeedReconnection; }

int32_t Client::GetLastCloseConnectionIp() const noexcept { return m_lastCloseConnectionIp; }

int16_t Client::GetLastCloseConnectionPort() const noexcept { return m_lastCloseConnectionPort; }
