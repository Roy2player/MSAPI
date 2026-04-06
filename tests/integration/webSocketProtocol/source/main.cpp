/**************************
 * @file        main.cpp
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
 *
 * 1. Common single checking part
 * 1.1. Check validity condition of websocket data
 * 1.2. Initial state of WebSocket Handler on server
 * 1.3. Merge in message without payload
 * 1.4. Merge message with payload in message with payload <= 125 to get total payload size > 65535
 * 1.5. Default constructor without parameters
 * 1.6. Constructor with only required parameters
 * 1.7. Try to create SplitGenerator with empty payload
 * 1.8. Try to create SplitGenerator with payload less than minimum
 * 1.9. Try to create SplitGenerator with zero step
 * 1.10. Try to create SplitGenerator with step greater than payload size
 * 1.11. Try to create split generator for different opcodes
 * 1.12. Try to create split generator for different buffer type
 * 1.13. Try to create close message with payload greater than 123 bytes
 * 1.14. Try to create close message with different buffer type
 * 1.15. Test expected header size function
 * 2. Parallel execution part. Two non-parallel threads: one without masking, another with
 * 2.1. Try to establish WebSocket handshake with unsupported protocol version
 * 2.2. Establish WebSocket handshake
 * 2.3. Send empty text message
 * 2.4. Send empty binary message
 * 2.5. Send empty ping message
 * 2.6. Send empty pong message
 * 2.7. Send small text message
 * 2.8. Send small binary message
 * 2.9. Send medium text message
 * 2.10. Send medium binary message
 * 2.11. Send large text message
 * 2.12. Send large binary message
 * 2.13. Send ping message with small payload
 * 2.14. Send pong message with small payload
 * 2.15. Try to create ping message with payload greater than 125 bytes
 * 2.16. Try to create pong message with payload greater than 125 bytes
 * 2.17. Send empty close message
 * 2.18. Send close message with small payload
 * 2.19. Send small fragmented message with different steps size
 * 2.20. Send medium fragmented message with different steps size
 * 2.21. Send large fragmented message with different steps size
 * 2.22. Overwrite fragmented message with new initial message
 * 3. Common single checking part
 * 3.1. Final state of WebSocket Handler on server
 * 3.2. Check fragmented data storage purging due to limits with one connection scenarios
 * 3.2.1. Send fragmented message with initial payload size greater than fragmented data limit
 * 3.2.2. Send continuation message without initial message
 * 3.2.3. Send fragmented message with total payload size greater than fragmented data limit
 * 3.2.4. Send fragmented message with payload size greater than fragmented data limit when there is already fragmented
 * data stored on server for client 3.2.5. Check zero fragmented data limit with 1 byte payload messages 3.2.6. Check
 * zero fragmented data limit with empty payload messages 3.2.7. Test for increasing, reducing and zeroing fragmented
 * data limit 3.2.8. Test clear and clear connection
 */

