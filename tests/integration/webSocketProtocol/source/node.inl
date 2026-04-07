/**************************
 * @file        node.inl
 * @version     6.0
 * @date        2026-02-21
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

#ifndef WEBSOCKET_NODE_INL
#define WEBSOCKET_NODE_INL

#include "../../../../library/source/protocol/http.h"
#include "../../../../library/source/protocol/webSocket.inl"
#include "../../../../library/source/server/server.h"
#include "../../../../library/source/test/actionsCounter.h"

namespace Test {

class Node : public MSAPI::Server, MSAPI::HTTP::IHandler, public MSAPI::Protocol::WebSocket::IHandler {
private:
	std::unordered_map<int, std::vector<MSAPI::Protocol::WebSocket::Data>> m_webSocketDataToConnection;
	MSAPI::Pthread::AtomicLock m_webSocketDataLock;
	std::unordered_map<int, std::vector<MSAPI::HTTP::Data>> m_httpDataToConnection;
	MSAPI::Pthread::AtomicLock m_httpDataLock;

public:
	FORCE_INLINE Node() noexcept
		: MSAPI::HTTP::IHandler{ this }
		, MSAPI::Protocol::WebSocket::IHandler{ this }
	{
	}

	// MSAPI::Server
	void HandleBuffer(MSAPI::RecvBufferInfo* const recvBufferInfo) final
	{
		MSAPI_HANDLER_WEBSOCKET_PRESET;

		MSAPI::DataHeader header{ *recvBufferInfo->buffer };

		if (MSAPI::HTTP::Data http(recvBufferInfo); http.IsValid()) {
			{
				MSAPI::Pthread::AtomicLock::ExitGuard lock{ m_httpDataLock };
				m_httpDataToConnection[recvBufferInfo->connection].emplace_back(http);
			}

			MSAPI_HANDLER_HTTP_PRESET_INTERNAL_PART;
		}

		MSAPI_HANDLER_HTTP_PRESET;

		LOG_ERROR_NEW("Unknown protocol: {}", header.ToString());
	}

	// MSAPI::HTTP::IHandler
	void HandleHttp([[maybe_unused]] const int connection, [[maybe_unused]] const MSAPI::HTTP::Data& data) final { }

	// MSAPI::Protocol::WebSocket::IHandler
	void HandleWebSocket(const int connection, MSAPI::Protocol::WebSocket::Data&& data) final
	{
		switch (data.GetOpcode()) {
		case MSAPI::Protocol::WebSocket::Data::Opcode::Text:
		case MSAPI::Protocol::WebSocket::Data::Opcode::Binary:
		case MSAPI::Protocol::WebSocket::Data::Opcode::Close:
		case MSAPI::Protocol::WebSocket::Data::Opcode::Continuation: {
			MSAPI::Pthread::AtomicLock::ExitGuard lock{ m_webSocketDataLock };
			m_webSocketDataToConnection[connection].emplace_back(std::move(data));
		} break;
		default:
			LOG_WARNING_NEW("Unexpected WebSocket opcode {}, connection: {}", U(data.GetOpcode()), connection);
			return;
		}
	}

	void HandleWebSocketPong(const int connection, MSAPI::Protocol::WebSocket::Data&& data) final
	{
		MSAPI::Pthread::AtomicLock::ExitGuard lock{ m_webSocketDataLock };
		m_webSocketDataToConnection[connection].emplace_back(std::move(data));
	}

	// Non const output as test have to modify websocket data in some case
	FORCE_INLINE [[nodiscard]] std::vector<MSAPI::Protocol::WebSocket::Data>* GetWebSocketData(const int connection)
	{
		MSAPI::Pthread::AtomicLock::ExitGuard lock{ m_webSocketDataLock };
		if (const auto it{ m_webSocketDataToConnection.find(connection) }; it != m_webSocketDataToConnection.end()) {
			return &it->second;
		}

		return nullptr;
	}

	FORCE_INLINE [[nodiscard]] const std::vector<MSAPI::HTTP::Data>* GetHttpData(const int connection)
	{
		MSAPI::Pthread::AtomicLock::ExitGuard lock{ m_httpDataLock };
		if (const auto it{ m_httpDataToConnection.find(connection) }; it != m_httpDataToConnection.end()) {
			return &it->second;
		}

		return nullptr;
	}

	FORCE_INLINE void SendHttp(const int id, const std::string& message)
	{
		if (const auto connect{ GetConnect(id) }; connect.has_value()) {
			MSAPI::HTTP::SendRequest(connect.value(), message);
			return;
		}

		LOG_ERROR_NEW("Connection not found for id: {}", id);
	}

	FORCE_INLINE void SendWebSocket(const int id, const MSAPI::Protocol::WebSocket::Data& data)
	{
		if (const auto connect{ GetConnect(id) }; connect.has_value()) {
			MSAPI::Protocol::WebSocket::Send(connect.value(), data);
			return;
		}

		LOG_ERROR_NEW("Connection not found for id: {}", id);
	}

	FORCE_INLINE [[nodiscard]] int DetectConnection(const std::string& key)
	{
		MSAPI::Pthread::AtomicLock::ExitGuard lock{ m_httpDataLock };
		for (const auto& [connection, dataVector] : m_httpDataToConnection) {
			for (const auto& data : dataVector) {
				if (const auto* value{ data.GetValue(key) }; value != nullptr) {
					return connection;
				}
			}
		}

		return -1;
	}

	FORCE_INLINE [[nodiscard]] std::optional<int> GetConnect(const int id) const
	{
		return MSAPI::Server::GetConnect(id);
	}
};

} // namespace Test

#endif // WEBSOCKET_NODE_INL