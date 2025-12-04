/**************************
 * @file        manager.h
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

#ifndef CONNECTION_CALLBACKS_MANAGER_H
#define CONNECTION_CALLBACKS_MANAGER_H

#include "../../../../library/source/server/server.h"
#include "../../../../library/source/test/actionsCounter.h"

/**************************
 * @brief Manager for MSAPI tests of connection callbacks based on standard protocol.
 */
class Manager : public MSAPI::Server, public MSAPI::ActionsCounter {
private:
int m_clientConnection{ -1 };

public:
//* MSAPI::Server
void HandleBuffer(MSAPI::RecvBufferInfo* recvBufferInfo) final;
//* MSAPI::Application
void HandleHello(int connection) final;

void SendActionConnect(int32_t ip, int16_t port, bool needReconnection);
void SendActionDisconnect(int32_t ip, int16_t port);
};

#endif //* CONNECTION_CALLBACKS_MANAGER_H
