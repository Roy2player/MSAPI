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

	static_assert(std::is_same_v<std::underlying_type_t<MSAPI::WebSocketProtocol::Data::Opcode>, int8_t>);
	static_assert(static_cast<int8_t>(MSAPI::WebSocketProtocol::Data::Opcode::Continuation) == 0x0);
	static_assert(static_cast<int8_t>(MSAPI::WebSocketProtocol::Data::Opcode::Text) == 0x1);
	static_assert(static_cast<int8_t>(MSAPI::WebSocketProtocol::Data::Opcode::Binary) == 0x2);
	static_assert(static_cast<int8_t>(MSAPI::WebSocketProtocol::Data::Opcode::Close) == 0x8);
	static_assert(static_cast<int8_t>(MSAPI::WebSocketProtocol::Data::Opcode::Ping) == 0x9);
	static_assert(static_cast<int8_t>(MSAPI::WebSocketProtocol::Data::Opcode::Pong) == 0xA);

	static_assert(std::is_same_v<std::underlying_type_t<MSAPI::WebSocketProtocol::Data::CloseStatusCode>, int16_t>);
	static_assert(static_cast<int16_t>(MSAPI::WebSocketProtocol::Data::CloseStatusCode::NormalClosure) == 1000);
	static_assert(static_cast<int16_t>(MSAPI::WebSocketProtocol::Data::CloseStatusCode::GoingAway) == 1001);
	static_assert(static_cast<int16_t>(MSAPI::WebSocketProtocol::Data::CloseStatusCode::ProtocolError) == 1002);
	static_assert(static_cast<int16_t>(MSAPI::WebSocketProtocol::Data::CloseStatusCode::UnsupportedData) == 1003);
	static_assert(static_cast<int16_t>(MSAPI::WebSocketProtocol::Data::CloseStatusCode::Reserved) == 1004);
	static_assert(static_cast<int16_t>(MSAPI::WebSocketProtocol::Data::CloseStatusCode::NoStatusReceived) == 1005);
	static_assert(static_cast<int16_t>(MSAPI::WebSocketProtocol::Data::CloseStatusCode::AbnormalClosure) == 1006);
	static_assert(
		static_cast<int16_t>(MSAPI::WebSocketProtocol::Data::CloseStatusCode::InvalidFramePayloadData) == 1007);
	static_assert(static_cast<int16_t>(MSAPI::WebSocketProtocol::Data::CloseStatusCode::PolicyViolation) == 1008);
	static_assert(static_cast<int16_t>(MSAPI::WebSocketProtocol::Data::CloseStatusCode::MessageTooBig) == 1009);
	static_assert(static_cast<int16_t>(MSAPI::WebSocketProtocol::Data::CloseStatusCode::MandatoryExtension) == 1010);
	static_assert(static_cast<int16_t>(MSAPI::WebSocketProtocol::Data::CloseStatusCode::InternalError) == 1011);
	static_assert(static_cast<int16_t>(MSAPI::WebSocketProtocol::Data::CloseStatusCode::ServiceRestart) == 1012);
	static_assert(static_cast<int16_t>(MSAPI::WebSocketProtocol::Data::CloseStatusCode::TryAgainLater) == 1013);
	static_assert(static_cast<int16_t>(MSAPI::WebSocketProtocol::Data::CloseStatusCode::BadGateway) == 1014);
	static_assert(static_cast<int16_t>(MSAPI::WebSocketProtocol::Data::CloseStatusCode::TLSHandshakeFailure) == 1015);

	// Server
	const int serverId{ 1 };
	auto serverPtr{ MSAPI::Daemon<Test::Node>::Create("Server") };
	if (serverPtr == nullptr) {
		return 1;
	}
	auto server{ static_cast<Test::Node*>(serverPtr->GetApp()) };
	server->HandleRunRequest();

	MSAPI::IntegrationTest::WebSocketProtocol::Observer serverObserver{ *server };

	std::array<char, 2131070> payload{};
	for (size_t i{}; i < payload.size(); i++) {
		payload[i] = static_cast<char>(i % (128 - 32) + 32); // Printable ASCII characters
	}
	const auto payloadSpan{ std::span<const uint8_t>{
		reinterpret_cast<const uint8_t*>(payload.data()), payload.size() } };

	MSAPI::Test test;

	const auto checkMessage{ [&test](const MSAPI::WebSocketProtocol::Data& message, const bool isFinal,
								 const uint8_t rsv1, const uint8_t rsv2, const uint8_t rsv3,
								 const MSAPI::WebSocketProtocol::Data::Opcode opcode, const bool isMasked,
								 const int8_t headerSize, const std::span<const uint8_t> payload, const bool isValid,
								 const std::string_view clientPortStr) -> bool {
		const auto payloadSize{ payload.size() };
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
		RETURN_IF_FALSE(test.Assert(message.IsValid(), isValid, std::format("Message is valid {}", clientPortStr)));

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
				static_cast<uint8_t>(isFinal), rsv1, rsv2, rsv3, MSAPI::WebSocketProtocol::Data::EnumToString(opcode),
				static_cast<uint8_t>(isMasked), headerSize, header >> 8, header & 0xFF, payloadSize, isValid),
			std::format("Message string representation {}", clientPortStr)));

		RETURN_IF_FALSE(
			test.Assert(message.GetPayload(), payload, std::format("Message payload content {}", clientPortStr)));

		return true;
	} };

	const auto testMergePayload{ [&checkMessage, payloadSpan](const std::span<const uint8_t> payloadTo,
									 const int8_t headerSizeTo, const std::span<const uint8_t> payloadFrom,
									 const int8_t headerSizeFrom) -> bool {
		LOG_INFO_NEW(
			"Testing merging message with {} payload size in message with {}", payloadFrom.size(), payloadTo.size());
		MSAPI::WebSocketProtocol::Data dataTo{ payloadTo, MSAPI::WebSocketProtocol::Data::Opcode::Text, 0, true, true,
			true, true };
		RETURN_IF_FALSE(checkMessage(dataTo, true, 1, 1, 1, MSAPI::WebSocketProtocol::Data::Opcode::Text, false,
			headerSizeTo, payloadTo, true, ""));
		MSAPI::WebSocketProtocol::Data dataFrom{ payloadFrom, MSAPI::WebSocketProtocol::Data::Opcode::Continuation, 0,
			false };

		const auto totalPayloadSize{ payloadTo.size() + payloadFrom.size() };
		int8_t expectedHeaderSize{};
		if (totalPayloadSize <= 125) {
			expectedHeaderSize = 2;
		}
		else if (totalPayloadSize <= 65535) {
			expectedHeaderSize = 4;
		}
		else {
			expectedHeaderSize = 10;
		}
		if (payloadTo.empty()) {
			dataFrom.MergePayload(dataTo);
		}
		RETURN_IF_FALSE(checkMessage(dataFrom, false, 0, 0, 0, MSAPI::WebSocketProtocol::Data::Opcode::Continuation,
			false, headerSizeFrom, payloadFrom, true, ""));
		dataTo.MergePayload(dataFrom);
		RETURN_IF_FALSE(checkMessage(dataTo, true, 1, 1, 1, MSAPI::WebSocketProtocol::Data::Opcode::Text, false,
			expectedHeaderSize, payloadSpan.subspan(0, totalPayloadSize), true, ""));
		return true;
	} };

	struct TestData {
		const size_t payloadSize;
		const int8_t expectedHeaderSize;
	};

	std::array<TestData, 9> emptyPayloadTestData{ { { 1, 2 }, { 125, 2 }, { 126, 4 }, { 127, 4 }, { 128, 4 },
		{ 65535, 4 }, { 65536, 10 }, { 65537, 10 }, { 131070, 10 } } };

	// 0.1. Initial state of WebSocket Handler on server
	RETURN_IF_FALSE(test.Assert(serverObserver.GetSizeOfFragmentedDataConnections(), 0,
		"Initial size of fragmented data connections on server"));

	// 0.2. Merge in message without payload
	for (const auto [payloadSize, headerSize] : emptyPayloadTestData) {
		RETURN_IF_FALSE(
			testMergePayload(std::span<const uint8_t>{}, 2, payloadSpan.subspan(0, payloadSize), headerSize));
	}

	// 0.3. Merge message with payload in message with payload <= 125 to get total payload size > 65535
	RETURN_IF_FALSE(testMergePayload(payloadSpan.subspan(0, 125), 2, payloadSpan.subspan(125, 131070 - 125), 10));

	// 0.4 Default constructor without parameters
	{
		const MSAPI::WebSocketProtocol::Data data;
		RETURN_IF_FALSE(checkMessage(data, false, 0, 0, 0, MSAPI::WebSocketProtocol::Data::Opcode::Continuation, false,
			2, std::span<const uint8_t>{}, true, "Default constructor without parameters"));
	}

	// 0.5. Constructor with only required parameters
	{
		const MSAPI::WebSocketProtocol::Data data{ payloadSpan.subspan(0, 125),
			MSAPI::WebSocketProtocol::Data::Opcode::Text };
		RETURN_IF_FALSE(checkMessage(data, true, 0, 0, 0, MSAPI::WebSocketProtocol::Data::Opcode::Text, false, 2,
			payloadSpan.subspan(0, 125), true, "Constructor with only required parameters"));
	}

	const auto doTest{ [serverId, &serverPtr, server, &test, payloadSpan, &checkMessage, &serverObserver]() -> bool {
		auto clientPtr{ MSAPI::Daemon<Test::Node>::Create("Client") };
		if (clientPtr == nullptr) {
			return false;
		}
		auto client{ static_cast<Test::Node*>(clientPtr->GetApp()) };
		if (!client->OpenConnect(serverId, INADDR_LOOPBACK, serverPtr->GetPort(), false)) {
			return false;
		}
		client->HandleRunRequest();
		const auto clientPort{ clientPtr->GetPort() };
		const std::string& clientPortStr{ _S(clientPort) };

		MSAPI::IntegrationTest::WebSocketProtocol::Observer clientObserver{ *client };
		RETURN_IF_FALSE(test.Assert(clientObserver.GetSizeOfFragmentedDataConnections(), 0,
			std::format("Initial size of fragmented data connections on client {}", clientPortStr)));

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

		// 1. Establish WebSocket handshake
		LOG_INFO_NEW("Establishing WebSocket handshake for client {}", clientPortStr);
		client->SendHttp(serverId, wsHandshakeRequestPure);
		int onServerConnect{ -1 };
		RETURN_IF_FALSE(test.Wait(
			50000,
			[&server, clientPortStr, &onServerConnect]() {
				onServerConnect = server->DetectConnection(clientPortStr);
				return onServerConnect != -1;
			},
			std::format("Server detected connection for WebSocket handshake request from client {}", clientPortStr)));
		const auto* serverHttpData{ server->GetHttpData(onServerConnect) };
		size_t expectedServerHttpDataSize{};
		RETURN_IF_FALSE(test.Assert(
			serverHttpData != nullptr, true, std::format("Server has HTTP data for client {}", clientPortStr)));
		RETURN_IF_FALSE(test.Assert(serverHttpData->size(), ++expectedServerHttpDataSize,
			std::format("Server has {} HTTP data messages for client {}", serverHttpData->size(), clientPortStr)));
		RETURN_IF_FALSE(test.Assert(serverHttpData->back().ToString(), wsHandshakeRequest,
			std::format("Server got correct WebSocket handshake request from client {}", clientPortStr)));
		const std::vector<MSAPI::WebSocketProtocol::Data>* serverWebSocketData{};
		size_t expectedServerWebSocketDataSize{};

		const auto onClientConnectOpt{ client->GetConnect(serverId) };
		RETURN_IF_FALSE(test.Assert(onClientConnectOpt.has_value(), true,
			std::format("Client {} detected connection for WebSocket handshake response", clientPortStr)));
		const auto onClientConnect{ onClientConnectOpt.value() };
		const std::vector<MSAPI::HTTP::Data>* clientHttpData{};
		size_t expectedClientHttpDataSize{};

		RETURN_IF_FALSE(test.Wait(
			50000,
			[&clientHttpData, &client, onClientConnect]() {
				clientHttpData = client->GetHttpData(onClientConnect);
				return clientHttpData != nullptr;
			},
			std::format("Client {} has HTTP data", clientPortStr)));
		RETURN_IF_FALSE(test.Assert(clientHttpData->size(), ++expectedClientHttpDataSize,
			std::format("Client has {} HTTP data messages for client {}", clientHttpData->size(), clientPortStr)));
		RETURN_IF_FALSE(test.Assert(clientHttpData->back().ToString(), wsHandshakeResponse,
			std::format("Client {} got correct WebSocket handshake response", clientPortStr)));
		const std::vector<MSAPI::WebSocketProtocol::Data>* clientWebSocketData{};
		size_t expectedClientWebSocketDataSize{};

		RETURN_IF_FALSE(test.Assert(clientObserver.GetSizeOfFragmentedDataConnections(), 0,
			std::format("Size of fragmented data connections on client {} after handshake", clientPortStr)));
		RETURN_IF_FALSE(test.Assert(serverObserver.HasConnectionFragmentedData(onServerConnect), false,
			std::format("Server does not have fragmented data with client {} after handshake", clientPortStr)));
		RETURN_IF_FALSE(test.Assert(clientObserver.HasConnectionFragmentedData(onClientConnect), false,
			std::format("Client {} does not have fragmented data with server after handshake", clientPortStr)));

		RETURN_IF_FALSE(test.Assert(client->GetWebSocketData(onClientConnect), nullptr,
			std::format("Client {} does not have WebSocket yet", clientPortStr)));
		RETURN_IF_FALSE(test.Assert(server->GetWebSocketData(onServerConnect), nullptr,
			std::format("Server does not have WebSocket yet for client {}", clientPortStr)));

		const auto sendAndCheck{ [&test, &serverWebSocketData, &expectedServerWebSocketDataSize, &server,
									 onServerConnect, &clientWebSocketData, &expectedClientWebSocketDataSize, &client,
									 onClientConnect, &clientPortStr,
									 &checkMessage](const MSAPI::WebSocketProtocol::Data& message) -> bool {
			const auto opcode{ message.GetOpcode() };

			if (opcode != MSAPI::WebSocketProtocol::Data::Opcode::Close) {
				if (opcode == MSAPI::WebSocketProtocol::Data::Opcode::Ping) {
					++expectedClientWebSocketDataSize;
				}
				else {
					++expectedServerWebSocketDataSize;
				}
			}

			MSAPI::WebSocketProtocol::Send(onClientConnect, message);

			if (expectedClientWebSocketDataSize > 0) {
				RETURN_IF_FALSE(test.Wait(
					50000,
					[&clientWebSocketData, &client, onClientConnect, expectedClientWebSocketDataSize]() {
						clientWebSocketData = client->GetWebSocketData(onClientConnect);
						return clientWebSocketData != nullptr
							&& clientWebSocketData->size() == expectedClientWebSocketDataSize;
					},
					std::format("Client {} has WebSocket data with {} messages", clientPortStr,
						expectedClientWebSocketDataSize)));
			}

			RETURN_IF_FALSE(test.Wait(
				50000,
				[&serverWebSocketData, &server, onServerConnect, expectedServerWebSocketDataSize]() {
					serverWebSocketData = server->GetWebSocketData(onServerConnect);
					return serverWebSocketData != nullptr
						&& serverWebSocketData->size() == expectedServerWebSocketDataSize;
				},
				std::format("Server has WebSocket data with {} messages for client {}", expectedServerWebSocketDataSize,
					clientPortStr)));

			if (opcode != MSAPI::WebSocketProtocol::Data::Opcode::Close) {
				if (opcode == MSAPI::WebSocketProtocol::Data::Opcode::Ping) {
					RETURN_IF_FALSE(
						checkMessage(clientWebSocketData->back(), message.IsFinal(), message.IsRsv1(), message.IsRsv2(),
							message.IsRsv3(), MSAPI::WebSocketProtocol::Data::Opcode::Pong, message.IsMasked(),
							message.GetHeaderSize(), message.GetPayload(), message.IsValid(), clientPortStr));
				}
				else {
					RETURN_IF_FALSE(test.Assert(serverWebSocketData->back(), message,
						std::format("Server got correct WebSocket message for client {}", clientPortStr)));
				}
			}

			return true;
		} };

		const auto getMessageWithoutPayload{ []<MSAPI::WebSocketProtocol::Data::Opcode O>() {
			if constexpr (O == MSAPI::WebSocketProtocol::Data::Opcode::Close) {
				return MSAPI::WebSocketProtocol::Data::CreateClose();
			}
			else {
				return MSAPI::WebSocketProtocol::Data{ {}, O };
			}
		} };

		const auto doSendAndCheckWithoutPayload{ [&checkMessage, &sendAndCheck, &getMessageWithoutPayload,
													 &clientPortStr]<MSAPI::WebSocketProtocol::Data::Opcode O>(
													 const bool isFinal, const uint8_t rsv1, const uint8_t rsv2,
													 const uint8_t rsv3, const bool isMasked, const int8_t headerSize,
													 const bool isValid) -> bool {
			const auto message { getMessageWithoutPayload.template operator()<O>() };
			RETURN_IF_FALSE(checkMessage(message, isFinal, rsv1, rsv2, rsv3, O, isMasked, headerSize,
				std::span<const uint8_t>{}, isValid, clientPortStr));
			RETURN_IF_FALSE(sendAndCheck(message));

			return true;
		} };

		// 2. Send empty text message
		LOG_INFO_NEW("Testing empty text message from client {}", clientPortStr);
		RETURN_IF_FALSE(doSendAndCheckWithoutPayload.template operator()<MSAPI::WebSocketProtocol::Data::Opcode::Text>(
			true, 0, 0, 0, false, 2, true));

		// 3. Send empty binary message
		LOG_INFO_NEW("Testing empty binary message from client {}", clientPortStr);
		RETURN_IF_FALSE(
			doSendAndCheckWithoutPayload.template operator()<MSAPI::WebSocketProtocol::Data::Opcode::Binary>(
				true, 0, 0, 0, false, 2, true));

		// 4. Send empty ping message
		LOG_INFO_NEW("Testing empty ping message from client {}", clientPortStr);
		RETURN_IF_FALSE(doSendAndCheckWithoutPayload.template operator()<MSAPI::WebSocketProtocol::Data::Opcode::Ping>(
			true, 0, 0, 0, false, 2, true));

		// 5. Send empty pong message
		LOG_INFO_NEW("Testing empty pong message from client {}", clientPortStr);
		RETURN_IF_FALSE(doSendAndCheckWithoutPayload.template operator()<MSAPI::WebSocketProtocol::Data::Opcode::Pong>(
			true, 0, 0, 0, false, 2, true));

		const auto getMessageWithPayload{ [&test, &clientPortStr]<MSAPI::WebSocketProtocol::Data::Opcode O, typename T>(
											  const std::span<const T> payload) {
			if constexpr (O == MSAPI::WebSocketProtocol::Data::Opcode::Close) {
				if (payload.size() < 2) {
					LOG_WARNING_NEW(
						"Close message payload must be empty if it does not contain status code, client: {}",
						clientPortStr);
					return MSAPI::WebSocketProtocol::Data::CreateClose();
				}
				const int16_t statusCode{ reinterpret_cast<const int16_t*>(payload.data())[0] };
				return MSAPI::WebSocketProtocol::Data::CreateClose<int16_t, const std::span<const T>, T>(
					statusCode, payload.subspan(sizeof(int16_t)));
			}
			else {
				return MSAPI::WebSocketProtocol::Data{
					std::span<const uint8_t>{ reinterpret_cast<const uint8_t*>(payload.data()), payload.size() }, O
				};
			}
		} };

		const auto doSendAndCheckWithPayload{ [&checkMessage, &sendAndCheck, &getMessageWithPayload, &clientPortStr,
												  &test]<MSAPI::WebSocketProtocol::Data::Opcode O>(const bool isFinal,
												  const uint8_t rsv1, const uint8_t rsv2, const uint8_t rsv3,
												  const bool isMasked, const int8_t headerSize,
												  const std::span<const uint8_t> payload, const bool isValid) -> bool {
			if constexpr (O == MSAPI::WebSocketProtocol::Data::Opcode::Close) {
				if (payload.size() < 2) {
					RETURN_IF_FALSE(test.Assert(payload.empty(), true,
						std::format(
							"Close message payload must be empty if it does not contain status code, client: {}",
							clientPortStr)));
				}
			}
			const auto message { getMessageWithPayload.template operator()<O>(payload) };
			RETURN_IF_FALSE(checkMessage(
				message, isFinal, rsv1, rsv2, rsv3, O, isMasked, headerSize, payload, isValid, clientPortStr));
			RETURN_IF_FALSE(sendAndCheck(message));

			return true;
		} };

		std::array<TestData, 6> testData{ { { 1, 2 }, { 125, 2 }, { 126, 4 }, { 65535, 4 }, { 65536, 10 },
			{ 131070, 10 } } };

		// 6. Send small text message
		// 7. Send small binary message
		// 8. Send medium text message
		// 9. Send medium binary message
		// 10. Send large text message
		// 11. Send large binary message
		for (const auto [payloadSize, expectedHeaderSize] : testData) {
			LOG_INFO_NEW("Testing payload size {}, expected header size {}, client {}", payloadSize, expectedHeaderSize,
				clientPortStr);
			RETURN_IF_FALSE(doSendAndCheckWithPayload.template operator()<MSAPI::WebSocketProtocol::Data::Opcode::Text>(
				true, 0, 0, 0, false, expectedHeaderSize, payloadSpan.subspan(0, payloadSize), true));

			RETURN_IF_FALSE(
				doSendAndCheckWithPayload.template operator()<MSAPI::WebSocketProtocol::Data::Opcode::Binary>(
					true, 0, 0, 0, false, expectedHeaderSize, payloadSpan.subspan(0, payloadSize), true));
		}

		// 12. Send ping message with small payload
		// 13. Send pong message with small payload
		for (size_t index{}; index < 2; index++) {
			const auto [payloadSize, expectedHeaderSize] = testData[index];
			LOG_INFO_NEW("Testing control message with payload size {}, expected header size {}, client {}",
				payloadSize, expectedHeaderSize, clientPortStr);
			RETURN_IF_FALSE(doSendAndCheckWithPayload.template operator()<MSAPI::WebSocketProtocol::Data::Opcode::Ping>(
				true, 0, 0, 0, false, expectedHeaderSize, payloadSpan.subspan(0, payloadSize), true));

			RETURN_IF_FALSE(doSendAndCheckWithPayload.template operator()<MSAPI::WebSocketProtocol::Data::Opcode::Pong>(
				true, 0, 0, 0, false, expectedHeaderSize, payloadSpan.subspan(0, payloadSize), true));
		}

		// 14. Try to create ping message with payload greater than 125 bytes
		// 15. Try to create pong message with payload greater than 125 bytes
		for (size_t index{ 2 }; index < testData.size(); index++) {
			const auto [payloadSize, expectedHeaderSize] = testData[index];
			LOG_INFO_NEW(
				"Testing creation of control message with payload size {}, client {}", payloadSize, clientPortStr);
			const MSAPI::WebSocketProtocol::Data pingMessage{ payloadSpan.subspan(0, payloadSize),
				MSAPI::WebSocketProtocol::Data::Opcode::Ping };
			RETURN_IF_FALSE(checkMessage(pingMessage, true, 0, 0, 0, MSAPI::WebSocketProtocol::Data::Opcode::Ping,
				false, 2, std::span<const uint8_t>{}, true, clientPortStr));

			const MSAPI::WebSocketProtocol::Data pongMessage{ payloadSpan.subspan(0, payloadSize),
				MSAPI::WebSocketProtocol::Data::Opcode::Pong };
			RETURN_IF_FALSE(checkMessage(pongMessage, true, 0, 0, 0, MSAPI::WebSocketProtocol::Data::Opcode::Pong,
				false, 2, std::span<const uint8_t>{}, true, clientPortStr));
		}

		// TODO: For now checking of close messages does not happen in terms of reserving and followed that actions.
		// TODO: When Application will be able to call Server methods to close connection, then deeper testing of close
		// messages should be done
		// 16. Send empty close message
		LOG_INFO_NEW("Testing empty close message from client {}", clientPortStr);
		RETURN_IF_FALSE(doSendAndCheckWithoutPayload.template operator()<MSAPI::WebSocketProtocol::Data::Opcode::Close>(
			true, 0, 0, 0, false, 2, true));

		// 17. Send close message with small payload
		std::array<TestData, 2> closeTestData{ { { 2, 2 }, { 125, 2 } } };
		for (const auto [payloadSize, expectedHeaderSize] : closeTestData) {
			LOG_INFO_NEW("Testing creation of close message with payload size {}, expected header size {}, client {}",
				payloadSize, expectedHeaderSize, clientPortStr);
			RETURN_IF_FALSE(
				doSendAndCheckWithPayload.template operator()<MSAPI::WebSocketProtocol::Data::Opcode::Close>(
					true, 0, 0, 0, false, expectedHeaderSize, payloadSpan.subspan(0, payloadSize), true));
		}

		// 18. Try to create close message with payload greater than 125 bytes
		for (size_t index{ 2 }; index < testData.size(); index++) {
			const auto [payloadSize, expectedHeaderSize] = testData[index];
			LOG_INFO_NEW(
				"Testing creation of close message with payload size {}, client {}", payloadSize, clientPortStr);
			const auto message{
				MSAPI::WebSocketProtocol::Data::CreateClose<int16_t, std::span<const uint8_t>, const uint8_t>(
					8480, payloadSpan.subspan(0, payloadSize))
			};
			RETURN_IF_FALSE(checkMessage(message, true, 0, 0, 0, MSAPI::WebSocketProtocol::Data::Opcode::Close, false,
				2, payloadSpan.subspan(0, 2), true, clientPortStr));
		}

		// 19. Try to create SplitGenerator with empty payload
		{
			MSAPI::WebSocketProtocol::Data data{ {}, MSAPI::WebSocketProtocol::Data::Opcode::Text };
			MSAPI::WebSocketProtocol::Data::SplitGenerator generator{ data, std::span<const uint8_t>{}, 10 };
			RETURN_IF_FALSE(test.Assert(generator.Get(), false,
				std::format("SplitGenerator with empty payload should not have fragments, client {}", clientPortStr)));
		}

		// 20. Send small fragmented message with different steps size
		// 21. Send medium fragmented message with different steps size
		// 22. Send large fragmented message with different steps size
		RETURN_IF_FALSE(test.Assert(clientObserver.GetSizeOfFragmentedDataConnections(), 0,
			std::format("Size of fragmented data connections on client {} before fragmented messages", clientPortStr)));
		RETURN_IF_FALSE(test.Assert(serverObserver.HasConnectionFragmentedData(onServerConnect), false,
			std::format(
				"Server does not have fragmented data with client {} before fragmented messages", clientPortStr)));
		RETURN_IF_FALSE(test.Assert(clientObserver.HasConnectionFragmentedData(onClientConnect), false,
			std::format(
				"Client {} does not have fragmented data with server before fragmented messages", clientPortStr)));
		{
			MSAPI::WebSocketProtocol::Data data{ {}, MSAPI::WebSocketProtocol::Data::Opcode::Text };
			auto originalOpcode{ data.GetOpcode() };
			size_t sendedPayload{};
			size_t expectedFragments{};
			size_t sendedFragments{};
			size_t expectedLastFragmentPayloadSize{};
			int8_t expectedFragmentHeaderSize{};
			int8_t expectedLastFragmentHeaderSize{};
			int8_t expectedTotalHeaderSize{};

			const auto prepareExpectedValues{ [&sendedPayload, &expectedFragments, &sendedFragments,
												  &expectedFragmentHeaderSize, &expectedLastFragmentHeaderSize,
												  &expectedLastFragmentPayloadSize,
												  &expectedTotalHeaderSize](const size_t payloadSize, size_t& step) {
				if (step == 0 || step >= payloadSize) {
					step = std::min(payloadSize / 4, static_cast<size_t>(65535));
				}

				if (payloadSize <= 125) {
					expectedTotalHeaderSize = 2;
				}
				else if (payloadSize <= 65535) {
					expectedTotalHeaderSize = 4;
				}
				else {
					expectedTotalHeaderSize = 10;
				}

				sendedPayload = 0;
				if (step <= 125) {
					expectedFragmentHeaderSize = 2;
				}
				else if (step <= 65535) {
					expectedFragmentHeaderSize = 4;
				}
				else {
					expectedFragmentHeaderSize = 10;
				}

				expectedFragments = payloadSize / step;
				sendedFragments = 0;
				if (payloadSize % step != 0) {
					expectedFragments++;
					expectedLastFragmentPayloadSize = payloadSize % step;

					if (expectedLastFragmentPayloadSize <= 125) {
						expectedLastFragmentHeaderSize = 2;
					}
					else if (expectedLastFragmentPayloadSize <= 65535) {
						expectedLastFragmentHeaderSize = 4;
					}
					else {
						expectedLastFragmentHeaderSize = 10;
					}
				}
				else {
					expectedLastFragmentPayloadSize = step;
					expectedLastFragmentHeaderSize = expectedFragmentHeaderSize;
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
				MSAPI::WebSocketProtocol::Data::SplitGenerator generator{ data, payloadSpan.subspan(0, payloadSize),
					step };
				prepareExpectedValues(payloadSize, step);

				while (generator.Get()) {
					++sendedFragments;
					if (sendedFragments != expectedFragments) {
						sendedPayload += step;
						RETURN_IF_FALSE(checkMessage(data, false, 0, 0, 0,
							sendedFragments == 1 ? originalOpcode
												 : MSAPI::WebSocketProtocol::Data::Opcode::Continuation,
							false, expectedFragmentHeaderSize, payloadSpan.subspan(sendedPayload - step, step), true,
							clientPortStr));
						if (sendPingsBetweenFragments) {
							RETURN_IF_FALSE(doSendAndCheckWithPayload
												.template operator()<MSAPI::WebSocketProtocol::Data::Opcode::Ping>(
													true, 0, 0, 0, false, 2, payloadSpan.subspan(0, 125), true));
						}

						MSAPI::WebSocketProtocol::Send(onClientConnect, data);

						if (sendedFragments == 1) {
							RETURN_IF_FALSE(test.Wait(
								50000,
								[&serverObserver, onServerConnect, &clientPortStr]() {
									return serverObserver.HasConnectionFragmentedData(onServerConnect);
								},
								std::format(
									"Server has fragmented data messages for client {} after fragmented message",
									clientPortStr)));
							RETURN_IF_FALSE(test.Assert(clientObserver.GetSizeOfFragmentedDataConnections(), 0,
								std::format("Size of fragmented data connections on client {} after fragmented message",
									clientPortStr)));
							RETURN_IF_FALSE(
								test.Assert(clientObserver.HasConnectionFragmentedData(onClientConnect), false,
									std::format(
										"Client {} does not have fragmented data with server after fragmented messages",
										clientPortStr)));
						}
						continue;
					}

					sendedPayload += expectedLastFragmentPayloadSize;
					RETURN_IF_FALSE(checkMessage(data, true, 0, 0, 0,
						MSAPI::WebSocketProtocol::Data::Opcode::Continuation, false, expectedLastFragmentHeaderSize,
						payloadSpan.subspan(
							sendedPayload - expectedLastFragmentPayloadSize, expectedLastFragmentPayloadSize),
						true, clientPortStr));

					MSAPI::WebSocketProtocol::Send(onClientConnect, data);
				}
				sendPingsBetweenFragments = !sendPingsBetweenFragments;

				RETURN_IF_FALSE(test.Assert(sendedFragments, expectedFragments,
					std::format("All fragments were generated for payload size {} and step {}, client {}", payloadSize,
						step, clientPortStr)));

				++expectedServerWebSocketDataSize;
				RETURN_IF_FALSE(test.Wait(
					50000,
					[&serverWebSocketData, &server, onServerConnect, expectedServerWebSocketDataSize]() {
						serverWebSocketData = server->GetWebSocketData(onServerConnect);
						return serverWebSocketData != nullptr
							&& serverWebSocketData->size() == expectedServerWebSocketDataSize;
					},
					std::format("Server has WebSocket data with {} messages for client {}",
						expectedServerWebSocketDataSize, clientPortStr)));

				RETURN_IF_FALSE(checkMessage(serverWebSocketData->back(), true, 0, 0, 0, originalOpcode, false,
					expectedTotalHeaderSize, payloadSpan.subspan(0, payloadSize), true, clientPortStr));

				RETURN_IF_FALSE(test.Assert(serverObserver.HasConnectionFragmentedData(onClientConnect), false,
					std::format(
						"Server does not have fragmented data messages for client {} after final fragmented message",
						clientPortStr)));
				RETURN_IF_FALSE(test.Assert(clientObserver.GetSizeOfFragmentedDataConnections(), 0,
					std::format("Size of fragmented data connections on client {} after final fragmented message",
						clientPortStr)));
				RETURN_IF_FALSE(test.Assert(clientObserver.HasConnectionFragmentedData(onClientConnect), false,
					std::format("Client {} does not have fragmented data with server after final fragmented messages",
						clientPortStr)));

				if (originalOpcode == MSAPI::WebSocketProtocol::Data::Opcode::Text) {
					originalOpcode = MSAPI::WebSocketProtocol::Data::Opcode::Binary;
				}
				else if (originalOpcode == MSAPI::WebSocketProtocol::Data::Opcode::Binary) {
					originalOpcode = MSAPI::WebSocketProtocol::Data::Opcode::Text;
				}
				else {
					RETURN_IF_FALSE(test.Assert(true, false,
						std::format("Original opcode should be text or binary, client {}", clientPortStr)));
				}
				data.GetHeader()[0] ^= static_cast<uint8_t>(MSAPI::WebSocketProtocol::Data::Opcode::Continuation);
				data.GetHeader()[0] |= static_cast<uint8_t>(originalOpcode);
			}
		}

		return true;
	} };

	std::thread thread1(doTest);
	thread1.join();

	// 0.6. Final state of WebSocket Handler on server
	RETURN_IF_FALSE(test.Assert(
		serverObserver.GetSizeOfFragmentedDataConnections(), 0, "Final size of fragmented data connections on server"));

	serverPtr.reset();

	return test.Passed<int32_t>();
}