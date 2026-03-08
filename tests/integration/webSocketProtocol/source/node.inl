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

class Node : public MSAPI::Server, MSAPI::HTTP::IHandler, public MSAPI::WebSocketProtocol::IHandler {
private:
	std::unordered_map<int, std::vector<MSAPI::WebSocketProtocol::Data>> m_webSocketDataToConnection;
	MSAPI::Pthread::AtomicLock m_webSocketDataLock;
	std::unordered_map<int, std::vector<MSAPI::HTTP::Data>> m_httpDataToConnection;
	MSAPI::Pthread::AtomicLock m_httpDataLock;

public:
	FORCE_INLINE Node() noexcept
		: MSAPI::HTTP::IHandler{ this }
		, MSAPI::WebSocketProtocol::IHandler{ this }
	{
	}

	// MSAPI::Server
	void HandleBuffer(MSAPI::RecvBufferInfo* const recvBufferInfo) final
	{
		MSAPI::DataHeader header{ *recvBufferInfo->buffer };

		MSAPI_HANDLE_WEBSOCKET_PRESET

		if (MSAPI::HTTP::Data http(recvBufferInfo); http.IsValid()) {
			if constexpr (std::is_base_of_v<MSAPI::WebSocketProtocol::IHandler, std::remove_cvref_t<decltype(*this)>>) {

				if (http.IsWebSocketUpgradeRequest()) {
					if (MSAPI::Application::IsRunning()) {
						LOG_PROTOCOL(http.ToString());
						recvBufferInfo->SetReadDataSize(2);
						http.SendWebSocketUpgradeResponse(recvBufferInfo->connection);
						{
							MSAPI::Pthread::AtomicLock::ExitGuard lock{ m_httpDataLock };
							m_httpDataToConnection[recvBufferInfo->connection].emplace_back(http);
						}
						return;
					}
					LOG_PROTOCOL_NEW("Application is not running. {}", http.ToString());
					return;
				}

				if (http.IsWebSocketUpgradeResponse()) {
					if (MSAPI::Application::IsRunning()) {
						LOG_PROTOCOL(http.ToString());
						recvBufferInfo->SetReadDataSize(2);
						{
							MSAPI::Pthread::AtomicLock::ExitGuard lock{ m_httpDataLock };
							m_httpDataToConnection[recvBufferInfo->connection].emplace_back(http);
						}
						return;
					}
					LOG_PROTOCOL_NEW("Application is not running. {}", http.ToString());
					return;
				}
			}

			MSAPI::HTTP::IHandler::Collect(recvBufferInfo->connection, http);
			return;
		}

		LOG_ERROR_NEW("Unknown protocol: {}", header.ToString());
	}

	// MSAPI::HTTP::IHandler
	void HandleHttp([[maybe_unused]] const int connection, [[maybe_unused]] const MSAPI::HTTP::Data& data) final { }

	// MSAPI::WebSocketProtocol::IHandler
	void HandleWebSocket(const int connection, MSAPI::WebSocketProtocol::Data&& data) final
	{
		switch (data.GetOpcode()) {
		case MSAPI::WebSocketProtocol::Data::Opcode::Text:
		case MSAPI::WebSocketProtocol::Data::Opcode::Binary:
		case MSAPI::WebSocketProtocol::Data::Opcode::Close:
		case MSAPI::WebSocketProtocol::Data::Opcode::Continuation: {
			MSAPI::Pthread::AtomicLock::ExitGuard lock{ m_webSocketDataLock };
			m_webSocketDataToConnection[connection].emplace_back(std::move(data));
		} break;
		default:
			LOG_WARNING_NEW("Unexpected WebSocket opcode {}, connection: {}", U(data.GetOpcode()), connection);
			return;
		}
	}

	void HandleWebSocketPong(const int connection, MSAPI::WebSocketProtocol::Data&& data) final
	{
		MSAPI::Pthread::AtomicLock::ExitGuard lock{ m_webSocketDataLock };
		m_webSocketDataToConnection[connection].emplace_back(std::move(data));
	}

	FORCE_INLINE [[nodiscard]] const std::vector<MSAPI::WebSocketProtocol::Data>* GetWebSocketData(const int connection)
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

	FORCE_INLINE void SendWebSocket(const int id, const MSAPI::WebSocketProtocol::Data& data)
	{
		if (const auto connect{ GetConnect(id) }; connect.has_value()) {
			MSAPI::WebSocketProtocol::Send(connect.value(), data);
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