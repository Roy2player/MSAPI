/**************************
 * @file        http.h
 * @date        2023-09-02
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
 * @brief HTTP message parsing and producing, redirects and keyed static file lookup.
 *
 * Recognizes content types for js, dat, ogg, pdf, xhtml, json, ldjson, xml, zip, mp3, wma, wav, gif, jpeg, jpg,
 * png, tiff, ico, djvu, svg, bmp, webp, css, csv, html, txt, mpeg, mp4, mov, wmv, avi and webm.
 *
 * @note MSAPI_HANDLER_HTTP_PRESET macro is used to reserve and collect HTTP message.
 */

#ifndef MSAPI_PROTOCOL_HTTP_H
#define MSAPI_PROTOCOL_HTTP_H

#include "../help/log.h"
#include "../server/recvBuffer.inl"
#include "webSocket.inl"
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <vector>

namespace MSAPI {

class Application;

namespace Protocol {

namespace HTTP {

/**************************
 * @brief Identifies supported HTTP response statuses by their numeric values on the wire.
 */
enum class StatusCode : uint16_t {
	Undefined = 0, // Sentinel value for undefined status
	Ok = 200,
	MovedPermanently = 301,
	Found = 302,
	SeeOther = 303,
	TemporaryRedirect = 307,
	PermanentRedirect = 308,
	NotFound = 404,
	Max // Sentinel value for the maximum status
};

/**************************
 * @brief Returns the HTTP reason phrase for a status code. Logs unrecognized values.
 *
 * @param code HTTP response status.
 *
 * @locking Is not required.
 *
 * @return Reason phrase, sentinel name for Undefined or Max, or "Unknown" if the status code is not recognized.
 *
 * @todo Add tests coverage.
 */
FORCE_INLINE [[nodiscard]] constexpr std::string_view EnumToString(const StatusCode code) noexcept
{
	// Static assert cannot guarantee all the items are presented

	switch (code) {
	case StatusCode::Undefined:
		return "Undefined";
	case StatusCode::Ok:
		return "OK";
	case StatusCode::MovedPermanently:
		return "Moved Permanently";
	case StatusCode::Found:
		return "Found";
	case StatusCode::SeeOther:
		return "See Other";
	case StatusCode::TemporaryRedirect:
		return "Temporary Redirect";
	case StatusCode::PermanentRedirect:
		return "Permanent Redirect";
	case StatusCode::NotFound:
		return "Not Found";
	case StatusCode::Max:
		return "Max";
	default:
		LOG_ERROR_NEW("Unknown HTTP status code: {}", U(code));
		return "Unknown";
	}
}

// Response policy: keeps the connection open after sending.
static inline constexpr bool KEEP_CONNECTION_ALIVE{};
// Response policy: closes the connection after sending.
static inline constexpr bool CLOSE_CONNECTION{ true };

/**************************
 * @brief Checks that a header value contains no CR, LF or NUL bytes.
 *
 * @param text Header value to inspect.
 *
 * @locking Is not required.
 *
 * @return True if the value contains no header delimiters or NUL bytes.
 *
 * @todo Add tests coverage.
 */
[[nodiscard]] bool IsHeaderValue(std::string_view text) noexcept;

/**************************
 * @brief Builds a null terminated HTTP response header without allocating. Reserves space for the terminator and
 * never returns a truncated header.
 *
 * @param out Receives the header, including its null terminator.
 * @param version HTTP version, 1.0 or 1.1.
 * @param code Supported response status, excluding Undefined and Max.
 * @param contentType Content type, or empty to omit the field.
 * @param length Response body size in bytes.
 * @param closeConnection Selects Connection: close if true, or keep-alive headers otherwise.
 * @param location Location value, or empty to omit the field.
 *
 * @locking Is not required.
 *
 * @return Header size excluding the terminator, or zero if validation fails or the header does not fit.
 *
 * @todo Add tests coverage.
 */
[[nodiscard]] uint64_t BuildResponseHeader(std::span<char> out, std::string_view version, StatusCode code,
	std::string_view contentType, uint64_t length, bool closeConnection, std::string_view location = {}) noexcept;

/**************************
 * @brief Builds a null terminated redirect response with Content-Length: 0 without allocating. Validates the redirect
 * status and location, then delegates formatting to the shared response header builder. Writes a null byte at the
 * beginning of a nonempty buffer on failure.
 *
 * @param out Receives the response, including its null terminator.
 * @param location Location header value.
 * @param code Redirect status: MovedPermanently, Found, SeeOther, TemporaryRedirect or PermanentRedirect.
 * @param closeConnection Selects Connection: close if true, or Connection: keep-alive and Keep-Alive otherwise.
 * @param version HTTP version, 1.0 or 1.1.
 *
 * @locking Is not required.
 *
 * @return Response size excluding the null terminator, or zero if validation fails or the response does not fit.
 *
 * @test Yes.
 */
[[nodiscard]] uint64_t BuildRedirectResponse(std::span<char> out, std::string_view location,
	StatusCode code = StatusCode::MovedPermanently, bool closeConnection = KEEP_CONNECTION_ALIVE,
	std::string_view version = "1.1") noexcept;

/**************************
 * @brief Checks redirect location components without allocating or constructing a URL. Accepts http and https,
 * a nonempty host without /, backslash, ?, #, @, spaces or horizontal tabs, and a nonempty path beginning with /.
 * Rejects CR, LF and NUL bytes in the host or path. Does not check output capacity or fully validate URI syntax.
 *
 * @param scheme Scheme without ://.
 * @param host Host with an optional port.
 * @param url Request path and optional query, beginning with /.
 *
 * @locking Is not required.
 *
 * @return True if the components satisfy the redirect location checks, false otherwise.
 *
 * @todo Add tests coverage.
 */
FORCE_INLINE [[nodiscard]] bool ValidateLocation(
	const std::string_view scheme, const std::string_view host, const std::string_view url) noexcept
{
	return (scheme == "http" || scheme == "https") && !host.empty() && !url.empty() && url.front() == '/'
		&& host.find_first_of("/\\?#@ \t\r\n") == std::string_view::npos && host.find('\0') == std::string_view::npos
		&& url.find_first_of("\r\n") == std::string_view::npos && url.find('\0') == std::string_view::npos;
}

/**************************
 * @brief Builds scheme://host plus the URL, including any query, without allocating. Clears out on failure.
 *
 * @tparam Capacity Output buffer capacity, including room for its null terminator.
 *
 * @param out Receives the complete location.
 * @param scheme Scheme without ://.
 * @param host Host with an optional port.
 * @param url Request path and optional query, beginning with /.
 *
 * @locking Is not required.
 *
 * @return True if the location fits and its components contain no header delimiters or NUL bytes.
 *
 * @test Yes.
 */
template <uint64_t Capacity>
FORCE_INLINE [[nodiscard]] bool BuildRedirectLocation(
	SString<Capacity>& out, std::string_view scheme, std::string_view host, std::string_view url) noexcept
{
	out.Clear();
	if (!ValidateLocation(scheme, host, url) || scheme.size() + 3 + host.size() >= Capacity
		|| url.size() >= Capacity - scheme.size() - 3 - host.size()) {
		return false;
	}

	return out.Copy(scheme) && out.Concatenate("://") && out.Concatenate(host) && out.Concatenate(url)
		&& out.NullTerminate();
}

/**************************
 * @brief Object for parsing and collecting information about a HTTP message.
 */
class Data {
private:
	// GUID from RFC 6455, concatenated with the client key to calculate the WebSocket accept key
	static inline constexpr std::string_view WEB_SOCKET_GUID{ "258EAFA5-E914-47DA-95CA-C5AB0DC85B11" };

private:
	std::map<std::string, std::string, std::less<>> m_headersMap;
	std::string m_messageType;
	std::string m_url;
	std::string m_query{};
	std::string m_HTTPtype;
	std::string m_version;
	std::string m_format;
	std::string m_code;
	std::string m_codeText;
	std::string m_body;
	size_t m_messageSize{};
	bool m_isRequest{};
	bool m_isValid{};

public:
	/**************************
	 * @brief Construct a new Data object, parse buffer and collect information about HTTP message. Response does not
	 * contain format. If buffer does not contain HTTP message, object will be invalid and additional data will not be
	 * read from socket.
	 *
	 * @attention HTTP header section cannot be longer than 2048 bytes.
	 * @attention If request does not contain format, it will be set as html.
	 *
	 * @param recvBuffer Recv buffer object.
	 *
	 * @test Yes.
	 *
	 * @todo Support optional spaces and tabs after the header key; the parser currently requires a literal space.
	 * @todo Handle repeated header fields according to each field's semantics; map insertion currently retains only
	 * the first value for an identical field name, including Connection.
	 * @todo Handle leading and trailing optional whitespace in header values according to HTTP rules; the parser
	 * currently skips only the first space after the colon and retains the remaining whitespace.
	 */
	Data(MSAPI::RecvBuffer& recvBuffer);

