/**************************
 * @file        http.inl
 * @date        2026-10-10
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

#ifndef MSAPI_UNIT_TEST_HTTP_INL
#define MSAPI_UNIT_TEST_HTTP_INL

#include "../../../../library/source/help/io.inl"
#include "../../../../library/source/protocol/http.h"
#include "../../../../library/source/test/test.inl"
#include <unistd.h>

namespace MSAPI {

namespace Test {

namespace Unit {

/*---------------------------------------------------------------------------------
Declarations
---------------------------------------------------------------------------------*/

/**************************
 * @brief Exercises HTTP Data through its normal RecvBuffer entry point using a socket pair.
 *
 * @concurrency No.
 */
class Request {
private:
	int32_t m_peer{ -1 };
	std::shared_ptr<MSAPI::Connection::Data> m_connection;
	std::optional<MSAPI::Protocol::HTTP::Data> m_data;

public:
	FORCE_INLINE Request() = default;
	FORCE_INLINE ~Request() noexcept;

	Request(const Request&) = delete;
	Request(Request&&) = delete;
	Request& operator=(const Request&) = delete;
	Request& operator=(Request&&) = delete;

	/**************************
	 * @brief Sends request bytes through a socket pair and constructs the parsed HTTP data.
	 *
	 * @param text Request bytes.
	 *
	 * @return True if the fixture is prepared; inspect Get().IsValid() for parsing validity.
	 */
	FORCE_INLINE [[nodiscard]] bool Parse(std::string_view text);

	/**************************
	 * @return Parsed HTTP data.
	 */
	FORCE_INLINE [[nodiscard]] const MSAPI::Protocol::HTTP::Data& Get() const;

	/**************************
	 * @return Connection used for parsing and responses.
	 */
	FORCE_INLINE [[nodiscard]] MSAPI::Connection& GetConnection() const;

	/**************************
	 * @return Peer socket descriptor.
	 */
	FORCE_INLINE [[nodiscard]] int32_t GetPeer() const noexcept;
};

/**************************
 * @brief Provides a concrete HTTP handler for source-path registration tests.
 *
 * @concurrency Yes. Inherits internal path synchronization from IHandler; the callback does not access state.
 */
class Handler final : public MSAPI::Protocol::HTTP::IHandler {
public:
	FORCE_INLINE Handler();

	Handler(const Handler&) = delete;
	Handler(Handler&&) = delete;
	Handler& operator=(const Handler&) = delete;
	Handler& operator=(Handler&&) = delete;

	// MSAPI::Protocol::HTTP::IHandler
	FORCE_INLINE void HandleHttp(
		const std::shared_ptr<MSAPI::Connection::Data>&, const MSAPI::Protocol::HTTP::Data&) final;
};

/**************************
 * @brief Owns executable-relative testData paths to test registration order, file existence and directory exclusion.
 *
 * @concurrency No.
 */
class Files {
private:
	std::string m_path;
	bool m_created{};
	bool m_isValid{};

public:
	FORCE_INLINE Files();
	FORCE_INLINE ~Files() noexcept;

	Files(const Files&) = delete;
	Files(Files&&) = delete;
	Files& operator=(const Files&) = delete;
	Files& operator=(Files&&) = delete;

	/**************************
	 * @return Executable-relative test data directory path.
	 */
	FORCE_INLINE [[nodiscard]] const std::string& GetPath() const noexcept;

	/**************************
	 * @return True if all fixture directories and files are created.
	 */
	FORCE_INLINE [[nodiscard]] bool IsValid() const noexcept;
};

/**************************
 * @brief Tests keyed source registration, priority and path validation.
 *
 * @param t Test assertions.
 *
 * @locking Is not required.
 *
 * @return True if all path tests pass.
 *
 * @test Yes.
 */
FORCE_INLINE [[nodiscard]] bool Paths(MSAPI::Test::Test& t);

/**************************
 * @brief Tests request parsing, host extraction and connection policies.
 *
 * @param t Test assertions.
 *
 * @return True if all parsing tests pass.
 *
 * @test Yes.
 */
FORCE_INLINE [[nodiscard]] bool Parsing(MSAPI::Test::Test& t);

