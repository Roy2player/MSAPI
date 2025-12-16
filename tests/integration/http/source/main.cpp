/**************************
 * @file        main.cpp
 * @version     6.0
 * @date        2024-05-02
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

#include "../../../../library/source/help/bin.h"
#include "../../../../library/source/test/daemon.hpp"
#include "../../../../library/source/test/test.h"
#include "httpClient.h"
#include "httpServer.h"
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
	const std::string serverWebPath{ path + "web/" };
	path += "logs/";

	//* Clear old files
	std::vector<std::string> files;
	if (MSAPI::Bin::List<MSAPI::Bin::FileType::Regular>(files, path)) {
		for (const auto& file : files) {
			MSAPI::Bin::Remove(path + file);
		}
	}

	MSAPI::logger.SetLevelSave(MSAPI::Log::Level::INFO);
	MSAPI::logger.SetName("TestHTTP");
	MSAPI::logger.SetToFile(true);
	MSAPI::logger.SetToConsole(true);
	MSAPI::logger.Start();

	//* Server
	const int serverId{ 1 };
	auto serverPtr{ MSAPI::Daemon<HTTPServer>::Create("Server") };
	if (serverPtr == nullptr) {
		return 1;
	}
	auto server{ static_cast<HTTPServer*>(serverPtr->GetApp()) };
	server->HandleModifyRequest({ { 1001, serverWebPath } });
	server->HandleRunRequest();
	if (!server->MSAPI::Application::IsRunning()) {
		LOG_ERROR("Server is not running, check parameters");
		return 1;
	}

	//* Client
	auto clientPtr{ MSAPI::Daemon<HTTPClient>::Create("Client") };
	if (clientPtr == nullptr) {
		return 1;
	}
	auto client{ static_cast<HTTPClient*>(clientPtr->GetApp()) };
	if (!client->OpenConnect(serverId, INADDR_LOOPBACK, serverPtr->GetPort(), false)) {
		return 1;
	}

	std::string indexPage;
	if (!MSAPI::Bin::ReadStr(indexPage, serverWebPath + "html/index.html")) {
		return 1;
	}
	std::string favicon;
	if (!MSAPI::Bin::ReadStr(favicon, serverWebPath + "images/favicon.ico")) {
		return 1;
	}
	std::string css;
	if (!MSAPI::Bin::ReadStr(css, serverWebPath + "css/style.css")) {
		return 1;
	}
	std::string js;
	if (!MSAPI::Bin::ReadStr(js, serverWebPath + "js/index.js")) {
		return 1;
	}

	MSAPI::Test test;

	const auto checkAll
		= [&test](const std::optional<MSAPI::HTTP::Data>& httpData, const size_t counter, const size_t expectedCounter,
			  const bool isRequest, const std::string& typeMessage, const std::string& url, const std::string& httpType,
			  const std::string& version, const size_t messageSize, const std::string& format, const std::string& body,
			  const std::string& code, const std::string& codeText,
			  const std::map<std::string, std::string>& headersMap, const std::string& toString) {
			  test.Assert(counter, expectedCounter, "Actions number is correct");
			  test.Assert(httpData.has_value(), true, "Has HTTP data");
			  if (!httpData.has_value()) {
				  return;
			  }
			  test.Assert(httpData->IsValid(), true, "HTTP data is valid");
			  test.Assert(httpData->IsRequest(), isRequest, "HTTP data is request");
			  test.Assert(httpData->GetTypeMessage(), typeMessage, "Message type is correct");
			  test.Assert(httpData->GetUrl(), url, "URL is correct");
			  test.Assert(httpData->GetHTTPType(), httpType, "HTTP type is correct");
			  test.Assert(httpData->GetVersion(), version, "HTTP version is correct");
			  test.Assert(httpData->GetMessageSize(), messageSize, "HTTP message size is correct");
			  test.Assert(httpData->GetFormat(), format, "HTTP format is correct");
			  test.Assert(httpData->GetBody(), body, "HTTP body is correct");
			  test.Assert(httpData->GetCode(), code, "HTTP code is correct");
			  test.Assert(httpData->GetCodeText(), codeText, "HTTP code text is correct");
			  test.Assert(httpData->GetSizeHeadersMap(), headersMap.size(), "HTTP headers map size is correct");
			  for (const auto& [key, value] : headersMap) {
				  const auto* header{ httpData->GetValue(key) };
				  test.Assert(header != nullptr, true, "HTTP header \"" + key + "\" is not empty");
				  if (header != nullptr) {
					  test.Assert(*header, value, "HTTP header \"" + key + "\" is correct");
				  }
			  }
			  test.Assert(httpData->ToString(), toString, "HTTP data to string is correct");
		  };

	//* 1) Send request to load index page with relative url
	client->SendRequest(serverId, "GET /index.html HTTP/1.1");
	server->WaitActionsNumber(test, 3000, 2);
	const auto& serverCounter{ server->GetActionsNumber() };
	auto& serverHTTPData{ server->GetHTTPData() };
	checkAll(serverHTTPData, serverCounter, 2, true, "GET", "/index.html", "HTTP", "1.1", 28, "html", "", "", "", {},
		"HTTP message:\n{\n\tis valid     : true\n\ttype         : Request\n\tmessage type : GET\n\turl          : "
		"/index.html\n\tHTTP type    : HTTP\n\tversion      : 1.1\n\tformat       : html\n\tcode         : \n\tcode "
		"text    : \n\tmessage size : 28\n\tHeaders      :\n{\n}\n}");

	//* 2) Reserved response with 200 and index page
	client->WaitActionsNumber(test, 3000, 2);
	const auto& clientCounter{ client->GetActionsNumber() };
	const auto& clientHTTPData{ client->GetHTTPData() };
	checkAll(clientHTTPData, clientCounter, 2, false, "", "", "HTTP", "1.1", 5159, "", indexPage, "200", "OK",
		{ { "Content-Type", "text/html; charset=utf-8" }, { "Content-Length", "5025" }, { "Connection", "keep-alive" },
			{ "Keep-Alive", "timeout=0,max=0" } },
		"HTTP message:\n{\n\tis valid       : true\n\ttype           : Response\n\tmessage type   : \n\turl            "
		": \n\tHTTP type      : HTTP\n\tversion        : 1.1\n\tformat         : \n\tcode           : 200\n\tcode text "
		"     : OK\n\tmessage size   : 5159\n\tHeaders        :\n{\n\tConnection     : keep-alive\n\tContent-Length : "
		"5025\n\tContent-Type   : text/html; charset=utf-8\n\tKeep-Alive     : timeout=0,max=0\n}\n}");

	//* 3) Send request to load index page with relative url withput page format specification
	client->SendRequest(serverId, "GET /index HTTP/1.1");
	server->WaitActionsNumber(test, 3000, 4);
	checkAll(serverHTTPData, serverCounter, 4, true, "GET", "/index", "HTTP", "1.1", 23, "html", "", "", "", {},
		"HTTP message:\n{\n\tis valid     : true\n\ttype         : Request\n\tmessage type : GET\n\turl          : "
		"/index\n\tHTTP type    : HTTP\n\tversion      : 1.1\n\tformat       : html\n\tcode         : \n\tcode text    "
		": \n\tmessage size : 23\n\tHeaders      :\n{\n}\n}");

	//* 4) Reserved response with 200 and index page
	client->WaitActionsNumber(test, 3000, 4);
	checkAll(clientHTTPData, clientCounter, 4, false, "", "", "HTTP", "1.1", 5159, "", indexPage, "200", "OK",
		{ { "Content-Type", "text/html; charset=utf-8" }, { "Content-Length", "5025" }, { "Connection", "keep-alive" },
			{ "Keep-Alive", "timeout=0,max=0" } },
		"HTTP message:\n{\n\tis valid       : true\n\ttype           : Response\n\tmessage type   : \n\turl            "
		": \n\tHTTP type      : HTTP\n\tversion        : 1.1\n\tformat         : \n\tcode           : 200\n\tcode text "
		"     : OK\n\tmessage size   : 5159\n\tHeaders        :\n{\n\tConnection     : keep-alive\n\tContent-Length : "
		"5025\n\tContent-Type   : text/html; charset=utf-8\n\tKeep-Alive     : timeout=0,max=0\n}\n}");

	//* 5) Send request to load unkown page
	client->SendRequest(serverId, "GET /info HTTP/1.1");
	server->WaitActionsNumber(test, 3000, 6);
	checkAll(serverHTTPData, serverCounter, 6, true, "GET", "/info", "HTTP", "1.1", 22, "html", "", "", "", {},
		"HTTP message:\n{\n\tis valid     : true\n\ttype         : Request\n\tmessage type : GET\n\turl          : "
		"/info\n\tHTTP type    : HTTP\n\tversion      : 1.1\n\tformat       : html\n\tcode         : \n\tcode text    "
		": \n\tmessage size : 22\n\tHeaders      :\n{\n}\n}");

	//* 6) Reserved response with 404 and JSON
	client->WaitActionsNumber(test, 3000, 6);
	checkAll(clientHTTPData, clientCounter, 6, false, "", "", "HTTP", "1.1", 180, "",
		"{\"Error\":\"Page \"/info\" not found\"}", "404", "Not Found",
		{ { "Content-Type", "application/json; charset=utf-8" }, { "Content-Length", "34" },
			{ "Connection", "keep-alive" }, { "Keep-Alive", "timeout=0,max=0" } },
		"HTTP message:\n{\n\tis valid       : true\n\ttype           : Response\n\tmessage type   : \n\turl            "
		": \n\tHTTP type      : HTTP\n\tversion        : 1.1\n\tformat         : \n\tcode           : 404\n\tcode text "
		"     : Not Found\n\tmessage size   : 180\n\tHeaders        :\n{\n\tConnection     : "
		"keep-alive\n\tContent-Length : 34\n\tContent-Type   : application/json; charset=utf-8\n\tKeep-Alive     : "
		"timeout=0,max=0\n}\n}");

	//* 7) Send request to unkown file with specific header
	client->SendRequest(serverId, "GET /archive.zip HTTP/1.1\r\nSome header: Hello for everybody 777!");
	server->WaitActionsNumber(test, 3000, 8);
	checkAll(serverHTTPData, serverCounter, 8, true, "GET", "/archive.zip", "HTTP", "1.1", 68, "zip", "", "", "",
		{ { "Some header", "Hello for everybody 777!" } },
		"HTTP message:\n{\n\tis valid     : true\n\ttype         : Request\n\tmessage type : GET\n\turl          : "
		"/archive.zip\n\tHTTP type    : HTTP\n\tversion      : 1.1\n\tformat       : zip\n\tcode         : \n\tcode "
		"text    : \n\tmessage size : 68\n\tHeaders      :\n{\n\tSome header  : Hello for everybody 777!\n}\n}");

	//* 8) Reserved response with 404 and JSON
	client->WaitActionsNumber(test, 3000, 8);
	checkAll(clientHTTPData, clientCounter, 8, false, "", "", "HTTP", "1.1", 189, "",
		"{\"Error\":\"Source \"/archive.zip\" not found\"}", "404", "Not Found",
		{ { "Content-Type", "application/json; charset=utf-8" }, { "Content-Length", "43" },
			{ "Connection", "keep-alive" }, { "Keep-Alive", "timeout=0,max=0" } },
		"HTTP message:\n{\n\tis valid       : true\n\ttype           : Response\n\tmessage type   : \n\turl            "
		": \n\tHTTP type      : HTTP\n\tversion        : 1.1\n\tformat         : \n\tcode           : 404\n\tcode text "
		"     : Not Found\n\tmessage size   : 189\n\tHeaders        :\n{\n\tConnection     : "
		"keep-alive\n\tContent-Length : 43\n\tContent-Type   : application/json; charset=utf-8\n\tKeep-Alive     : "
		"timeout=0,max=0\n}\n}");

	//* 9) Send request to load index page by slash
	client->SendRequest(serverId, "GET / HTTP/1.1");
	server->WaitActionsNumber(test, 3000, 10);
	checkAll(serverHTTPData, serverCounter, 10, true, "GET", "/", "HTTP", "1.1", 18, "html", "", "", "", {},
		"HTTP message:\n{\n\tis valid     : true\n\ttype         : Request\n\tmessage type : GET\n\turl          : "
		"/\n\tHTTP type    : HTTP\n\tversion      : 1.1\n\tformat       : html\n\tcode         : \n\tcode text    : "
		"\n\tmessage size : 18\n\tHeaders      :\n{\n}\n}");

	//* 10) Reserved response with 200 and index page
	client->WaitActionsNumber(test, 3000, 10);
	checkAll(clientHTTPData, clientCounter, 10, false, "", "", "HTTP", "1.1", 5159, "", indexPage, "200", "OK",
		{ { "Content-Type", "text/html; charset=utf-8" }, { "Content-Length", "5025" }, { "Connection", "keep-alive" },
			{ "Keep-Alive", "timeout=0,max=0" } },
		"HTTP message:\n{\n\tis valid       : true\n\ttype           : Response\n\tmessage type   : \n\turl            "
		": \n\tHTTP type      : HTTP\n\tversion        : 1.1\n\tformat         : \n\tcode           : 200\n\tcode text "
		"     : OK\n\tmessage size   : 5159\n\tHeaders        :\n{\n\tConnection     : keep-alive\n\tContent-Length : "
		"5025\n\tContent-Type   : text/html; charset=utf-8\n\tKeep-Alive     : timeout=0,max=0\n}\n}");

	//* 11) Send request to load unknown page
	client->SendRequest(serverId, "GET /unknown.html HTTP/1.1");
	server->WaitActionsNumber(test, 3000, 12);
	checkAll(serverHTTPData, serverCounter, 12, true, "GET", "/unknown.html", "HTTP", "1.1", 30, "html", "", "", "", {},
		"HTTP message:\n{\n\tis valid     : true\n\ttype         : Request\n\tmessage type : GET\n\turl          : "
		"/unknown.html\n\tHTTP type    : HTTP\n\tversion      : 1.1\n\tformat       : html\n\tcode         : \n\tcode "
		"text    : \n\tmessage size : 30\n\tHeaders      :\n{\n}\n}");

	//* 12) Reserved response with 404
	client->WaitActionsNumber(test, 3000, 12);
	checkAll(clientHTTPData, clientCounter, 12, false, "", "", "HTTP", "1.1", 119, "", "", "404", "Not Found",
		{ { "Content-Type", "text/html; charset=utf-8" }, { "Connection", "keep-alive" },
			{ "Keep-Alive", "timeout=0,max=0" } },
		"HTTP message:\n{\n\tis valid     : true\n\ttype         : Response\n\tmessage type : \n\turl          : "
		"\n\tHTTP type    : HTTP\n\tversion      : 1.1\n\tformat       : \n\tcode         : 404\n\tcode text    : Not "
		"Found\n\tmessage size : 119\n\tHeaders      :\n{\n\tConnection   : keep-alive\n\tContent-Type : text/html; "
		"charset=utf-8\n\tKeep-Alive   : timeout=0,max=0\n}\n}");

	//* 13) Send request to load favicon
	client->SendRequest(serverId, "GET /favicon.ico HTTP/1.1");
	server->WaitActionsNumber(test, 3000, 14);
	checkAll(serverHTTPData, serverCounter, 14, true, "GET", "/favicon.ico", "HTTP", "1.1", 29, "ico", "", "", "", {},
		"HTTP message:\n{\n\tis valid     : true\n\ttype         : Request\n\tmessage type : GET\n\turl          : "
		"/favicon.ico\n\tHTTP type    : HTTP\n\tversion      : 1.1\n\tformat       : ico\n\tcode         : \n\tcode "
		"text    : \n\tmessage size : 29\n\tHeaders      :\n{\n}\n}");

	//* 14) Reserved response with 200 and favicon
	client->WaitActionsNumber(test, 3000, 14);
	checkAll(clientHTTPData, clientCounter, 14, false, "", "", "HTTP", "1.1", 15544, "", favicon, "200", "OK",
		{ { "Content-Type", "image/x-icon; charset=utf-8" }, { "Content-Length", "15406" },
			{ "Connection", "keep-alive" }, { "Keep-Alive", "timeout=0,max=0" } },
		"HTTP message:\n{\n\tis valid       : true\n\ttype           : Response\n\tmessage type   : \n\turl            "
		": \n\tHTTP type      : HTTP\n\tversion        : 1.1\n\tformat         : \n\tcode           : 200\n\tcode text "
		"     : OK\n\tmessage size   : 15544\n\tHeaders        :\n{\n\tConnection     : keep-alive\n\tContent-Length : "
		"15406\n\tContent-Type   : image/x-icon; charset=utf-8\n\tKeep-Alive     : timeout=0,max=0\n}\n}");

	//* 15) Send request to load css
	client->SendRequest(serverId, "GET /style.css HTTP/1.1");
	server->WaitActionsNumber(test, 3000, 16);
	checkAll(serverHTTPData, serverCounter, 16, true, "GET", "/style.css", "HTTP", "1.1", 27, "css", "", "", "", {},
		"HTTP message:\n{\n\tis valid     : true\n\ttype         : Request\n\tmessage type : GET\n\turl          : "
		"/style.css\n\tHTTP type    : HTTP\n\tversion      : 1.1\n\tformat       : css\n\tcode         : \n\tcode text "
		"   : \n\tmessage size : 27\n\tHeaders      :\n{\n}\n}");

	//* 16) Reserved response with 200 and css
	client->WaitActionsNumber(test, 3000, 16);
	checkAll(clientHTTPData, clientCounter, 16, false, "", "", "HTTP", "1.1", 4366, "", css, "200", "OK",
		{ { "Content-Type", "text/css; charset=utf-8" }, { "Content-Length", "4233" }, { "Connection", "keep-alive" },
			{ "Keep-Alive", "timeout=0,max=0" } },
		"HTTP message:\n{\n\tis valid       : true\n\ttype           : Response\n\tmessage type   : \n\turl            "
		": \n\tHTTP type      : HTTP\n\tversion        : 1.1\n\tformat         : \n\tcode           : 200\n\tcode text "
		"     : OK\n\tmessage size   : 4366\n\tHeaders        :\n{\n\tConnection     : keep-alive\n\tContent-Length : "
		"4233\n\tContent-Type   : text/css; charset=utf-8\n\tKeep-Alive     : timeout=0,max=0\n}\n}");

	//* 17) Send request to load js
	client->SendRequest(serverId, "GET /index.js HTTP/1.1");
	server->WaitActionsNumber(test, 3000, 18);
	checkAll(serverHTTPData, serverCounter, 18, true, "GET", "/index.js", "HTTP", "1.1", 26, "js", "", "", "", {},
		"HTTP message:\n{\n\tis valid     : true\n\ttype         : Request\n\tmessage type : GET\n\turl          : "
		"/index.js\n\tHTTP type    : HTTP\n\tversion      : 1.1\n\tformat       : js\n\tcode         : \n\tcode text   "
		" : \n\tmessage size : 26\n\tHeaders      :\n{\n}\n}");

	//* 18) Reserved response with 200 and js
	client->WaitActionsNumber(test, 3000, 18);
	checkAll(clientHTTPData, clientCounter, 18, false, "", "", "HTTP", "1.1", 5261, "", js, "200", "OK",
		{ { "Content-Type", "application/javascript; charset=utf-8" }, { "Content-Length", "5114" },
			{ "Connection", "keep-alive" }, { "Keep-Alive", "timeout=0,max=0" } },
		"HTTP message:\n{\n\tis valid       : true\n\ttype           : Response\n\tmessage type   : \n\turl            "
		": \n\tHTTP type      : HTTP\n\tversion        : 1.1\n\tformat         : \n\tcode           : 200\n\tcode text "
		"     : OK\n\tmessage size   : 5261\n\tHeaders        :\n{\n\tConnection     : keep-alive\n\tContent-Length : "
		"5114\n\tContent-Type   : application/javascript; charset=utf-8\n\tKeep-Alive     : timeout=0,max=0\n}\n}");

	//* 19) Send request with specific header
	client->SendRequest(serverId, "GET /api HTTP/1.1\r\nIdentifier: 369\nAction: Send me some JSON, please");
	server->WaitActionsNumber(test, 3000, 20);
	checkAll(serverHTTPData, serverCounter, 20, true, "GET", "/api", "HTTP", "1.1", 72, "html", "", "", "",
		{ { "Identifier", "369" }, { "Action", "Send me some JSON, please" } },
		"HTTP message:\n{\n\tis valid     : true\n\ttype         : Request\n\tmessage type : GET\n\turl          : "
		"/api\n\tHTTP type    : HTTP\n\tversion      : 1.1\n\tformat       : html\n\tcode         : \n\tcode text    : "
		"\n\tmessage size : 72\n\tHeaders      :\n{\n\tAction       : Send me some JSON, please\n\tIdentifier   : "
		"369\n}\n}");

	//* 20) Reserved response with 200 and JSON
	client->WaitActionsNumber(test, 3000, 20);
	checkAll(clientHTTPData, clientCounter, 20, false, "", "", "HTTP", "1.1", 170, "",
		"{\"Message\":\"Here is your JSON\"}", "200", "OK",
		{ { "Content-Type", "application/json; charset=utf-8" }, { "Content-Length", "31" },
			{ "Connection", "keep-alive" }, { "Keep-Alive", "timeout=0,max=0" } },
		"HTTP message:\n{\n\tis valid       : true\n\ttype           : Response\n\tmessage type   : \n\turl            "
		": \n\tHTTP type      : HTTP\n\tversion        : 1.1\n\tformat         : \n\tcode           : 200\n\tcode text "
		"     : OK\n\tmessage size   : 170\n\tHeaders        :\n{\n\tConnection     : keep-alive\n\tContent-Length : "
		"31\n\tContent-Type   : application/json; charset=utf-8\n\tKeep-Alive     : timeout=0,max=0\n}\n}");

	//* 21) Send request with wrong specific header
	client->SendRequest(serverId, "GET /api HTTP/1.1\r\nIdentifier: 368\nAction: Send me some JSON, please");
	server->WaitActionsNumber(test, 3000, 22);
	checkAll(serverHTTPData, serverCounter, 22, true, "GET", "/api", "HTTP", "1.1", 72, "html", "", "", "",
		{ { "Identifier", "368" }, { "Action", "Send me some JSON, please" } },
		"HTTP message:\n{\n\tis valid     : true\n\ttype         : Request\n\tmessage type : GET\n\turl          : "
		"/api\n\tHTTP type    : HTTP\n\tversion      : 1.1\n\tformat       : html\n\tcode         : \n\tcode text    : "
		"\n\tmessage size : 72\n\tHeaders      :\n{\n\tAction       : Send me some JSON, please\n\tIdentifier   : "
		"368\n}\n}");

	//* 22) Reserved response with 404 and JSON
	client->WaitActionsNumber(test, 3000, 22);
	checkAll(clientHTTPData, clientCounter, 22, false, "", "", "HTTP", "1.1", 181, "",
		"{\"Error\":\"Identifier is not valid\"}", "404", "Not Found",
		{ { "Content-Type", "application/json; charset=utf-8" }, { "Content-Length", "35" },
			{ "Connection", "keep-alive" }, { "Keep-Alive", "timeout=0,max=0" } },
		"HTTP message:\n{\n\tis valid       : true\n\ttype           : Response\n\tmessage type   : \n\turl            "
		": \n\tHTTP type      : HTTP\n\tversion        : 1.1\n\tformat         : \n\tcode           : 404\n\tcode text "
		"     : Not Found\n\tmessage size   : 181\n\tHeaders        :\n{\n\tConnection     : "
		"keep-alive\n\tContent-Length : 35\n\tContent-Type   : application/json; charset=utf-8\n\tKeep-Alive     : "
		"timeout=0,max=0\n}\n}");

	//* 23) Send request with wrong HTTP type
	client->SendRequest(serverId, "POST /api HTTP/1.1\r\nIdentifier: 369\nAction: Send me some JSON, please");
	server->WaitActionsNumber(test, 3000, 24);
	checkAll(serverHTTPData, serverCounter, 24, true, "POST", "/api", "HTTP", "1.1", 73, "html", "", "", "",
		{ { "Identifier", "369" }, { "Action", "Send me some JSON, please" } },
		"HTTP message:\n{\n\tis valid     : true\n\ttype         : Request\n\tmessage type : POST\n\turl          : "
		"/api\n\tHTTP type    : HTTP\n\tversion      : 1.1\n\tformat       : html\n\tcode         : \n\tcode text    : "
		"\n\tmessage size : 73\n\tHeaders      :\n{\n\tAction       : Send me some JSON, please\n\tIdentifier   : "
		"369\n}\n}");

	//* 24) Reserved response with 404 and JSON
	client->WaitActionsNumber(test, 3000, 24);
	checkAll(clientHTTPData, clientCounter, 24, false, "", "", "HTTP", "1.1", 183, "",
		"{\"Error\":\"Method \"POST\" not allowed\"}", "404", "Not Found",
		{ { "Content-Type", "application/json; charset=utf-8" }, { "Content-Length", "37" },
			{ "Connection", "keep-alive" }, { "Keep-Alive", "timeout=0,max=0" } },
		"HTTP message:\n{\n\tis valid       : true\n\ttype           : Response\n\tmessage type   : \n\turl            "
		": \n\tHTTP type      : HTTP\n\tversion        : 1.1\n\tformat         : \n\tcode           : 404\n\tcode text "
		"     : Not Found\n\tmessage size   : 183\n\tHeaders        :\n{\n\tConnection     : "
		"keep-alive\n\tContent-Length : 37\n\tContent-Type   : application/json; charset=utf-8\n\tKeep-Alive     : "
		"timeout=0,max=0\n}\n}");

	//* 25) Send request to load index page with symbol "?" in URL and following data
	client->SendRequest(serverId, "GET /index?parameter=83648&additionalData=GTP HTTP/1.1");
	server->WaitActionsNumber(test, 3000, 26);
	checkAll(serverHTTPData, serverCounter, 26, true, "GET", "/index", "HTTP", "1.1", 58, "html", "", "", "", {},
		"HTTP message:\n{\n\tis valid     : true\n\ttype         : Request\n\tmessage type : GET\n\turl          : "
		"/index\n\tHTTP type    : HTTP\n\tversion      : 1.1\n\tformat       : html\n\tcode         : \n\tcode text    "
		": \n\tmessage size : 58\n\tHeaders      :\n{\n}\n}");

	//* 26) Reserved response with 200 and index page, symbol "?" and following data is ignored
	client->WaitActionsNumber(test, 3000, 26);
	checkAll(clientHTTPData, clientCounter, 26, false, "", "", "HTTP", "1.1", 5159, "", indexPage, "200", "OK",
		{ { "Content-Type", "text/html; charset=utf-8" }, { "Content-Length", "5025" }, { "Connection", "keep-alive" },
			{ "Keep-Alive", "timeout=0,max=0" } },
		"HTTP message:\n{\n\tis valid       : true\n\ttype           : Response\n\tmessage type   : \n\turl            "
		": \n\tHTTP type      : HTTP\n\tversion        : 1.1\n\tformat         : \n\tcode           : 200\n\tcode text "
		"     : OK\n\tmessage size   : 5159\n\tHeaders        :\n{\n\tConnection     : keep-alive\n\tContent-Length : "
		"5025\n\tContent-Type   : text/html; charset=utf-8\n\tKeep-Alive     : timeout=0,max=0\n}\n}");

	//* 27) Send request to load index page with symbol "#" in URL and following data
	client->SendRequest(serverId, "GET /index.html#section HTTP/1.1");
	server->WaitActionsNumber(test, 3000, 28);
	checkAll(serverHTTPData, serverCounter, 28, true, "GET", "/index.html", "HTTP", "1.1", 36, "html", "", "", "", {},
		"HTTP message:\n{\n\tis valid     : true\n\ttype         : Request\n\tmessage type : GET\n\turl          : "
		"/index.html\n\tHTTP type    : HTTP\n\tversion      : 1.1\n\tformat       : html\n\tcode         : \n\tcode "
		"text    : \n\tmessage size : 36\n\tHeaders      :\n{\n}\n}");

	//* 28) Reserved response with 200 and index page, symbol "#" and following data is ignored
	client->WaitActionsNumber(test, 3000, 28);
	checkAll(clientHTTPData, clientCounter, 28, false, "", "", "HTTP", "1.1", 5159, "", indexPage, "200", "OK",
		{ { "Content-Type", "text/html; charset=utf-8" }, { "Content-Length", "5025" }, { "Connection", "keep-alive" },
			{ "Keep-Alive", "timeout=0,max=0" } },
		"HTTP message:\n{\n\tis valid       : true\n\ttype           : Response\n\tmessage type   : \n\turl            "
		": \n\tHTTP type      : HTTP\n\tversion        : 1.1\n\tformat         : \n\tcode           : 200\n\tcode text "
		"     : OK\n\tmessage size   : 5159\n\tHeaders        :\n{\n\tConnection     : keep-alive\n\tContent-Length : "
		"5025\n\tContent-Type   : text/html; charset=utf-8\n\tKeep-Alive     : timeout=0,max=0\n}\n}");

	serverPtr.reset();
	clientPtr.reset();

	return test.Passed<int32_t>();
}