	FORCE_INLINE Data(const Data&) = default;
	FORCE_INLINE Data(Data&&) noexcept = default;
	FORCE_INLINE Data& operator=(const Data&) = default;
	FORCE_INLINE Data& operator=(Data&&) noexcept = default;

	/**************************
	 * @return Readable link to maps with headers like { header, value }.
	 */
	const std::map<std::string, std::string, std::less<>>& GetHeadersMap() const noexcept;

	/**************************
	 * @return Size of headers map.
	 *
	 * @test Yes.
	 */
	size_t GetSizeHeadersMap() const noexcept;

	/**************************
	 * @param key Header name.
	 *
	 * @note HTTP field names are case-insensitive RFC 9110 §5.1.
	 *
	 * @return Readable pointer to value from headers map by key, nullptr if does not exist.
	 *
	 * @test Yes.
	 */
	const std::string* GetValue(std::string_view key) const;

	/**************************
	 * @return Version of HTTP message.
	 *
	 * @test Yes.
	 */
	const std::string& GetVersion() const noexcept;

	/**************************
	 * @return Type of HTTP message, GET/POST/PUT/DELETE/HEAD/CONNECT/OPTIONS/TRACE/PATCH.
	 *
	 * @test Yes.
	 */
	const std::string& GetTypeMessage() const noexcept;

	/**************************
	 * @return Type of HTTP message, HTTP or HTTPS.
	 *
	 * @test Yes.
	 */
	const std::string& GetHTTPType() const noexcept;

