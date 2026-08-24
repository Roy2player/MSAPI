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

class Node : public MSAPI::Server, MSAPI::Protocol::HTTP::IHandler, public MSAPI::Protocol::WebSocket::IHandler {
private:
	std::unordered_map<uint64_t, std::vector<MSAPI::Protocol::WebSocket::Data>> m_connectionIdTowebSocketData;
	MSAPI::Lock::Atomic m_webSocketDataLock;
	std::unordered_map<uint64_t, std::vector<MSAPI::Protocol::HTTP::Data>> m_connectionIdTohttpData;
	MSAPI::Lock::Atomic m_httpDataLock;

public:
	FORCE_INLINE Node() noexcept
		: MSAPI::Protocol::HTTP::IHandler{ this }
		, MSAPI::Protocol::WebSocket::IHandler{ this }
	{
	}

	// MSAPI::Server
	void HandleBuffer(MSAPI::RecvBuffer& recvBuffer) final
	{
		MSAPI_HANDLER_WEBSOCKET_PRESET;

		MSAPI::DataHeader header{ recvBuffer.GetBuffer() };

		if (MSAPI::Protocol::HTTP::Data http(recvBuffer); http.IsValid()) {
			{
				MSAPI::Lock::Atomic::Guard _{ m_httpDataLock };
				m_connectionIdTohttpData[recvBuffer.GetConnectionId()].emplace_back(http);
			}

			MSAPI_HANDLER_HTTP_PRESET_INTERNAL_PART;
		}

		MSAPI_HANDLER_HTTP_PRESET;

		LOG_ERROR_NEW("Unknown protocol: {}", header.ToString());
	}

	// MSAPI::Protocol::HTTP::IHandler
	void HandleHttp([[maybe_unused]] const std::shared_ptr<MSAPI::Connection::Data>& connectionData,
		[[maybe_unused]] const MSAPI::Protocol::HTTP::Data& data) final
	{
	}

	// MSAPI::Protocol::WebSocket::IHandler
	void HandleWebSocket(
		const std::shared_ptr<MSAPI::Connection::Data>& connectionData, MSAPI::Protocol::WebSocket::Data&& data) final
	{
		switch (data.GetOpcode()) {
		case MSAPI::Protocol::WebSocket::Data::Opcode::Text:
		case MSAPI::Protocol::WebSocket::Data::Opcode::Binary:
		case MSAPI::Protocol::WebSocket::Data::Opcode::Close:
		case MSAPI::Protocol::WebSocket::Data::Opcode::Continuation: {
			MSAPI::Lock::Atomic::Guard _{ m_webSocketDataLock };
			m_connectionIdTowebSocketData[connectionData->GetConnectionId()].emplace_back(std::move(data));
		} break;
		default:
			LOG_WARNING_NEW("Unexpected WebSocket opcode {}, connection id: {}", U(data.GetOpcode()),
				connectionData->GetConnectionId());
			return;
		}
	}

	void HandleWebSocketPong(
		const std::shared_ptr<MSAPI::Connection::Data>& connectionData, MSAPI::Protocol::WebSocket::Data&& data) final
	{
		MSAPI::Lock::Atomic::Guard _{ m_webSocketDataLock };
		m_connectionIdTowebSocketData[connectionData->GetConnectionId()].emplace_back(std::move(data));
	}

	// Non const output as test have to modify websocket data in some case
	FORCE_INLINE [[nodiscard]] std::vector<MSAPI::Protocol::WebSocket::Data>* GetWebSocketData(
		const std::shared_ptr<MSAPI::Connection::Data>& connectionData)
	{
		MSAPI::Lock::Atomic::Guard _{ m_webSocketDataLock };
		if (const auto it{ m_connectionIdTowebSocketData.find(connectionData->GetConnectionId()) };
			it != m_connectionIdTowebSocketData.end()) {
			return &it->second;
		}

		return nullptr;
	}

	FORCE_INLINE [[nodiscard]] const std::vector<MSAPI::Protocol::HTTP::Data>* GetHttpData(const uint64_t connectionId)
	{
		MSAPI::Lock::Atomic::Guard _{ m_httpDataLock };
		if (const auto it{ m_connectionIdTohttpData.find(connectionId) }; it != m_connectionIdTohttpData.end()) {
			return &it->second;
		}

		return nullptr;
	}

	FORCE_INLINE void SendHttp(const uint64_t id, const std::string& message)
	{
		if (const auto connect{ GetConnection(id) }; connect != nullptr) {
			MSAPI::Protocol::HTTP::SendRequest(connect, message);
			return;
		}

		LOG_ERROR_NEW("Connection id: {} is not found", id);
	}

	FORCE_INLINE void SendWebSocket(const uint64_t id, const MSAPI::Protocol::WebSocket::Data& data)
	{
		if (const auto connect{ GetConnection(id) }; connect != nullptr) {
			MSAPI::Protocol::WebSocket::Send(connect, data);
			return;
		}

		LOG_ERROR_NEW("Connection id: {} is not found", id);
	}

	FORCE_INLINE [[nodiscard]] uint64_t DetectConnection(const std::string& key)
	{
		MSAPI::Lock::Atomic::Guard _{ m_httpDataLock };
		for (const auto& [connectionId, dataVector] : m_connectionIdTohttpData) {
			for (const auto& data : dataVector) {
				if (const auto* value{ data.GetValue(key) }; value != nullptr) {
					return connectionId;
				}
			}
		}

		return -1;
	}

	FORCE_INLINE [[nodiscard]] std::shared_ptr<MSAPI::Connection::Data> GetConnectionData(const uint64_t id) const
	{
		return MSAPI::Server::GetConnectionData(id);
	}
};

} // namespace Test

#endif // WEBSOCKET_NODE_INL