#include "../../../../library/source/help/io.inl"
#include "../../../../library/source/test/daemon.hpp"
#include "../../../../library/source/test/test.h"
#include "node.inl"
#include "observer.inl"
#include <memory>
#include <sys/mman.h>
#include <sys/resource.h>

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
	MSAPI_MLOCKALL_CURRENT_FUTURE

	std::string path;
	path.resize(512);
	MSAPI::Helper::GetExecutableDir(path);
	if (path.empty()) [[unlikely]] {
		return 1;
	}
	path += "../";
	MSAPI::logger.SetParentPath(path);
	path += "logs/";

	// Clear old files
	std::vector<std::string> files;
	if (MSAPI::IO::List<MSAPI::IO::FileType::Regular>(files, path.c_str())) {
		for (const auto& file : files) {
			(void)MSAPI::IO::Remove((path + file).c_str());
		}
	}

	MSAPI::logger.SetLevelSave(MSAPI::Log::Level::INFO);
	MSAPI::logger.SetName("TestWSP");
	MSAPI::logger.SetToFile(true);
	MSAPI::logger.SetToConsole(true);
	MSAPI::logger.Start();

	static_assert(std::is_same_v<std::underlying_type_t<MSAPI::Protocol::WebSocket::Data::Opcode>, int8_t>);
	static_assert(static_cast<int8_t>(MSAPI::Protocol::WebSocket::Data::Opcode::Continuation) == 0x0);
	static_assert(static_cast<int8_t>(MSAPI::Protocol::WebSocket::Data::Opcode::Text) == 0x1);
	static_assert(static_cast<int8_t>(MSAPI::Protocol::WebSocket::Data::Opcode::Binary) == 0x2);
	static_assert(static_cast<int8_t>(MSAPI::Protocol::WebSocket::Data::Opcode::Close) == 0x8);
	static_assert(static_cast<int8_t>(MSAPI::Protocol::WebSocket::Data::Opcode::Ping) == 0x9);
	static_assert(static_cast<int8_t>(MSAPI::Protocol::WebSocket::Data::Opcode::Pong) == 0xA);

	static_assert(std::is_same_v<std::underlying_type_t<MSAPI::Protocol::WebSocket::Data::CloseStatusCode>, int16_t>);
	static_assert(static_cast<int16_t>(MSAPI::Protocol::WebSocket::Data::CloseStatusCode::NormalClosure) == 1000);
	static_assert(static_cast<int16_t>(MSAPI::Protocol::WebSocket::Data::CloseStatusCode::GoingAway) == 1001);
	static_assert(static_cast<int16_t>(MSAPI::Protocol::WebSocket::Data::CloseStatusCode::ProtocolError) == 1002);
	static_assert(static_cast<int16_t>(MSAPI::Protocol::WebSocket::Data::CloseStatusCode::UnsupportedData) == 1003);
	static_assert(static_cast<int16_t>(MSAPI::Protocol::WebSocket::Data::CloseStatusCode::Reserved) == 1004);
	static_assert(static_cast<int16_t>(MSAPI::Protocol::WebSocket::Data::CloseStatusCode::NoStatusReceived) == 1005);
	static_assert(static_cast<int16_t>(MSAPI::Protocol::WebSocket::Data::CloseStatusCode::AbnormalClosure) == 1006);
	static_assert(
		static_cast<int16_t>(MSAPI::Protocol::WebSocket::Data::CloseStatusCode::InvalidFramePayloadData) == 1007);
	static_assert(static_cast<int16_t>(MSAPI::Protocol::WebSocket::Data::CloseStatusCode::PolicyViolation) == 1008);
	static_assert(static_cast<int16_t>(MSAPI::Protocol::WebSocket::Data::CloseStatusCode::MessageTooBig) == 1009);
	static_assert(static_cast<int16_t>(MSAPI::Protocol::WebSocket::Data::CloseStatusCode::MandatoryExtension) == 1010);
	static_assert(static_cast<int16_t>(MSAPI::Protocol::WebSocket::Data::CloseStatusCode::InternalError) == 1011);
	static_assert(static_cast<int16_t>(MSAPI::Protocol::WebSocket::Data::CloseStatusCode::ServiceRestart) == 1012);
	static_assert(static_cast<int16_t>(MSAPI::Protocol::WebSocket::Data::CloseStatusCode::TryAgainLater) == 1013);
	static_assert(static_cast<int16_t>(MSAPI::Protocol::WebSocket::Data::CloseStatusCode::BadGateway) == 1014);
	static_assert(static_cast<int16_t>(MSAPI::Protocol::WebSocket::Data::CloseStatusCode::TLSHandshakeFailure) == 1015);

	// Server
	const int32_t serverId{ 1 };
	auto serverPtr{ MSAPI::Daemon<Test::Node>::Create("Server") };
	if (serverPtr == nullptr) {
		return 1;
	}
	auto server{ static_cast<Test::Node*>(serverPtr->GetApp()) };
	server->HandleRunRequest();

	MSAPI::Tests::Protocol::WebSocket::Observer serverObserver{ *server };

	std::array<char, static_cast<size_t>(MSAPI::Protocol::WebSocket::Data::MB) * 5> payload{};
	for (size_t i{}; i < payload.size(); i++) {
		payload[i] = static_cast<char>(i % (128 - 32) + 32); // Printable ASCII characters
	}
	const auto payloadSpan{ std::span<const uint8_t>{
		reinterpret_cast<const uint8_t*>(payload.data()), payload.size() } };
	const auto nonConstPayloadSpan{ std::span<uint8_t>{ reinterpret_cast<uint8_t*>(payload.data()), payload.size() } };

	MSAPI::Test test;

	// 1.1. Check validity condition of websocket data
	{
		struct DataCondition {
			const size_t payloadSize;
			const int8_t headerSize;
			const int8_t opcode;
			const bool isMasked;
			const bool isValid;
		};
		std::array<DataCondition, 45> conditions{ { // Opcode based conditions
			{ .payloadSize = 0, .headerSize = 2, .opcode = -1, .isMasked = false, .isValid = false },
			{ .payloadSize = 0, .headerSize = 2, .opcode = 0x0, .isMasked = false, .isValid = true },
			{ .payloadSize = 0, .headerSize = 2, .opcode = 0x1, .isMasked = false, .isValid = true },
			{ .payloadSize = 0, .headerSize = 2, .opcode = 0x2, .isMasked = false, .isValid = true },
			{ .payloadSize = 0, .headerSize = 2, .opcode = 0x3, .isMasked = false, .isValid = false },
			{ .payloadSize = 0, .headerSize = 2, .opcode = 0x4, .isMasked = false, .isValid = false },
			{ .payloadSize = 0, .headerSize = 2, .opcode = 0x5, .isMasked = false, .isValid = false },
			{ .payloadSize = 0, .headerSize = 2, .opcode = 0x6, .isMasked = false, .isValid = false },
			{ .payloadSize = 0, .headerSize = 2, .opcode = 0x7, .isMasked = false, .isValid = false },
			{ .payloadSize = 0, .headerSize = 2, .opcode = 0x8, .isMasked = false, .isValid = true },
			{ .payloadSize = 0, .headerSize = 2, .opcode = 0x9, .isMasked = false, .isValid = true },
			{ .payloadSize = 0, .headerSize = 2, .opcode = 0xA, .isMasked = false, .isValid = true },
			{ .payloadSize = 0, .headerSize = 2, .opcode = 0xB, .isMasked = false, .isValid = false }
			// Header size calculation conditions for small data
			,
			{ .payloadSize = 0, .headerSize = 3, .opcode = 0x0, .isMasked = false, .isValid = false },
			{ .payloadSize = 0, .headerSize = 5, .opcode = 0x0, .isMasked = true, .isValid = false },
			{ .payloadSize = 0, .headerSize = 6, .opcode = 0x0, .isMasked = true, .isValid = true },
			{ .payloadSize = 0, .headerSize = 7, .opcode = 0x0, .isMasked = true, .isValid = false },
			{ .payloadSize = 125, .headerSize = 3, .opcode = 0x0, .isMasked = false, .isValid = false },
			{ .payloadSize = 125, .headerSize = 5, .opcode = 0x0, .isMasked = true, .isValid = false },
			{ .payloadSize = 125, .headerSize = 6, .opcode = 0x0, .isMasked = true, .isValid = true },
			{ .payloadSize = 125, .headerSize = 7, .opcode = 0x0, .isMasked = true, .isValid = false }
			// Header size calculation conditions for medium data
			,
			{ .payloadSize = 126, .headerSize = 3, .opcode = 0x0, .isMasked = false, .isValid = false },
			{ .payloadSize = 126, .headerSize = 4, .opcode = 0x0, .isMasked = false, .isValid = true },
			{ .payloadSize = 126, .headerSize = 5, .opcode = 0x0, .isMasked = false, .isValid = false },
			{ .payloadSize = 126, .headerSize = 7, .opcode = 0x0, .isMasked = true, .isValid = false },
			{ .payloadSize = 126, .headerSize = 8, .opcode = 0x0, .isMasked = true, .isValid = true },
			{ .payloadSize = 126, .headerSize = 9, .opcode = 0x0, .isMasked = true, .isValid = false },
			{ .payloadSize = 65535, .headerSize = 3, .opcode = 0x0, .isMasked = false, .isValid = false },
			{ .payloadSize = 65535, .headerSize = 4, .opcode = 0x0, .isMasked = false, .isValid = true },
			{ .payloadSize = 65535, .headerSize = 5, .opcode = 0x0, .isMasked = false, .isValid = false },
			{ .payloadSize = 65535, .headerSize = 7, .opcode = 0x0, .isMasked = true, .isValid = false },
			{ .payloadSize = 65535, .headerSize = 8, .opcode = 0x0, .isMasked = true, .isValid = true },
			{ .payloadSize = 65535, .headerSize = 9, .opcode = 0x0, .isMasked = true, .isValid = false }
			// Header size calculation conditions for huge data
			,
			{ .payloadSize = 65536, .headerSize = 9, .opcode = 0x0, .isMasked = false, .isValid = false },
			{ .payloadSize = 65536, .headerSize = 10, .opcode = 0x0, .isMasked = false, .isValid = true },
			{ .payloadSize = 65536, .headerSize = 11, .opcode = 0x0, .isMasked = false, .isValid = false },
			{ .payloadSize = 65536, .headerSize = 13, .opcode = 0x0, .isMasked = true, .isValid = false },
			{ .payloadSize = 65536, .headerSize = 14, .opcode = 0x0, .isMasked = true, .isValid = true },
			{ .payloadSize = 65536, .headerSize = 15, .opcode = 0x0, .isMasked = true, .isValid = false },
			{ .payloadSize = 65536 * 2, .headerSize = 9, .opcode = 0x0, .isMasked = false, .isValid = false },
			{ .payloadSize = 65536 * 2, .headerSize = 10, .opcode = 0x0, .isMasked = false, .isValid = true },
			{ .payloadSize = 65536 * 2, .headerSize = 11, .opcode = 0x0, .isMasked = false, .isValid = false },
			{ .payloadSize = 65536 * 2, .headerSize = 13, .opcode = 0x0, .isMasked = true, .isValid = false },
			{ .payloadSize = 65536 * 2, .headerSize = 14, .opcode = 0x0, .isMasked = true, .isValid = true },
			{ .payloadSize = 65536 * 2, .headerSize = 15, .opcode = 0x0, .isMasked = true, .isValid = false } } };

		MSAPI::Protocol::WebSocket::Data data;
		for (const auto& condition : conditions) {
			MSAPI::Tests::Protocol::WebSocket::Observer::SetDataCondition(
				data, condition.opcode, condition.isMasked, condition.headerSize, condition.payloadSize);
			RETURN_IF_FALSE(test.Assert(data.IsValid(), condition.isValid, "Expected data validity state"));
		}
	}

	const auto checkMessage{ [&test](MSAPI::Protocol::WebSocket::Data& message, const bool isFinal, const bool rsv1,
								 const bool rsv2, const bool rsv3,
								 const MSAPI::Protocol::WebSocket::Data::Opcode opcode, const bool isMasked,
								 const std::span<const uint8_t> payload, const std::string_view clientPortStr) -> bool {
		const auto payloadSize{ payload.size() };
		const auto headerSize{ MSAPI::Protocol::WebSocket::Data::GetExpectedHeaderSize(payloadSize, isMasked) };
		RETURN_IF_FALSE(test.Assert(message.IsFinal(), isFinal, std::format("Message is final {}", clientPortStr)));
		RETURN_IF_FALSE(test.Assert(message.IsRsv1(), rsv1, std::format("Message RSV1 {}", clientPortStr)));
		RETURN_IF_FALSE(test.Assert(message.IsRsv2(), rsv2, std::format("Message RSV2 {}", clientPortStr)));
		RETURN_IF_FALSE(test.Assert(message.IsRsv3(), rsv3, std::format("Message RSV3 {}", clientPortStr)));
		RETURN_IF_FALSE(test.Assert(message.GetOpcode(), opcode, std::format("Message opcode {}", clientPortStr)));
		RETURN_IF_FALSE(test.Assert(message.IsMasked(), isMasked, std::format("Message is masked {}", clientPortStr)));
		RETURN_IF_FALSE(
			test.Assert(message.GetHeaderSize(), headerSize, std::format("Message header size {}", clientPortStr)));
		RETURN_IF_FALSE(
			test.Assert(message.GetPayloadSize(), payloadSize, std::format("Message payload size {}", clientPortStr)));
		RETURN_IF_FALSE(test.Assert(message.GetBufferSize(), static_cast<size_t>(headerSize) + payloadSize,
			std::format("Message buffer size {}", clientPortStr)));
		RETURN_IF_FALSE(test.Assert(message.IsValid(), true, std::format("Message is valid {}", clientPortStr)));

		uint16_t header{};
		header |= static_cast<uint16_t>(isFinal) << 15;
		header |= static_cast<uint16_t>(rsv1) << 14;
		header |= static_cast<uint16_t>(rsv2) << 13;
		header |= static_cast<uint16_t>(rsv3) << 12;
		header |= static_cast<uint16_t>(opcode) << 8;
		header |= static_cast<uint16_t>(isMasked) << 7;
		if (payloadSize <= 125) {
			header |= static_cast<uint16_t>(payloadSize);
		}
		else if (payloadSize <= 65535) {
			header |= 126;
		}
		else {
			header |= 127;
		}

		const auto maskingKey{ message.GetMaskingKey() };
		RETURN_IF_FALSE(test.Assert(message.ToString(),
			std::format("WebSocket data:\n{{"
						"\n\tFIN          : {}"
						"\n\tRSV1         : {}"
						"\n\tRSV2         : {}"
						"\n\tRSV3         : {}"
						"\n\tOPCODE       : {}"
						"\n\tMASK         : {}"
						"\n\tHeader size  : {} | {:08b} {:08b}"
						"\n\tPayload size : {}"
						"\n\tIs valid     : {}"
						"\n}}",
				isFinal, rsv1, rsv2, rsv3, MSAPI::Protocol::WebSocket::Data::EnumToString(opcode), maskingKey,
				headerSize, header >> 8, header & 0xFF, payloadSize, true),
			std::format("Message string representation {}", clientPortStr)));

		if (isMasked) {
			if (payloadSize == 0) {
				RETURN_IF_FALSE(test.Assert(maskingKey != 0, true, std::format("Masking key {}", clientPortStr)));
				RETURN_IF_FALSE(test.Assert(
					message.GetPayload(), payload, std::format("Message payload payload content {}", clientPortStr)));
			}
			else {
				// In case if masking does not mutate small payload
				if (payloadSize < 5 && message.GetPayload() == payload) {
					MSAPI::Tests::Protocol::WebSocket::Observer::ApplyMaskToData(message);
					if (message.GetPayload() != payload) {
						RETURN_IF_FALSE(test.Assert(message.GetPayload(), payload,
							std::format("Masked message payload content {}", clientPortStr)));
					}
				}
				else {
					RETURN_IF_FALSE(test.Assert(message.GetPayload() != payload, true,
						std::format("Masked message payload content {}", clientPortStr)));
				}
				RETURN_IF_FALSE(test.Assert(maskingKey != 0, true, std::format("Masking key {}", clientPortStr)));
				MSAPI::Tests::Protocol::WebSocket::Observer::ApplyMaskToData(message);
				RETURN_IF_FALSE(test.Assert(
					message.GetPayload(), payload, std::format("Unasked message payload content {}", clientPortStr)));
				// Message can be used after checking and must save original state
				MSAPI::Tests::Protocol::WebSocket::Observer::ApplyMaskToData(message);
			}
		}
		else {
			RETURN_IF_FALSE(
				test.Assert(message.GetPayload(), payload, std::format("Message payload content {}", clientPortStr)));
		}

		return true;
	} };

	const auto testMergePayload{ [&checkMessage, &test, payloadSpan](const std::span<const uint8_t> payloadTo,
									 const std::span<const uint8_t> payloadFrom, const bool isMasked) -> bool {
		LOG_INFO_NEW(
			"Testing merging message with {} payload size in message with {}", payloadFrom.size(), payloadTo.size());

		if (isMasked && !payloadTo.empty() && payloadTo.size() % 4 != 0) {
			RETURN_IF_FALSE(test.Assert(true, false,
				"Merge payload check will not pass if destination data is masked and contain payload size not multiply "
				"of 4"));
		}

		const auto mask{ isMasked ? MSAPI::Protocol::WebSocket::Data::GenerateMaskingKey() : 0 };

		MSAPI::Protocol::WebSocket::Data dataTo{ payloadTo, MSAPI::Protocol::WebSocket::Data::Opcode::Text, mask, true,
			true, false, true };
		RETURN_IF_FALSE(checkMessage(
			dataTo, true, true, false, true, MSAPI::Protocol::WebSocket::Data::Opcode::Text, isMasked, payloadTo, ""));

		MSAPI::Protocol::WebSocket::Data dataFrom{ payloadFrom, MSAPI::Protocol::WebSocket::Data::Opcode::Continuation,
			mask, false };
		const auto totalPayloadSize{ payloadTo.size() + payloadFrom.size() };
		if (payloadTo.empty()) {
			dataFrom.MergePayload(dataTo);
		}
		RETURN_IF_FALSE(checkMessage(dataFrom, false, false, false, false,
			MSAPI::Protocol::WebSocket::Data::Opcode::Continuation, isMasked, payloadFrom, ""));

		dataTo.MergePayload(dataFrom);
		RETURN_IF_FALSE(checkMessage(dataTo, true, true, false, true, MSAPI::Protocol::WebSocket::Data::Opcode::Text,
			isMasked, payloadSpan.subspan(0, totalPayloadSize), ""));
		return true;
	} };

	// 1.2. Initial state of WebSocket Handler on server
	RETURN_IF_FALSE(test.Assert(serverObserver.GetSizeOfFragmentedDataConnections(), 0,
		"Initial size of fragmented data connections on server"));
	RETURN_IF_FALSE(test.Assert(serverObserver.GetSizeOfFragmentedDataTimerToConnection(), 0,
		"Initial size of fragmented data timer to connection on server"));
	RETURN_IF_FALSE(
		test.Assert(serverObserver.GetStoredFragmentedDataSize(), 0., "Initial stored fragmented data size on server"));
	RETURN_IF_FALSE(test.Assert(server->GetFragmentedDataLimit(), 10., "Default fragmented data limit is correct"));
	RETURN_IF_FALSE(test.Assert(
		server->SetFragmentedDataLimit(-1.), false, "Setting fragmented data limit to negative value should fail"));
	RETURN_IF_FALSE(
		test.Assert(server->GetFragmentedDataLimit(), 10., "Fragmented data limit should remain 10 after failed set"));
	RETURN_IF_FALSE(
		test.Assert(server->SetFragmentedDataLimit(0.), true, "Setting fragmented data limit to 0 should succeed"));
	RETURN_IF_FALSE(
		test.Assert(server->GetFragmentedDataLimit(), 0., "Fragmented data limit should be 0 after successful set"));
	RETURN_IF_FALSE(
		test.Assert(server->SetFragmentedDataLimit(5.), true, "Setting fragmented data limit to 5 should succeed"));
	RETURN_IF_FALSE(
		test.Assert(server->GetFragmentedDataLimit(), 5., "Fragmented data limit should be 5 after successful set"));

	// 1.3. Merge in message without payload
	for (const auto payloadSize : std::array<size_t, 9>{ 1, 125, 126, 127, 128, 65535, 65536, 65537, 131070 }) {
		RETURN_IF_FALSE(testMergePayload(std::span<const uint8_t>{}, payloadSpan.subspan(0, payloadSize), false));
		RETURN_IF_FALSE(testMergePayload(std::span<const uint8_t>{}, payloadSpan.subspan(0, payloadSize), true));
	}

	// 1.4. Merge message with payload in message with payload <= 125 to get total payload size > 65535
	RETURN_IF_FALSE(testMergePayload(payloadSpan.subspan(0, 125), payloadSpan.subspan(125, 131070 - 125), false));

	// 1.5. Default constructor without parameters
	{
		MSAPI::Protocol::WebSocket::Data data;
		RETURN_IF_FALSE(test.Assert(data.GetMaskingKey(), 0, "Masking key is expected"));
		RETURN_IF_FALSE(
			checkMessage(data, false, false, false, false, MSAPI::Protocol::WebSocket::Data::Opcode::Continuation,
				false, std::span<const uint8_t>{}, "Default constructor without parameters"));
	}

	// 1.6. Constructor with only required parameters
	{
		MSAPI::Protocol::WebSocket::Data data{ payloadSpan.subspan(0, 125),
			MSAPI::Protocol::WebSocket::Data::Opcode::Text };
		RETURN_IF_FALSE(test.Assert(data.GetMaskingKey(), 0, "Masking key is expected"));
		RETURN_IF_FALSE(checkMessage(data, true, false, false, false, MSAPI::Protocol::WebSocket::Data::Opcode::Text,
			false, payloadSpan.subspan(0, 125), "Constructor with only required parameters"));
	}

	// 1.7. Try to create SplitGenerator with empty payload
	{
		MSAPI::Protocol::WebSocket::Data data{ {}, MSAPI::Protocol::WebSocket::Data::Opcode::Text };
		RETURN_IF_FALSE(test.Assert(data.GetMaskingKey(), 0, "Masking key is expected"));
		MSAPI::Protocol::WebSocket::Data::SplitGenerator generator{ data, std::span<const uint8_t>{}, 10 };
		RETURN_IF_FALSE(
			test.Assert(generator.Get(), false, "SplitGenerator with empty payload should not have fragments"));
		RETURN_IF_FALSE(test.Assert(data.GetMaskingKey(), 0, "Masking key is expected"));
	}
	{
		MSAPI::Protocol::WebSocket::Data data{ {}, MSAPI::Protocol::WebSocket::Data::Opcode::Text, 1234 };
		RETURN_IF_FALSE(test.Assert(data.GetMaskingKey(), 1234, "Masking key is expected"));
		MSAPI::Protocol::WebSocket::Data::SplitGenerator generator{ data, std::span<const uint8_t>{}, 10, true };
		RETURN_IF_FALSE(
			test.Assert(generator.Get(), false, "SplitGenerator with empty payload should not have fragments"));
		RETURN_IF_FALSE(test.Assert(data.GetMaskingKey() == 1234, true, "Masking key is expected"));
	}

	// 1.8. Try to create SplitGenerator with payload less than minimum
	{
		MSAPI::Protocol::WebSocket::Data data{ {}, MSAPI::Protocol::WebSocket::Data::Opcode::Text };
		RETURN_IF_FALSE(test.Assert(data.GetMaskingKey(), 0, "Masking key is expected"));
		MSAPI::Protocol::WebSocket::Data::SplitGenerator generator{ data, payloadSpan.subspan(0, 3), 10 };
		RETURN_IF_FALSE(
			test.Assert(generator.Get(), false, "SplitGenerator with too small payload should not have fragments"));
		RETURN_IF_FALSE(test.Assert(data.GetMaskingKey(), 0, "Masking key is expected"));
	}
	{
		MSAPI::Protocol::WebSocket::Data data{ {}, MSAPI::Protocol::WebSocket::Data::Opcode::Text, 23423 };
		RETURN_IF_FALSE(test.Assert(data.GetMaskingKey(), 23423, "Masking key is expected"));
		MSAPI::Protocol::WebSocket::Data::SplitGenerator generator{ data, payloadSpan.subspan(0, 3), 10, true };
		RETURN_IF_FALSE(
			test.Assert(generator.Get(), false, "SplitGenerator with too small payload should not have fragments"));
		RETURN_IF_FALSE(test.Assert(data.GetMaskingKey() == 23423, true, "Masking key is expected"));
	}
	{
		MSAPI::Protocol::WebSocket::Data data{ {}, MSAPI::Protocol::WebSocket::Data::Opcode::Text };
		RETURN_IF_FALSE(test.Assert(data.GetMaskingKey(), 0, "Masking key is expected"));
		MSAPI::Protocol::WebSocket::Data::SplitGenerator generator{ data, payloadSpan.subspan(0, 3), 10, true };
		RETURN_IF_FALSE(
			test.Assert(generator.Get(), false, "SplitGenerator with too small payload should not have fragments"));
		RETURN_IF_FALSE(test.Assert(data.IsMasked(), false, "Is masked is expected"));
		RETURN_IF_FALSE(test.Assert(data.GetMaskingKey(), 0, "Masking key is expected"));
	}
	{
		MSAPI::Protocol::WebSocket::Data data{ {}, MSAPI::Protocol::WebSocket::Data::Opcode::Text, 23423 };
		RETURN_IF_FALSE(test.Assert(data.GetMaskingKey(), 23423, "Masking key is expected"));
		MSAPI::Protocol::WebSocket::Data::SplitGenerator generator{ data, payloadSpan.subspan(0, 3), 10 };
		RETURN_IF_FALSE(
			test.Assert(generator.Get(), false, "SplitGenerator with too small payload should not have fragments"));
		RETURN_IF_FALSE(test.Assert(data.IsMasked(), true, "Is masked is expected"));
		RETURN_IF_FALSE(test.Assert(data.GetMaskingKey(), 23423, "Masking key is expected"));
	}

	// 1.9. Try to create SplitGenerator with zero step
	{
		MSAPI::Protocol::WebSocket::Data data{ {}, MSAPI::Protocol::WebSocket::Data::Opcode::Text };
		RETURN_IF_FALSE(test.Assert(data.GetMaskingKey(), 0, "Masking key is expected"));
		MSAPI::Protocol::WebSocket::Data::SplitGenerator generator{ data, payloadSpan.subspan(0, 4), 0 };
		RETURN_IF_FALSE(test.Assert(generator.Get(), true, "SplitGenerator get with zero"));
		RETURN_IF_FALSE(checkMessage(data, false, false, false, false, MSAPI::Protocol::WebSocket::Data::Opcode::Text,
			false, payloadSpan.subspan(0, 1), ""));
		RETURN_IF_FALSE(test.Assert(data.GetMaskingKey(), 0, "Masking key is expected"));
	}
	{
		MSAPI::Protocol::WebSocket::Data data{ {}, MSAPI::Protocol::WebSocket::Data::Opcode::Text, 6542342 };
		RETURN_IF_FALSE(test.Assert(data.GetMaskingKey(), 6542342, "Masking key is expected"));
		MSAPI::Protocol::WebSocket::Data::SplitGenerator generator{ data, payloadSpan.subspan(0, 4), 0, true };
		RETURN_IF_FALSE(test.Assert(generator.Get(), true, "SplitGenerator get with zero"));
		RETURN_IF_FALSE(checkMessage(data, false, false, false, false, MSAPI::Protocol::WebSocket::Data::Opcode::Text,
			true, payloadSpan.subspan(0, 1), ""));
		RETURN_IF_FALSE(test.Assert(data.GetMaskingKey() != 6542342, true, "Masking key is expected"));
		RETURN_IF_FALSE(test.Assert(data.GetMaskingKey() != 0, true, "Masking key is expected"));
	}
	{
		MSAPI::Protocol::WebSocket::Data data{ {}, MSAPI::Protocol::WebSocket::Data::Opcode::Text };
		RETURN_IF_FALSE(test.Assert(data.GetMaskingKey(), 0, "Masking key is expected"));
		MSAPI::Protocol::WebSocket::Data::SplitGenerator generator{ data, payloadSpan.subspan(0, 4), 0, true };
		RETURN_IF_FALSE(test.Assert(generator.Get(), true, "SplitGenerator get with zero"));
		RETURN_IF_FALSE(checkMessage(data, false, false, false, false, MSAPI::Protocol::WebSocket::Data::Opcode::Text,
			true, payloadSpan.subspan(0, 1), ""));
		RETURN_IF_FALSE(test.Assert(data.GetMaskingKey() != 0, true, "Masking key is expected"));
	}
	{
		MSAPI::Protocol::WebSocket::Data data{ {}, MSAPI::Protocol::WebSocket::Data::Opcode::Text, 543345 };
		RETURN_IF_FALSE(test.Assert(data.GetMaskingKey(), 543345, "Masking key is expected"));
		MSAPI::Protocol::WebSocket::Data::SplitGenerator generator{ data, payloadSpan.subspan(0, 4), 0 };
		RETURN_IF_FALSE(test.Assert(generator.Get(), true, "SplitGenerator get with zero"));
		RETURN_IF_FALSE(checkMessage(data, false, false, false, false, MSAPI::Protocol::WebSocket::Data::Opcode::Text,
			false, payloadSpan.subspan(0, 1), ""));
		RETURN_IF_FALSE(test.Assert(data.GetMaskingKey(), 0, "Masking key is expected"));
	}

	// 1.10. Try to create SplitGenerator with step greater than payload size
	{
		MSAPI::Protocol::WebSocket::Data data{ {}, MSAPI::Protocol::WebSocket::Data::Opcode::Text };
		MSAPI::Protocol::WebSocket::Data::SplitGenerator generator{ data, payloadSpan.subspan(0, 65535 * 4 + 1),
			65535 * 4 + 2 };
		RETURN_IF_FALSE(test.Assert(generator.Get(), true, "SplitGenerator get with step greater than payload size"));
		RETURN_IF_FALSE(checkMessage(data, false, false, false, false, MSAPI::Protocol::WebSocket::Data::Opcode::Text,
			false, payloadSpan.subspan(0, 65535), ""));
	}

	// 1.11. Try to create split generator for different opcodes
	{
		struct SplitOpcodesTestData {
			const MSAPI::Protocol::WebSocket::Data::Opcode opcode;
			const bool expectedResult;
		};

		std::array<SplitOpcodesTestData, 6> opcodes{
			{ { MSAPI::Protocol::WebSocket::Data::Opcode::Continuation, false },
				{ MSAPI::Protocol::WebSocket::Data::Opcode::Text, true },
				{ MSAPI::Protocol::WebSocket::Data::Opcode::Binary, true },
				{ MSAPI::Protocol::WebSocket::Data::Opcode::Close, false },
				{ MSAPI::Protocol::WebSocket::Data::Opcode::Ping, false },
				{ MSAPI::Protocol::WebSocket::Data::Opcode::Pong, false } }
		};

		for (const auto testData : opcodes) {
			MSAPI::Protocol::WebSocket::Data data{ {}, testData.opcode };
			MSAPI::Protocol::WebSocket::Data::SplitGenerator generator{ data, payloadSpan.subspan(0, 5), 1 };
			RETURN_IF_FALSE(test.Assert(generator.Get(), testData.expectedResult,
				std::format("SplitGenerator try with opcode {}",
					MSAPI::Protocol::WebSocket::Data::EnumToString(testData.opcode))));
		}
	}

	// 1.12. Try to create split generator for different buffer type
	{
#define TMP_MSAPI_PROTOCOL_WEBSOCKET_DATA_SPLIT_GENERATOR_CONSTRUCT_WITH_PAYLOAD_TYPE(type)                            \
	{                                                                                                                  \
		MSAPI::Protocol::WebSocket::Data data{ {}, MSAPI::Protocol::WebSocket::Data::Opcode::Text };                   \
		std::span<type> specificPayload{ reinterpret_cast<type*>(nonConstPayloadSpan.data()), 5 };                     \
		MSAPI::Protocol::WebSocket::Data::SplitGenerator generator{ data, specificPayload, 4 };                        \
		RETURN_IF_FALSE(test.Assert(generator.Get(), true, "Successfully get from split generator"));                  \
		RETURN_IF_FALSE(checkMessage(data, false, false, false, false, MSAPI::Protocol::WebSocket::Data::Opcode::Text, \
			false, payloadSpan.subspan(0, 4), ""));                                                                    \
	}

		TMP_MSAPI_PROTOCOL_WEBSOCKET_DATA_SPLIT_GENERATOR_CONSTRUCT_WITH_PAYLOAD_TYPE(int8_t);
		TMP_MSAPI_PROTOCOL_WEBSOCKET_DATA_SPLIT_GENERATOR_CONSTRUCT_WITH_PAYLOAD_TYPE(uint8_t);
		TMP_MSAPI_PROTOCOL_WEBSOCKET_DATA_SPLIT_GENERATOR_CONSTRUCT_WITH_PAYLOAD_TYPE(char);
		TMP_MSAPI_PROTOCOL_WEBSOCKET_DATA_SPLIT_GENERATOR_CONSTRUCT_WITH_PAYLOAD_TYPE(std::byte);
		TMP_MSAPI_PROTOCOL_WEBSOCKET_DATA_SPLIT_GENERATOR_CONSTRUCT_WITH_PAYLOAD_TYPE(bool);
		TMP_MSAPI_PROTOCOL_WEBSOCKET_DATA_SPLIT_GENERATOR_CONSTRUCT_WITH_PAYLOAD_TYPE(const int8_t);
		TMP_MSAPI_PROTOCOL_WEBSOCKET_DATA_SPLIT_GENERATOR_CONSTRUCT_WITH_PAYLOAD_TYPE(const uint8_t);
		TMP_MSAPI_PROTOCOL_WEBSOCKET_DATA_SPLIT_GENERATOR_CONSTRUCT_WITH_PAYLOAD_TYPE(const char);
		TMP_MSAPI_PROTOCOL_WEBSOCKET_DATA_SPLIT_GENERATOR_CONSTRUCT_WITH_PAYLOAD_TYPE(const std::byte);
		TMP_MSAPI_PROTOCOL_WEBSOCKET_DATA_SPLIT_GENERATOR_CONSTRUCT_WITH_PAYLOAD_TYPE(const bool);

#undef TMP_MSAPI_PROTOCOL_WEBSOCKET_DATA_SPLIT_GENERATOR_CONSTRUCT_WITH_PAYLOAD_TYPE
	}

	struct TestData {
		const size_t payloadSize;
		const uint32_t mask;
		const bool isMasked;

		FORCE_INLINE TestData(const size_t payloadSize, const bool isMasked) noexcept
			: payloadSize{ payloadSize }
			, mask{ isMasked ? MSAPI::Protocol::WebSocket::Data::GenerateMaskingKey() : 0 }
			, isMasked{ isMasked }
		{
		}
	};

	// 1.13. Try to create close message with payload greater than 123 bytes
	for (const auto testData : std::array<TestData, 8>{ { { 124, false }, { 125, true }, { 126, false }, { 128, true },
			 { 65535, false }, { 65536, true }, { 65537, false }, { 131070, true } } }) {
		LOG_INFO_NEW("Testing creation of close message with payload size {}", testData.payloadSize);
		auto message{ MSAPI::Protocol::WebSocket::Data::CreateClose(
			int16_t{ 8480 } /* value in the buffer */, payloadSpan.subspan(0, testData.payloadSize), testData.mask) };
		RETURN_IF_FALSE(checkMessage(message, true, false, false, false,
			MSAPI::Protocol::WebSocket::Data::Opcode::Close, testData.isMasked, payloadSpan.subspan(0, 2), ""));
	}

	// 1.14. Try to create close message with different buffer type
	{
#define TMP_MSAPI_PROTOCOL_WEBSOCKET_DATA_CLOSE_CONSTRUCT_WITH_PAYLOAD_TYPE(type)                                      \
	{                                                                                                                  \
		std::span<type> specificPayload{ reinterpret_cast<type*>(nonConstPayloadSpan.data() + 2), 5 };                 \
		auto message{ MSAPI::Protocol::WebSocket::Data::CreateClose(                                                   \
			int16_t{ 8480 } /* value in the buffer */, specificPayload) };                                             \
		RETURN_IF_FALSE(checkMessage(message, true, false, false, false,                                               \
			MSAPI::Protocol::WebSocket::Data::Opcode::Close, false, payloadSpan.subspan(0, 7), ""));                   \
	}

		TMP_MSAPI_PROTOCOL_WEBSOCKET_DATA_CLOSE_CONSTRUCT_WITH_PAYLOAD_TYPE(int8_t);
		TMP_MSAPI_PROTOCOL_WEBSOCKET_DATA_CLOSE_CONSTRUCT_WITH_PAYLOAD_TYPE(uint8_t);
		TMP_MSAPI_PROTOCOL_WEBSOCKET_DATA_CLOSE_CONSTRUCT_WITH_PAYLOAD_TYPE(char);
		TMP_MSAPI_PROTOCOL_WEBSOCKET_DATA_CLOSE_CONSTRUCT_WITH_PAYLOAD_TYPE(std::byte);
		TMP_MSAPI_PROTOCOL_WEBSOCKET_DATA_CLOSE_CONSTRUCT_WITH_PAYLOAD_TYPE(bool);
		TMP_MSAPI_PROTOCOL_WEBSOCKET_DATA_CLOSE_CONSTRUCT_WITH_PAYLOAD_TYPE(const int8_t);
		TMP_MSAPI_PROTOCOL_WEBSOCKET_DATA_CLOSE_CONSTRUCT_WITH_PAYLOAD_TYPE(const uint8_t);
		TMP_MSAPI_PROTOCOL_WEBSOCKET_DATA_CLOSE_CONSTRUCT_WITH_PAYLOAD_TYPE(const char);
		TMP_MSAPI_PROTOCOL_WEBSOCKET_DATA_CLOSE_CONSTRUCT_WITH_PAYLOAD_TYPE(const std::byte);
		TMP_MSAPI_PROTOCOL_WEBSOCKET_DATA_CLOSE_CONSTRUCT_WITH_PAYLOAD_TYPE(const bool);

#undef TMP_MSAPI_PROTOCOL_WEBSOCKET_DATA_CLOSE_CONSTRUCT_WITH_PAYLOAD_TYPE
	}

	// 1.15. Test expected header size function
	RETURN_IF_FALSE(test.Assert(MSAPI::Protocol::WebSocket::Data::GetExpectedHeaderSize(0, false), 2,
		"Expected header size for unmasked zero payload"));
	RETURN_IF_FALSE(test.Assert(MSAPI::Protocol::WebSocket::Data::GetExpectedHeaderSize(125, false), 2,
		"Expected header size for unmasked 125 payload"));
	RETURN_IF_FALSE(test.Assert(MSAPI::Protocol::WebSocket::Data::GetExpectedHeaderSize(0, true), 6,
		"Expected header size for masked zero payload"));
	RETURN_IF_FALSE(test.Assert(MSAPI::Protocol::WebSocket::Data::GetExpectedHeaderSize(125, true), 6,
		"Expected header size for masked 125 payload"));
	RETURN_IF_FALSE(test.Assert(MSAPI::Protocol::WebSocket::Data::GetExpectedHeaderSize(126, false), 4,
		"Expected header size for unmasked 126 payload"));
	RETURN_IF_FALSE(test.Assert(MSAPI::Protocol::WebSocket::Data::GetExpectedHeaderSize(65535, false), 4,
		"Expected header size for unmasked 65535 payload"));
	RETURN_IF_FALSE(test.Assert(MSAPI::Protocol::WebSocket::Data::GetExpectedHeaderSize(126, true), 8,
		"Expected header size for masked 126 payload"));
	RETURN_IF_FALSE(test.Assert(MSAPI::Protocol::WebSocket::Data::GetExpectedHeaderSize(65535, true), 8,
		"Expected header size for masked 65535 payload"));
	RETURN_IF_FALSE(test.Assert(MSAPI::Protocol::WebSocket::Data::GetExpectedHeaderSize(65536, false), 10,
		"Expected header size for unmasked 65536 payload"));
	RETURN_IF_FALSE(test.Assert(MSAPI::Protocol::WebSocket::Data::GetExpectedHeaderSize(3000000, false), 10,
		"Expected header size for unmasked 3000000 payload"));
	RETURN_IF_FALSE(test.Assert(MSAPI::Protocol::WebSocket::Data::GetExpectedHeaderSize(65536, true), 14,
		"Expected header size for masked 65536 payload"));
	RETURN_IF_FALSE(test.Assert(MSAPI::Protocol::WebSocket::Data::GetExpectedHeaderSize(3000000, true), 14,
		"Expected header size for masked 3000000 payload"));

	struct ClientDeamon {
		std::unique_ptr<MSAPI::DaemonBase> ptr;
		std::string portStr;
		int32_t clientConnect{};
		int32_t serverConnect{};

		FORCE_INLINE ClientDeamon(std::unique_ptr<MSAPI::DaemonBase>&& daemonPtr)
			: ptr(std::move(daemonPtr))
		{
			if (ptr == nullptr) {
				return;
			}

			auto* const client{ static_cast<Test::Node*>(ptr->GetApp()) };
			if (client == nullptr) {
				return;
			}

			portStr = _S(ptr->GetPort());
		}
	};
	std::vector<ClientDeamon> clientDaemons;

	const auto doTest{ [serverId, &serverPtr, server, &test, payloadSpan, &checkMessage, &serverObserver,
						   &clientDaemons](const bool isMasked) -> bool {
		auto& clientDeamon{ clientDaemons.emplace_back(ClientDeamon{ MSAPI::Daemon<Test::Node>::Create("Client") }) };
		if (clientDeamon.ptr == nullptr) {
			return false;
		}
		auto client{ static_cast<Test::Node*>(clientDeamon.ptr->GetApp()) };
		if (!client->OpenConnect(serverId, INADDR_LOOPBACK, serverPtr->GetPort(), false)) {
			return false;
		}
		client->HandleRunRequest();
		const std::string& clientPortStr{ clientDeamon.portStr };

		MSAPI::Tests::Protocol::WebSocket::Observer clientObserver{ *client };
		RETURN_IF_FALSE(test.Assert(clientObserver.GetSizeOfFragmentedDataConnections(), 0,
			std::format("Initial size of fragmented data connections on client {}", clientPortStr)));

		// Key port is used to detect connection
		const std::string& wsHandshakeRequestPureInvalid{ std::format("GET / HTTP/1.1\r\n"
																	  "Host: everybody-dance-now.ru:1997\n"
																	  "Upgrade: websocket\n"
																	  "Connection: Upgrade\n"
																	  "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\n"
																	  "Sec-WebSocket-Version: 12\n{}: port",
			clientPortStr) };
		const std::string& wsHandshakeRequestInvalid{ std::format(
			"HTTP message:\n{{\n\tis valid              : true\n\ttype                  : Request\n\tmessage type  "
			"    "
			" "
			"   "
			": GET\n\turl                   : /\n\tHTTP type             : HTTP\n\tversion               : "
			"1.1\n\tformat   "
			"             : html\n\tcode                  : \n\tcode text             : \n\tmessage size          "
			": "
			"{}\n\tHeaders               :\n{{\n\t{:21} : port\n\tConnection            : Upgrade\n\tHost          "
			"    "
			"    : "
			"everybody-dance-now.ru:1997\n\tSec-WebSocket-Key     : "
			"dGhlIHNhbXBsZSBub25jZQ==\n\tSec-WebSocket-Version "
			": "
			"12\n\tUpgrade               : websocket\n}}\n}}",
			169 + clientPortStr.size(), clientPortStr) };

		const std::string& wsHandshakeResponseInvalid{
			"HTTP message:\n{\n\tis valid     : true\n\ttype         : Response\n\tmessage type : \n\turl          : "
			"\n\tHTTP type    : HTTP\n\tversion      : 1.1\n\tformat       : \n\tcode         : 404\n\tcode text    : "
			"Not Found\n\tmessage size : 119\n\tHeaders      :\n{\n\tConnection   : keep-alive\n\tContent-Type : "
			"text/html; charset=utf-8\n\tKeep-Alive   : timeout=0,max=0\n}\n}"
		};

		// 2.1. Try to establish WebSocket handshake with unsupported protocol version
		LOG_INFO_NEW(
			"Try to establish WebSocket handshake with unsupported protocol version for client {}", clientPortStr);
		client->SendHttp(serverId, wsHandshakeRequestPureInvalid);
		int clientConnect{ -1 };
		RETURN_IF_FALSE(test.Wait(
			50000,
			[&server, clientPortStr, &clientConnect]() {
				clientConnect = server->DetectConnection(clientPortStr);
				return clientConnect != -1;
			},
			std::format("Server detected connection for WebSocket handshake request from client {}", clientPortStr)));
		clientDeamon.clientConnect = clientConnect;
		const auto* serverHttpData{ server->GetHttpData(clientConnect) };
		size_t expectedServerHttpDataSize{};
		RETURN_IF_FALSE(test.Assert(
			serverHttpData != nullptr, true, std::format("Server has HTTP data for client {}", clientPortStr)));
		RETURN_IF_FALSE(test.Assert(serverHttpData->size(), ++expectedServerHttpDataSize,
			std::format("Server has {} HTTP data messages for client {}", serverHttpData->size(), clientPortStr)));
		RETURN_IF_FALSE(test.Assert(serverHttpData->back().ToString(), wsHandshakeRequestInvalid,
			std::format("Server got correct WebSocket handshake request from client {}", clientPortStr)));
		std::vector<MSAPI::Protocol::WebSocket::Data>* serverWebSocketData{};

		const auto serverConnectOpt{ client->GetConnect(serverId) };
		RETURN_IF_FALSE(test.Assert(serverConnectOpt.has_value(), true,
			std::format("Client {} detected connection for WebSocket handshake response", clientPortStr)));
		const auto serverConnect{ serverConnectOpt.value() };
		clientDeamon.serverConnect = serverConnect;
		const std::vector<MSAPI::HTTP::Data>* clientHttpData{};
		size_t expectedClientHttpDataSize{};

		RETURN_IF_FALSE(test.Wait(
			50000,
			[&clientHttpData, &client, serverConnect]() {
				clientHttpData = client->GetHttpData(serverConnect);
				return clientHttpData != nullptr;
			},
			std::format("Client {} has HTTP data", clientPortStr)));
		RETURN_IF_FALSE(test.Assert(clientHttpData->size(), ++expectedClientHttpDataSize,
			std::format("Client {} has {} HTTP data messages", clientHttpData->size(), clientPortStr)));
		RETURN_IF_FALSE(test.Assert(clientHttpData->back().ToString(), wsHandshakeResponseInvalid,
			std::format("Client {} got correct WebSocket handshake response", clientPortStr)));
		std::vector<MSAPI::Protocol::WebSocket::Data>* clientWebSocketData{};

		const std::string& wsHandshakeRequestPure{ std::format("GET / HTTP/1.1\r\n"
															   "Host: everybody-dance-now.ru:1997\n"
															   "Upgrade: websocket\n"
															   "Connection: Upgrade\n"
															   "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\n"
															   "Sec-WebSocket-Version: 13\n{}: port",
			clientPortStr) };
		const std::string& wsHandshakeRequest{ std::format(
			"HTTP message:\n{{\n\tis valid              : true\n\ttype                  : Request\n\tmessage type  "
			"    "
			" "
			"   "
			": GET\n\turl                   : /\n\tHTTP type             : HTTP\n\tversion               : "
			"1.1\n\tformat   "
			"             : html\n\tcode                  : \n\tcode text             : \n\tmessage size          "
			": "
			"{}\n\tHeaders               :\n{{\n\t{:21} : port\n\tConnection            : Upgrade\n\tHost          "
			"    "
			"    : "
			"everybody-dance-now.ru:1997\n\tSec-WebSocket-Key     : "
			"dGhlIHNhbXBsZSBub25jZQ==\n\tSec-WebSocket-Version "
			": "
			"13\n\tUpgrade               : websocket\n}}\n}}",
			169 + clientPortStr.size(), clientPortStr) };
		const std::string& wsHandshakeResponse{
			"HTTP message:\n{\n\tis valid              : true\n\ttype                  : Response\n\tmessage type  "
			"    "
			"    "
			": \n\turl                   : \n\tHTTP type             : HTTP\n\tversion               : "
			"1.1\n\tformat   "
			"    "
			"         : \n\tcode                  : 101\n\tcode text             : Switching Protocols\n\tmessage "
			"size "
			"    "
			"     : 156\n\tHeaders               :\n{\n\tConnection            : Upgrade\n\tSec-WebSocket-Accept  "
			": "
			"s3pPLMBiTxaQ9kYGzzhZRbK+xOo=\n\tSec-WebSocket-Version : 13\n\tUpgrade               : websocket\n}\n}"
		};

		// 2.2. Establish WebSocket handshake
		LOG_INFO_NEW("Establish WebSocket handshake for client {}", clientPortStr);
		client->SendHttp(serverId, wsHandshakeRequestPure);
		++expectedServerHttpDataSize;
		RETURN_IF_FALSE(test.Wait(
			50000,
			[&serverHttpData, clientPortStr, expectedServerHttpDataSize]() {
				return serverHttpData->size() == expectedServerHttpDataSize;
			},
			std::format("Server has expected number HTTP data messages for client", clientPortStr)));
		RETURN_IF_FALSE(test.Assert(serverHttpData->back().ToString(), wsHandshakeRequest,
			std::format("Server got correct WebSocket handshake request from client {}", clientPortStr)));

		++expectedClientHttpDataSize;
		RETURN_IF_FALSE(test.Wait(
			50000,
			[&clientHttpData, clientPortStr, expectedClientHttpDataSize]() {
				return clientHttpData->size() == expectedClientHttpDataSize;
			},
			std::format("Client {} has expected number HTTP data messages", clientPortStr)));
		RETURN_IF_FALSE(test.Assert(clientHttpData->back().ToString(), wsHandshakeResponse,
			std::format("Client {} got correct WebSocket handshake response", clientPortStr)));

		RETURN_IF_FALSE(test.Assert(clientObserver.GetSizeOfFragmentedDataConnections(), 0,
			std::format("Size of fragmented data connections on client {} after handshake", clientPortStr)));
		RETURN_IF_FALSE(test.Assert(serverObserver.HasConnectionFragmentedData(clientConnect), false,
			std::format("Server does not have fragmented data with client {} after handshake", clientPortStr)));
		RETURN_IF_FALSE(test.Assert(serverObserver.GetLastFragmentedDataTimer(clientConnect), MSAPI::Timer{ 0 },
			std::format("Server does not have timestamp of last fragmented data with client {} after handshake",
				clientPortStr)));
		RETURN_IF_FALSE(test.Assert(clientObserver.HasConnectionFragmentedData(serverConnect), false,
			std::format("Client {} does not have fragmented data with server after handshake", clientPortStr)));

		RETURN_IF_FALSE(test.Assert(client->GetWebSocketData(serverConnect), nullptr,
			std::format("Client {} does not have WebSocket yet", clientPortStr)));
		RETURN_IF_FALSE(test.Assert(server->GetWebSocketData(clientConnect), nullptr,
			std::format("Server does not have WebSocket yet for client {}", clientPortStr)));

		size_t expectedServerWebSocketDataSize{};
		size_t expectedClientWebSocketDataSize{};
		const auto sendAndCheck{ [&test, &serverWebSocketData, &expectedServerWebSocketDataSize, &server, clientConnect,
									 &clientWebSocketData, &expectedClientWebSocketDataSize, &client, serverConnect,
									 &clientPortStr, &checkMessage](MSAPI::Protocol::WebSocket::Data& message) -> bool {
			const auto opcode{ message.GetOpcode() };

			if (opcode != MSAPI::Protocol::WebSocket::Data::Opcode::Close) {
				if (opcode == MSAPI::Protocol::WebSocket::Data::Opcode::Ping) {
					++expectedClientWebSocketDataSize;
				}
				else {
					++expectedServerWebSocketDataSize;
				}
			}

			MSAPI::Protocol::WebSocket::Send(serverConnect, message);

			if (expectedClientWebSocketDataSize > 0) {
				RETURN_IF_FALSE(test.Wait(
					50000,
					[&clientWebSocketData, &client, serverConnect, expectedClientWebSocketDataSize]() {
						clientWebSocketData = client->GetWebSocketData(serverConnect);
						return clientWebSocketData != nullptr
							&& clientWebSocketData->size() == expectedClientWebSocketDataSize;
					},
					std::format("Client {} has WebSocket data with {} messages", clientPortStr,
						expectedClientWebSocketDataSize)));
			}

			RETURN_IF_FALSE(test.Wait(
				50000,
				[&serverWebSocketData, &server, clientConnect, expectedServerWebSocketDataSize]() {
					serverWebSocketData = server->GetWebSocketData(clientConnect);
					return serverWebSocketData != nullptr
						&& serverWebSocketData->size() == expectedServerWebSocketDataSize;
				},
				std::format("Server has WebSocket data with {} messages for client {}", expectedServerWebSocketDataSize,
					clientPortStr)));

			// TODO: Close message reserving cannot be checked, look at another TODOs in this file
			if (opcode != MSAPI::Protocol::WebSocket::Data::Opcode::Close) {
				if (opcode == MSAPI::Protocol::WebSocket::Data::Opcode::Ping) {
					auto& clientLastMessage{ clientWebSocketData->back() };
					RETURN_IF_FALSE(test.Assert(clientLastMessage.GetMaskingKey() == 0, message.GetMaskingKey() != 0,
						std::format("Masking is reversed for answer on ping {}", clientPortStr)));
					if (message.IsMasked()) {
						// Collected message always unmasked, checkMessage expect masked payload for masked message
						MSAPI::Tests::Protocol::WebSocket::Observer::ApplyMaskToData(message);
					}
					else {
						MSAPI::Tests::Protocol::WebSocket::Observer::ApplyMaskToData(
							message, clientLastMessage.GetMaskingKey());
					}
					RETURN_IF_FALSE(checkMessage(clientLastMessage, message.IsFinal(), message.IsRsv1(),
						message.IsRsv2(), message.IsRsv3(), MSAPI::Protocol::WebSocket::Data::Opcode::Pong,
						!message.IsMasked(), message.GetPayload(), clientPortStr));
				}
				else {
					const auto& lastServerMessage{ serverWebSocketData->back() };
					if (message.IsMasked()) {
						if (!message.GetPayload().empty()) {
							RETURN_IF_FALSE(test.Assert(message.GetPayload() != lastServerMessage.GetPayload(), true,
								std::format("Masked message payload content {}", clientPortStr)));
							// Collected message always unmasked, checkMessage expect masked payload for masked message
							MSAPI::Tests::Protocol::WebSocket::Observer::ApplyMaskToData(message);
						}
					}
					RETURN_IF_FALSE(test.Assert(lastServerMessage, message,
						std::format("Server got correct WebSocket message for client {}", clientPortStr)));
				}
			}

			return true;
		} };

		const auto getMessageWithoutPayload{ []<MSAPI::Protocol::WebSocket::Data::Opcode O>(const bool isFinal,
												 const bool rsv1, const bool rsv2, const bool rsv3,
												 const bool isMasked) {
			if constexpr (O == MSAPI::Protocol::WebSocket::Data::Opcode::Close) {
				// Different path instead of same with generic parameter is used for testing purpose
				if (isMasked) {
					return MSAPI::Protocol::WebSocket::Data::CreateClose(
						int16_t{ -1 }, {}, MSAPI::Protocol::WebSocket::Data::GenerateMaskingKey());
				}
				return MSAPI::Protocol::WebSocket::Data::CreateClose();
			}
			else {
				if (isMasked) {
					return MSAPI::Protocol::WebSocket::Data{ {}, O,
						MSAPI::Protocol::WebSocket::Data::GenerateMaskingKey(), isFinal, rsv1, rsv2, rsv3 };
				}
				return MSAPI::Protocol::WebSocket::Data{ {}, O, 0, isFinal, rsv1, rsv2, rsv3 };
			}
		} };

		const auto doSendAndCheckWithoutPayload{ [&checkMessage, &sendAndCheck, &getMessageWithoutPayload,
													 &clientPortStr]<MSAPI::Protocol::WebSocket::Data::Opcode O>(
													 const bool isFinal, const bool rsv1, const bool rsv2,
													 const bool rsv3, const bool isMasked) -> bool {
			auto message { getMessageWithoutPayload.template operator()<O>(isFinal, rsv1, rsv2, rsv3, isMasked) };
			RETURN_IF_FALSE(checkMessage(
				message, isFinal, rsv1, rsv2, rsv3, O, isMasked, std::span<const uint8_t>{}, clientPortStr));
			RETURN_IF_FALSE(sendAndCheck(message));

			return true;
		} };

		// 2.3. Send empty text message
		LOG_INFO_NEW("Testing empty text message from client {}", clientPortStr);
		RETURN_IF_FALSE(
			doSendAndCheckWithoutPayload.template operator()<MSAPI::Protocol::WebSocket::Data::Opcode::Text>(
				true, true, true, true, isMasked));

		// 2.4. Send empty binary message
		LOG_INFO_NEW("Testing empty binary message from client {}", clientPortStr);
		RETURN_IF_FALSE(
			doSendAndCheckWithoutPayload.template operator()<MSAPI::Protocol::WebSocket::Data::Opcode::Binary>(
				true, false, true, true, isMasked));

		// 2.5. Send empty ping message
		LOG_INFO_NEW("Testing empty ping message from client {}", clientPortStr);
		RETURN_IF_FALSE(
			doSendAndCheckWithoutPayload.template operator()<MSAPI::Protocol::WebSocket::Data::Opcode::Ping>(
				true, true, false, true, isMasked));

		// 2.6. Send empty pong message
		LOG_INFO_NEW("Testing empty pong message from client {}", clientPortStr);
		RETURN_IF_FALSE(
			doSendAndCheckWithoutPayload.template operator()<MSAPI::Protocol::WebSocket::Data::Opcode::Pong>(
				true, true, true, false, isMasked));

		const auto getMessageWithPayload{ [&test,
											  &clientPortStr]<MSAPI::Protocol::WebSocket::Data::Opcode O, typename T>(
											  const std::span<const T> payload, const bool isFinal, const bool isMasked,
											  const bool rsv1, const bool rsv2, const bool rsv3) {
			if constexpr (O == MSAPI::Protocol::WebSocket::Data::Opcode::Close) {
				if (payload.size() < 2) {
					if (!payload.empty()) {
						(void)test.Assert(false, true,
							std::format(
								"Close message payload must be empty if it does not contain status code, client: {}",
								clientPortStr));
					}
					// Different path instead of same with generic parameter is used for testing purpose
					if (isMasked) {
						return MSAPI::Protocol::WebSocket::Data::CreateClose(
							int16_t{ -1 }, {}, MSAPI::Protocol::WebSocket::Data::GenerateMaskingKey());
					}
					return MSAPI::Protocol::WebSocket::Data::CreateClose();
				}
				const int16_t statusCode{ reinterpret_cast<const int16_t*>(payload.data())[0] };
				if (isMasked) {
					return MSAPI::Protocol::WebSocket::Data::CreateClose(statusCode, payload.subspan(sizeof(int16_t)),
						MSAPI::Protocol::WebSocket::Data::GenerateMaskingKey());
				}
				return MSAPI::Protocol::WebSocket::Data::CreateClose(statusCode, payload.subspan(sizeof(int16_t)));
			}
			else {
				if (isMasked) {
					return MSAPI::Protocol::WebSocket::Data{
						std::span<const uint8_t>{ reinterpret_cast<const uint8_t*>(payload.data()), payload.size() }, O,
						MSAPI::Protocol::WebSocket::Data::GenerateMaskingKey(), isFinal, rsv1, rsv2, rsv3
					};
				}
				return MSAPI::Protocol::WebSocket::Data{
					std::span<const uint8_t>{ reinterpret_cast<const uint8_t*>(payload.data()), payload.size() }, O, 0,
					isFinal, rsv1, rsv2, rsv3
				};
			}
		} };

		const auto doSendAndCheckWithPayload{ [&checkMessage, &sendAndCheck, &getMessageWithPayload, &clientPortStr,
												  &test]<MSAPI::Protocol::WebSocket::Data::Opcode O>(const bool isFinal,
												  const bool rsv1, const bool rsv2, const bool rsv3,
												  const bool isMasked, const std::span<const uint8_t> payload) -> bool {
			if constexpr (O == MSAPI::Protocol::WebSocket::Data::Opcode::Close) {
				if (payload.size() < 2) {
					RETURN_IF_FALSE(test.Assert(payload.empty(), true,
						std::format(
							"Close message payload must be empty if it does not contain status code, client: {}",
							clientPortStr)));
				}
			}
			auto message { getMessageWithPayload.template operator()<O>(payload, isFinal, isMasked, rsv1, rsv2, rsv3) };
			RETURN_IF_FALSE(checkMessage(message, isFinal, rsv1, rsv2, rsv3, O, isMasked, payload, clientPortStr));
			RETURN_IF_FALSE(sendAndCheck(message));

			return true;
		} };

		// 2.7. Send small text message
		// 2.8. Send small binary message
		// 2.9. Send medium text message
		// 2.10. Send medium binary message
		// 2.11. Send large text message
		// 2.12. Send large binary message
		for (const auto payloadSize : std::array<size_t, 6>{ 1, 125, 126, 65535, 65536, 131070 }) {
			LOG_INFO_NEW("Testing payload size {}, client {}", payloadSize, clientPortStr);
			RETURN_IF_FALSE(
				doSendAndCheckWithPayload.template operator()<MSAPI::Protocol::WebSocket::Data::Opcode::Text>(
					true, true, false, false, isMasked, payloadSpan.subspan(0, payloadSize)));

			RETURN_IF_FALSE(
				doSendAndCheckWithPayload.template operator()<MSAPI::Protocol::WebSocket::Data::Opcode::Binary>(
					true, false, true, false, isMasked, payloadSpan.subspan(0, payloadSize)));
		}

		// 2.13. Send ping message with small payload
		// 2.14. Send pong message with small payload
		for (const auto payloadSize : std::array<size_t, 2>{ 1, 125 }) {
			LOG_INFO_NEW("Testing control message with payload size {}, client {}", payloadSize, clientPortStr);
			RETURN_IF_FALSE(
				doSendAndCheckWithPayload.template operator()<MSAPI::Protocol::WebSocket::Data::Opcode::Ping>(
					true, true, true, false, isMasked, payloadSpan.subspan(0, payloadSize)));

			RETURN_IF_FALSE(
				doSendAndCheckWithPayload.template operator()<MSAPI::Protocol::WebSocket::Data::Opcode::Pong>(
					true, false, true, false, isMasked, payloadSpan.subspan(0, payloadSize)));
		}

		// 2.15. Try to create ping message with payload greater than 125 bytes
		// 2.16. Try to create pong message with payload greater than 125 bytes
		for (const auto payloadSize : std::array<size_t, 4>{ 126, 65535, 65536, 131070 }) {
			LOG_INFO_NEW(
				"Testing creation of control message with payload size {}, client {}", payloadSize, clientPortStr);
			MSAPI::Protocol::WebSocket::Data pingMessage{ payloadSpan.subspan(0, payloadSize),
				MSAPI::Protocol::WebSocket::Data::Opcode::Ping,
				isMasked ? MSAPI::Protocol::WebSocket::Data::GenerateMaskingKey() : 0 };
			RETURN_IF_FALSE(checkMessage(pingMessage, true, false, false, false,
				MSAPI::Protocol::WebSocket::Data::Opcode::Ping, isMasked, std::span<const uint8_t>{}, clientPortStr));

			MSAPI::Protocol::WebSocket::Data pongMessage{ payloadSpan.subspan(0, payloadSize),
				MSAPI::Protocol::WebSocket::Data::Opcode::Pong,
				isMasked ? MSAPI::Protocol::WebSocket::Data::GenerateMaskingKey() : 0 };
			RETURN_IF_FALSE(checkMessage(pongMessage, true, false, false, false,
				MSAPI::Protocol::WebSocket::Data::Opcode::Pong, isMasked, std::span<const uint8_t>{}, clientPortStr));
		}

		// TODO: For now checking of close messages does not happen in terms of reserving and followed that actions.
		// TODO: When Application will be able to call Server methods to close connection, then deeper testing of close
		// messages should be done
		// 2.17. Send empty close message
		LOG_INFO_NEW("Testing empty close message from client {}", clientPortStr);
		RETURN_IF_FALSE(
			doSendAndCheckWithoutPayload.template operator()<MSAPI::Protocol::WebSocket::Data::Opcode::Close>(
				true, false, false, false, isMasked));

		// 2.18. Send close message with small payload
		for (const auto payloadSize : std::array<size_t, 2>{ 2, 125 }) {
			LOG_INFO_NEW(
				"Testing creation of close message with payload size {}, client {}", payloadSize, clientPortStr);
			RETURN_IF_FALSE(
				doSendAndCheckWithPayload.template operator()<MSAPI::Protocol::WebSocket::Data::Opcode::Close>(
					true, false, false, false, isMasked, payloadSpan.subspan(0, payloadSize)));
		}

		const auto waitForFragmentedData{ [&test, &serverObserver, clientConnect, &clientPortStr](
											  MSAPI::Timer& lastFragmentTime) {
			RETURN_IF_FALSE(test.Wait(
				50000,
				[&serverObserver, clientConnect, &clientPortStr, &lastFragmentTime]() {
					const auto currentLastFragmentTime{ serverObserver.GetLastFragmentedDataTimer(clientConnect) };
					if (currentLastFragmentTime <= lastFragmentTime) {
						return false;
					}

					lastFragmentTime = currentLastFragmentTime;
					return true;
				},
				std::format("Timestamp of last fragmented data message on server for client {} was updated "
							"after fragmented message",
					clientPortStr)));

			return true;
		} };

		// 2.19. Send small fragmented message with different steps size
		// 2.20. Send medium fragmented message with different steps size
		// 2.21. Send large fragmented message with different steps size
		RETURN_IF_FALSE(test.Assert(clientObserver.GetSizeOfFragmentedDataConnections(), 0,
			std::format("Size of fragmented data connections on client {} before fragmented messages", clientPortStr)));
		RETURN_IF_FALSE(test.Assert(serverObserver.HasConnectionFragmentedData(clientConnect), false,
			std::format(
				"Server does not have fragmented data with client {} before fragmented messages", clientPortStr)));
		RETURN_IF_FALSE(test.Assert(serverObserver.GetLastFragmentedDataTimer(clientConnect), MSAPI::Timer{ 0 },
			std::format(
				"Server does not have timestamp of last fragmented data with client {} before fragmented messages",
				clientPortStr)));
		RETURN_IF_FALSE(test.Assert(clientObserver.HasConnectionFragmentedData(serverConnect), false,
			std::format(
				"Client {} does not have fragmented data with server before fragmented messages", clientPortStr)));
		{
			MSAPI::Protocol::WebSocket::Data data{ {}, MSAPI::Protocol::WebSocket::Data::Opcode::Text };
			size_t sendedPayload{};
			size_t expectedFragments{};
			size_t sendedFragments{};
			size_t expectedLastFragmentPayloadSize{};
			MSAPI::Timer lastFragmentTime{ 0 };
			uint32_t previousMaskingKey{};
			auto originalOpcode{ data.GetOpcode() };

			const auto prepareExpectedValues{ [&sendedPayload, &expectedFragments, &sendedFragments,
												  &expectedLastFragmentPayloadSize,
												  &lastFragmentTime](const size_t payloadSize, size_t& step) {
				if (step == 0 || step >= payloadSize) {
					step = std::min(payloadSize / 4, size_t{ 65535 });
				}

				sendedPayload = 0;
				lastFragmentTime = MSAPI::Timer{ 0 };
				expectedFragments = payloadSize / step;
				sendedFragments = 0;
				if (payloadSize % step != 0) {
					expectedFragments++;
					expectedLastFragmentPayloadSize = payloadSize % step;
				}
				else {
					expectedLastFragmentPayloadSize = step;
				}
			} };

			struct ContinuationTestData {
				const size_t payloadSize;
				size_t step;
			};

			std::array<ContinuationTestData, 35> continuationTestData{ { { 4, 0 }, { 4, 4 }, { 4, 4 / 2 }, { 4, 5 },
				{ 125 * 4, 0 }, { 125 * 4, 125 * 4 }, { 125 * 4, 125 * 2 }, { 125 * 4, 125 * 4 + 1 }, { 126 * 4, 0 },
				{ 126 * 4, 125 }, { 126 * 4, 126 * 4 }, { 126 * 4, 126 * 2 }, { 126 * 4, 126 * 4 + 1 },
				{ 65535 * 4, 0 }, { 65535 * 4, 126 }, { 65535 * 4, 127 }, { 65535 * 4, 128 }, { 65535 * 4, 1000 },
				{ 65535 * 4, 65535 * 4 }, { 65535 * 4, 65535 * 2 }, { 65535 * 4, 65535 * 4 + 1 }, { 65536 * 4, 0 },
				{ 65536 * 4, 65535 }, { 65536 * 4, 65536 * 4 }, { 65536 * 4, 65536 * 2 }, { 65536 * 4, 65536 * 4 + 1 },
				{ 131072 * 4, 0 }, { 141123 * 4 + 66123, 70123 }, { 131072 * 4, 131072 * 4 }, { 131072 * 4, 70000 },
				{ 131072 * 4, 76000 }, { 131072 * 4 + 120, 65536 }, { 131072 * 4, 131072 * 2 },
				{ 131072 * 4, 131072 * 4 + 1 }, { 2131060, 50 } } };

			bool sendPingsBetweenFragments{ false };
			for (auto [payloadSize, step] : continuationTestData) {
				if (payloadSize > payloadSpan.size()) [[unlikely]] {
					RETURN_IF_FALSE(test.Assert(true, false,
						std::format("Test payload size {} is greater than generated payload size {}, client {}",
							payloadSize, payloadSpan.size(), clientPortStr)));
				}
				LOG_INFO_NEW("Testing fragmented message by SplitGenerator with payload size {} and step {}, client {}",
					payloadSize, step, clientPortStr);
				MSAPI::Protocol::WebSocket::Data::SplitGenerator generator{ data, payloadSpan.subspan(0, payloadSize),
					step, isMasked };
				prepareExpectedValues(payloadSize, step);

				while (generator.Get()) {
					++sendedFragments;
					if (sendedFragments != expectedFragments) {
						sendedPayload += step;
						RETURN_IF_FALSE(checkMessage(data, false, false, false, false,
							sendedFragments == 1 ? originalOpcode
												 : MSAPI::Protocol::WebSocket::Data::Opcode::Continuation,
							isMasked, payloadSpan.subspan(sendedPayload - step, step), clientPortStr));
						if (sendPingsBetweenFragments) {
							RETURN_IF_FALSE(doSendAndCheckWithPayload
												.template operator()<MSAPI::Protocol::WebSocket::Data::Opcode::Ping>(
													true, false, false, false, isMasked, payloadSpan.subspan(0, 125)));
						}

						if (isMasked) {
							const auto newMaskingKey{ data.GetMaskingKey() };
							RETURN_IF_FALSE(test.Assert(previousMaskingKey != newMaskingKey, true,
								std::format("Unique masking key is generated, client {}", clientPortStr)));
							previousMaskingKey = newMaskingKey;
						}

						MSAPI::Protocol::WebSocket::Send(serverConnect, data);
						RETURN_IF_FALSE(waitForFragmentedData(lastFragmentTime));

						if (sendedFragments == 1) {
							RETURN_IF_FALSE(test.Wait(
								50000,
								[&serverObserver, clientConnect, &clientPortStr]() {
									return serverObserver.HasConnectionFragmentedData(clientConnect);
								},
								std::format(
									"Server has fragmented data messages for client {} after fragmented message",
									clientPortStr)));
							RETURN_IF_FALSE(test.Assert(clientObserver.GetSizeOfFragmentedDataConnections(), 0,
								std::format("Size of fragmented data connections on client {} after fragmented message",
									clientPortStr)));
							RETURN_IF_FALSE(
								test.Assert(clientObserver.HasConnectionFragmentedData(serverConnect), false,
									std::format(
										"Client {} does not have fragmented data with server after fragmented messages",
										clientPortStr)));
						}
						continue;
					}

					sendedPayload += expectedLastFragmentPayloadSize;
					RETURN_IF_FALSE(checkMessage(data, true, false, false, false,
						MSAPI::Protocol::WebSocket::Data::Opcode::Continuation, isMasked,
						payloadSpan.subspan(
							sendedPayload - expectedLastFragmentPayloadSize, expectedLastFragmentPayloadSize),
						clientPortStr));

					MSAPI::Protocol::WebSocket::Send(serverConnect, data);
				}
				sendPingsBetweenFragments = !sendPingsBetweenFragments;

				RETURN_IF_FALSE(test.Assert(sendedFragments, expectedFragments,
					std::format("All fragments were generated for payload size {} and step {}, client {}", payloadSize,
						step, clientPortStr)));

				++expectedServerWebSocketDataSize;
				RETURN_IF_FALSE(test.Wait(
					50000,
					[&serverWebSocketData, clientConnect, expectedServerWebSocketDataSize]() {
						return serverWebSocketData->size() == expectedServerWebSocketDataSize;
					},
					std::format("Server has WebSocket data with {} messages for client {}",
						expectedServerWebSocketDataSize, clientPortStr)));

				auto& lastServerWebSocketData{ serverWebSocketData->back() };
				if (lastServerWebSocketData.IsMasked()) {
					// Collected message always unmasked, checkMessage expect masked payload for masked message
					MSAPI::Tests::Protocol::WebSocket::Observer::ApplyMaskToData(lastServerWebSocketData);
				}

				RETURN_IF_FALSE(checkMessage(lastServerWebSocketData, true, false, false, false, originalOpcode,
					isMasked, payloadSpan.subspan(0, payloadSize), clientPortStr));

				RETURN_IF_FALSE(test.Assert(serverObserver.HasConnectionFragmentedData(clientConnect), false,
					std::format(
						"Server does not have fragmented data messages for client {} after final fragmented message",
						clientPortStr)));
				RETURN_IF_FALSE(test.Assert(serverObserver.GetLastFragmentedDataTimer(clientConnect), MSAPI::Timer{ 0 },
					std::format("Server does not have timestamp of last fragmented data with client {} after final "
								"fragmented message",
						clientPortStr)));
				RETURN_IF_FALSE(test.Assert(clientObserver.GetSizeOfFragmentedDataConnections(), 0,
					std::format("Size of fragmented data connections on client {} after final fragmented message",
						clientPortStr)));
				RETURN_IF_FALSE(test.Assert(clientObserver.HasConnectionFragmentedData(serverConnect), false,
					std::format("Client {} does not have fragmented data with server after final fragmented messages",
						clientPortStr)));

				if (originalOpcode == MSAPI::Protocol::WebSocket::Data::Opcode::Text) {
					originalOpcode = MSAPI::Protocol::WebSocket::Data::Opcode::Binary;
				}
				else if (originalOpcode == MSAPI::Protocol::WebSocket::Data::Opcode::Binary) {
					originalOpcode = MSAPI::Protocol::WebSocket::Data::Opcode::Text;
				}
				else {
					RETURN_IF_FALSE(test.Assert(true, false,
						std::format("Original opcode should be text or binary, client {}", clientPortStr)));
				}
				auto& firstByteOfData{ MSAPI::Tests::Protocol::WebSocket::Observer::GetFirstByte(data) };
				firstByteOfData ^= static_cast<uint8_t>(MSAPI::Protocol::WebSocket::Data::Opcode::Continuation);
				firstByteOfData |= static_cast<uint8_t>(originalOpcode);
			}
		}

		// 2.22. Overwrite fragmented message with new initial message
		{
			const auto getMaskingKeyIfNeeded{ [isMasked]() -> uint32_t {
				if (isMasked) {
					return MSAPI::Protocol::WebSocket::Data::GenerateMaskingKey();
				}

				return 0;
			} };

			LOG_INFO_NEW("Testing overwriting fragmented message with new initial message, client {}", clientPortStr);
			MSAPI::Protocol::WebSocket::Data initialMessage{ payloadSpan.subspan(0, 100),
				MSAPI::Protocol::WebSocket::Data::Opcode::Text, getMaskingKeyIfNeeded(), false };
			MSAPI::Protocol::WebSocket::Send(serverConnect, initialMessage);
			MSAPI::Timer lastFragmentTime{ 0 };
			RETURN_IF_FALSE(waitForFragmentedData(lastFragmentTime));

			MSAPI::Protocol::WebSocket::Data continuationMessage{ payloadSpan.subspan(100, 100),
				MSAPI::Protocol::WebSocket::Data::Opcode::Continuation, getMaskingKeyIfNeeded(), false };
			MSAPI::Protocol::WebSocket::Send(serverConnect, continuationMessage);
			RETURN_IF_FALSE(waitForFragmentedData(lastFragmentTime));

			MSAPI::Protocol::WebSocket::Send(serverConnect, initialMessage);
			RETURN_IF_FALSE(waitForFragmentedData(lastFragmentTime));

			MSAPI::Protocol::WebSocket::Data finalMessage{ payloadSpan.subspan(0, 100),
				MSAPI::Protocol::WebSocket::Data::Opcode::Binary, getMaskingKeyIfNeeded() };
			MSAPI::Protocol::WebSocket::Send(serverConnect, finalMessage);
			++expectedServerWebSocketDataSize;
			RETURN_IF_FALSE(test.Wait(
				50000,
				[&serverWebSocketData, clientConnect, expectedServerWebSocketDataSize]() {
					return serverWebSocketData->size() == expectedServerWebSocketDataSize;
				},
				std::format("Server has WebSocket data with {} messages for client {}", expectedServerWebSocketDataSize,
					clientPortStr)));

			auto& lastServerWebSocketData1{ serverWebSocketData->back() };
			if (isMasked) {
				// Collected message always unmasked, checkMessage expect masked payload for masked message
				MSAPI::Tests::Protocol::WebSocket::Observer::ApplyMaskToData(lastServerWebSocketData1);
			}

			RETURN_IF_FALSE(checkMessage(lastServerWebSocketData1, true, false, false, false,
				MSAPI::Protocol::WebSocket::Data::Opcode::Binary, isMasked, payloadSpan.subspan(0, 100),
				clientPortStr));
			RETURN_IF_FALSE(test.Assert(serverObserver.HasConnectionFragmentedData(clientConnect), true,
				std::format(
					"Server still has fragmented data messages for client {} after overwritten fragmented message",
					clientPortStr)));
			RETURN_IF_FALSE(test.Assert(clientObserver.GetSizeOfFragmentedDataConnections(), 0,
				std::format("Size of fragmented data connections on client {} after overwritten fragmented message",
					clientPortStr)));
			RETURN_IF_FALSE(test.Assert(clientObserver.HasConnectionFragmentedData(serverConnect), false,
				std::format("Client {} does not have fragmented data with server after overwritten fragmented messages",
					clientPortStr)));

			MSAPI::Protocol::WebSocket::Data finalFragmentMessage{ payloadSpan.subspan(100, 100),
				MSAPI::Protocol::WebSocket::Data::Opcode::Continuation, getMaskingKeyIfNeeded(), true };
			MSAPI::Protocol::WebSocket::Send(serverConnect, finalFragmentMessage);
			++expectedServerWebSocketDataSize;
			RETURN_IF_FALSE(test.Wait(
				50000,
				[&serverWebSocketData, clientConnect, expectedServerWebSocketDataSize]() {
					return serverWebSocketData->size() == expectedServerWebSocketDataSize;
				},
				std::format("Server has WebSocket data with {} messages for client {}", expectedServerWebSocketDataSize,
					clientPortStr)));

			auto& lastServerWebSocketData2{ serverWebSocketData->back() };
			if (isMasked) {
				// Collected message always unmasked, checkMessage expect masked payload for masked message
				MSAPI::Tests::Protocol::WebSocket::Observer::ApplyMaskToData(lastServerWebSocketData2);
			}

			RETURN_IF_FALSE(checkMessage(lastServerWebSocketData2, true, false, false, false,
				MSAPI::Protocol::WebSocket::Data::Opcode::Text, isMasked, payloadSpan.subspan(0, 200), clientPortStr));
			RETURN_IF_FALSE(test.Assert(serverObserver.HasConnectionFragmentedData(clientConnect), false,
				std::format(
					"Server does not have fragmented data messages for client {} after final fragmented message",
					clientPortStr)));
			RETURN_IF_FALSE(test.Assert(serverObserver.GetLastFragmentedDataTimer(clientConnect), MSAPI::Timer{ 0 },
				std::format("Server does not have timestamp of last fragmented data with client {} after final "
							"fragmented message",
					clientPortStr)));
			RETURN_IF_FALSE(test.Assert(clientObserver.GetSizeOfFragmentedDataConnections(), 0,
				std::format(
					"Size of fragmented data connections on client {} after final fragmented message", clientPortStr)));
			RETURN_IF_FALSE(test.Assert(clientObserver.HasConnectionFragmentedData(serverConnect), false,
				std::format("Client {} does not have fragmented data with server after final fragmented messages",
					clientPortStr)));
		}

		return true;
	} };

	std::thread thread1(doTest, false);
	thread1.join();

	std::thread thread2(doTest, true);
	thread2.join();

	// 3.1. Final state of WebSocket Handler on server
	RETURN_IF_FALSE(test.Assert(
		serverObserver.GetSizeOfFragmentedDataConnections(), 0, "Final size of fragmented data connections on server"));
	RETURN_IF_FALSE(test.Assert(serverObserver.GetSizeOfFragmentedDataTimerToConnection(), 0,
		"Final size of fragmented data timer to connection on server"));
	RETURN_IF_FALSE(
		test.Assert(serverObserver.GetStoredFragmentedDataSize(), 0., "Final stored fragmented data size on server"));

	// 3.2. Check fragmented data storage purging due to limits with one connection scenarios
	{
		if (clientDaemons.size() < 1) [[unlikely]] {
			RETURN_IF_FALSE(test.Assert(
				true, false, "At least one client should be created for testing fragmented data storage purging"));
		}
		auto& clientDeamon{ clientDaemons[0] };
		auto client{ static_cast<Test::Node*>(clientDeamon.ptr->GetApp()) };
		const std::string& clientPortStr{ clientDeamon.portStr };
		const auto serverConnect{ clientDeamon.serverConnect };
		const auto clientConnect{ clientDeamon.clientConnect };

		if (client->GetWebSocketData(serverConnect) == nullptr) [[unlikely]] {
			RETURN_IF_FALSE(
				test.Assert(true, false, std::format("Client {} does not have web socket data", clientPortStr)));
		}
		if (server->GetWebSocketData(clientConnect) == nullptr) [[unlikely]] {
			RETURN_IF_FALSE(test.Assert(
				true, false, std::format("Server does not have web socket data for client {}", clientPortStr)));
		}

		auto clientWebSocketDataSize{ client->GetWebSocketData(serverConnect)->size() };
		auto serverWebSocketDataSize{ server->GetWebSocketData(clientConnect)->size() };

		// 3.2.1. Send fragmented message with initial payload size greater than fragmented data limit
		// 3.2.2. Send continuation message without initial message
		// 3.2.3. Send fragmented message with total payload size greater than fragmented data limit
		{
			LOG_INFO_NEW("Testing fragmented message purging with initial payload size greater than fragmented data "
						 "limit, client {}",
				clientPortStr);
			{
				const MSAPI::Protocol::WebSocket::Data data{
					payloadSpan.subspan(0,
						static_cast<size_t>(
							std::ceil(server->GetFragmentedDataLimit() * MSAPI::Protocol::WebSocket::Data::MB))
							+ 1),
					MSAPI::Protocol::WebSocket::Data::Opcode::Text, 0, false
				};
				MSAPI::Protocol::WebSocket::Send(serverConnect, data);
			}
			{
				const MSAPI::Protocol::WebSocket::Data data{
					payloadSpan.subspan(0,
						static_cast<size_t>(
							std::ceil(server->GetFragmentedDataLimit() * MSAPI::Protocol::WebSocket::Data::MB / 2))),
					MSAPI::Protocol::WebSocket::Data::Opcode::Continuation, 0, false
				};
				MSAPI::Protocol::WebSocket::Send(serverConnect, data);
			}
			{
				const MSAPI::Protocol::WebSocket::Data data{
					payloadSpan.subspan(0,
						static_cast<size_t>(
							std::ceil(server->GetFragmentedDataLimit() * MSAPI::Protocol::WebSocket::Data::MB / 2))),
					MSAPI::Protocol::WebSocket::Data::Opcode::Continuation, 0, true
				};
				MSAPI::Protocol::WebSocket::Send(serverConnect, data);
			}
			{
				const MSAPI::Protocol::WebSocket::Data data{
					payloadSpan.subspan(0,
						static_cast<size_t>(
							std::ceil(server->GetFragmentedDataLimit() * MSAPI::Protocol::WebSocket::Data::MB / 2))),
					MSAPI::Protocol::WebSocket::Data::Opcode::Text, 0, false
				};
				MSAPI::Protocol::WebSocket::Send(serverConnect, data);
			}
			{
				const MSAPI::Protocol::WebSocket::Data emptyFragment{ {},
					MSAPI::Protocol::WebSocket::Data::Opcode::Continuation, 0, false };
				MSAPI::Protocol::WebSocket::Send(serverConnect, emptyFragment);
				MSAPI::Protocol::WebSocket::Send(serverConnect, emptyFragment);
				MSAPI::Protocol::WebSocket::Send(serverConnect, emptyFragment);
				MSAPI::Protocol::WebSocket::Send(serverConnect, emptyFragment);
				MSAPI::Protocol::WebSocket::Send(serverConnect, emptyFragment);
				MSAPI::Protocol::WebSocket::Send(serverConnect, emptyFragment);
				MSAPI::Protocol::WebSocket::Send(serverConnect, emptyFragment);
				MSAPI::Protocol::WebSocket::Send(serverConnect, emptyFragment);
			}
			RETURN_IF_FALSE(test.Wait(
				50000,
				[&serverObserver, clientConnect, &clientPortStr]() {
					return serverObserver.HasConnectionFragmentedData(clientConnect);
				},
				std::format(
					"Server has fragmented data messages for client {} after fragmented message", clientPortStr)));
			RETURN_IF_FALSE(
				test.Assert(serverObserver.GetLastFragmentedDataTimer(clientConnect) != MSAPI::Timer{ 0 }, true,
					std::format("Server has timestamp of last fragmented data with client {} after fragmented message",
						clientPortStr)));
			RETURN_IF_FALSE(test.Assert(serverObserver.GetStoredFragmentedDataSize(),
				server->GetFragmentedDataLimit() / 2 + MSAPI::Protocol::WebSocket::Data::MAXIMUM_HEADER_MB,
				std::format("Server stored fragmented data size is correct for client {}", clientPortStr)));

			const MSAPI::Protocol::WebSocket::Data data{
				payloadSpan.subspan(0,
					static_cast<size_t>(
						std::ceil(server->GetFragmentedDataLimit() * MSAPI::Protocol::WebSocket::Data::MB / 2))
						+ 1),
				MSAPI::Protocol::WebSocket::Data::Opcode::Continuation
			};
			MSAPI::Protocol::WebSocket::Send(serverConnect, data);
			RETURN_IF_FALSE(test.Wait(
				50000,
				[&serverObserver, clientConnect, &clientPortStr]() {
					return !serverObserver.HasConnectionFragmentedData(clientConnect);
				},
				std::format("Server does not have fragmented data messages for client {} after sending fragmented "
							"message with payload size greater than fragmented data limit",
					clientPortStr)));

			RETURN_IF_FALSE(test.Assert(serverObserver.GetLastFragmentedDataTimer(clientConnect), MSAPI::Timer{ 0 },
				std::format("Server does not have timestamp of last fragmented data with client {} after sending "
							"fragmented message with payload size greater than fragmented data limit",
					clientPortStr)));
			RETURN_IF_FALSE(test.Assert(clientWebSocketDataSize, client->GetWebSocketData(serverConnect)->size(),
				std::format(
					"Size of WebSocket data on client {} should not change after sending fragmented message with "
					"payload size greater than fragmented data limit",
					clientPortStr)));
			RETURN_IF_FALSE(test.Assert(serverWebSocketDataSize, server->GetWebSocketData(clientConnect)->size(),
				std::format("Size of WebSocket data on server for client {} should not change after sending fragmented "
							"message with "
							"payload size greater than fragmented data limit",
					clientPortStr)));

			RETURN_IF_FALSE(test.Assert(serverObserver.GetSizeOfFragmentedDataConnections(), 0,
				std::format("Size of fragmented data connections on server after "
							"purging fragment data for client {}",
					clientPortStr)));
			RETURN_IF_FALSE(test.Assert(serverObserver.GetSizeOfFragmentedDataTimerToConnection(), 0,
				std::format("Size of fragmented data timer to connection on server after purging fragment data for "
							"client {}",
					clientPortStr)));
			RETURN_IF_FALSE(test.Assert(serverObserver.GetStoredFragmentedDataSize(), 0.,
				std::format(
					"Stored fragmented data size on server after purging fragment data for client {}", clientPortStr)));
		}

		// 3.2.4. Send fragmented message with payload size greater than fragmented data limit when there is already
		// fragmented data stored on server for client
		{
			const MSAPI::Protocol::WebSocket::Data data{
				payloadSpan.subspan(0,
					static_cast<size_t>(
						std::ceil(server->GetFragmentedDataLimit() * MSAPI::Protocol::WebSocket::Data::MB / 2))),
				MSAPI::Protocol::WebSocket::Data::Opcode::Binary, 0, false
			};
			MSAPI::Protocol::WebSocket::Send(serverConnect, data);
			RETURN_IF_FALSE(test.Wait(
				50000,
				[&serverObserver, clientConnect, &clientPortStr]() {
					return serverObserver.HasConnectionFragmentedData(clientConnect);
				},
				std::format(
					"Server has fragmented data messages for client {} after fragmented message", clientPortStr)));
			RETURN_IF_FALSE(
				test.Assert(serverObserver.GetLastFragmentedDataTimer(clientConnect) != MSAPI::Timer{ 0 }, true,
					std::format("Server has timestamp of last fragmented data with client {} after fragmented message",
						clientPortStr)));
			RETURN_IF_FALSE(test.Assert(serverObserver.GetStoredFragmentedDataSize(),
				server->GetFragmentedDataLimit() / 2. + MSAPI::Protocol::WebSocket::Data::MAXIMUM_HEADER_MB,
				std::format("Server stored fragmented data size is correct for client {}", clientPortStr)));
			const MSAPI::Protocol::WebSocket::Data newData{
				payloadSpan.subspan(0,
					static_cast<size_t>(
						std::ceil(server->GetFragmentedDataLimit() * MSAPI::Protocol::WebSocket::Data::MB))
						+ 1),
				MSAPI::Protocol::WebSocket::Data::Opcode::Continuation
			};
			MSAPI::Protocol::WebSocket::Send(serverConnect, newData);
			RETURN_IF_FALSE(test.Wait(
				50000,
				[&serverObserver, clientConnect, &clientPortStr]() {
					return !serverObserver.HasConnectionFragmentedData(clientConnect);
				},
				std::format("Server does not have fragmented data messages for client {} after sending fragmented "
							"message with payload size greater than fragmented data limit",
					clientPortStr)));
			RETURN_IF_FALSE(test.Assert(serverObserver.GetLastFragmentedDataTimer(clientConnect), MSAPI::Timer{ 0 },
				std::format("Server does not have timestamp of last fragmented data with client {} after sending "
							"fragmented message with payload size greater than fragmented data limit",
					clientPortStr)));
			RETURN_IF_FALSE(test.Assert(clientWebSocketDataSize, client->GetWebSocketData(serverConnect)->size(),
				std::format(
					"Size of WebSocket data on client {} should not change after sending fragmented message with "
					"payload size greater than fragmented data limit",
					clientPortStr)));
			RETURN_IF_FALSE(test.Assert(serverWebSocketDataSize, server->GetWebSocketData(clientConnect)->size(),
				std::format("Size of WebSocket data on server for client {} should not change after sending fragmented "
							"message with "
							"payload size greater than fragmented data limit",
					clientPortStr)));

			RETURN_IF_FALSE(test.Assert(serverObserver.GetSizeOfFragmentedDataConnections(), 0,
				std::format("Size of fragmented data connections on server after "
							"purging fragment data for client {}",
					clientPortStr)));
			RETURN_IF_FALSE(test.Assert(serverObserver.GetSizeOfFragmentedDataTimerToConnection(), 0,
				std::format("Size of fragmented data timer to connection on server after purging fragment data for "
							"client {}",
					clientPortStr)));
			RETURN_IF_FALSE(test.Assert(serverObserver.GetStoredFragmentedDataSize(), 0.,
				std::format(
					"Stored fragmented data size on server after purging fragment data for client {}", clientPortStr)));
		}

		// 3.2.5. Check zero fragmented data limit with 1 byte payload messages
		{
			RETURN_IF_FALSE(test.Assert(
				server->SetFragmentedDataLimit(0.), true, "Setting fragmented data limit to 0 should succeed"));
			RETURN_IF_FALSE(test.Assert(
				server->GetFragmentedDataLimit(), 0., "Fragmented data limit should be 0 after successful set"));

			const MSAPI::Protocol::WebSocket::Data initialFragment{ payloadSpan.subspan(0, 1),
				MSAPI::Protocol::WebSocket::Data::Opcode::Binary, 0, false };
			MSAPI::Protocol::WebSocket::Send(serverConnect, initialFragment);

			const MSAPI::Protocol::WebSocket::Data continuationFragment{ payloadSpan.subspan(0, 1),
				MSAPI::Protocol::WebSocket::Data::Opcode::Continuation, 0, false };
			MSAPI::Protocol::WebSocket::Send(serverConnect, continuationFragment);
			MSAPI::Protocol::WebSocket::Send(serverConnect, continuationFragment);
			MSAPI::Protocol::WebSocket::Send(serverConnect, continuationFragment);
			MSAPI::Protocol::WebSocket::Send(serverConnect, continuationFragment);

			const MSAPI::Protocol::WebSocket::Data data{ payloadSpan.subspan(0, 1),
				MSAPI::Protocol::WebSocket::Data::Opcode::Binary, 0, true };
			MSAPI::Protocol::WebSocket::Send(serverConnect, data);
			++serverWebSocketDataSize;
			RETURN_IF_FALSE(test.Wait(
				50000,
				[&server, clientConnect, serverWebSocketDataSize]() {
					return server->GetWebSocketData(clientConnect)->size() == serverWebSocketDataSize;
				},
				std::format("Server has WebSocket data with {} messages for client {}", serverWebSocketDataSize,
					clientPortStr)));

			RETURN_IF_FALSE(test.Assert(serverObserver.HasConnectionFragmentedData(clientConnect), false,
				std::format("Server does not have fragmented data messages for client {}", clientPortStr)));
			RETURN_IF_FALSE(test.Assert(serverObserver.GetLastFragmentedDataTimer(clientConnect), MSAPI::Timer{ 0 },
				std::format("Server does not have timestamp of last fragmented data with client {} after sending "
							"fragmented messages with zero fragmented data limit",
					clientPortStr)));
			RETURN_IF_FALSE(test.Assert(clientWebSocketDataSize, client->GetWebSocketData(serverConnect)->size(),
				std::format("Size of WebSocket data on client {} should not change after sending fragmented messages "
							"with zero fragmented data limit",
					clientPortStr)));

			RETURN_IF_FALSE(test.Assert(serverObserver.GetSizeOfFragmentedDataConnections(), 0,
				std::format(
					"Size of fragmented data connections on server fragment data for client {}", clientPortStr)));
			RETURN_IF_FALSE(test.Assert(serverObserver.GetSizeOfFragmentedDataTimerToConnection(), 0,
				std::format("Size of fragmented data timer to connection on server client {}", clientPortStr)));
			RETURN_IF_FALSE(test.Assert(serverObserver.GetStoredFragmentedDataSize(), 0.,
				std::format("Stored fragmented data size on server for client {}", clientPortStr)));
		}

		// 3.2.6. Check zero fragmented data limit with empty payload messages
		{
			const MSAPI::Protocol::WebSocket::Data initialFragment{ {},
				MSAPI::Protocol::WebSocket::Data::Opcode::Binary, 0, false };
			MSAPI::Protocol::WebSocket::Send(serverConnect, initialFragment);

			const MSAPI::Protocol::WebSocket::Data continuationFragment{ {},
				MSAPI::Protocol::WebSocket::Data::Opcode::Continuation, 0, false };
			MSAPI::Protocol::WebSocket::Send(serverConnect, continuationFragment);
			MSAPI::Protocol::WebSocket::Send(serverConnect, continuationFragment);
			MSAPI::Protocol::WebSocket::Send(serverConnect, continuationFragment);
			MSAPI::Protocol::WebSocket::Send(serverConnect, continuationFragment);

			const MSAPI::Protocol::WebSocket::Data data{ {}, MSAPI::Protocol::WebSocket::Data::Opcode::Binary, 0,
				true };
			MSAPI::Protocol::WebSocket::Send(serverConnect, data);
			++serverWebSocketDataSize;
			RETURN_IF_FALSE(test.Wait(
				50000,
				[&server, clientConnect, serverWebSocketDataSize]() {
					return server->GetWebSocketData(clientConnect)->size() == serverWebSocketDataSize;
				},
				std::format("Server has WebSocket data with {} messages for client {}", serverWebSocketDataSize,
					clientPortStr)));

			RETURN_IF_FALSE(test.Assert(serverObserver.HasConnectionFragmentedData(clientConnect), false,
				std::format("Server does not have fragmented data messages for client {}", clientPortStr)));
			RETURN_IF_FALSE(test.Assert(serverObserver.GetLastFragmentedDataTimer(clientConnect), MSAPI::Timer{ 0 },
				std::format("Server does not have timestamp of last fragmented data with client {} after sending "
							"fragmented messages with zero fragmented data limit",
					clientPortStr)));
			RETURN_IF_FALSE(test.Assert(clientWebSocketDataSize, client->GetWebSocketData(serverConnect)->size(),
				std::format("Size of WebSocket data on client {} should not change after sending fragmented messages "
							"with zero fragmented data limit",
					clientPortStr)));

			RETURN_IF_FALSE(test.Assert(serverObserver.GetSizeOfFragmentedDataConnections(), 0,
				std::format(
					"Size of fragmented data connections on server fragment data for client {}", clientPortStr)));
			RETURN_IF_FALSE(test.Assert(serverObserver.GetSizeOfFragmentedDataTimerToConnection(), 0,
				std::format("Size of fragmented data timer to connection on server client {}", clientPortStr)));
			RETURN_IF_FALSE(test.Assert(serverObserver.GetStoredFragmentedDataSize(), 0.,
				std::format("Stored fragmented data size on server for client {}", clientPortStr)));
		}

		// 3.2.7. Test for increasing, reducing and zeroing fragmented data limit
		// 3.2.8. Test clear and clear connection
		{
			RETURN_IF_FALSE(test.Assert(
				server->SetFragmentedDataLimit(1.), true, "Setting fragmented data limit to 1 should succeed"));
			RETURN_IF_FALSE(test.Assert(
				server->GetFragmentedDataLimit(), 1., "Fragmented data limit should be 1 after successful set"));

			const MSAPI::Protocol::WebSocket::Data initialFragment{ payloadSpan.subspan(
																		0, MSAPI::Protocol::WebSocket::Data::MB / 2),
				MSAPI::Protocol::WebSocket::Data::Opcode::Binary, 0, false };
			MSAPI::Protocol::WebSocket::Send(serverConnect, initialFragment);

			RETURN_IF_FALSE(test.Wait(
				50000,
				[&serverObserver, clientConnect, &clientPortStr]() {
					return serverObserver.HasConnectionFragmentedData(clientConnect);
				},
				std::format("Server has fragmented data messages for client {}", clientPortStr)));
			RETURN_IF_FALSE(test.Assert(serverObserver.GetLastFragmentedDataTimer(clientConnect) != MSAPI::Timer{ 0 },
				true, std::format("Server has timestamp of last fragmented data with client {}", clientPortStr)));
			RETURN_IF_FALSE(test.Assert(clientWebSocketDataSize, client->GetWebSocketData(serverConnect)->size(),
				std::format("Size of WebSocket data on client {} should not change", clientPortStr)));
			RETURN_IF_FALSE(test.Assert(serverWebSocketDataSize, server->GetWebSocketData(clientConnect)->size(),
				std::format("Size of WebSocket data on server for client {} should not change", clientPortStr)));

			RETURN_IF_FALSE(test.Assert(serverObserver.GetSizeOfFragmentedDataConnections(), 1,
				std::format("Size of fragmented data connections on server", clientPortStr)));
			RETURN_IF_FALSE(test.Assert(serverObserver.GetSizeOfFragmentedDataTimerToConnection(), 1,
				std::format("Size of fragmented data timer to connection on server for client {}", clientPortStr)));
			RETURN_IF_FALSE(test.Assert(serverObserver.GetStoredFragmentedDataSize(),
				0.5 + MSAPI::Protocol::WebSocket::Data::MAXIMUM_HEADER_MB,
				std::format("Stored fragmented data size on for client {}", clientPortStr)));

			RETURN_IF_FALSE(test.Assert(
				server->SetFragmentedDataLimit(1.1), true, "Setting fragmented data limit to 1.1 should succeed"));
			RETURN_IF_FALSE(test.Assert(
				server->GetFragmentedDataLimit(), 1.1, "Fragmented data limit should be 1.1 after successful set"));

			const MSAPI::Protocol::WebSocket::Data continuationFragment{ payloadSpan.subspan(0,
																			 MSAPI::Protocol::WebSocket::Data::MB / 2),
				MSAPI::Protocol::WebSocket::Data::Opcode::Continuation, 0, false };
			MSAPI::Protocol::WebSocket::Send(serverConnect, continuationFragment);

			const MSAPI::Protocol::WebSocket::Data data{ {}, MSAPI::Protocol::WebSocket::Data::Opcode::Binary, 0,
				true };
			MSAPI::Protocol::WebSocket::Send(serverConnect, data);
			++serverWebSocketDataSize;
			RETURN_IF_FALSE(test.Wait(
				50000,
				[&server, clientConnect, serverWebSocketDataSize]() {
					return server->GetWebSocketData(clientConnect)->size() == serverWebSocketDataSize;
				},
				std::format("Server has WebSocket data with {} messages for client {}", serverWebSocketDataSize,
					clientPortStr)));

			RETURN_IF_FALSE(test.Assert(serverObserver.HasConnectionFragmentedData(clientConnect), true,
				std::format("Server has fragmented data messages for client {}", clientPortStr)));
			RETURN_IF_FALSE(test.Assert(serverObserver.GetLastFragmentedDataTimer(clientConnect) != MSAPI::Timer{ 0 },
				true, std::format("Server has timestamp of last fragmented data with client {}", clientPortStr)));
			RETURN_IF_FALSE(test.Assert(clientWebSocketDataSize, client->GetWebSocketData(serverConnect)->size(),
				std::format("Size of WebSocket data on client {} should not change", clientPortStr)));

			RETURN_IF_FALSE(test.Assert(serverObserver.GetSizeOfFragmentedDataConnections(), 1,
				std::format("Size of fragmented data connections on server", clientPortStr)));
			RETURN_IF_FALSE(test.Assert(serverObserver.GetSizeOfFragmentedDataTimerToConnection(), 1,
				std::format("Size of fragmented data timer to connection on server for client {}", clientPortStr)));
			RETURN_IF_FALSE(test.Assert(serverObserver.GetStoredFragmentedDataSize(),
				1 + MSAPI::Protocol::WebSocket::Data::MAXIMUM_HEADER_MB,
				std::format("Stored fragmented data size on for client {}", clientPortStr)));

			RETURN_IF_FALSE(test.Assert(server->SetFragmentedDataLimit(1.00001), true,
				"Setting fragmented data limit to 1.00001 should succeed"));
			RETURN_IF_FALSE(test.Assert(server->GetFragmentedDataLimit(), 1.00001,
				"Fragmented data limit should be 1.00001 after successful set"));

			MSAPI::Protocol::WebSocket::Send(serverConnect, data);
			++serverWebSocketDataSize;
			RETURN_IF_FALSE(test.Wait(
				50000,
				[&server, clientConnect, serverWebSocketDataSize]() {
					return server->GetWebSocketData(clientConnect)->size() == serverWebSocketDataSize;
				},
				std::format("Server has WebSocket data with {} messages for client {}", serverWebSocketDataSize,
					clientPortStr)));

			RETURN_IF_FALSE(test.Assert(serverObserver.HasConnectionFragmentedData(clientConnect), true,
				std::format("Server has fragmented data messages for client {}", clientPortStr)));
			RETURN_IF_FALSE(test.Assert(serverObserver.GetLastFragmentedDataTimer(clientConnect) != MSAPI::Timer{ 0 },
				true, std::format("Server has timestamp of last fragmented data with client {}", clientPortStr)));
			RETURN_IF_FALSE(test.Assert(clientWebSocketDataSize, client->GetWebSocketData(serverConnect)->size(),
				std::format("Size of WebSocket data on client {} should not change", clientPortStr)));

			RETURN_IF_FALSE(test.Assert(serverObserver.GetSizeOfFragmentedDataConnections(), 1,
				std::format("Size of fragmented data connections on server", clientPortStr)));
			RETURN_IF_FALSE(test.Assert(serverObserver.GetSizeOfFragmentedDataTimerToConnection(), 1,
				std::format("Size of fragmented data timer to connection on server for client {}", clientPortStr)));
			RETURN_IF_FALSE(test.Assert(serverObserver.GetStoredFragmentedDataSize(),
				1 + MSAPI::Protocol::WebSocket::Data::MAXIMUM_HEADER_MB,
				std::format("Stored fragmented data size on for client {}", clientPortStr)));

			RETURN_IF_FALSE(test.Assert(
				server->SetFragmentedDataLimit(1.), true, "Setting fragmented data limit to 1 should succeed"));
			RETURN_IF_FALSE(test.Assert(
				server->GetFragmentedDataLimit(), 1., "Fragmented data limit should be 1 after successful set"));

			MSAPI::Protocol::WebSocket::Send(serverConnect, data);
			++serverWebSocketDataSize;
			RETURN_IF_FALSE(test.Wait(
				50000,
				[&server, clientConnect, serverWebSocketDataSize]() {
					return server->GetWebSocketData(clientConnect)->size() == serverWebSocketDataSize;
				},
				std::format("Server has WebSocket data with {} messages for client {}", serverWebSocketDataSize,
					clientPortStr)));

			RETURN_IF_FALSE(test.Assert(serverObserver.HasConnectionFragmentedData(clientConnect), false,
				std::format("Server does not have fragmented data messages for client {}", clientPortStr)));
			RETURN_IF_FALSE(test.Assert(serverObserver.GetLastFragmentedDataTimer(clientConnect), MSAPI::Timer{ 0 },
				std::format("Server does not have timestamp of last fragmented data with client {}", clientPortStr)));
			RETURN_IF_FALSE(test.Assert(clientWebSocketDataSize, client->GetWebSocketData(serverConnect)->size(),
				std::format("Size of WebSocket data on client {} should not change", clientPortStr)));

			RETURN_IF_FALSE(test.Assert(serverObserver.GetSizeOfFragmentedDataConnections(), 0,
				std::format("Size of fragmented data connections on server", clientPortStr)));
			RETURN_IF_FALSE(test.Assert(serverObserver.GetSizeOfFragmentedDataTimerToConnection(), 0,
				std::format("Size of fragmented data timer to connection on server for client {}", clientPortStr)));
			RETURN_IF_FALSE(test.Assert(serverObserver.GetStoredFragmentedDataSize(), 0.,
				std::format("Stored fragmented data size on for client {}", clientPortStr)));

			MSAPI::Protocol::WebSocket::Send(serverConnect, initialFragment);

			RETURN_IF_FALSE(test.Wait(
				50000,
				[&serverObserver, clientConnect, &clientPortStr]() {
					return serverObserver.HasConnectionFragmentedData(clientConnect);
				},
				std::format("Server has fragmented data messages for client {}", clientPortStr)));
			RETURN_IF_FALSE(test.Assert(serverObserver.GetLastFragmentedDataTimer(clientConnect) != MSAPI::Timer{ 0 },
				true, std::format("Server has timestamp of last fragmented data with client {}", clientPortStr)));
			RETURN_IF_FALSE(test.Assert(clientWebSocketDataSize, client->GetWebSocketData(serverConnect)->size(),
				std::format("Size of WebSocket data on client {} should not change", clientPortStr)));
			RETURN_IF_FALSE(test.Assert(serverWebSocketDataSize, server->GetWebSocketData(clientConnect)->size(),
				std::format("Size of WebSocket data on server for client {} should not change", clientPortStr)));

			RETURN_IF_FALSE(test.Assert(serverObserver.GetSizeOfFragmentedDataConnections(), 1,
				std::format("Size of fragmented data connections on server", clientPortStr)));
			RETURN_IF_FALSE(test.Assert(serverObserver.GetSizeOfFragmentedDataTimerToConnection(), 1,
				std::format("Size of fragmented data timer to connection on server for client {}", clientPortStr)));
			RETURN_IF_FALSE(test.Assert(serverObserver.GetStoredFragmentedDataSize(),
				0.5 + MSAPI::Protocol::WebSocket::Data::MAXIMUM_HEADER_MB,
				std::format("Stored fragmented data size on for client {}", clientPortStr)));

			server->Clear();
			RETURN_IF_FALSE(test.Assert(serverObserver.HasConnectionFragmentedData(clientConnect), false,
				std::format("Server does not have fragmented data messages for client {}", clientPortStr)));
			RETURN_IF_FALSE(test.Assert(serverObserver.GetLastFragmentedDataTimer(clientConnect), MSAPI::Timer{ 0 },
				std::format("Server does not have timestamp of last fragmented data with client {}", clientPortStr)));
			RETURN_IF_FALSE(test.Assert(clientWebSocketDataSize, client->GetWebSocketData(serverConnect)->size(),
				std::format("Size of WebSocket data on client {} should not change", clientPortStr)));

			RETURN_IF_FALSE(test.Assert(serverObserver.GetSizeOfFragmentedDataConnections(), 0,
				std::format("Size of fragmented data connections on server", clientPortStr)));
			RETURN_IF_FALSE(test.Assert(serverObserver.GetSizeOfFragmentedDataTimerToConnection(), 0,
				std::format("Size of fragmented data timer to connection on server for client {}", clientPortStr)));
			RETURN_IF_FALSE(test.Assert(serverObserver.GetStoredFragmentedDataSize(), 0.,
				std::format("Stored fragmented data size on for client {}", clientPortStr)));

			MSAPI::Protocol::WebSocket::Send(serverConnect, initialFragment);

			RETURN_IF_FALSE(test.Wait(
				50000,
				[&serverObserver, clientConnect, &clientPortStr]() {
					return serverObserver.HasConnectionFragmentedData(clientConnect);
				},
				std::format("Server has fragmented data messages for client {}", clientPortStr)));
			RETURN_IF_FALSE(test.Assert(serverObserver.GetLastFragmentedDataTimer(clientConnect) != MSAPI::Timer{ 0 },
				true, std::format("Server has timestamp of last fragmented data with client {}", clientPortStr)));
			RETURN_IF_FALSE(test.Assert(clientWebSocketDataSize, client->GetWebSocketData(serverConnect)->size(),
				std::format("Size of WebSocket data on client {} should not change", clientPortStr)));
			RETURN_IF_FALSE(test.Assert(serverWebSocketDataSize, server->GetWebSocketData(clientConnect)->size(),
				std::format("Size of WebSocket data on server for client {} should not change", clientPortStr)));

			RETURN_IF_FALSE(test.Assert(serverObserver.GetSizeOfFragmentedDataConnections(), 1,
				std::format("Size of fragmented data connections on server", clientPortStr)));
			RETURN_IF_FALSE(test.Assert(serverObserver.GetSizeOfFragmentedDataTimerToConnection(), 1,
				std::format("Size of fragmented data timer to connection on server for client {}", clientPortStr)));
			RETURN_IF_FALSE(test.Assert(serverObserver.GetStoredFragmentedDataSize(),
				0.5 + MSAPI::Protocol::WebSocket::Data::MAXIMUM_HEADER_MB,
				std::format("Stored fragmented data size on for client {}", clientPortStr)));

			server->ClearConnection(clientConnect);
			RETURN_IF_FALSE(test.Assert(serverObserver.HasConnectionFragmentedData(clientConnect), false,
				std::format("Server does not have fragmented data messages for client {}", clientPortStr)));
			RETURN_IF_FALSE(test.Assert(serverObserver.GetLastFragmentedDataTimer(clientConnect), MSAPI::Timer{ 0 },
				std::format("Server does not have timestamp of last fragmented data with client {}", clientPortStr)));
			RETURN_IF_FALSE(test.Assert(clientWebSocketDataSize, client->GetWebSocketData(serverConnect)->size(),
				std::format("Size of WebSocket data on client {} should not change", clientPortStr)));

			RETURN_IF_FALSE(test.Assert(serverObserver.GetSizeOfFragmentedDataConnections(), 0,
				std::format("Size of fragmented data connections on server", clientPortStr)));
			RETURN_IF_FALSE(test.Assert(serverObserver.GetSizeOfFragmentedDataTimerToConnection(), 0,
				std::format("Size of fragmented data timer to connection on server for client {}", clientPortStr)));
			RETURN_IF_FALSE(test.Assert(serverObserver.GetStoredFragmentedDataSize(), 0.,
				std::format("Stored fragmented data size on for client {}", clientPortStr)));
		}
	}

	serverPtr.reset();

	return test.Passed<int32_t>();
}