	/**************************
	 * @return Code of HTTP message.
	 *
	 * @test Yes.
	 */
	const std::string& GetCode() const noexcept;

	/**************************
	 * @return Code text of HTTP message.
	 *
	 * @test Yes.
	 */
	const std::string& GetCodeText() const noexcept;

	/**************************
	 * @return Whole size of HTTP message, including headers and body.
	 *
	 * @test Yes.
	 */
	size_t GetMessageSize() const noexcept;

	/**************************
	 * @return Request url of HTTP message.
	 *
	 * @test Yes.
	 */
	const std::string& GetUrl() const noexcept;

	/**************************
	 * @return View of the request query, including its leading ?, or an empty view.
	 *
	 * @test Yes.
	 */
	[[nodiscard]] std::string_view GetQuery() const noexcept;

	/**************************
	 * @return Host header without its port; preserves brackets around IPv6 literals. Empty if absent.
	 *
	 * @test Yes.
	 */
	[[nodiscard]] std::string_view GetHost() const noexcept;

	/**************************
	 * @return True for Connection: close, or HTTP/1.0 without a keep-alive token. Header names and tokens ignore case.
	 *
	 * @test Yes.
	 */
	[[nodiscard]] bool IsConnectionClose() const noexcept;

	/**************************
	 * @return Format of HTTP request, html as default.
	 *
	 * @test Yes.
	 */
	const std::string& GetFormat() const noexcept;

	/**************************
	 * @return Body of HTTP message.
	 *
	 * @test Yes.
	 */
	const std::string& GetBody() const noexcept;

	/**************************
	 * @return True if HTTP message is request.
	 *
	 * @test Yes.
	 */
	bool IsRequest() const noexcept;

	/**************************
	 * @return True if HTTP message is valid.
	 *
	 * @test Yes.
	 */
	bool IsValid() const noexcept;

	/**************************
	 * @brief Distributor function for sending HTTP 200 OK response message. Providing content type manually is faster.
	 *
	 * @attention Uses application/octet-stream if the inferred content type exceeds 64 bytes.
	 *
	 * @param connection Connection to which send HTTP message.
	 * @param body Body of HTTP message.
	 * @param contentType Content type of HTTP message, if empty, will be used format of data.
	 * @param closeConnection Closes the connection after sending if true; defaults to keeping it open.
	 *
	 * @return True if HTTP message was sent successfully, false otherwise.
	 *
	 * @test Yes.
	 */
	[[nodiscard]] bool SendResponse(Connection& connection, std::string_view body, std::string_view contentType = {},
		bool closeConnection = KEEP_CONNECTION_ALIVE) const;

	/**************************
	 * @brief Sends a source file using a non-owning path view.
	 *
	 * @attention Uses application/octet-stream for oversized inferred content types, as in SendResponse.
	 *
	 * @param connection Connection receiving the response.
	 * @param path Full file path.
	 * @param contentType Explicit content type, or empty to use the request format.
	 * @param closeConnection Closes the connection after sending if true; defaults to keeping it open.
	 *
	 * @return True if the complete response is sent.
	 *
	 * @test Yes.
	 */
	[[nodiscard]] bool SendSource(Connection& connection, std::string_view path, std::string_view contentType = {},
		bool closeConnection = KEEP_CONNECTION_ALIVE) const;

	/**************************
	 * @brief Distributor function for sending HTTP 404 response message.
	 *
	 * @param connection Connection to which send HTTP message.
	 * @param body Body bytes. Empty by default.
	 * @param contentType Content type of HTTP message; uses text/html if empty.
	 * @param closeConnection Closes the connection after sending if true; defaults to keeping it open.
	 *
	 * @return True if HTTP message was sent successfully, false otherwise.
	 *
	 * @test Yes.
	 */
	[[nodiscard]] bool Send404(Connection& connection, std::string_view body = {}, std::string_view contentType = {},
		bool closeConnection = KEEP_CONNECTION_ALIVE) const;

	/**************************
	 * @brief Sends an empty redirect response using the shared redirect formatter.
	 *
	 * @param connection Connection receiving the response.
	 * @param location Complete Location value, including any query.
	 * @param code Redirect status: MovedPermanently, Found, SeeOther, TemporaryRedirect or PermanentRedirect.
	 * @param closeConnection Closes the connection after sending if true; keeps it open otherwise.
	 *
	 * @return True if the complete response is sent; false for invalid input or an oversized header.
	 *
	 * @test Yes.
	 */
	[[nodiscard]] bool SendRedirect(Connection& connection, std::string_view location,
		StatusCode code = StatusCode::MovedPermanently, bool closeConnection = KEEP_CONNECTION_ALIVE) const;

	/**************************
	 * @return True if HTTP message is WebSocket upgrade request, false otherwise.
	 */
	[[nodiscard]] bool IsWebSocketUpgradeRequest() const noexcept;

	/**************************
	 * @return True if HTTP message is WebSocket upgrade response, false otherwise.
	 */
	[[nodiscard]] bool IsWebSocketUpgradeResponse() const noexcept;

	/**************************
	 * @brief Check validity of request and send response.
	 *
	 * @param connection Connection of request.
	 *
	 * @return True if response sended successfully, false otherwise.
	 *
	 * @test Yes.
	 */
	[[nodiscard]] bool SendWebSocketUpgradeResponse(Connection& connection) const;

