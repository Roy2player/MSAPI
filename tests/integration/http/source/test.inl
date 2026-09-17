/**************************
 * @file        test.inl
 * @date        2026-09-12
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
 * @brief Test covers HTTP communication between HTTP server and HTTP client with next scenarios:
 * 1) Send request to load index page with relative url;
 * 2) Reserved response with 200 and index page;
 * 3) Send request to load index page with relative url withput page format specification;
 * 4) Reserved response with 200 and index page;
 * 5) Send request to load unkown page;
 * 6) Reserved response with 404 and JSON;
 * 7) Send request to unkown file with specific header;
 * 8) Reserved response with 404 and JSON;
 * 9) Send request to load index page by slash;
 * 10) Reserved response with 200 and index page;
 * 11) Send request to load unknown page;
 * 12) Reserved response with 404;
 * 13) Send request to load favicon;
 * 14) Reserved response with 200 and favicon;
 * 15) Send request to load css;
 * 16) Reserved response with 200 and css;
 * 17) Send request to load js;
 * 18) Reserved response with 200 and js;
 * 19) Send request with specific header;
 * 20) Reserved response with 200 and JSON;
 * 21) Send request with wrong specific header;
 * 22) Reserved response with 404 and JSON;
 * 23) Send request with wrong HTTP type;
 * 24) Reserved response with 404 and JSON;
 * 25) Send request to load index page with symbol "?" in URL and following data;
 * 26) Reserved response with 200 and index page, symbol "?" and following data is ignored;
 * 27) Send request to load index page with symbol "#" in URL and following data;
 * 28) Reserved response with 200 and index page, symbol "#" and following data is ignored.
 */

#ifndef MSAPI_INTEGRATION_TEST_HTTP_PROTOCOL_INL
#define MSAPI_INTEGRATION_TEST_HTTP_PROTOCOL_INL

#include "../../../../library/source/help/io.inl"
#include "../../../../library/source/test/daemon.inl"
#include "../../../../library/source/test/test.inl"
#include "httpClient.h"
#include "httpServer.h"
#include <memory>
#include <sys/resource.h>