/**************************
 * @brief Tests response framing, short writes, redirects and socket closure.
 *
 * @param t Test assertions.
 *
 * @return True if all response tests pass.
 *
 * @test Yes.
 */
FORCE_INLINE [[nodiscard]] bool Responses(MSAPI::Test::Test& t);

/**************************
 * @brief Runs HTTP unit tests.
 *
 * @return True if all tests pass.
 *
 * @test Yes.
 */
FORCE_INLINE [[nodiscard]] bool Http();

/*---------------------------------------------------------------------------------
Definitions
---------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------
Request
---------------------------------------------------------------------------------*/

FORCE_INLINE Request::~Request() noexcept
{
	if (m_peer != -1) {
		(void)close(m_peer);
	}
}

FORCE_INLINE [[nodiscard]] bool Request::Parse(const std::string_view text)
{
	std::array<int32_t, 2> sockets{};
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets.data()) != 0) {
		return false;
	}

	m_peer = sockets[1];
	MSAPI::SString<16> ip;
	if (!ip.Copy("127.0.0.1")) {
		(void)close(sockets[0]);
		return false;
	}

	m_connection = std::make_shared<MSAPI::Connection::Data>(
		std::make_unique<MSAPI::Connection>(sockets[0]), std::move(ip), 0, 0, MSAPI::Connection::Type::Income, false);
	if (send(m_peer, text.data(), text.size(), MSG_NOSIGNAL) != static_cast<int64_t>(text.size())) {
		return false;
	}

	MSAPI::RecvBuffer buffer{ m_connection, 8192, 1 };
	m_data.emplace(buffer);
	return true;
}

FORCE_INLINE [[nodiscard]] const MSAPI::Protocol::HTTP::Data& Request::Get() const { return *m_data; }

FORCE_INLINE [[nodiscard]] MSAPI::Connection& Request::GetConnection() const { return m_connection->GetConnection(); }

FORCE_INLINE [[nodiscard]] int32_t Request::GetPeer() const noexcept { return m_peer; }

/*---------------------------------------------------------------------------------
Handler
---------------------------------------------------------------------------------*/

FORCE_INLINE Handler::Handler()
	: MSAPI::Protocol::HTTP::IHandler(nullptr)
{
}

FORCE_INLINE void Handler::HandleHttp(
	const std::shared_ptr<MSAPI::Connection::Data>&, const MSAPI::Protocol::HTTP::Data&)
{
}

/*---------------------------------------------------------------------------------
Files
---------------------------------------------------------------------------------*/

FORCE_INLINE Files::Files()
{
	m_path.resize(512);
	MSAPI::Helper::GetExecutableDir(m_path);
	if (m_path.empty()) [[unlikely]] {
		LOG_ERROR_NEW("{}", "Cannot get executable path");
		return;
	}

	m_path += "testData";
	if (MSAPI::IO::HasPath(std::string_view{ m_path })) {
		LOG_ERROR_NEW("Test directory already exists, path: \"{}\", clean it before next test execution", m_path);
		return;
	}

	m_created = MSAPI::IO::CreateDir(std::string_view{ m_path });
	if (!m_created) {
		LOG_ERROR_NEW("Cannot create test directory, path: \"{}\"", m_path);
		return;
	}

	m_isValid = MSAPI::IO::CreateDir(std::string_view{ m_path + "/first" })
		&& MSAPI::IO::CreateDir(std::string_view{ m_path + "/second" })
		&& MSAPI::IO::SaveStr("first", std::string_view{ m_path + "/first/shared.txt" })
		&& MSAPI::IO::SaveStr("second", std::string_view{ m_path + "/second/shared.txt" })
		&& MSAPI::IO::SaveStr("fallback", std::string_view{ m_path + "/second/fallback.txt" });
	if (!m_isValid) {
		LOG_ERROR_NEW("Cannot create HTTP test fixtures, path: \"{}\"", m_path);
	}
}

FORCE_INLINE Files::~Files() noexcept
{
	if (m_created && !MSAPI::IO::Remove(m_path)) {
		LOG_ERROR_NEW("Cannot remove test directory, path: \"{}\", clean it before next test execution", m_path);
	}
}

FORCE_INLINE [[nodiscard]] const std::string& Files::GetPath() const noexcept { return m_path; }