	/**************************
	 * @example HTTP message:
	 * {
	 * 		is valid        : true
	 * 		type            : Request
	 * 		message type    : GET
	 * 		url             : /api
	 * 		HTTP type       : HTTP
	 * 		version         : 1.1
	 * 		format          :
	 * 		code            :
	 * 		code text       :
	 *      message size    : 435
	 * 		Headers         :
	 * {
	 *      Accept          : application/json
	 *      Accept-Encoding : gzip, deflate, br
	 *      Accept-Language : en-US,en;q=0.5
	 *      Connection      : keep-alive
	 *      Host            : 127.0.0.1:1110
	 *      Referer         : http://127.0.0.1:1110/
	 *      Sec-Fetch-Dest  : empty
	 *      Sec-Fetch-Mode  : cors
	 *      Sec-Fetch-Site  : same-origin
	 *      SessionsDate    : 2023-09-21
	 *      Type            : getSessions
	 *      User-Agent      : Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:125.0) Gecko/20100101 Firefox/125.0
	 *      figi            : 1937652074990171732
	 * }
	 * }
	 *
	 * @test Yes.
	 */
	std::string ToString() const;

	/**************************
	 * @brief Call ToString() inside.
	 */
	operator std::string() const;

private:
	/**************************
	 * @brief Derives a content type from the parsed file format. Logs unsupported formats and uses text/ plus the
	 * format as the fallback. Logs content types exceeding 64 bytes and uses application/octet-stream instead.
	 *
	 * @locking Is not required.
	 *
	 * @return Content type for the response body, or application/octet-stream if it exceeds capacity.
	 *
	 * @test Yes.
	 */
	[[nodiscard]] SString<64> GetContentType() const;

	/**************************
	 * @brief Sends a prepared response, repeating sends until all bytes are sent or a send fails. Closes the connection
	 * after the send attempt when requested, including on failure.
	 *
	 * @param connection Connection receiving the response.
	 * @param response Complete response bytes, including headers and body.
	 * @param closeConnection Closes the connection after sending if true; keeps it open otherwise.
	 *
	 * @locking Is not required.
	 *
	 * @return True if all response bytes are sent, false otherwise.
	 *
	 * @test Yes.
	 */
	[[nodiscard]] bool SendPrepared(Connection& connection, std::string_view response, bool closeConnection) const;

public:
	/**************************
	 * @brief Call ToString() inside.
	 */
	friend std::ostream& operator<<(std::ostream& out, Data& data);
};

/**************************
 * @brief Reserves HTTP messages, stores file type and path pairs to source lookup.
 *
 * @attention Use MSAPI_HANDLER_HTTP_PRESET macro to reserve and collect HTTP messages.
 *
 * @concurrency Yes.
 *
 * @purging RemoveSourcePath releases one keyed path; ClearSourcePaths and destruction release every path and its
 * storage.
 */
class IHandler {
public:
	// Source path buffer capacity in bytes, including its trailing slash and null terminator.
	static inline constexpr uint64_t DEFAULT_PATH_CAPACITY{ 256 };
	// Default capacity of an endpoint destination (scheme://host[:port]) in bytes.
	static inline constexpr uint64_t DEFAULT_DESTINATION_CAPACITY{ 256 };

private:
	const MSAPI::Application* m_application;
	std::multimap<std::string, SString<DEFAULT_PATH_CAPACITY>, std::less<>> m_paths;
	mutable MSAPI::Lock::AtomicRW m_pathsLock;

public:
	/**************************
	 * @brief Construct a new IHandler object, empty constructor.
	 *
	 * @param application Readable pointer to application object.
	 *
	 * @test Yes.
	 */
	IHandler(const MSAPI::Application* application);

	FORCE_INLINE virtual ~IHandler() = default;

	IHandler(const IHandler&) = delete;
	IHandler(IHandler&&) = delete;
	IHandler& operator=(const IHandler&) = delete;
	IHandler& operator=(IHandler&&) = delete;

	/**************************
	 * @brief Adds a trailing / if missing, null terminates the path and checks its existence with MSAPI::IO::HasPath.
	 * Removes one trailing /, null terminates and checks the resulting path, then stores it under key.
	 * Preserves insertion order for multiple paths registered under the same key.
	 *
	 * @param key Nonempty key of up to 32 bytes without NUL characters.
	 * @param path Existing directory path without NUL characters. Its trailing-slash form and null terminator must
	 * fit in DEFAULT_PATH_CAPACITY bytes. The path must remain nonempty after removing one trailing /.
	 *
	 * @locking Write lock m_pathsLock inside.
	 *
	 * @return True if the checked path is registered. Returns false for invalid key or path, insufficient capacity,
	 * either failed path check, or registration allocation failure.
	 *
	 * @test Yes.
	 */
	[[nodiscard]] bool AddSourcePath(std::string_view key, std::string_view path) noexcept;

	/**************************
	 * @brief Removes the first registration matching key and path, preserving the order of all other paths.
	 *
	 * @param key Key of the path to remove.
	 * @param path Registered path, with or without one trailing /. Removes one trailing / before comparison.
	 *
	 * @locking Write lock m_pathsLock inside.
	 *
	 * @return True if a matching registration is removed, false if path is invalid or no match exists.
	 *
	 * @test Yes.
	 */
	[[nodiscard]] bool RemoveSourcePath(std::string_view key, std::string_view path) noexcept;

