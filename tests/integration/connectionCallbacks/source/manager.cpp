/**************************
 * @file        manager.cpp
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

#include "manager.h"

void Manager::HandleBuffer(MSAPI::RecvBufferInfo* recvBufferInfo)
{
MSAPI::DataHeader header{ *recvBufferInfo->buffer };
LOG_ERROR("Unexpected buffer received: " + header.ToString());
}

void Manager::HandleHello(const int connection)
{
if (m_clientConnection == -1) {
m_clientConnection = connection;
}
MSAPI::ActionsCounter::IncrementActionsNumber();
}

void Manager::SendActionConnect(const int32_t ip, const int16_t port, const bool needReconnection)
{
if (m_clientConnection == -1) {
LOG_ERROR("Client connection is not set");
return;
}

MSAPI::StandardProtocol::SendActionConnect(m_clientConnection, ip, port, needReconnection);
}

void Manager::SendActionDisconnect(const int32_t ip, const int16_t port)
{
if (m_clientConnection == -1) {
LOG_ERROR("Client connection is not set");
return;
}

MSAPI::StandardProtocol::SendActionDisconnect(m_clientConnection, ip, port);
}
