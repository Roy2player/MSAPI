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

#ifndef MSAPI_PROTOCOL_WEBSOCKET_OBSERVER_INL
#define MSAPI_PROTOCOL_WEBSOCKET_OBSERVER_INL

#include "../../../../library/source/protocol/webSocket.inl"

namespace MSAPI {

namespace Tests {

namespace Protocol {

namespace WebSocket {

class Observer {
private:
	MSAPI::Protocol::WebSocket::IHandler& m_handler;

public:
	FORCE_INLINE explicit Observer(MSAPI::Protocol::WebSocket::IHandler& handler) noexcept
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

	FORCE_INLINE [[nodiscard]] double GetStoredFragmentedDataSize() const noexcept
	{
		return m_handler.m_storedFragmentedDataSizeMb;
	}

	FORCE_INLINE [[nodiscard]] size_t GetSizeOfFragmentedDataTimerToConnection() noexcept
	{
		Pthread::AtomicLock::ExitGuard guard{ m_handler.m_fragmentedDataLock };
		return m_handler.m_fragmentedDataTimerToConnection.size();
	}

	FORCE_INLINE [[nodiscard]] MSAPI::Timer GetLastFragmentedDataTimer(const int connection) noexcept
	{
		Pthread::AtomicLock::ExitGuard guard{ m_handler.m_fragmentedDataLock };
		if (const auto it{ m_handler.m_fragmentedDataToConnection.find(connection) };
			it != m_handler.m_fragmentedDataToConnection.end()) {
			return it->second.timestamp;
		}

		return { 0 };
	}

	FORCE_INLINE static uint8_t& GetFirstByte(MSAPI::Protocol::WebSocket::Data& data) noexcept
	{
		return data.m_buffer[0];
	}

	FORCE_INLINE static void ApplyMaskToData(MSAPI::Protocol::WebSocket::Data& data, uint32_t mask = 0) noexcept
	{
		const auto payloadSize{ data.GetPayloadSize() };
		if (payloadSize == 0) [[unlikely]] {
			return;
		}
		const std::span<uint8_t> nonConstPayload{ data.m_buffer.data() + static_cast<size_t>(data.GetHeaderSize()),
			payloadSize };
		if (mask == 0) {
			mask = data.GetMaskingKey();
		}
		MSAPI::Protocol::WebSocket::Data::ApplyMask(nonConstPayload.data(), nonConstPayload.size(), mask);
	}

	FORCE_INLINE static void SetDataCondition(MSAPI::Protocol::WebSocket::Data& data, const int8_t opcode,
		const bool isMasked, const int8_t headerSize, const size_t payloadSize) noexcept
	{
		data.m_buffer[0] = static_cast<uint8_t>(opcode) & 0x0F;
		if (isMasked) {
			data.m_buffer[1] |= 0b10000000;
		}
		else {
			data.m_buffer[1] &= 0b00000000;
		}
		data.m_headerSize = headerSize;
		data.m_buffer.resize(static_cast<size_t>(headerSize) + payloadSize);
	}
};

} // namespace WebSocket

} // namespace Protocol

} // namespace Tests

} // namespace MSAPI

#endif // MSAPI_PROTOCOL_WEBSOCKET_OBSERVER_INL