	/**************************
	 * @brief Clears registered paths for configuration reloads.
	 *
	 * @locking Write lock m_pathsLock inside.
	 *
	 * @test Yes.
	 */
	void ClearSourcePaths() noexcept;

	/**************************
	 * @brief Finds the first existing regular file under the paths for key without allocating. Clears out on failure.
	 * Does not fall back to later paths if the first existing file's full path exceeds the output capacity.
	 *
	 * @tparam Capacity Caller buffer capacity, including its null terminator.
	 *
	 * @param key Registered path key.
	 * @param filename Path beginning with /; rejects .., NUL and backslash anywhere.
	 * @param out Receives the full path of the first existing regular file.
	 *
	 * @locking Read lock m_pathsLock inside.
	 *
	 * @return True if a matching regular file exists and its full path fits.
	 *
	 * @test Yes.
	 */
	template <uint64_t Capacity>
	FORCE_INLINE [[nodiscard]] bool SourceLookup(
		std::string_view key, std::string_view filename, SString<Capacity>& out) const noexcept
	{
		out.Clear();
		if (filename.empty() || filename.front() != '/' || filename.find("..") != std::string_view::npos
			|| filename.find('\0') != std::string_view::npos || filename.find('\\') != std::string_view::npos)
			[[unlikely]] {

			LOG_WARNING_NEW("Invalid filename: {}", filename);
			return false;
		}

		if (filename.size() >= Capacity) [[unlikely]] {

			LOG_WARNING_NEW("Filename exceeds buffer capacity: {}", filename);
			return false;
		}

		{
			const MSAPI::Lock::AtomicRW::Guard<MSAPI::Lock::READ> _{ m_pathsLock };
			const auto [begin, end]{ m_paths.equal_range(key) };
			for (auto it{ begin }; it != end; ++it) {
				const auto& path{ it->second };

				// Check file existence independently of output capacity to preserve path priority.
				SString<DEFAULT_PATH_CAPACITY + Capacity> candidate;
				if (!candidate.Copy(path.Get()) || !candidate.Concatenate(filename) || !candidate.NullTerminate())
					[[unlikely]] {

					LOG_WARNING_NEW(
						"Source path construction failed, path: \"{}\", filename: \"{}\"", path.Get(), filename);
					return false;
				}

				struct stat info { };
				if (stat(candidate.GetBuffer(), &info) == 0 && S_ISREG(info.st_mode)) {
					if (candidate.GetSize() >= Capacity) [[unlikely]] {
						LOG_WARNING_NEW("Source path exceeds buffer capacity: \"{}\"", candidate.Get());
						return false;
					}

					return out.Copy(candidate.Get()) && out.NullTerminate();
				}
			}
		}

		out.Clear();
		return false;
	}

	/**************************
	 * @brief Collect HTTP message from socket connection and call Handler function if Application is running.
	 *
	 * @param connectionData Connection data from which reserved message.
	 * @param data Reserved HTTP message.
	 *
	 * @todo Data can be transferred by rvalue reference.
	 *
	 * @test Yes.
	 */
	void Collect(const std::shared_ptr<Connection::Data>& connectionData, const Data& data);

	/**************************
	 * @brief Handler function for HTTP message.
	 *
	 * @param connectionData Connection data from which reserved message.
	 * @param data Reserved HTTP message.
	 */
	virtual void HandleHttp(const std::shared_ptr<Connection::Data>& connectionData, const Data& data) = 0;
};

/**************************
 * @brief Runs independent IPv4 redirect endpoints with at most 32 concurrent connections and 16 endpoints.
 * Each connection reads at most 2048 bytes, validates a complete HTTP request header and replies with an empty
 * 301 or 308 response. Request paths and queries are preserved. Receives use a bounded RecvBuffer; HTTP::Data parses
 * the message using owned strings and a header map.
 * Connections close after a redirect, invalid input or a receive timeout while waiting for request data.
 *
 * @attention The destructor stops and joins pthreads.
 *
 * @concurrency Yes.
 *
 * @purging Stop closes listeners and connections and joins every owned pthread. Configuration remains for restart.
 *
 * @todo Redirector's ownership of connection lifecycles is temporary until Server resolves its technical debt.
 * Revisit this ownership once Server supports multiple listen addresses and a main loop that waits for a lock to be
 * unlocked.
 */
class Redirector {
private:
	/**************************
	 * @brief Holds fixed endpoint configuration and listener pthread ownership.
	 *
	 * @concurrency No.
	 */
	class Endpoint {
	private:
		SString<16> m_ip{};
		SString<IHandler::DEFAULT_DESTINATION_CAPACITY> m_destination{};
		pthread_t m_pthread{};
		Redirector* m_owner{};
		int32_t m_socket{ -1 };
		uint16_t m_port{};
		bool m_isStarted{};

	public:
		/**************************
		 * @brief Initializes an unassigned endpoint.
		 *
		 * @todo Add tests coverage.
		 */
		FORCE_INLINE Endpoint() noexcept = default;