namespace MSAPI {

namespace Test {

namespace Integration {

/*---------------------------------------------------------------------------------
Declarations
---------------------------------------------------------------------------------*/

/**************************
 * @brief Integration test for http protocol.
 *
 * @return True if all tests passed and false if something went wrong.
 */
FORCE_INLINE [[nodiscard]] bool HttpProtocol();

/*---------------------------------------------------------------------------------
Definitions
---------------------------------------------------------------------------------*/

FORCE_INLINE [[nodiscard]] bool HttpProtocol()
{
	LOG_INFO("MSAPI INTEGRATION TEST: Http protocol");

	std::string path;
	path.resize(512);
	MSAPI::Helper::GetExecutableDir(path);
	if (path.empty()) [[unlikely]] {
		return false;
	}
	const std::string serverWebPath{ path + "../web/" };

	// Server
	auto serverPtr{ MSAPI::Daemon<HTTPServer>::Create("Server") };
	if (serverPtr == nullptr) [[unlikely]] {
		return false;
	}
	auto& server{ serverPtr->GetApp() };
	server.HandleModifyRequest({ { 1001, serverWebPath } });
	server.HandleRunRequest();
	if (!server.MSAPI::Application::IsRunning()) [[unlikely]] {
		LOG_ERROR("Server is not running, check parameters");
		return false;
	}

	// Client
	auto clientPtr{ MSAPI::Daemon<HTTPClient>::Create("Client") };
	if (clientPtr == nullptr) [[unlikely]] {
		return false;
	}
	auto& client{ clientPtr->GetApp() };
	const auto clientToServerConnectionData{ client.OpenConnection(
		INADDR_LOOPBACK, serverPtr->GetPort(), /*doReconnection=*/false) };
	if (clientToServerConnectionData == nullptr) [[unlikely]] {
		return -1;
	}
	auto& clientToServerConnection{ clientToServerConnectionData->GetConnection() };

	std::string indexPage;
	if (!MSAPI::IO::ReadStr<std::string_view>(indexPage, serverWebPath + "html/index.html")) [[unlikely]] {
		return false;
	}
	std::string favicon;
	if (!MSAPI::IO::ReadStr<std::string_view>(favicon, serverWebPath + "images/favicon.ico")) [[unlikely]] {
		return false;
	}
	std::string css;
	if (!MSAPI::IO::ReadStr<std::string_view>(css, serverWebPath + "css/style.css")) [[unlikely]] {
		return false;
	}
	std::string js;
	if (!MSAPI::IO::ReadStr<std::string_view>(js, serverWebPath + "js/index.js")) [[unlikely]] {
		return false;
	}

	MSAPI::Test::Test t;

	const auto checkAll{ [&t] [[nodiscard]] (const std::optional<MSAPI::Protocol::HTTP::Data>& httpData,
							 const bool isRequest, const std::string& typeMessage, const std::string& url,
							 const std::string& httpType, const std::string& version, const size_t messageSize,
							 const std::string& format, const std::string& body, const std::string& code,
							 const std::string& codeText, const std::map<std::string, std::string>& headersMap,
							 const std::string& toString) {
		RETURN_IF_FALSE(t.Assert(httpData.has_value(), true, "Has HTTP data"));
		RETURN_IF_FALSE(t.Assert(httpData->IsValid(), true, "HTTP data is valid"));
		RETURN_IF_FALSE(t.Assert(httpData->IsRequest(), isRequest, "HTTP data is request"));
		RETURN_IF_FALSE(t.Assert(httpData->GetTypeMessage(), typeMessage, "Message type is correct"));
		RETURN_IF_FALSE(t.Assert(httpData->GetUrl(), url, "URL is correct"));
		RETURN_IF_FALSE(t.Assert(httpData->GetHTTPType(), httpType, "HTTP type is correct"));
		RETURN_IF_FALSE(t.Assert(httpData->GetVersion(), version, "HTTP version is correct"));
		RETURN_IF_FALSE(t.Assert(httpData->GetMessageSize(), messageSize, "HTTP message size is correct"));
		RETURN_IF_FALSE(t.Assert(httpData->GetFormat(), format, "HTTP format is correct"));
		RETURN_IF_FALSE(t.Assert(httpData->GetBody(), body, "HTTP body is correct"));
		RETURN_IF_FALSE(t.Assert(httpData->GetCode(), code, "HTTP code is correct"));
		RETURN_IF_FALSE(t.Assert(httpData->GetCodeText(), codeText, "HTTP code text is correct"));
		RETURN_IF_FALSE(t.Assert(httpData->GetSizeHeadersMap(), headersMap.size(), "HTTP headers map size is correct"));
		for (const auto& [key, value] : headersMap) {
			RETURN_IF_FALSE(
				t.Assert(httpData->GetValue(key) != nullptr, true, "HTTP header \"" + key + "\" is not empty"));
		}
		RETURN_IF_FALSE(t.Assert(httpData->ToString(), toString, "HTTP data to string is correct"));

		return true;
	} };

	// 1) Send request to load index page with relative url
	RETURN_IF_FALSE(t.Assert(MSAPI::Protocol::HTTP::SendRequest(clientToServerConnection, "GET /index.html HTTP/1.1"),
		true, "Request is sent"));
	RETURN_IF_FALSE(t.Wait<uint64_t>(
		3000, [&server]() { return server.GetActionsNumber(); }, 2, "Server actions number is correct"));
	auto& serverHTTPData{ server.GetHTTPData() };
	RETURN_IF_FALSE(checkAll(serverHTTPData, true, "GET", "/index.html", "HTTP", "1.1", 28, "html", "", "", "", {},
		"HTTP message:\n{\n\tis valid     : true\n\ttype         : Request\n\tmessage type : GET\n\turl          : "
		"/index.html\n\tHTTP type    : HTTP\n\tversion      : 1.1\n\tformat       : html\n\tcode         : \n\tcode "
		"text    : \n\tmessage size : 28\n\tHeaders      :\n{\n}\n}"));

	// 2) Reserved response with 200 and index page
	RETURN_IF_FALSE(t.Wait<uint64_t>(
		3000, [&client]() { return client.GetActionsNumber(); }, 2, "Client actions number is correct"));
	const auto& clientHTTPData{ client.GetHTTPData() };
	RETURN_IF_FALSE(checkAll(clientHTTPData, false, "", "", "HTTP", "1.1", 5159, "", indexPage, "200", "OK",
		{ { "Content-Type", "text/html; charset=utf-8" }, { "Content-Length", "5025" }, { "Connection", "keep-alive" },
			{ "Keep-Alive", "timeout=0,max=0" } },
		"HTTP message:\n{\n\tis valid       : true\n\ttype           : Response\n\tmessage type   : \n\turl            "
		": \n\tHTTP type      : HTTP\n\tversion        : 1.1\n\tformat         : \n\tcode           : 200\n\tcode text "
		"     : OK\n\tmessage size   : 5159\n\tHeaders        :\n{\n\tConnection     : keep-alive\n\tContent-Length : "
		"5025\n\tContent-Type   : text/html; charset=utf-8\n\tKeep-Alive     : timeout=0,max=0\n}\n}"));

	// 3) Send request to load index page with relative url withput page format specification
	RETURN_IF_FALSE(t.Assert(
		MSAPI::Protocol::HTTP::SendRequest(clientToServerConnection, "GET /index HTTP/1.1"), true, "Request is sent"));
	RETURN_IF_FALSE(t.Wait<uint64_t>(
		3000, [&server]() { return server.GetActionsNumber(); }, 4, "Server actions number is correct"));
	RETURN_IF_FALSE(checkAll(serverHTTPData, true, "GET", "/index", "HTTP", "1.1", 23, "html", "", "", "", {},
		"HTTP message:\n{\n\tis valid     : true\n\ttype         : Request\n\tmessage type : GET\n\turl          : "
		"/index\n\tHTTP type    : HTTP\n\tversion      : 1.1\n\tformat       : html\n\tcode         : \n\tcode text    "
		": \n\tmessage size : 23\n\tHeaders      :\n{\n}\n}"));

	// 4) Reserved response with 200 and index page
	RETURN_IF_FALSE(t.Wait<uint64_t>(
		3000, [&client]() { return client.GetActionsNumber(); }, 4, "Client actions number is correct"));
	RETURN_IF_FALSE(checkAll(clientHTTPData, false, "", "", "HTTP", "1.1", 5159, "", indexPage, "200", "OK",
		{ { "Content-Type", "text/html; charset=utf-8" }, { "Content-Length", "5025" }, { "Connection", "keep-alive" },
			{ "Keep-Alive", "timeout=0,max=0" } },
		"HTTP message:\n{\n\tis valid       : true\n\ttype           : Response\n\tmessage type   : \n\turl            "
		": \n\tHTTP type      : HTTP\n\tversion        : 1.1\n\tformat         : \n\tcode           : 200\n\tcode text "
		"     : OK\n\tmessage size   : 5159\n\tHeaders        :\n{\n\tConnection     : keep-alive\n\tContent-Length : "
		"5025\n\tContent-Type   : text/html; charset=utf-8\n\tKeep-Alive     : timeout=0,max=0\n}\n}"));

	// 5) Send request to load unkown page
	RETURN_IF_FALSE(t.Assert(
		MSAPI::Protocol::HTTP::SendRequest(clientToServerConnection, "GET /info HTTP/1.1"), true, "Request is sent"));
	RETURN_IF_FALSE(t.Wait<uint64_t>(
		3000, [&server]() { return server.GetActionsNumber(); }, 6, "Server actions number is correct"));
	RETURN_IF_FALSE(checkAll(serverHTTPData, true, "GET", "/info", "HTTP", "1.1", 22, "html", "", "", "", {},
		"HTTP message:\n{\n\tis valid     : true\n\ttype         : Request\n\tmessage type : GET\n\turl          : "
		"/info\n\tHTTP type    : HTTP\n\tversion      : 1.1\n\tformat       : html\n\tcode         : \n\tcode text    "
		": \n\tmessage size : 22\n\tHeaders      :\n{\n}\n}"));

	// 6) Reserved response with 404 and JSON
	RETURN_IF_FALSE(t.Wait<uint64_t>(
		3000, [&client]() { return client.GetActionsNumber(); }, 6, "Client actions number is correct"));
	RETURN_IF_FALSE(checkAll(clientHTTPData, false, "", "", "HTTP", "1.1", 180, "",
		"{\"Error\":\"Page \"/info\" not found\"}", "404", "Not Found",
		{ { "Content-Type", "application/json; charset=utf-8" }, { "Content-Length", "34" },
			{ "Connection", "keep-alive" }, { "Keep-Alive", "timeout=0,max=0" } },
		"HTTP message:\n{\n\tis valid       : true\n\ttype           : Response\n\tmessage type   : \n\turl            "
		": \n\tHTTP type      : HTTP\n\tversion        : 1.1\n\tformat         : \n\tcode           : 404\n\tcode text "
		"     : Not Found\n\tmessage size   : 180\n\tHeaders        :\n{\n\tConnection     : "
		"keep-alive\n\tContent-Length : 34\n\tContent-Type   : application/json; charset=utf-8\n\tKeep-Alive     : "
		"timeout=0,max=0\n}\n}"));

	// 7) Send request to unkown file with specific header
	RETURN_IF_FALSE(t.Assert(MSAPI::Protocol::HTTP::SendRequest(clientToServerConnection,
								 "GET /archive.zip HTTP/1.1\r\nSome header: Hello for everybody 777!"),
		true, "Request is sent"));
	RETURN_IF_FALSE(t.Wait<uint64_t>(
		3000, [&server]() { return server.GetActionsNumber(); }, 8, "Server actions number is correct"));
	RETURN_IF_FALSE(checkAll(serverHTTPData, true, "GET", "/archive.zip", "HTTP", "1.1", 68, "zip", "", "", "",
		{ { "Some header", "Hello for everybody 777!" } },
		"HTTP message:\n{\n\tis valid     : true\n\ttype         : Request\n\tmessage type : GET\n\turl          : "
		"/archive.zip\n\tHTTP type    : HTTP\n\tversion      : 1.1\n\tformat       : zip\n\tcode         : \n\tcode "
		"text    : \n\tmessage size : 68\n\tHeaders      :\n{\n\tSome header  : Hello for everybody 777!\n}\n}"));

	// 8) Reserved response with 404 and JSON
	RETURN_IF_FALSE(t.Wait<uint64_t>(
		3000, [&client]() { return client.GetActionsNumber(); }, 8, "Client actions number is correct"));
	RETURN_IF_FALSE(checkAll(clientHTTPData, false, "", "", "HTTP", "1.1", 189, "",
		"{\"Error\":\"Source \"/archive.zip\" not found\"}", "404", "Not Found",
		{ { "Content-Type", "application/json; charset=utf-8" }, { "Content-Length", "43" },
			{ "Connection", "keep-alive" }, { "Keep-Alive", "timeout=0,max=0" } },
		"HTTP message:\n{\n\tis valid       : true\n\ttype           : Response\n\tmessage type   : \n\turl            "
		": \n\tHTTP type      : HTTP\n\tversion        : 1.1\n\tformat         : \n\tcode           : 404\n\tcode text "
		"     : Not Found\n\tmessage size   : 189\n\tHeaders        :\n{\n\tConnection     : "
		"keep-alive\n\tContent-Length : 43\n\tContent-Type   : application/json; charset=utf-8\n\tKeep-Alive     : "
		"timeout=0,max=0\n}\n}"));

	// 9) Send request to load index page by slash
	RETURN_IF_FALSE(t.Assert(
		MSAPI::Protocol::HTTP::SendRequest(clientToServerConnection, "GET / HTTP/1.1"), true, "Request is sent"));
	RETURN_IF_FALSE(t.Wait<uint64_t>(
		3000, [&server]() { return server.GetActionsNumber(); }, 10, "Server actions number is correct"));
	RETURN_IF_FALSE(checkAll(serverHTTPData, true, "GET", "/", "HTTP", "1.1", 18, "html", "", "", "", {},
		"HTTP message:\n{\n\tis valid     : true\n\ttype         : Request\n\tmessage type : GET\n\turl          : "
		"/\n\tHTTP type    : HTTP\n\tversion      : 1.1\n\tformat       : html\n\tcode         : \n\tcode text    : "
		"\n\tmessage size : 18\n\tHeaders      :\n{\n}\n}"));

	// 10) Reserved response with 200 and index page
	RETURN_IF_FALSE(t.Wait<uint64_t>(
		3000, [&client]() { return client.GetActionsNumber(); }, 10, "Client actions number is correct"));
	RETURN_IF_FALSE(checkAll(clientHTTPData, false, "", "", "HTTP", "1.1", 5159, "", indexPage, "200", "OK",
		{ { "Content-Type", "text/html; charset=utf-8" }, { "Content-Length", "5025" }, { "Connection", "keep-alive" },
			{ "Keep-Alive", "timeout=0,max=0" } },
		"HTTP message:\n{\n\tis valid       : true\n\ttype           : Response\n\tmessage type   : \n\turl            "
		": \n\tHTTP type      : HTTP\n\tversion        : 1.1\n\tformat         : \n\tcode           : 200\n\tcode text "
		"     : OK\n\tmessage size   : 5159\n\tHeaders        :\n{\n\tConnection     : keep-alive\n\tContent-Length : "
		"5025\n\tContent-Type   : text/html; charset=utf-8\n\tKeep-Alive     : timeout=0,max=0\n}\n}"));

	// 11) Send request to load unknown page
	RETURN_IF_FALSE(t.Assert(MSAPI::Protocol::HTTP::SendRequest(clientToServerConnection, "GET /unknown.html HTTP/1.1"),
		true, "Request is sent"));
	RETURN_IF_FALSE(t.Wait<uint64_t>(
		3000, [&server]() { return server.GetActionsNumber(); }, 12, "Server actions number is correct"));
	RETURN_IF_FALSE(checkAll(serverHTTPData, true, "GET", "/unknown.html", "HTTP", "1.1", 30, "html", "", "", "", {},
		"HTTP message:\n{\n\tis valid     : true\n\ttype         : Request\n\tmessage type : GET\n\turl          : "
		"/unknown.html\n\tHTTP type    : HTTP\n\tversion      : 1.1\n\tformat       : html\n\tcode         : \n\tcode "
		"text    : \n\tmessage size : 30\n\tHeaders      :\n{\n}\n}"));

	// 12) Reserved response with 404
	RETURN_IF_FALSE(t.Wait<uint64_t>(
		3000, [&client]() { return client.GetActionsNumber(); }, 12, "Client actions number is correct"));
	RETURN_IF_FALSE(checkAll(clientHTTPData, false, "", "", "HTTP", "1.1", 119, "", "", "404", "Not Found",
		{ { "Content-Type", "text/html; charset=utf-8" }, { "Connection", "keep-alive" },
			{ "Keep-Alive", "timeout=0,max=0" } },
		"HTTP message:\n{\n\tis valid     : true\n\ttype         : Response\n\tmessage type : \n\turl          : "
		"\n\tHTTP type    : HTTP\n\tversion      : 1.1\n\tformat       : \n\tcode         : 404\n\tcode text    : Not "
		"Found\n\tmessage size : 119\n\tHeaders      :\n{\n\tConnection   : keep-alive\n\tContent-Type : text/html; "
		"charset=utf-8\n\tKeep-Alive   : timeout=0,max=0\n}\n}"));

	// 13) Send request to load favicon
	RETURN_IF_FALSE(t.Assert(MSAPI::Protocol::HTTP::SendRequest(clientToServerConnection, "GET /favicon.ico HTTP/1.1"),
		true, "Request is sent"));
	RETURN_IF_FALSE(t.Wait<uint64_t>(
		3000, [&server]() { return server.GetActionsNumber(); }, 14, "Server actions number is correct"));
	RETURN_IF_FALSE(checkAll(serverHTTPData, true, "GET", "/favicon.ico", "HTTP", "1.1", 29, "ico", "", "", "", {},
		"HTTP message:\n{\n\tis valid     : true\n\ttype         : Request\n\tmessage type : GET\n\turl          : "
		"/favicon.ico\n\tHTTP type    : HTTP\n\tversion      : 1.1\n\tformat       : ico\n\tcode         : \n\tcode "
		"text    : \n\tmessage size : 29\n\tHeaders      :\n{\n}\n}"));

	// 14) Reserved response with 200 and favicon
	RETURN_IF_FALSE(t.Wait<uint64_t>(
		3000, [&client]() { return client.GetActionsNumber(); }, 14, "Client actions number is correct"));
	RETURN_IF_FALSE(checkAll(clientHTTPData, false, "", "", "HTTP", "1.1", 15544, "", favicon, "200", "OK",
		{ { "Content-Type", "image/x-icon; charset=utf-8" }, { "Content-Length", "15406" },
			{ "Connection", "keep-alive" }, { "Keep-Alive", "timeout=0,max=0" } },
		"HTTP message:\n{\n\tis valid       : true\n\ttype           : Response\n\tmessage type   : \n\turl            "
		": \n\tHTTP type      : HTTP\n\tversion        : 1.1\n\tformat         : \n\tcode           : 200\n\tcode text "
		"     : OK\n\tmessage size   : 15544\n\tHeaders        :\n{\n\tConnection     : keep-alive\n\tContent-Length : "
		"15406\n\tContent-Type   : image/x-icon; charset=utf-8\n\tKeep-Alive     : timeout=0,max=0\n}\n}"));

	// 15) Send request to load css
	RETURN_IF_FALSE(t.Assert(MSAPI::Protocol::HTTP::SendRequest(clientToServerConnection, "GET /style.css HTTP/1.1"),
		true, "Request is sent"));
	RETURN_IF_FALSE(t.Wait<uint64_t>(
		3000, [&server]() { return server.GetActionsNumber(); }, 16, "Server actions number is correct"));
	RETURN_IF_FALSE(checkAll(serverHTTPData, true, "GET", "/style.css", "HTTP", "1.1", 27, "css", "", "", "", {},
		"HTTP message:\n{\n\tis valid     : true\n\ttype         : Request\n\tmessage type : GET\n\turl          : "
		"/style.css\n\tHTTP type    : HTTP\n\tversion      : 1.1\n\tformat       : css\n\tcode         : \n\tcode text "
		"   : \n\tmessage size : 27\n\tHeaders      :\n{\n}\n}"));

	// 16) Reserved response with 200 and css
	RETURN_IF_FALSE(t.Wait<uint64_t>(
		3000, [&client]() { return client.GetActionsNumber(); }, 16, "Client actions number is correct"));
	RETURN_IF_FALSE(checkAll(clientHTTPData, false, "", "", "HTTP", "1.1", 4366, "", css, "200", "OK",
		{ { "Content-Type", "text/css; charset=utf-8" }, { "Content-Length", "4233" }, { "Connection", "keep-alive" },
			{ "Keep-Alive", "timeout=0,max=0" } },
		"HTTP message:\n{\n\tis valid       : true\n\ttype           : Response\n\tmessage type   : \n\turl            "
		": \n\tHTTP type      : HTTP\n\tversion        : 1.1\n\tformat         : \n\tcode           : 200\n\tcode text "
		"     : OK\n\tmessage size   : 4366\n\tHeaders        :\n{\n\tConnection     : keep-alive\n\tContent-Length : "
		"4233\n\tContent-Type   : text/css; charset=utf-8\n\tKeep-Alive     : timeout=0,max=0\n}\n}"));

	// 17) Send request to load js
	RETURN_IF_FALSE(t.Assert(MSAPI::Protocol::HTTP::SendRequest(clientToServerConnection, "GET /index.js HTTP/1.1"),
		true, "Request is sent"));
	RETURN_IF_FALSE(t.Wait<uint64_t>(
		3000, [&server]() { return server.GetActionsNumber(); }, 18, "Server actions number is correct"));
	RETURN_IF_FALSE(checkAll(serverHTTPData, true, "GET", "/index.js", "HTTP", "1.1", 26, "js", "", "", "", {},
		"HTTP message:\n{\n\tis valid     : true\n\ttype         : Request\n\tmessage type : GET\n\turl          : "
		"/index.js\n\tHTTP type    : HTTP\n\tversion      : 1.1\n\tformat       : js\n\tcode         : \n\tcode text   "
		" : \n\tmessage size : 26\n\tHeaders      :\n{\n}\n}"));

	// 18) Reserved response with 200 and js
	RETURN_IF_FALSE(t.Wait<uint64_t>(
		3000, [&client]() { return client.GetActionsNumber(); }, 18, "Client actions number is correct"));
	RETURN_IF_FALSE(checkAll(clientHTTPData, false, "", "", "HTTP", "1.1", 5261, "", js, "200", "OK",
		{ { "Content-Type", "application/javascript; charset=utf-8" }, { "Content-Length", "5114" },
			{ "Connection", "keep-alive" }, { "Keep-Alive", "timeout=0,max=0" } },
		"HTTP message:\n{\n\tis valid       : true\n\ttype           : Response\n\tmessage type   : \n\turl            "
		": \n\tHTTP type      : HTTP\n\tversion        : 1.1\n\tformat         : \n\tcode           : 200\n\tcode text "
		"     : OK\n\tmessage size   : 5261\n\tHeaders        :\n{\n\tConnection     : keep-alive\n\tContent-Length : "
		"5114\n\tContent-Type   : application/javascript; charset=utf-8\n\tKeep-Alive     : timeout=0,max=0\n}\n}"));

	// 19) Send request with specific header
	RETURN_IF_FALSE(t.Assert(MSAPI::Protocol::HTTP::SendRequest(clientToServerConnection,
								 "GET /api HTTP/1.1\r\nIdentifier: 369\nAction: Send me some JSON, please"),
		true, "Request is sent"));
	RETURN_IF_FALSE(t.Wait<uint64_t>(
		3000, [&server]() { return server.GetActionsNumber(); }, 20, "Server actions number is correct"));
	RETURN_IF_FALSE(checkAll(serverHTTPData, true, "GET", "/api", "HTTP", "1.1", 72, "html", "", "", "",
		{ { "Identifier", "369" }, { "Action", "Send me some JSON, please" } },
		"HTTP message:\n{\n\tis valid     : true\n\ttype         : Request\n\tmessage type : GET\n\turl          : "
		"/api\n\tHTTP type    : HTTP\n\tversion      : 1.1\n\tformat       : html\n\tcode         : \n\tcode text    : "
		"\n\tmessage size : 72\n\tHeaders      :\n{\n\tAction       : Send me some JSON, please\n\tIdentifier   : "
		"369\n}\n}"));

	// 20) Reserved response with 200 and JSON
	RETURN_IF_FALSE(t.Wait<uint64_t>(
		3000, [&client]() { return client.GetActionsNumber(); }, 20, "Client actions number is correct"));
	RETURN_IF_FALSE(checkAll(clientHTTPData, false, "", "", "HTTP", "1.1", 170, "",
		"{\"Message\":\"Here is your JSON\"}", "200", "OK",
		{ { "Content-Type", "application/json; charset=utf-8" }, { "Content-Length", "31" },
			{ "Connection", "keep-alive" }, { "Keep-Alive", "timeout=0,max=0" } },
		"HTTP message:\n{\n\tis valid       : true\n\ttype           : Response\n\tmessage type   : \n\turl            "
		": \n\tHTTP type      : HTTP\n\tversion        : 1.1\n\tformat         : \n\tcode           : 200\n\tcode text "
		"     : OK\n\tmessage size   : 170\n\tHeaders        :\n{\n\tConnection     : keep-alive\n\tContent-Length : "
		"31\n\tContent-Type   : application/json; charset=utf-8\n\tKeep-Alive     : timeout=0,max=0\n}\n}"));

	// 21) Send request with wrong specific header
	RETURN_IF_FALSE(t.Assert(MSAPI::Protocol::HTTP::SendRequest(clientToServerConnection,
								 "GET /api HTTP/1.1\r\nIdentifier: 368\nAction: Send me some JSON, please"),
		true, "Request is sent"));
	RETURN_IF_FALSE(t.Wait<uint64_t>(
		3000, [&server]() { return server.GetActionsNumber(); }, 22, "Server actions number is correct"));
	RETURN_IF_FALSE(checkAll(serverHTTPData, true, "GET", "/api", "HTTP", "1.1", 72, "html", "", "", "",
		{ { "Identifier", "368" }, { "Action", "Send me some JSON, please" } },
		"HTTP message:\n{\n\tis valid     : true\n\ttype         : Request\n\tmessage type : GET\n\turl          : "
		"/api\n\tHTTP type    : HTTP\n\tversion      : 1.1\n\tformat       : html\n\tcode         : \n\tcode text    : "
		"\n\tmessage size : 72\n\tHeaders      :\n{\n\tAction       : Send me some JSON, please\n\tIdentifier   : "
		"368\n}\n}"));

	// 22) Reserved response with 404 and JSON
	RETURN_IF_FALSE(t.Wait<uint64_t>(
		3000, [&client]() { return client.GetActionsNumber(); }, 22, "Client actions number is correct"));
	RETURN_IF_FALSE(checkAll(clientHTTPData, false, "", "", "HTTP", "1.1", 181, "",
		"{\"Error\":\"Identifier is not valid\"}", "404", "Not Found",
		{ { "Content-Type", "application/json; charset=utf-8" }, { "Content-Length", "35" },
			{ "Connection", "keep-alive" }, { "Keep-Alive", "timeout=0,max=0" } },
		"HTTP message:\n{\n\tis valid       : true\n\ttype           : Response\n\tmessage type   : \n\turl            "
		": \n\tHTTP type      : HTTP\n\tversion        : 1.1\n\tformat         : \n\tcode           : 404\n\tcode text "
		"     : Not Found\n\tmessage size   : 181\n\tHeaders        :\n{\n\tConnection     : "
		"keep-alive\n\tContent-Length : 35\n\tContent-Type   : application/json; charset=utf-8\n\tKeep-Alive     : "
		"timeout=0,max=0\n}\n}"));

	// 23) Send request with wrong HTTP type
	RETURN_IF_FALSE(t.Assert(MSAPI::Protocol::HTTP::SendRequest(clientToServerConnection,
								 "POST /api HTTP/1.1\r\nIdentifier: 369\nAction: Send me some JSON, please"),
		true, "Request is sent"));
	RETURN_IF_FALSE(t.Wait<uint64_t>(
		3000, [&server]() { return server.GetActionsNumber(); }, 24, "Server actions number is correct"));
	RETURN_IF_FALSE(checkAll(serverHTTPData, true, "POST", "/api", "HTTP", "1.1", 73, "html", "", "", "",
		{ { "Identifier", "369" }, { "Action", "Send me some JSON, please" } },
		"HTTP message:\n{\n\tis valid     : true\n\ttype         : Request\n\tmessage type : POST\n\turl          : "
		"/api\n\tHTTP type    : HTTP\n\tversion      : 1.1\n\tformat       : html\n\tcode         : \n\tcode text    : "
		"\n\tmessage size : 73\n\tHeaders      :\n{\n\tAction       : Send me some JSON, please\n\tIdentifier   : "
		"369\n}\n}"));

	// 24) Reserved response with 404 and JSON
	RETURN_IF_FALSE(t.Wait<uint64_t>(
		3000, [&client]() { return client.GetActionsNumber(); }, 24, "Client actions number is correct"));
	RETURN_IF_FALSE(checkAll(clientHTTPData, false, "", "", "HTTP", "1.1", 183, "",
		"{\"Error\":\"Method \"POST\" not allowed\"}", "404", "Not Found",
		{ { "Content-Type", "application/json; charset=utf-8" }, { "Content-Length", "37" },
			{ "Connection", "keep-alive" }, { "Keep-Alive", "timeout=0,max=0" } },
		"HTTP message:\n{\n\tis valid       : true\n\ttype           : Response\n\tmessage type   : \n\turl            "
		": \n\tHTTP type      : HTTP\n\tversion        : 1.1\n\tformat         : \n\tcode           : 404\n\tcode text "
		"     : Not Found\n\tmessage size   : 183\n\tHeaders        :\n{\n\tConnection     : "
		"keep-alive\n\tContent-Length : 37\n\tContent-Type   : application/json; charset=utf-8\n\tKeep-Alive     : "
		"timeout=0,max=0\n}\n}"));

	// 25) Send request to load index page with symbol "?" in URL and following data
	RETURN_IF_FALSE(t.Assert(MSAPI::Protocol::HTTP::SendRequest(
								 clientToServerConnection, "GET /index?parameter=83648&additionalData=GTP HTTP/1.1"),
		true, "Request is sent"));
	RETURN_IF_FALSE(t.Wait<uint64_t>(
		3000, [&server]() { return server.GetActionsNumber(); }, 26, "Server actions number is correct"));
	RETURN_IF_FALSE(checkAll(serverHTTPData, true, "GET", "/index", "HTTP", "1.1", 58, "html", "", "", "", {},
		"HTTP message:\n{\n\tis valid     : true\n\ttype         : Request\n\tmessage type : GET\n\turl          : "
		"/index\n\tHTTP type    : HTTP\n\tversion      : 1.1\n\tformat       : html\n\tcode         : \n\tcode text    "
		": \n\tmessage size : 58\n\tHeaders      :\n{\n}\n}"));

	// 26) Reserved response with 200 and index page, symbol "?" and following data is ignored
	RETURN_IF_FALSE(t.Wait<uint64_t>(
		3000, [&client]() { return client.GetActionsNumber(); }, 26, "Client actions number is correct"));
	RETURN_IF_FALSE(checkAll(clientHTTPData, false, "", "", "HTTP", "1.1", 5159, "", indexPage, "200", "OK",
		{ { "Content-Type", "text/html; charset=utf-8" }, { "Content-Length", "5025" }, { "Connection", "keep-alive" },
			{ "Keep-Alive", "timeout=0,max=0" } },
		"HTTP message:\n{\n\tis valid       : true\n\ttype           : Response\n\tmessage type   : \n\turl            "
		": \n\tHTTP type      : HTTP\n\tversion        : 1.1\n\tformat         : \n\tcode           : 200\n\tcode text "
		"     : OK\n\tmessage size   : 5159\n\tHeaders        :\n{\n\tConnection     : keep-alive\n\tContent-Length : "
		"5025\n\tContent-Type   : text/html; charset=utf-8\n\tKeep-Alive     : timeout=0,max=0\n}\n}"));

	// 27) Send request to load index page with symbol "#" in URL and following data
	RETURN_IF_FALSE(
		t.Assert(MSAPI::Protocol::HTTP::SendRequest(clientToServerConnection, "GET /index.html#section HTTP/1.1"), true,
			"Request is sent"));
	RETURN_IF_FALSE(t.Wait<uint64_t>(
		3000, [&server]() { return server.GetActionsNumber(); }, 28, "Server actions number is correct"));
	RETURN_IF_FALSE(checkAll(serverHTTPData, true, "GET", "/index.html", "HTTP", "1.1", 36, "html", "", "", "", {},
		"HTTP message:\n{\n\tis valid     : true\n\ttype         : Request\n\tmessage type : GET\n\turl          : "
		"/index.html\n\tHTTP type    : HTTP\n\tversion      : 1.1\n\tformat       : html\n\tcode         : \n\tcode "
		"text    : \n\tmessage size : 36\n\tHeaders      :\n{\n}\n}"));

	// 28) Reserved response with 200 and index page, symbol "#" and following data is ignored
	RETURN_IF_FALSE(t.Wait<uint64_t>(
		3000, [&client]() { return client.GetActionsNumber(); }, 28, "Client actions number is correct"));
	RETURN_IF_FALSE(checkAll(clientHTTPData, false, "", "", "HTTP", "1.1", 5159, "", indexPage, "200", "OK",
		{ { "Content-Type", "text/html; charset=utf-8" }, { "Content-Length", "5025" }, { "Connection", "keep-alive" },
			{ "Keep-Alive", "timeout=0,max=0" } },
		"HTTP message:\n{\n\tis valid       : true\n\ttype           : Response\n\tmessage type   : \n\turl            "
		": \n\tHTTP type      : HTTP\n\tversion        : 1.1\n\tformat         : \n\tcode           : 200\n\tcode text "
		"     : OK\n\tmessage size   : 5159\n\tHeaders        :\n{\n\tConnection     : keep-alive\n\tContent-Length : "
		"5025\n\tContent-Type   : text/html; charset=utf-8\n\tKeep-Alive     : timeout=0,max=0\n}\n}"));

	serverPtr.reset();
	clientPtr.reset();

	return t.Passed<bool>();
}

} // namespace Integration

} // namespace Test

} // namespace MSAPI

#endif // MSAPI_INTEGRATION_TEST_HTTP_PROTOCOL_INL