FORCE_INLINE [[nodiscard]] bool Files::IsValid() const noexcept { return m_isValid; }

/*---------------------------------------------------------------------------------
Free
---------------------------------------------------------------------------------*/

FORCE_INLINE [[nodiscard]] bool Paths(MSAPI::Test::Test& t)
{
	Files files;
	RETURN_IF_FALSE(t.Assert(files.IsValid(), true, "Executable-relative test files created"));

	Handler handler;
	RETURN_IF_FALSE(
		t.Assert(handler.AddSourcePath("text", files.GetPath() + "/first"), true, "First path accepts missing slash"));
	RETURN_IF_FALSE(
		t.Assert(handler.AddSourcePath("text", files.GetPath() + "/second/"), true, "Second path accepts slash"));

	MSAPI::SString<512> out;
	RETURN_IF_FALSE(t.Assert(handler.SourceLookup("text", "/shared.txt", out), true, "File exists in both paths"));
	RETURN_IF_FALSE(t.Assert(out.Get() == files.GetPath() + "/first/shared.txt", true, "First path wins"));
	RETURN_IF_FALSE(
		t.Assert(handler.SourceLookup("text", "/fallback.txt", out), true, "Later path supplies missing file"));
	RETURN_IF_FALSE(
		t.Assert(out.Get() == files.GetPath() + "/second/fallback.txt", true, "Full fallback path returned"));

	RETURN_IF_FALSE(t.Assert(handler.SourceLookup("missing", "/shared.txt", out), false, "Unknown key fails"));
	RETURN_IF_FALSE(t.Assert(handler.SourceLookup("text", "/missing", out), false, "Missing file fails"));
	RETURN_IF_FALSE(t.Assert(handler.SourceLookup("text", "/", out), false, "Directory fails"));
	for (const std::string_view filename : { "/../shared.txt", "/a..b", "/a\\b", "shared.txt", "", "/.." }) {
		RETURN_IF_FALSE(t.Assert(handler.SourceLookup("text", filename, out), false, "Unsafe filename fails"));
	}
	RETURN_IF_FALSE(t.Assert(
		handler.SourceLookup("text", std::string_view{ "/shared.txt\0suffix", 18 }, out), false, "Embedded NUL fails"));

	Handler priority;
	std::string priorityPath{ files.GetPath() + "/second/" };
	RETURN_IF_FALSE(t.Assert(priorityPath.size() <= 246, true, "Priority path fits boundary fixture"));
	// Repeated separators preserve the file location while making its full path exactly 256 bytes.
	priorityPath.resize(246, '/');
	RETURN_IF_FALSE(t.Assert(priority.AddSourcePath("text", priorityPath), true, "Longer priority path added"));
	RETURN_IF_FALSE(
		t.Assert(priority.AddSourcePath("text", files.GetPath() + "/first"), true, "Shorter fallback path added"));
	MSAPI::SString<256> boundary;
	RETURN_IF_FALSE(t.Assert(priority.SourceLookup("text", "/shared.txt", boundary), false,
		"Oversized first existing file does not change path priority"));
	RETURN_IF_FALSE(t.Assert(boundary.Empty(), true, "Oversized priority path leaves output empty"));
	MSAPI::SString<257> exact;
	RETURN_IF_FALSE(t.Assert(
		priority.SourceLookup("text", "/shared.txt", exact), true, "Priority path fits with terminator space"));
	RETURN_IF_FALSE(t.Assert(exact.Get() == priorityPath + "shared.txt", true, "Boundary path preserves first path"));
	RETURN_IF_FALSE(t.Assert(exact.GetSize(), uint64_t{ 256 }, "Boundary path has exactly 256 bytes"));
	RETURN_IF_FALSE(t.Assert(exact.GetBuffer()[256] == '\0', true, "Boundary path is null terminated"));

	Handler missingPriority;
	std::string missingPath{ files.GetPath() + "/first/" };
	missingPath.resize(246, '/');
	RETURN_IF_FALSE(
		t.Assert(missingPriority.AddSourcePath("text", missingPath), true, "Longer missing-file path added"));
	RETURN_IF_FALSE(t.Assert(
		missingPriority.AddSourcePath("text", files.GetPath() + "/second"), true, "Existing fallback path added"));
	RETURN_IF_FALSE(t.Assert(missingPriority.SourceLookup("text", "/fallback.txt", boundary), true,
		"Oversized missing path allows fallback"));
	RETURN_IF_FALSE(t.Assert(boundary.Get() == files.GetPath() + "/second/fallback.txt", true,
		"Missing priority file returns fallback path"));

	MSAPI::SString<8> tiny;
	RETURN_IF_FALSE(t.Assert(handler.SourceLookup("text", "/shared.txt", tiny), false, "Long full path fails"));
	RETURN_IF_FALSE(t.Assert(tiny.Empty(), true, "No truncated path returned"));
	RETURN_IF_FALSE(
		t.Assert(handler.SourceLookup("text", "/" + std::string(600, 'x'), out), false, "Long filename fails"));
	RETURN_IF_FALSE(t.Assert(handler.AddSourcePath(std::string(33, 'x'), "/tmp"), false, "Oversized key fails"));
	RETURN_IF_FALSE(t.Assert(handler.AddSourcePath("text", std::string(256, 'x')), false, "Trailing slash must fit"));
	RETURN_IF_FALSE(t.Assert(handler.AddSourcePath("text", std::string(257, '/')), false, "Oversized path fails"));
	RETURN_IF_FALSE(t.Assert(handler.AddSourcePath("text", std::string_view{ "/tmp\0x", 6 }), false, "NUL path fails"));
	RETURN_IF_FALSE(t.Assert(handler.AddSourcePath(std::string_view{ "a\0b", 3 }, "/tmp"), false, "NUL key fails"));
	RETURN_IF_FALSE(t.Assert(handler.AddSourcePath("", files.GetPath()), false, "Empty key fails"));
	RETURN_IF_FALSE(t.Assert(handler.AddSourcePath("text", ""), false, "Empty path fails"));
	RETURN_IF_FALSE(
		t.Assert(handler.AddSourcePath("text", "/"), false, "Path that becomes empty after removing slash fails"));
	RETURN_IF_FALSE(
		t.Assert(handler.AddSourcePath("absent", files.GetPath() + "/absent"), false, "Missing path fails"));
	RETURN_IF_FALSE(t.Assert(handler.AddSourcePath("file", files.GetPath() + "/first/shared.txt"), false,
		"Existing file fails after trailing-slash normalization"));
	const std::string pathWithSuffix{ files.GetPath() + "/first/absent" };
	RETURN_IF_FALSE(
		t.Assert(handler.AddSourcePath("view", std::string_view{ pathWithSuffix }.substr(0, pathWithSuffix.size() - 7)),
			true, "Path view does not require a trailing NUL"));
	RETURN_IF_FALSE(t.Assert(handler.SourceLookup("view", "/shared.txt", out), true, "Path view supports file lookup"));
	RETURN_IF_FALSE(t.Assert(
		out.Get() == files.GetPath() + "/first/shared.txt", true, "Stored path joins the filename with one slash"));

	// Additional registrations preserve existing paths and their priority.
	for (uint64_t index{}; index < 32; ++index) {
		RETURN_IF_FALSE(t.Assert(handler.AddSourcePath("text", "/tmp"), true, "Additional path accepted"));
	}
	RETURN_IF_FALSE(
		t.Assert(handler.AddSourcePath("late", files.GetPath() + "/second"), true, "Path beyond former cap added"));
	RETURN_IF_FALSE(t.Assert(handler.SourceLookup("text", "/shared.txt", out), true, "Existing paths survive growth"));
	RETURN_IF_FALSE(
		t.Assert(out.Get() == files.GetPath() + "/first/shared.txt", true, "Growth preserves path priority"));
	RETURN_IF_FALSE(t.Assert(handler.SourceLookup("late", "/fallback.txt", out), true, "Path beyond former cap found"));
	RETURN_IF_FALSE(
		t.Assert(out.Get() == files.GetPath() + "/second/fallback.txt", true, "Late path returns full path"));

	RETURN_IF_FALSE(t.Assert(
		handler.RemoveSourcePath("missing", files.GetPath() + "/first"), false, "Removal does not affect other keys"));
	RETURN_IF_FALSE(t.Assert(handler.RemoveSourcePath("text", files.GetPath() + "/first"), true,
		"First path removed with trailing-slash normalization"));
	RETURN_IF_FALSE(t.Assert(handler.SourceLookup("text", "/shared.txt", out), true, "Remaining path found"));
	RETURN_IF_FALSE(
		t.Assert(out.Get() == files.GetPath() + "/second/shared.txt", true, "Removal preserves remaining priority"));
	RETURN_IF_FALSE(
		t.Assert(handler.AddSourcePath("text", files.GetPath() + "/first"), true, "Removed path added again"));
	RETURN_IF_FALSE(t.Assert(handler.SourceLookup("text", "/shared.txt", out), true, "Reinserted key found"));
	RETURN_IF_FALSE(
		t.Assert(out.Get() == files.GetPath() + "/second/shared.txt", true, "Reinserted path follows existing paths"));
	RETURN_IF_FALSE(t.Assert(handler.RemoveSourcePath("text", files.GetPath() + "/first/"), true,
		"Registered path removed using a trailing slash"));

	handler.ClearSourcePaths();
	RETURN_IF_FALSE(t.Assert(handler.SourceLookup("text", "/shared.txt", out), false, "Cleared paths are unavailable"));
	// The directory form needs one byte for the slash and one for the null terminator.
	const uint64_t pathCapacity{ MSAPI::Protocol::HTTP::IHandler::DEFAULT_PATH_CAPACITY };
	std::string maximumPath{ files.GetPath() + "/" };
	maximumPath.resize(pathCapacity - 1, '/');
	RETURN_IF_FALSE(t.Assert(handler.AddSourcePath(std::string(32, 'x'), maximumPath), true,
		"Maximum key and directory form fit with null terminator"));
	maximumPath += '/';
	RETURN_IF_FALSE(
		t.Assert(handler.AddSourcePath("full", maximumPath), false, "Full directory form leaves no terminator space"));
	maximumPath.resize(pathCapacity - 2 - std::string_view{ "first" }.size());
	maximumPath += "first";
	RETURN_IF_FALSE(t.Assert(handler.AddSourcePath("boundary", maximumPath), true,
		"Maximum path without slash reserves slash and terminator space"));
	RETURN_IF_FALSE(
		t.Assert(handler.SourceLookup("boundary", "/shared.txt", out), true, "Maximum path supports file lookup"));
	RETURN_IF_FALSE(t.Assert(out.Get() == maximumPath + "/shared.txt", true, "Maximum path preserves the full path"));
	RETURN_IF_FALSE(t.Assert(handler.RemoveSourcePath("boundary", maximumPath + "/"), true,
		"Maximum path removed using its directory form"));

	return true;
}