		Endpoint(const Endpoint&) = delete;
		Endpoint(Endpoint&&) = delete;
		Endpoint& operator=(const Endpoint&) = delete;
		Endpoint& operator=(Endpoint&&) = delete;

		/**************************
		 * @return Null terminated IPv4 address.
		 *
		 * @todo Add tests coverage.
		 */
		FORCE_INLINE [[nodiscard]] const char* GetIp() const noexcept;

		/**************************
		 * @brief Copies the IPv4 address, appends its null terminator and validates its syntax.
		 *
		 * @param ip IPv4 address to store.
		 *
		 * @return True if the address is valid and fits with its terminator, false otherwise.
		 *
		 * @todo Add tests coverage.
		 */
		FORCE_INLINE [[nodiscard]] bool SetIp(std::string_view ip) noexcept;

		/**************************
		 * @return Configured listener port.
		 *
		 * @todo Add tests coverage.
		 */
		FORCE_INLINE [[nodiscard]] uint16_t GetPort() const noexcept;

		/**************************
		 * @brief Records the validated listener port.
		 *
		 * @param port Validated nonzero listener port.
		 *
		 * @todo Add tests coverage.
		 */
		FORCE_INLINE void SetPort(uint16_t port) noexcept;

		/**************************
		 * @return Configured redirect destination.
		 *
		 * @todo Add tests coverage.
		 */
		FORCE_INLINE [[nodiscard]] std::string_view GetDestination() const noexcept;

		/**************************
		 * @brief Copies the redirect destination.
		 *
		 * @param destination Redirect destination including its scheme.
		 *
		 * @return True if the destination fits, false otherwise.
		 *
		 * @todo Add tests coverage.
		 */
		FORCE_INLINE [[nodiscard]] bool SetDestination(std::string_view destination) noexcept;

		/**************************
		 * @return Socket descriptor, or -1 when no socket is recorded.
		 *
		 * @todo Add tests coverage.
		 */
		FORCE_INLINE [[nodiscard]] int32_t GetSocket() const noexcept;

		/**************************
		 * @brief Records the socket descriptor without closing the previous descriptor.
		 *
		 * @param socket Socket descriptor, or -1 to clear the recorded descriptor.
		 *
		 * @todo Add tests coverage.
		 */
		FORCE_INLINE void SetSocket(int32_t socket) noexcept;

		/**************************
		 * @return Pthread identifier recorded after successful pthread creation.
		 *
		 * @todo Add tests coverage.
		 */
		FORCE_INLINE [[nodiscard]] pthread_t GetPthread() const noexcept;

		/**************************
		 * @brief Records the pthread identifier after successful pthread creation.
		 *
		 * @param pthread Identifier returned by pthread_create.
		 *
		 * @todo Add tests coverage.
		 */
		FORCE_INLINE void SetPthread(pthread_t pthread) noexcept;

		/**************************
		 * @return True if the recorded pthread has started and has not been joined.
		 *
		 * @todo Add tests coverage.
		 */
		FORCE_INLINE [[nodiscard]] bool IsStarted() const noexcept;

		/**************************
		 * @brief Records whether the pthread has started and has not been joined.
		 *
		 * @param isStarted True while the pthread has started and has not been joined.
		 *
		 * @todo Add tests coverage.
		 */
		FORCE_INLINE void SetStarted(bool isStarted) noexcept;

		/**************************
		 * @pre SetOwner has assigned an owner.
		 *
		 * @return Redirector that owns this endpoint.
		 *
		 * @todo Add tests coverage.
		 */
		FORCE_INLINE [[nodiscard]] Redirector& GetOwner() const noexcept;

		/**************************
		 * @brief Records the Redirector that owns this endpoint.
		 *
		 * @param owner Redirector that outlives this endpoint.
		 *
		 * @todo Add tests coverage.
		 */
		FORCE_INLINE void SetOwner(Redirector& owner) noexcept;
	};

	/**************************
	 * @brief Tracks one capped peer socket and its joinable handling pthread.
	 *
	 * @concurrency No.
	 */
	class Worker {
	private:
		pthread_t m_pthread{};
		Endpoint* m_endpoint{};
		int32_t m_socket{ -1 };
		bool m_isStarted{};
		bool m_isFinished{};

	public:
		/**************************
		 * @brief Initializes an unassigned worker.
		 *
		 * @todo Add tests coverage.
		 */
		FORCE_INLINE Worker() noexcept = default;

		Worker(const Worker&) = delete;
		Worker(Worker&&) = delete;
		Worker& operator=(const Worker&) = delete;
		Worker& operator=(Worker&&) = delete;

		/**************************
		 * @return Socket descriptor, or -1 when no socket is recorded.
		 *
		 * @todo Add tests coverage.
		 */
		FORCE_INLINE [[nodiscard]] int32_t GetSocket() const noexcept;

		/**************************
		 * @brief Records the socket descriptor without closing the previous descriptor.
		 *
		 * @param socket Socket descriptor, or -1 to clear the recorded descriptor.
		 *
		 * @todo Add tests coverage.
		 */
		FORCE_INLINE void SetSocket(int32_t socket) noexcept;

		/**************************
		 * @return Pthread identifier recorded after successful pthread creation.
		 *
		 * @todo Add tests coverage.
		 */
		FORCE_INLINE [[nodiscard]] pthread_t GetPthread() const noexcept;

