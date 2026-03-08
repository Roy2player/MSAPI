/**************************
 * @file        observer.inl
 * @version     6.0
 * @date        2026-03-08
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

#ifndef MSAPI_WEBSOCKET_PROTOCOL_OBSERVER_INL
#define MSAPI_WEBSOCKET_PROTOCOL_OBSERVER_INL

#include "../../../../library/source/protocol/webSocket.inl"

namespace MSAPI {

namespace IntegrationTest {

namespace WebSocketProtocol {

class Observer {
private:
	MSAPI::WebSocketProtocol::IHandler& m_handler;

public:
	FORCE_INLINE explicit Observer(MSAPI::WebSocketProtocol::IHandler& handler) noexcept
		: m_handler{ handler }
	{
	}

	FORCE_INLINE [[nodiscard]] bool HasConnectionFragmentedData(const int connection) noexcept
	{
		Pthread::AtomicLock::ExitGuard guard{ m_handler.m_fragmentedDataLock };
		return m_handler.m_fragmentedDataToConnection.contains(connection);
	}

	FORCE_INLINE [[nodiscard]] size_t GetSizeOfFragmentedDataConnections() noexcept
	{
		Pthread::AtomicLock::ExitGuard guard{ m_handler.m_fragmentedDataLock };
		return m_handler.m_fragmentedDataToConnection.size();
	}
};

} // namespace WebSocketProtocol

} // namespace IntegrationTest

} // namespace MSAPI

#endif // MSAPI_WEBSOCKET_PROTOCOL_OBSERVER_INL