FORCE_INLINE [[nodiscard]] bool Parsing(MSAPI::Test::Test& t)
{
	const std::array<std::pair<std::string_view, bool>, 11> cases{
		std::pair{ "GET /a?b=1&c=2 HTTP/1.1\r\nHost: www.site.com:8080\r\nConnection: close\r\n\r\n", true },
		std::pair{ "GET / HTTP/1.1\r\nconnection: Keep-Alive, CLOSE\r\n\r\n", true },
		std::pair{ "GET / HTTP/1.0\r\n\r\n", true },
		std::pair{ "GET / HTTP/1.0\r\nConnection: Keep-Alive\r\n\r\n", false },
		std::pair{ "GET / HTTP/1.1\r\n\r\n", false },
		std::pair{ "GET / HTTP/1.1\r\nConnection: disclose\r\n\r\n", false },
		std::pair{ "GET / HTTP/1.0\r\nConnection: upgrade, keep-alive\r\n\r\n", false },
		std::pair{ "GET / HTTP/1.0\r\nConnection: close, keep-alive\r\n\r\n", true },
		std::pair{ "GET / HTTP/1.1\r\nConnection:close\r\n\r\n", false },
		std::pair{ "GET / HTTP/1.1\r\nConnection: keep-alive\r\nConnection: close\r\n\r\n", false },
		std::pair{ "GET / HTTP/1.0\r\nConnection:\t keep-alive \t\r\n\r\n", true }
	};
	for (const auto& [request, close] : cases) {
		Request input;
		RETURN_IF_FALSE(t.Assert(input.Parse(request), true, "Request parsed"));
		RETURN_IF_FALSE(t.Assert(input.Get().IsValid(), true, "Request valid"));
		RETURN_IF_FALSE(t.Assert(input.Get().IsConnectionClose(), close, "Connection policy correct"));
	}

	Request headers;
	RETURN_IF_FALSE(
		t.Assert(headers.Parse("GET / HTTP/1.1\r\nX-Value:  padded \t\r\nX-Value: second\r\nX-No-Space:value\r\n\r\n"),
			true, "Original header syntax parsed"));
	const auto* value{ headers.Get().GetValue("X-Value") };
	RETURN_IF_FALSE(t.Assert(value != nullptr, true, "Spaced header retained"));
	RETURN_IF_FALSE(t.Assert(*value, std::string{ " padded \t" }, "First header value preserved without trimming"));
	RETURN_IF_FALSE(t.Assert(headers.Get().GetValue("X-No-Space") == nullptr, true, "Unspaced header not collected"));

	Request query;
	RETURN_IF_FALSE(t.Assert(query.Parse(cases[0].first), true, "Query request parsed"));
	RETURN_IF_FALSE(t.Assert(query.Get().GetUrl(), std::string{ "/a" }, "Routing path excludes query"));
	RETURN_IF_FALSE(t.Assert(query.Get().GetQuery(), std::string_view{ "?b=1&c=2" }, "Query retained with delimiter"));
	RETURN_IF_FALSE(t.Assert(query.Get().GetHost(), std::string_view{ "www.site.com" }, "Host port stripped"));

	Request ipv6;
	RETURN_IF_FALSE(
		t.Assert(ipv6.Parse("GET /a?b#fragment HTTP/1.1\r\nhost: [::1]:8080\r\n\r\n"), true, "IPv6 host parsed"));
	RETURN_IF_FALSE(t.Assert(ipv6.Get().GetHost(), std::string_view{ "[::1]" }, "IPv6 brackets preserved"));
	RETURN_IF_FALSE(t.Assert(ipv6.Get().GetQuery(), std::string_view{ "?b" }, "Fragment excluded from query"));

	Request missing;
	RETURN_IF_FALSE(t.Assert(missing.Parse("GET /a? HTTP/1.1\r\n\r\n"), true, "Empty query parsed"));
	RETURN_IF_FALSE(t.Assert(missing.Get().GetHost().empty(), true, "Missing host empty"));
	RETURN_IF_FALSE(t.Assert(missing.Get().GetQuery(), std::string_view{ "?" }, "Empty query delimiter preserved"));

	Request malformed;
	RETURN_IF_FALSE(t.Assert(malformed.Parse("GET /a?unterminated"), true, "Truncated request inspected"));
	RETURN_IF_FALSE(t.Assert(malformed.Get().IsValid(), false, "Truncated query rejected safely"));

	return true;
}