		/**************************
		 * @brief Records the pthread identifier after successful pthread creation.
		 *
		 * @param pthread Identifier returned by pthread_create.
		 *
		 * @todo Add tests coverage.
		 */
		FORCE_INLINE void SetPthread(pthread_t pthread) noexcept;

		/**************************
		 * @return True if the recorded pthread has started and has not been joined.
		 *
		 * @todo Add tests coverage.
		 */
		FORCE_INLINE [[nodiscard]] bool IsStarted() const noexcept;

		/**************************
		 * @brief Records whether the pthread has started and has not been joined.
		 *
		 * @param isStarted True while the pthread has started and has not been joined.
		 *
		 * @todo Add tests coverage.
		 */
		FORCE_INLINE void SetStarted(bool isStarted) noexcept;

		/**************************
		 * @return True if the worker has completed connection handling.
		 *
		 * @todo Add tests coverage.
		 */
		FORCE_INLINE [[nodiscard]] bool IsFinished() const noexcept;

		/**************************
		 * @brief Records whether the worker has completed connection handling.
		 *
		 * @param isFinished True after connection handling has finished.
		 *
		 * @todo Add tests coverage.
		 */
		FORCE_INLINE void SetFinished(bool isFinished) noexcept;

		/**************************
		 * @pre SetEndpoint has assigned an endpoint.
		 *
		 * @return Endpoint assigned to this worker.
		 *
		 * @todo Add tests coverage.
		 */
		FORCE_INLINE [[nodiscard]] Endpoint& GetEndpoint() const noexcept;

		/**************************
		 * @brief Records the endpoint assigned to this worker.
		 *
		 * @param endpoint Endpoint that outlives this worker assignment.
		 *
		 * @todo Add tests coverage.
		 */
		FORCE_INLINE void SetEndpoint(Endpoint& endpoint) noexcept;
	};

private:
	std::array<Endpoint, 16> m_endpoints{};
	mutable MSAPI::Lock::Atomic m_endpointsLock;
	std::array<Worker, 32> m_workers{};
	mutable MSAPI::Lock::Atomic m_workersLock;
	uint64_t m_endpointCount{};
	uint64_t m_maxConnections{ 32 };
	uint32_t m_receiveTimeoutMs{ 2000 };
	StatusCode m_code{ StatusCode::MovedPermanently };
	std::atomic<bool> m_isRunning{};

public:
	/**************************
	 * @brief Initializes an empty, stopped redirector.
	 *
	 * @locking Is not required.
	 */
	FORCE_INLINE Redirector() noexcept = default;

	/**************************
	 * @brief Stops listeners and joins their pthreads before releasing state.
	 *
	 * @locking Locks in Stop call.
	 */
	FORCE_INLINE ~Redirector() noexcept { Stop(); }

	Redirector(const Redirector&) = delete;
	Redirector(Redirector&&) = delete;
	Redirector& operator=(const Redirector&) = delete;
	Redirector& operator=(Redirector&&) = delete;

	/**************************
	 * @brief Registers an endpoint while stopped; retains registration order.
	 *
	 * @param ip IPv4 address, at most 15 characters.
	 * @param port Explicit listen port, from 1 to 65535. Zero is rejected.
	 * @param destination http://host[:port] or https://host[:port], up to IHandler::DEFAULT_DESTINATION_CAPACITY bytes
	 * without a path or query.
	 *
	 * @locking Locks m_endpointsLock inside.
	 *
	 * @return False if running, invalid configuration, a value does not fit or 16 entries are occupied.
	 *
	 * @test Yes.
	 */
	[[nodiscard]] bool AddEndpoint(std::string_view ip, uint16_t port, std::string_view destination) noexcept;

	/**************************
	 * @brief Opens all configured listeners and starts their accept pthreads. Rolls back completely on failure.
	 *
	 * @param code Redirect status: MovedPermanently or PermanentRedirect.
	 * @param receiveTimeoutMs Maximum wait for data in each blocking receive, in milliseconds; must be positive.
	 * Applies both before the first bytes and while waiting for the rest of an incomplete request header.
	 * @param maxConnections Total connection cap across endpoints, from 1 to 32. Excess peers close immediately.
	 *
	 * @locking Locks m_endpointsLock inside. Locks on Stop call if startup fails.
	 *
	 * @return True if every endpoint starts, false for invalid options or socket/pthread failure.
	 *
	 * @test Yes.
	 *
	 * @todo Stop rollback on failure is called after first lock is realized; consider revising to ensure proper
	 * cleanup.
	 */
	[[nodiscard]] bool Start(StatusCode code = StatusCode::MovedPermanently, uint32_t receiveTimeoutMs = 2000,
		uint64_t maxConnections = 32) noexcept;

	/**************************
	 * @brief Interrupts socket I/O, joins pthreads and releases sockets. Safe to call repeatedly.
	 *
	 * @locking Locks m_endpointsLock, then m_workersLock inside. Keeps m_endpointsLock
	 * until all pthreads exit; m_workersLock is released before joining workers.
	 *
	 * @test Yes.
	 */
	void Stop() noexcept;

private:
	/**************************
	 * @brief Accepts peers and assigns available worker slots.
	 *
	 * @param endpoint Listening endpoint.
	 *
	 * @locking Lock m_workersLock inside while accessing worker slots.
	 *
	 * @test Yes.
	 */
	void AcceptLoop(Endpoint& endpoint) noexcept;

	/**************************
	 * @brief Receives at most 2048 bytes through RecvBuffer with an idle timeout and parses the request with
	 * HTTP::Data's existing constructor. Sends a redirect for a valid request, then closes the connection.
	 * Invalid input or a receive timeout closes the connection without a response. Shutdown interrupts pending reads.
	 *
	 * @param worker Slot owning the peer connection.
	 *
	 * @locking Lock m_workersLock inside while closing the socket and publishing completion.
	 *
	 * @todo Consider adding header only parcing mode to avoid unecessary work.
	 */
	void HandleConnection(Worker& worker) noexcept;

private:
	/**************************
	 * @brief Opens an IPv4 redirect listener with address reuse enabled.
	 *
	 * @param ip Null terminated IPv4 address.
	 * @param port Listener port, or zero to select an available port.
	 *
	 * @locking Is not required.
	 *
	 * @return Listener descriptor, or -1 if address validation or socket setup fails.
	 *
	 * @test Yes.
	 *
	 * @todo Add tests coverage.
	 */
	FORCE_INLINE [[nodiscard]] static int32_t OpenRedirectListener(const char* ip, uint16_t port) noexcept;

	/**************************
	 * @brief Runs an endpoint's accept loop.
	 *
	 * @param endpoint Listening endpoint passed as the pthread context.
	 *
	 * @locking Locks m_workersLock in AcceptLoop call.
	 *
	 * @return Null pointer when the accept loop finishes.
	 *
	 * @test Yes.
	 */
	[[nodiscard]] static void* AcceptPthread(void* endpoint) noexcept;

	/**************************
	 * @brief Runs the recv pthread for the peer assigned to a worker slot.
	 *
	 * @param worker Worker slot passed as the pthread context.
	 *
	 * @locking Locks m_workersLock in HandleConnection call.
	 *
	 * @return Null pointer when peer handling finishes.
	 *
	 * @test Yes.
	 */
	[[nodiscard]] static void* RecvPthread(void* worker) noexcept;
};

/**************************
 * @brief Send HTTP message to socket connection. End of request \r\n\r\n will be added inside. Cannot include body.
 *
 * @param connection Connection to which send HTTP message.
 * @param http HTTP message view.
 *
 * @return True if HTTP message was sent successfully, false otherwise.
 *
 * @test Yes.
 */
[[nodiscard]] bool SendRequest(Connection& connection, std::string_view http);

} // namespace HTTP

} // namespace Protocol

} // namespace MSAPI

#define MSAPI_HANDLER_HTTP_PRESET                                                                                      \
	if (MSAPI::Protocol::HTTP::Data http(recvBuffer); http.IsValid()) {                                                \
		MSAPI_HANDLER_HTTP_PRESET_INTERNAL_PART                                                                        \
	}

#define MSAPI_HANDLER_WEBSOCKET_PRESET                                                                                 \
	if (recvBuffer.GetDataType() == typeid(MSAPI::Protocol::WebSocket::Data).hash_code()) {                            \
		MSAPI::Protocol::WebSocket::IHandler::Collect(                                                                 \
			recvBuffer.GetConnectionData(), MSAPI::Protocol::WebSocket::Data{ recvBuffer });                           \
		return;                                                                                                        \
	}

#define MSAPI_HANDLER_HTTP_PRESET_INTERNAL_PART                                                                        \
	if constexpr (std::is_base_of_v<MSAPI::Protocol::WebSocket::IHandler, std::remove_cvref_t<decltype(*this)>>) {     \
		if (http.IsWebSocketUpgradeRequest()) {                                                                        \
			if (MSAPI::Application::IsRunning()) {                                                                     \
				LOG_PROTOCOL(http.ToString());                                                                         \
				if (!http.SendWebSocketUpgradeResponse(recvBuffer.GetConnectionData()->GetConnection())) {             \
					return;                                                                                            \
				}                                                                                                      \
				recvBuffer.SetToProcessSize(2);                                                                        \
				recvBuffer.SetDataType(typeid(MSAPI::Protocol::WebSocket::Data).hash_code());                          \
				return;                                                                                                \
			}                                                                                                          \
			LOG_PROTOCOL_NEW("Application is not running. {}", http.ToString());                                       \
			return;                                                                                                    \
		}                                                                                                              \
                                                                                                                       \
		if (http.IsWebSocketUpgradeResponse()) {                                                                       \
			if (MSAPI::Application::IsRunning()) {                                                                     \
				LOG_PROTOCOL(http.ToString());                                                                         \
				recvBuffer.SetToProcessSize(2);                                                                        \
				recvBuffer.SetDataType(typeid(MSAPI::Protocol::WebSocket::Data).hash_code());                          \
				return;                                                                                                \
			}                                                                                                          \
			LOG_PROTOCOL_NEW("Application is not running. {}", http.ToString());                                       \
			return;                                                                                                    \
		}                                                                                                              \
	}                                                                                                                  \
                                                                                                                       \
	MSAPI::Protocol::HTTP::IHandler::Collect(recvBuffer.GetConnectionData(), http);                                    \
	return;

#endif // MSAPI_PROTOCOL_HTTP_H