FORCE_INLINE [[nodiscard]] bool Responses(MSAPI::Test::Test& t)
{
	for (const uint64_t extensionSize : std::array<uint64_t, 2>{ 59, 60 }) {
		Request boundary;
		const std::string extension(extensionSize, 'x');
		RETURN_IF_FALSE(t.Assert(
			boundary.Parse("GET /a." + extension + " HTTP/1.1\r\n\r\n"), true, "Content type boundary fixture parsed"));
		RETURN_IF_FALSE(t.Assert(boundary.Get().IsValid(), true, "Content type boundary request valid"));
		std::string captured;
		boundary.GetConnection().SetSend(
			[&captured](int32_t, const void* buffer, const uint64_t size, int32_t) -> int64_t {
				captured.append(static_cast<const char*>(buffer), size);
				return static_cast<int64_t>(size);
			});
		RETURN_IF_FALSE(t.Assert(boundary.Get().SendResponse(boundary.GetConnection(), "body"), true,
			"Response sends with boundary or oversized inferred content type"));
		if (extensionSize == 59) {
			RETURN_IF_FALSE(
				t.Assert(captured.find("Content-Type: text/" + extension + "; charset=utf-8\r\n") != std::string::npos,
					true, "Boundary content type is not truncated"));
		}
		else {
			RETURN_IF_FALSE(t.Assert(
				captured.find("Content-Type: application/octet-stream; charset=utf-8\r\n") != std::string::npos, true,
				"Oversized inferred content type uses binary default"));
			RETURN_IF_FALSE(t.Assert(captured.ends_with("\r\n\r\nbody"), true, "Default response preserves body"));
			captured.clear();
			RETURN_IF_FALSE(t.Assert(boundary.Get().SendResponse(boundary.GetConnection(), "body", "text/plain"), true,
				"Explicit content type bypasses inference limit"));
			RETURN_IF_FALSE(t.Assert(captured.find("Content-Type: text/plain; charset=utf-8\r\n") != std::string::npos,
				true, "Explicit content type is preserved"));
		}
	}

	Request input;
	RETURN_IF_FALSE(t.Assert(input.Parse("GET /a.txt HTTP/1.1\r\n\r\n"), true, "Response fixture parsed"));

	std::string output;
	// Force repeated short writes to verify the sender completes the response.
	input.GetConnection().SetSend([&output](int32_t, const void* buffer, const uint64_t size, int32_t) -> int64_t {
		const auto count{ std::min<uint64_t>(7, size) };
		output.append(static_cast<const char*>(buffer), count);
		return static_cast<int64_t>(count);
	});
	RETURN_IF_FALSE(t.Assert(
		input.Get().SendResponse(input.GetConnection(), "hello"), true, "Response sends through short writes"));
	RETURN_IF_FALSE(t.Assert(output,
		std::string{ "HTTP/1.1 200 OK\r\nContent-Type: text/plain; charset=utf-8\r\nConnection: "
					 "keep-alive\r\nKeep-Alive: timeout=0,max=0\r\nContent-Length: 5\r\n\r\nhello" },
		"Default response bytes correct"));

	output.clear();
	RETURN_IF_FALSE(t.Assert(input.Get().Send404(input.GetConnection()), true, "Empty 404 sends"));
	RETURN_IF_FALSE(t.Assert(output,
		std::string{ "HTTP/1.1 404 Not Found\r\nContent-Type: text/html; charset=utf-8\r\nConnection: "
					 "keep-alive\r\nKeep-Alive: timeout=0,max=0\r\nContent-Length: 0\r\n\r\n" },
		"Empty 404 is framed"));

	Files files;
	RETURN_IF_FALSE(t.Assert(files.IsValid(), true, "Response source files created"));
	output.clear();
	const std::string path{ files.GetPath() + "/first/shared.txt" };
	const std::string pathWithSuffix{ path + "extra" };
	RETURN_IF_FALSE(t.Assert(
		input.Get().SendSource(input.GetConnection(), std::string_view{ pathWithSuffix }.substr(0, path.size())), true,
		"Source view need not be null terminated"));
	RETURN_IF_FALSE(t.Assert(output.ends_with("Content-Length: 5\r\n\r\nfirst"), true, "Source bytes correct"));

	MSAPI::SString<256> location;
	RETURN_IF_FALSE(t.Assert(
		MSAPI::Protocol::HTTP::BuildRedirectLocation(location, "https", "site.com", "/a?b"), true, "Location built"));
	RETURN_IF_FALSE(t.Assert(location.Get(), std::string_view{ "https://site.com/a?b" }, "Location keeps query"));

	const std::array<std::pair<MSAPI::Protocol::HTTP::StatusCode, std::string_view>, 5> codes{
		{ { MSAPI::Protocol::HTTP::StatusCode::MovedPermanently, "Moved Permanently" },
			{ MSAPI::Protocol::HTTP::StatusCode::Found, "Found" },
			{ MSAPI::Protocol::HTTP::StatusCode::SeeOther, "See Other" },
			{ MSAPI::Protocol::HTTP::StatusCode::TemporaryRedirect, "Temporary Redirect" },
			{ MSAPI::Protocol::HTTP::StatusCode::PermanentRedirect, "Permanent Redirect" } }
	};
	for (const auto& [code, reason] : codes) {
		MSAPI::SString<512> response;
		RETURN_IF_FALSE(
			t.Assert(MSAPI::Protocol::HTTP::BuildRedirectResponse({ response.GetBuffer(), response.GetCapacity() },
						 location.Get(), code, MSAPI::Protocol::HTTP::CLOSE_CONNECTION)
					!= 0,
				true, "Redirect formatter accepts supported status"));
		const std::string expected{ "HTTP/1.1 " + std::to_string(static_cast<uint32_t>(code)) + " "
			+ std::string{ reason }
			+ "\r\nLocation: https://site.com/a?b\r\nConnection: close\r\nContent-Length: 0\r\n\r\n" };
		RETURN_IF_FALSE(t.Assert(std::string_view{ response.GetBuffer() } == expected, true, "Redirect bytes correct"));

		output.clear();
		RETURN_IF_FALSE(t.Assert(
			input.Get().SendRedirect(input.GetConnection(), location.Get(), code), true, "Data redirect sends"));
		RETURN_IF_FALSE(t.Assert(output.find("Location: https://site.com/a?b\r\n") != std::string::npos, true,
			"Data redirect location correct"));
		RETURN_IF_FALSE(t.Assert(output.find("Keep-Alive:") != std::string::npos, true, "Data default keeps alive"));
	}

	MSAPI::SString<32> small;
	RETURN_IF_FALSE(
		t.Assert(MSAPI::Protocol::HTTP::BuildRedirectLocation(small, "https", "site.com", std::string(60, '/')), false,
			"Oversized location fails"));
	RETURN_IF_FALSE(t.Assert(small.Empty(), true, "Oversized location cleared"));
	RETURN_IF_FALSE(t.Assert(MSAPI::Protocol::HTTP::BuildRedirectLocation(small, "https", "site.com\r\nX: y", "/"),
		false, "Host injection fails"));
	RETURN_IF_FALSE(
		t.Assert(MSAPI::Protocol::HTTP::BuildRedirectResponse({ small.GetBuffer(), small.GetCapacity() }, "/a") != 0,
			false, "Oversized response fails"));

	MSAPI::SString<512> response;
	RETURN_IF_FALSE(
		t.Assert(MSAPI::Protocol::HTTP::BuildRedirectResponse({ response.GetBuffer(), response.GetCapacity() }, "/a",
					 static_cast<MSAPI::Protocol::HTTP::StatusCode>(300))
				!= 0,
			false, "Unsupported status fails"));
	RETURN_IF_FALSE(t.Assert(
		MSAPI::Protocol::HTTP::BuildRedirectResponse({ response.GetBuffer(), response.GetCapacity() }, "/a\r\nX: y")
			!= 0,
		false, "Location injection fails"));
	RETURN_IF_FALSE(t.Assert(MSAPI::Protocol::HTTP::BuildRedirectResponse(
								 { response.GetBuffer(), response.GetCapacity() }, std::string_view{ "/a\0x", 4 })
			!= 0,
		false, "Location NUL fails"));
	RETURN_IF_FALSE(
		t.Assert(input.Get().SendRedirect(input.GetConnection(), "/a", MSAPI::Protocol::HTTP::StatusCode::Ok), false,
			"Data rejects unsupported redirect"));

	output.clear();
	RETURN_IF_FALSE(t.Assert(
		input.Get().SendResponse(input.GetConnection(), "bye", "text/plain", MSAPI::Protocol::HTTP::CLOSE_CONNECTION),
		true, "Close response completes"));
	RETURN_IF_FALSE(t.Assert(output.find("Connection: close\r\n") != std::string::npos, true, "Close header present"));
	RETURN_IF_FALSE(t.Assert(output.find("Keep-Alive:") == std::string::npos, true, "Close omits Keep-Alive"));

	std::array<char, 1> byte{};
	RETURN_IF_FALSE(t.Assert(recv(input.GetPeer(), byte.data(), 1, 0) == 0, true, "Close reaches peer as EOF"));

	return true;
}

FORCE_INLINE [[nodiscard]] bool Http()
{
	MSAPI::Test::Test test;
	if (!Paths(test) || !Parsing(test) || !Responses(test)) {
		return false;
	}

	return test.Passed<bool>();
}

} // namespace Unit

} // namespace Test

} // namespace MSAPI

#endif // MSAPI_UNIT_TEST_HTTP_INL
