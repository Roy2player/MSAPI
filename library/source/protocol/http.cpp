/**************************
 * @file        http.cpp
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
 */

#include "http.h"
#include "../help/helper.h"
#include "../help/log.h"
#include "../help/sha1.inl"
#include "../server/application.h"
#include <charconv>
#include <cstring>
#include <exception>
#include <iomanip>
#include <sys/socket.h>
#include <sys/time.h>

namespace MSAPI {

namespace Protocol {

namespace HTTP {

[[nodiscard]] bool IsHeaderValue(const std::string_view text) noexcept
{
	return text.find_first_of("\r\n") == std::string_view::npos && text.find('\0') == std::string_view::npos;
}

[[nodiscard]] uint64_t BuildResponseHeader(const std::span<char> out, const std::string_view version,
	const StatusCode code, const std::string_view contentType, const uint64_t length, const bool closeConnection,
	const std::string_view location) noexcept
{
	if (code == StatusCode::Undefined || code == StatusCode::Max) [[unlikely]] {
		return 0;
	}

	const auto reason{ EnumToString(code) };
	if (reason == "Unknown") [[unlikely]] {
		return 0;
	}

	if (out.empty() || (version != "1.0" && version != "1.1") || !IsHeaderValue(contentType)
		|| !IsHeaderValue(location)) {

		return 0;
	}

	// Every append reserves one byte for the terminator, so truncated headers are never returned.
	uint64_t used{};
	bool fits{ true };
	const auto append{ [out, &used, &fits](const std::string_view value) noexcept {
		if (!fits || value.size() >= out.size() - used) [[unlikely]] {
			fits = false;
			return;
		}

		std::copy(value.begin(), value.end(), out.begin() + static_cast<int64_t>(used));
		used += value.size();
	} };

	std::array<char, 24> number{};
	const auto status{ std::to_chars(number.data(), number.data() + number.size(), static_cast<uint16_t>(code)) };
	append("HTTP/");
	append(version);
	append(" ");
	append({ number.data(), static_cast<uint64_t>(status.ptr - number.data()) });
	append(" ");
	append(reason);
	append("\r\n");

	if (!contentType.empty()) {
		append("Content-Type: ");
		append(contentType);
		append("; charset=utf-8\r\n");
	}

	if (!location.empty()) {
		append("Location: ");
		append(location);
		append("\r\n");
	}

	append(closeConnection ? "Connection: close\r\n" : "Connection: keep-alive\r\n");
	if (!closeConnection) {
		append("Keep-Alive: timeout=0,max=0\r\n");
	}

	const auto count{ std::to_chars(number.data(), number.data() + number.size(), length) };
	append("Content-Length: ");
	append({ number.data(), static_cast<uint64_t>(count.ptr - number.data()) });
	append("\r\n\r\n");

	out[fits ? used : 0] = '\0';
	return fits ? used : 0;
}

[[nodiscard]] uint64_t BuildRedirectResponse(const std::span<char> out, const std::string_view location,
	const StatusCode code, const bool closeConnection, const std::string_view version) noexcept
{
	if (out.empty()) {
		return 0;
	}

	out.front() = '\0';
	if ((code != StatusCode::MovedPermanently && code != StatusCode::Found && code != StatusCode::SeeOther
			&& code != StatusCode::TemporaryRedirect && code != StatusCode::PermanentRedirect)
		|| location.empty()) [[unlikely]] {

		return 0;
	}

	return BuildResponseHeader(out, version, code, /*contentType=*/{}, /*length=*/0, closeConnection, location);
}

/*---------------------------------------------------------------------------------
Data
---------------------------------------------------------------------------------*/

Data::Data(MSAPI::RecvBuffer& recvBuffer)
{
	bool isHeaders{};
	bool isHtmlFormat{ true };
	bool isUrl{};
	bool isHTTPtype{};
	bool isVersion{};
	bool isCode{};
	bool isCodeText{};
	bool isKeyLine{ true };
	std::string value{ "" };
	std::string key{ "" };

	const auto bufferSize{ recvBuffer.RecvAdditionalPeek(2048) };
	if (bufferSize == 0) [[unlikely]] {
		return;
	}
	const void* buffer{ recvBuffer.GetData() };

	const auto fillHeaderIdentifier = [buffer](size_t& index, bool& currentType, std::string& currentValue,
										  bool& nextType, const char separator = '/', const bool includeSpace = false) {
		if (separator == '/') {
			if (static_cast<const char*>(buffer)[index] != ' ') {
				if (static_cast<const char*>(buffer)[index] != '/' && static_cast<const char*>(buffer)[index] != '\r') {
					currentValue += static_cast<const char*>(buffer)[index];
				}
				else {
					if (static_cast<const char*>(buffer)[index] == '\r') {
						--index;
					}
					currentType = false;
					nextType = true;
				}
			}
			else if (includeSpace) {
				currentValue += static_cast<const char*>(buffer)[index];
			}
		}
		else {
			if (static_cast<const char*>(buffer)[index] != separator
				&& static_cast<const char*>(buffer)[index] != '\r') {

				currentValue += static_cast<const char*>(buffer)[index];
			}
			else {
				if (static_cast<const char*>(buffer)[index] == '\r') {
					--index;
				}
				currentType = false;
				nextType = true;
			}
		}
	};

	{
		bool hasSpace{};
		bool validity{};
		for (size_t index{}; index < bufferSize; ++index) {
			if (static_cast<const char*>(buffer)[index] == ' ') {
				hasSpace = true;
			}
			else if (static_cast<const char*>(buffer)[index] == '/') {
				validity = true;
				break;
			}
		}

		if (validity) {
			if (hasSpace) {
				m_isRequest = true;
			}
		}
		else {
			LOG_ERROR_NEW("Invalid HTTP message format, connection id: {} ", recvBuffer.GetConnectionId());
			return;
		}
	}

	if (m_isRequest) {
		bool isMessageType{ true };
		for (size_t index{}; index < bufferSize; ++index) {
			if (!isHeaders) {
				if (static_cast<const char*>(buffer)[index] != '\n') {
					if (isMessageType) {
						fillHeaderIdentifier(index, isMessageType, m_messageType, isUrl);
					}
					else if (isUrl) {
						if (m_url.size() == 0) {
							index--;
						}
						if (static_cast<const char*>(buffer)[index] != ' ') {
							if (static_cast<const char*>(buffer)[index] == '?'
								|| static_cast<const char*>(buffer)[index] == '#') {

								// Keep the query separately while retaining path-only routing and ignoring fragments.
								const bool query{ static_cast<const char*>(buffer)[index] == '?' };
								bool fragment{};
								while (index < bufferSize && static_cast<const char*>(buffer)[index] != ' ') {
									const char ch{ static_cast<const char*>(buffer)[index++] };
									fragment = fragment || ch == '#';
									if (query && !fragment) {
										m_query += ch;
									}
								}
								if (index == bufferSize) {
									return;
								}

								isUrl = false;
								isHTTPtype = true;
								continue;
							}

							m_url += static_cast<const char*>(buffer)[index];

							if (!isHtmlFormat && static_cast<const char*>(buffer)[index] != '/') {
								m_format += static_cast<const char*>(buffer)[index];
							}
							if (static_cast<const char*>(buffer)[index] == '.') {
								isHtmlFormat = false;
							}
						}
						else {
							isUrl = false;
							isHTTPtype = true;
						}
					}
					else if (isHTTPtype) {
						fillHeaderIdentifier(index, isHTTPtype, m_HTTPtype, isVersion);
					}
					else if (isVersion) {
						fillHeaderIdentifier(index, isVersion, m_version, isHeaders);
					}
					continue;
				}

				LOG_ERROR_NEW(
					"Unexpected symbol inside request start line, connection id: {}", recvBuffer.GetConnectionId());
				return;
			}

			if (static_cast<const char*>(buffer)[index] != '\n') {
				if (static_cast<const char*>(buffer)[index] == ':'
					&& index + 2 /* 1 + 1 because of can be used below */ < bufferSize
					&& static_cast<const char*>(buffer)[index + 1] == ' ') {

					index += 2;
					isKeyLine = false;
				}

				if (static_cast<const char*>(buffer)[index] != '\r') {
					if (isKeyLine) {
						key += static_cast<const char*>(buffer)[index];
					}
					else {
						value += static_cast<const char*>(buffer)[index];
					}
				}

				continue;
			}

			if (!isKeyLine) {
				m_headersMap.insert({ std::move(key), std::move(value) });
				key.clear();
				value.clear();
			}

			if (static_cast<const char*>(buffer)[index - 1] == '\r' && index + 2 < bufferSize
				&& static_cast<const char*>(buffer)[index + 1] == '\r'
				&& static_cast<const char*>(buffer)[index + 2] == '\n') {

				m_messageSize = index + 3 /* 2 + 1 because of first index is 0 */;
				break;
			}

			isKeyLine = true;
		}

		if (m_messageType != "GET" && m_messageType != "POST" && m_messageType != "PUT" && m_messageType != "DELETE"
			&& m_messageType != "HEAD" && m_messageType != "CONNECT" && m_messageType != "OPTIONS"
			&& m_messageType != "TRACE" && m_messageType != "PATCH") {

			LOG_ERROR_NEW(
				"Invalid HTTP message type \"{}\", connection id: {}", m_messageType, recvBuffer.GetConnectionId());
			return;
		}
		if (m_HTTPtype != "HTTP" && m_HTTPtype != "HTTPS") {
			LOG_ERROR_NEW("Invalid HTTP type \"{}\", connection id: {}", m_HTTPtype, recvBuffer.GetConnectionId());
			return;
		}
		if (m_version.empty()) {
			LOG_ERROR_NEW("Empty HTTP version, connection id: {}", recvBuffer.GetConnectionId());
			return;
		}
		if (m_url.empty()) {
			LOG_ERROR_NEW("Empty HTTP url, connection id: {}", recvBuffer.GetConnectionId());
			return;
		}

		if (m_format.empty()) {
			m_format = "html";
		}
		m_isValid = true;
	}
	else {
		isHTTPtype = true;
		for (size_t index{}; index < bufferSize; ++index) {
			if (isHeaders) {
				if (static_cast<const char*>(buffer)[index] != '\n') {
					if (static_cast<const char*>(buffer)[index] == ':'
						&& index + 2 /* 1 + 1 because of can be used below */ < bufferSize
						&& static_cast<const char*>(buffer)[index + 1] == ' ') {

						index += 2;
						isKeyLine = false;
					}

					if (static_cast<const char*>(buffer)[index] != '\r') {
						if (isKeyLine) {
							key += static_cast<const char*>(buffer)[index];
						}
						else {
							value += static_cast<const char*>(buffer)[index];
						}
					}

					continue;
				}

				if (!isKeyLine) {
					m_headersMap.insert({ std::move(key), std::move(value) });
					key.clear();
					value.clear();
				}

				if (index + 1 < bufferSize && static_cast<const char*>(buffer)[index + 1] == '\r') {
					if (index + 2 < bufferSize && static_cast<const char*>(buffer)[index + 2] == '\n') {
						m_messageSize = index + 3 /* 2 + 1 because of first index is 0 */;
						break;
					}
					m_messageSize = index + 2 /* 1 + 1 because of first index is 0 */;
					break;
				}

				isKeyLine = true;
				continue;
			}

			if (static_cast<const char*>(buffer)[index] != '\n') {
				if (isHTTPtype) {
					fillHeaderIdentifier(index, isHTTPtype, m_HTTPtype, isVersion);
				}
				else if (isVersion) {
					fillHeaderIdentifier(index, isVersion, m_version, isCode, ' ');
				}
				else if (isCode) {
					fillHeaderIdentifier(index, isCode, m_code, isCodeText, ' ');
				}
				else if (isCodeText) {
					fillHeaderIdentifier(index, isCodeText, m_codeText, isHeaders, '/', true);
				}
				continue;
			}

			++index;
			continue;
		}

		if (m_HTTPtype != "HTTP" && m_HTTPtype != "HTTPS") {
			LOG_ERROR_NEW("Invalid HTTP type: {}, connection id: {}", m_HTTPtype, recvBuffer.GetConnectionId());
			return;
		}
		if (m_version.empty()) {
			LOG_ERROR_NEW("Empty HTTP version, connection id: {}", recvBuffer.GetConnectionId());
			return;
		}
		if (m_code.empty()) {
			LOG_ERROR_NEW("Empty HTTP code, connection id: {}", recvBuffer.GetConnectionId());
			return;
		}
		if (m_codeText.empty()) {
			LOG_ERROR_NEW("Empty HTTP code text, connection id: {}", recvBuffer.GetConnectionId());
			return;
		}
		m_isValid = true;
	}

	if (const auto* contentLengthPtr{ GetValue("Content-Length") }; contentLengthPtr != nullptr) {
		size_t contentLength{};
		const auto error{ std::from_chars(
			contentLengthPtr->data(), contentLengthPtr->data() + contentLengthPtr->size(), contentLength)
							  .ec };
		if (error != std::errc{}) {
			LOG_ERROR("Cannot convert string to size_t: " + *contentLengthPtr
				+ ". Error: " + std::make_error_code(error).message());
			return;
		}

		const size_t httpMessageSize{ contentLength + m_messageSize };
		if (!recvBuffer.RecvAdditional(httpMessageSize)) [[unlikely]] {
			return;
		}

		m_body = std::string{ &reinterpret_cast<const char*>(recvBuffer.GetData())[m_messageSize], contentLength };
		m_messageSize = httpMessageSize;
	}
	else {
		(void)recvBuffer.RecvAdditional(m_messageSize);
	}
}

const std::map<std::string, std::string, std::less<>>& Data::GetHeadersMap() const noexcept { return m_headersMap; }

size_t Data::GetSizeHeadersMap() const noexcept { return m_headersMap.size(); }

const std::string* Data::GetValue(const std::string_view key) const
{
	if (const auto it = m_headersMap.find(key); it != m_headersMap.end()) {
		return &it->second;
	}

	for (const auto& [name, value] : m_headersMap) {
		if (MSAPI::Helper::EqualIgnoreCase(name, key)) {
			return &value;
		}
	}

	return nullptr;
}

bool Data::IsValid() const noexcept { return m_isValid; }

const std::string& Data::GetVersion() const noexcept { return m_version; }

const std::string& Data::GetTypeMessage() const noexcept { return m_messageType; }

const std::string& Data::GetHTTPType() const noexcept { return m_HTTPtype; }

size_t Data::GetMessageSize() const noexcept { return m_messageSize; }

bool Data::IsRequest() const noexcept { return m_isRequest; }

[[nodiscard]] SString<64> Data::GetContentType() const
{
	std::string_view format;
	if (m_format == "js") {
		format = "application/javascript";
	}
	else if (m_format == "dat") {
		format = "application/octet-stream";
	}
	else if (m_format == "ogg") {
		format = "application/ogg";
	}
	else if (m_format == "pdf") {
		format = "application/pdf";
	}
	else if (m_format == "xhtml") {
		format = "application/xhtml+xml";
	}
	else if (m_format == "json") {
		format = "application/json";
	}
	else if (m_format == "ldjson") {
		format = "application/ld+json";
	}
	else if (m_format == "xml") {
		format = "application/xml";
	}
	else if (m_format == "zip") {
		format = "application/zip";
	}
	else if (m_format == "mp3") {
		format = "audio/mpeg";
	}
	else if (m_format == "wma") {
		format = "audio/x-ms-wma";
	}
	else if (m_format == "wav") {
		format = "audio/x-wav";
	}
	else if (m_format == "gif") {
		format = "image/gif";
	}
	else if (m_format == "jpeg" || m_format == "jpg") {
		format = "image/jpeg";
	}
	else if (m_format == "png") {
		format = "image/png";
	}
	else if (m_format == "tiff") {
		format = "image/tiff";
	}
	else if (m_format == "ico") {
		format = "image/x-icon";
	}
	else if (m_format == "djvu") {
		format = "image/vnd.djvu";
	}
	else if (m_format == "svg") {
		format = "image/svg+xml";
	}
	else if (m_format == "bmp") {
		format = "image/bmp";
	}
	else if (m_format == "webp") {
		format = "image/webp";
	}
	else if (m_format == "css") {
		format = "text/css";
	}
	else if (m_format == "csv") {
		format = "text/csv";
	}
	else if (m_format == "html") {
		format = "text/html";
	}
	else if (m_format == "txt") {
		format = "text/plain";
	}
	else if (m_format == "mpeg") {
		format = "video/mpeg";
	}
	else if (m_format == "mp4") {
		format = "video/mp4";
	}
	else if (m_format == "mov") {
		format = "video/quicktime";
	}
	else if (m_format == "wmv") {
		format = "video/x-ms-wmv";
	}
	else if (m_format == "avi") {
		format = "video/x-msvideo";
	}
	else if (m_format == "webm") {
		format = "video/webm";
	}
	else {
		LOG_WARNING_NEW("Format is not supported: \"{}\"", m_format);
		SString<64> contentType;
		if (!contentType.Copy("text/") || !contentType.Concatenate(m_format)) [[unlikely]] {
			LOG_WARNING_NEW("Content type exceeds capacity: {}, format: \"{}\"", contentType.GetCapacity(), m_format);
			(void)contentType.Copy("application/octet-stream");
		}

		return contentType;
	}

	SString<64> contentType;
	if (!contentType.Copy(format)) [[unlikely]] {
		LOG_WARNING_NEW("Content type exceeds capacity: {}, content type: \"{}\"", contentType.GetCapacity(), format);
		(void)contentType.Copy("application/octet-stream");
	}

	return contentType;
}

const std::string& Data::GetUrl() const noexcept { return m_url; }

[[nodiscard]] std::string_view Data::GetQuery() const noexcept { return m_query; }

[[nodiscard]] std::string_view Data::GetHost() const noexcept
{
	const auto* value{ GetValue("Host") };
	if (value == nullptr) {
		return {};
	}

	// The parser skips only one space after the colon and preserves remaining whitespace, e.g.
	// "Host:  example.com:8080 \t" or "Host: \t[::1]:8080 ". Trim before detecting IPv6 brackets and stripping the
	// port.
	const auto host{ MSAPI::Helper::Trim(*value) };
	// A bracketed IPv6 address can contain colons; only a port after its closing bracket is stripped.
	if (!host.empty() && host.front() == '[') {
		const auto bracket{ host.find(']') };
		return bracket == std::string_view::npos ? host : host.substr(0, bracket + 1);
	}

	return host.substr(0, host.find(':'));
}

[[nodiscard]] bool Data::IsConnectionClose() const noexcept
{
	bool keepAlive{};
	for (const auto& [key, value] : m_headersMap) {
		if (MSAPI::Helper::EqualIgnoreCase(key, "Connection")) {
			if (MSAPI::Helper::ContainsCommaSeparatedStringIgnoreCase(value, "close")) {
				return true;
			}

			keepAlive = keepAlive || MSAPI::Helper::ContainsCommaSeparatedStringIgnoreCase(value, "keep-alive");
		}
	}

	return m_version == "1.0" && !keepAlive;
}

const std::string& Data::GetFormat() const noexcept { return m_format; }

const std::string& Data::GetCode() const noexcept { return m_code; }

const std::string& Data::GetCodeText() const noexcept { return m_codeText; }

[[nodiscard]] bool Data::SendPrepared(
	Connection& connection, const std::string_view response, const bool closeConnection) const
{
	const auto sent{ connection.Send(response.data(), response.size(), MSG_CONFIRM | MSG_NOSIGNAL) };

	if (closeConnection) {
		connection.Close();
	}

	return sent;
}

[[nodiscard]] bool Data::SendResponse(Connection& connection, const std::string_view body,
	const std::string_view contentType, const bool closeConnection) const
{
	// Keep the inferred content type alive while the header builder reads its view.
	const auto inferredContentType{ contentType.empty() ? GetContentType() : SString<64>{} };
	SString<4096> header{};
	const auto length{ BuildResponseHeader({ header.GetBuffer(), header.GetCapacity() }, m_version, StatusCode::Ok,
		contentType.empty() ? inferredContentType.Get() : contentType, body.size(), closeConnection) };
	if (length == 0) {
		return false;
	}

	std::string response{ header.GetBuffer(), length };
	response += body;

	return SendPrepared(connection, response, closeConnection);
}

const std::string& Data::GetBody() const noexcept { return m_body; }

std::ostream& operator<<(std::ostream& out, Data& data) { return out << data.ToString(); }

Data::operator std::string() const { return ToString(); }

std::string Data::ToString() const
{
	const auto maxKeySize = [this]() {
		size_t out{ 12 };
		for (const auto& [key, value] : m_headersMap) {
			if (out < key.length()) {
				out = key.length();
			}
		}
		return static_cast<int>(out);
	}();

#define format std::left << std::setw(maxKeySize)

	std::stringstream stream;
	stream << "HTTP message:\n{" << std::fixed << std::setprecision(16) << "\n\t" << format << "is valid" << " : "
		   << _S(m_isValid) << "\n\t" << format << "type" << " : " << (m_isRequest ? "Request" : "Response") << "\n\t"
		   << format << "message type" << " : " << m_messageType << "\n\t" << format << "url" << " : " << m_url
		   << "\n\t" << format << "HTTP type" << " : " << m_HTTPtype << "\n\t" << format << "version" << " : "
		   << m_version << "\n\t" << format << "format" << " : " << m_format << "\n\t" << format << "code" << " : "
		   << m_code << "\n\t" << format << "code text" << " : " << m_codeText << "\n\t" << format << "message size"
		   << " : " << m_messageSize << "\n\t" << format << "Headers" << " :\n{";
	for (const auto& [key, value] : m_headersMap) {
		stream << "\n\t" << format << key << " : " << value;
	}
	stream << "\n}\n}";

#undef format

	return stream.str();
}

[[nodiscard]] bool Data::SendSource(Connection& connection, const std::string_view path,
	const std::string_view contentType, const bool closeConnection) const
{
	// Filesystem paths cannot contain NUL; opening one would truncate it and could serve a different file.
	if (path.find('\0') != std::string_view::npos) {
		(void)Send404(connection, /*body=*/{}, /*contentType=*/{}, closeConnection);
		return false;
	}

	std::ifstream file{ std::string{ path }, std::ios::binary };
	if (!file.is_open()) {
		LOG_WARNING_NEW("File is not opened, path: \"{}\"", path);
		(void)Send404(connection, /*body=*/{}, /*contentType=*/{}, closeConnection);
		return false;
	}

	const std::string body{ std::istreambuf_iterator<char>{ file }, {} };
	if (file.bad()) {
		(void)Send404(connection, /*body=*/{}, /*contentType=*/{}, closeConnection);
		return false;
	}

	return SendResponse(connection, body, contentType, closeConnection);
}

[[nodiscard]] bool Data::Send404(Connection& connection, const std::string_view body,
	const std::string_view contentType, const bool closeConnection) const
{
	SString<4096> header{};
	const auto length{ BuildResponseHeader({ header.GetBuffer(), header.GetCapacity() }, m_version,
		StatusCode::NotFound, contentType.empty() ? "text/html" : contentType, body.size(), closeConnection) };
	if (length == 0) {
		return false;
	}

	std::string response{ header.GetBuffer(), length };
	response += body;

	return SendPrepared(connection, response, closeConnection);
}

[[nodiscard]] bool Data::SendRedirect(
	Connection& connection, const std::string_view location, const StatusCode code, const bool closeConnection) const
{
	SString<4096> response{};
	const auto length{ BuildRedirectResponse(
		{ response.GetBuffer(), response.GetCapacity() }, location, code, closeConnection, m_version) };
	if (length == 0) {
		return false;
	}

	return SendPrepared(connection, { response.GetBuffer(), length }, closeConnection);
}

[[nodiscard]] bool Data::IsWebSocketUpgradeRequest() const noexcept
{
	if (const auto* connection{ GetValue("Connection") }; connection != nullptr && *connection == "Upgrade") {
		if (const auto* upgrade{ GetValue("Upgrade") }; upgrade != nullptr && *upgrade == "websocket") {
			if (const auto* webSocketKey{ GetValue("Sec-WebSocket-Key") }; webSocketKey != nullptr) {
				return true;
			}
		}
	}

	return false;
}

[[nodiscard]] bool Data::IsWebSocketUpgradeResponse() const noexcept
{
	if (const auto* connection{ GetValue("Connection") }; connection != nullptr && *connection == "Upgrade") {
		if (const auto* upgrade{ GetValue("Upgrade") }; upgrade != nullptr && *upgrade == "websocket") {
			if (const auto* webSocketAccept{ GetValue("Sec-WebSocket-Accept") }; webSocketAccept != nullptr) {
				return true;
			}
		}
	}

	return false;
}

[[nodiscard]] bool Data::SendWebSocketUpgradeResponse(Connection& connection) const
{
	const auto* webSocketKey{ GetValue("Sec-WebSocket-Key") };
	if (webSocketKey == nullptr) [[unlikely]] {
		LOG_WARNING_NEW(
			"Sec-WebSocket-Key header is missing in WebSocket upgrade request, connection id: {}", connection.GetId());
		(void)Send404(connection);
		return false;
	}

	if (const auto* webSocketVersion{ GetValue("Sec-WebSocket-Version") };
		webSocketVersion == nullptr || *webSocketVersion != "13") {
		LOG_WARNING_NEW("Unsupported Sec-WebSocket-Version header in WebSocket upgrade request, connection id: {}",
			connection.GetId());
		(void)Send404(connection);
		return false;
	}

	const auto webSocketKeySize{ webSocketKey->size() };
	const auto acceptKeySize{ webSocketKeySize + WEB_SOCKET_GUID.size() };
	if (acceptKeySize > 128) [[unlikely]] {
		LOG_WARNING_NEW("Sec-WebSocket-Key header value is too long, connection id: {}", connection.GetId());
		(void)Send404(connection);
		return false;
	}

	std::array<char, 128> bufferEncode{};

	std::array<char, 128> acceptKey{};
	std::copy(webSocketKey->begin(), webSocketKey->end(), acceptKey.begin());
	std::copy(WEB_SOCKET_GUID.begin(), WEB_SOCKET_GUID.end(), acceptKey.begin() + webSocketKeySize);

	MSAPI::Sha1 sha1;
	sha1.Update(std::span<const uint8_t>{ reinterpret_cast<const uint8_t*>(acceptKey.data()), acceptKeySize });
	const auto hash{ sha1.Final<MSAPI::Sha1::DO_NOT_RESET>() };
	const auto acceptKeyHash{ MSAPI::Helper::Base64Encode(
		std::span<const int8_t>{ reinterpret_cast<const int8_t*>(hash.data()), hash.size() }, bufferEncode) };

	const std::string response{ std::format("{}/{} 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: "
											"Upgrade\r\nSec-WebSocket-Accept: {}\r\nSec-WebSocket-Version: 13\r\n\r\n",
		m_HTTPtype, m_version, acceptKeyHash) };

	const auto size{ response.length() };
	LOG_PROTOCOL_NEW("Send response size: {} to connection id: {}", size, connection.GetId());

	return connection.Send(response.c_str(), size, MSG_CONFIRM);
}

/*---------------------------------------------------------------------------------
IHandler
---------------------------------------------------------------------------------*/

IHandler::IHandler(const MSAPI::Application* application)
	: m_application{ application }
{
}

[[nodiscard]] bool IHandler::AddPath(const std::string_view key, const std::string_view root) noexcept
{
	if (key.empty() || key.size() > 32 || key.find('\0') != std::string_view::npos) [[unlikely]] {
		LOG_WARNING_NEW("Invalid source key: \"{}\"", key);
		return false;
	}

	if (root.empty() || root.size() > DEFAULT_PATH_CAPACITY || root.find('\0') != std::string_view::npos
		|| (root.back() != '/' && root.size() == DEFAULT_PATH_CAPACITY)) [[unlikely]] {

		LOG_WARNING_NEW("Invalid source root: \"{}\"", root);
		return false;
	}

	SString<DEFAULT_PATH_CAPACITY> entryRoot{};
	if (!entryRoot.Copy(root) || (root.back() != '/' && !entryRoot.Concatenate("/"))) {
		return false;
	}

	try {
		const MSAPI::Lock::AtomicRW::Guard<MSAPI::Lock::WRITE> _{ m_pathsLock };
		m_paths.emplace(key, std::move(entryRoot));
	}
	catch (const std::exception& error) {
		LOG_ERROR_NEW("Source registration failed, key: \"{}\", root: \"{}\", error: \"{}\"", key, root, error.what());
		return false;
	}

	return true;
}

[[nodiscard]] bool IHandler::RemovePath(const std::string_view key, const std::string_view root) noexcept
{
	// Apply the same trailing-slash normalization as AddPath before comparing registrations.
	SString<DEFAULT_PATH_CAPACITY> normalizedRoot{};
	if (root.empty() || root.find('\0') != std::string_view::npos || !normalizedRoot.Copy(root)
		|| (root.back() != '/' && !normalizedRoot.Concatenate("/"))) [[unlikely]] {

		LOG_WARNING_NEW("Invalid source root for removal: \"{}\"", root);
		return false;
	}

	const MSAPI::Lock::AtomicRW::Guard<MSAPI::Lock::WRITE> _{ m_pathsLock };
	const auto [begin, end]{ m_paths.equal_range(key) };
	for (auto it{ begin }; it != end; ++it) {
		if (it->second.Get() == normalizedRoot.Get()) {
			m_paths.erase(it);
			return true;
		}
	}

	return false;
}

void IHandler::ClearPaths() noexcept
{
	const MSAPI::Lock::AtomicRW::Guard<MSAPI::Lock::WRITE> _{ m_pathsLock };
	m_paths.clear();
}

void IHandler::Collect(const std::shared_ptr<Connection::Data>& connectionData, const Data& data)
{
	if (m_application->IsRunning()) {
		LOG_PROTOCOL(data.ToString());
		HandleHttp(connectionData, data);
		return;
	}

	LOG_PROTOCOL("Application is not running. " + data.ToString());
}

/*---------------------------------------------------------------------------------
Redirector::Endpoint
---------------------------------------------------------------------------------*/

FORCE_INLINE [[nodiscard]] const char* Redirector::Endpoint::GetIp() const noexcept { return m_ip.Get().data(); }

FORCE_INLINE [[nodiscard]] bool Redirector::Endpoint::SetIp(const std::string_view ip) noexcept
{
	in_addr address{};
	return m_ip.Copy(ip) && m_ip.NullTerminate() && inet_pton(AF_INET, m_ip.Get().data(), &address) == 1;
}

FORCE_INLINE [[nodiscard]] uint16_t Redirector::Endpoint::GetPort() const noexcept { return m_port; }

FORCE_INLINE void Redirector::Endpoint::SetPort(const uint16_t port) noexcept { m_port = port; }

FORCE_INLINE [[nodiscard]] std::string_view Redirector::Endpoint::GetDestination() const noexcept
{
	return m_destination.Get();
}

FORCE_INLINE [[nodiscard]] bool Redirector::Endpoint::SetDestination(const std::string_view destination) noexcept
{
	return m_destination.Copy(destination);
}

FORCE_INLINE [[nodiscard]] int32_t Redirector::Endpoint::GetSocket() const noexcept { return m_socket; }

FORCE_INLINE void Redirector::Endpoint::SetSocket(const int32_t socket) noexcept { m_socket = socket; }

FORCE_INLINE [[nodiscard]] pthread_t Redirector::Endpoint::GetPthread() const noexcept { return m_pthread; }

FORCE_INLINE void Redirector::Endpoint::SetPthread(const pthread_t pthread) noexcept { m_pthread = pthread; }

FORCE_INLINE [[nodiscard]] bool Redirector::Endpoint::IsStarted() const noexcept { return m_isStarted; }

FORCE_INLINE void Redirector::Endpoint::SetStarted(const bool isStarted) noexcept { m_isStarted = isStarted; }

FORCE_INLINE [[nodiscard]] Redirector& Redirector::Endpoint::GetOwner() const noexcept { return *m_owner; }

FORCE_INLINE void Redirector::Endpoint::SetOwner(Redirector& owner) noexcept { m_owner = &owner; }

/*---------------------------------------------------------------------------------
Redirector::Worker
---------------------------------------------------------------------------------*/

FORCE_INLINE [[nodiscard]] int32_t Redirector::Worker::GetSocket() const noexcept { return m_socket; }

FORCE_INLINE void Redirector::Worker::SetSocket(const int32_t socket) noexcept { m_socket = socket; }

FORCE_INLINE [[nodiscard]] pthread_t Redirector::Worker::GetPthread() const noexcept { return m_pthread; }

FORCE_INLINE void Redirector::Worker::SetPthread(const pthread_t pthread) noexcept { m_pthread = pthread; }

FORCE_INLINE [[nodiscard]] bool Redirector::Worker::IsStarted() const noexcept { return m_isStarted; }

FORCE_INLINE void Redirector::Worker::SetStarted(const bool isStarted) noexcept { m_isStarted = isStarted; }

FORCE_INLINE [[nodiscard]] bool Redirector::Worker::IsFinished() const noexcept { return m_isFinished; }

FORCE_INLINE void Redirector::Worker::SetFinished(const bool isFinished) noexcept { m_isFinished = isFinished; }

FORCE_INLINE [[nodiscard]] Redirector::Endpoint& Redirector::Worker::GetEndpoint() const noexcept
{
	return *m_endpoint;
}

FORCE_INLINE void Redirector::Worker::SetEndpoint(Endpoint& endpoint) noexcept { m_endpoint = &endpoint; }

/*---------------------------------------------------------------------------------
Redirector
---------------------------------------------------------------------------------*/

FORCE_INLINE [[nodiscard]] int32_t Redirector::OpenRedirectListener(const char* const ip, const uint16_t port) noexcept
{
	const auto listener{ socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, IPPROTO_TCP) };
	if (listener == -1) [[unlikely]] {
		LOG_WARNING_NEW("Redirect socket creation failed. Error №{}: {}", errno, std::strerror(errno));
		return -1;
	}

	const int32_t reuse{ 1 };
	sockaddr_in address{};
	address.sin_family = AF_INET;
	address.sin_port = htons(port);

	if (inet_pton(AF_INET, ip, &address.sin_addr) != 1) [[unlikely]] {
		LOG_WARNING_NEW("Invalid IPv4 address for redirect listener: {}", ip);
		(void)close(listener);
		return -1;
	}

	if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1
		|| bind(listener, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) == -1
		|| listen(listener, SOMAXCONN) == -1) [[unlikely]] {

		LOG_WARNING_NEW(
			"Redirect listener setup failed, address {}:{}. Error №{}: {}", ip, port, errno, std::strerror(errno));
		(void)close(listener);
		return -1;
	}

	return listener;
}

[[nodiscard]] bool Redirector::AddEndpoint(
	const std::string_view ip, const uint16_t port, const std::string_view destination) noexcept
{
	if (port == 0 || ip.empty() || ip.size() >= 16 || ip.find('\0') != std::string_view::npos
		|| destination.size() > IHandler::DEFAULT_DESTINATION_CAPACITY) {

		LOG_WARNING_NEW("Invalid redirect endpoint, address {}:{}, destination: \"{}\"", ip, port, destination);
		return false;
	}

	const auto delimiter{ destination.find("://") };
	if (delimiter == std::string_view::npos) [[unlikely]] {
		LOG_WARNING_NEW("Failed to add redirect endpoint, address {}:{}, destination: \"{}\". Missing scheme.", ip,
			port, destination);
		return false;
	}

	if (!ValidateLocation(destination.substr(0, delimiter), destination.substr(delimiter + 3), /*url=*/"/"))
		[[unlikely]] {

		LOG_WARNING_NEW("Failed to add redirect endpoint, address {}:{}, destination: \"{}\". Invalid location.", ip,
			port, destination);
		return false;
	}

	{
		if (m_isRunning.load(std::memory_order_acquire)) [[unlikely]] {
			LOG_DEBUG_NEW(
				"Failed to add redirect endpoint while redirector is running, address {}:{}, destination: \"{}\"", ip,
				port, destination);
			return false;
		}

		const MSAPI::Lock::Atomic::Guard _{ m_endpointsLock };
		if (m_isRunning.load(std::memory_order_acquire) || m_endpointCount == m_endpoints.size()) {
			return false;
		}

		auto& endpoint{ m_endpoints[m_endpointCount] };
		if (!endpoint.SetIp(ip) || !endpoint.SetDestination(destination)) {
			return false;
		}

		endpoint.SetPort(port);
		endpoint.SetOwner(*this);
		++m_endpointCount;
	}

	LOG_INFO_NEW("Added redirect endpoint, address {}:{}, destination: \"{}\"", ip, port, destination);

	return true;
}

[[nodiscard]] bool Redirector::Start(
	const StatusCode code, const uint32_t receiveTimeoutMs, const uint64_t maxConnections) noexcept
{
	do {
		const MSAPI::Lock::Atomic::Guard _{ m_endpointsLock };
		if (m_isRunning.load(std::memory_order_acquire) || m_endpointCount == 0
			|| (code != StatusCode::MovedPermanently && code != StatusCode::PermanentRedirect) || receiveTimeoutMs == 0
			|| maxConnections == 0 || maxConnections > m_workers.size()) {

			LOG_WARNING_NEW("{}", "Failed to start redirector. Invalid configuration.");
			return false;
		}

		m_code = code;
		m_receiveTimeoutMs = receiveTimeoutMs;
		m_maxConnections = maxConnections;

		LOG_INFO_NEW("Starting redirector with {} endpoints, receive timeout: {} ms, max connections: {}",
			m_endpointCount, receiveTimeoutMs, maxConnections);

		m_isRunning.store(true, std::memory_order_release);
		uint64_t index{};
		for (; index < m_endpointCount; ++index) {
			auto& endpoint{ m_endpoints[index] };
			endpoint.SetSocket(OpenRedirectListener(endpoint.GetIp(), endpoint.GetPort()));
			if (endpoint.GetSocket() == -1) [[unlikely]] {
				break;
			}
		}
		if (index != m_endpointCount) [[unlikely]] {
			break;
		}

		for (index = 0; index < m_endpointCount; ++index) {
			auto& endpoint{ m_endpoints[index] };
			pthread_t pthread{};
			const auto result{ pthread_create(&pthread, nullptr, AcceptPthread, &endpoint) };
			if (result != 0) [[unlikely]] {
				LOG_ERROR_NEW("Redirect accept pthread creation failed. Error №{}: {}", result, std::strerror(result));
				break;
			}

			endpoint.SetPthread(pthread);
			endpoint.SetStarted(true);
		}
		if (index != m_endpointCount) [[unlikely]] {
			break;
		}

		return true;
	} while (false);

	// Locks inside
	Stop();
	return false;
}

void Redirector::Stop() noexcept
{
	const MSAPI::Lock::Atomic::Guard _{ m_endpointsLock };

	m_isRunning.store(false, std::memory_order_release);

	// Keep listener descriptors open until accept pthreads exit, avoiding races with descriptor reuse.
	for (auto& endpoint : m_endpoints) {
		if (endpoint.GetSocket() != -1) {
			(void)shutdown(endpoint.GetSocket(), SHUT_RDWR);
		}
	}

	for (auto& endpoint : m_endpoints) {
		if (endpoint.IsStarted()) {
			(void)pthread_join(endpoint.GetPthread(), nullptr);
			endpoint.SetStarted(false);
		}

		if (endpoint.GetSocket() != -1) {
			(void)close(endpoint.GetSocket());
			endpoint.SetSocket(-1);
		}
	}

	{
		const MSAPI::Lock::Atomic::Guard _{ m_workersLock };
		for (auto& worker : m_workers) {
			if (worker.GetSocket() != -1) {
				(void)shutdown(worker.GetSocket(), SHUT_RDWR);
			}
		}
	}

	// Workers close their own descriptors under the lock; join without holding it.
	for (auto& worker : m_workers) {
		if (worker.IsStarted()) {
			(void)pthread_join(worker.GetPthread(), nullptr);
			worker.SetStarted(false);
		}
	}
}

[[nodiscard]] void* Redirector::AcceptPthread(void* const context) noexcept
{
	// Dispatch the accept pthread through the endpoint owner.
	auto& endpoint{ *static_cast<Endpoint*>(context) };
	endpoint.GetOwner().AcceptLoop(endpoint);
	return nullptr;
}

[[nodiscard]] void* Redirector::RecvPthread(void* const context) noexcept
{
	// Dispatch the recv pthread through its endpoint owner.
	auto& worker{ *static_cast<Worker*>(context) };
	worker.GetEndpoint().GetOwner().HandleConnection(worker);
	return nullptr;
}

void Redirector::AcceptLoop(Endpoint& endpoint) noexcept
{
	while (m_isRunning.load(std::memory_order_acquire)) {
		// Raw socket acceptance avoids Connection allocation and any Server lifecycle dependency.
		const auto socket{ accept4(
			endpoint.GetSocket(), nullptr, nullptr, SOCK_CLOEXEC /* close-on-exec atomically */) };
		if (socket == -1) {
			const auto error{ errno };
			// Retry transient accept errors:
			// 1. EINTR: interrupted by a signal.
			// 2. ECONNABORTED: a pending connection was aborted.
			// 3. ENETDOWN: the network subsystem is down.
			// 4. EPROTO: a protocol error occurred.
			// 5. ENOPROTOOPT: a protocol option is unavailable.
			// 6. EHOSTDOWN: the peer host is down.
			// 7. ENONET: the network is unreachable locally.
			// 8. EHOSTUNREACH: the peer host is unreachable.
			// 9. ENETUNREACH: the network is unreachable.
			// 10. EOPNOTSUPP: a pending connection reported an unsupported operation.
			if (error == EINTR || error == ECONNABORTED || error == ENETDOWN || error == EPROTO || error == ENOPROTOOPT
				|| error == EHOSTDOWN || error == ENONET || error == EHOSTUNREACH || error == ENETUNREACH
				|| error == EOPNOTSUPP) {

				continue;
			}

			if (m_isRunning.load(std::memory_order_acquire)) {
				LOG_ERROR_NEW("Redirect accept failed. Error №{}: {}", error, std::strerror(error));
			}
			return;
		}

		// Keep receives blocking, but close peers that stop sending request data.
		timeval timeout{};
		timeout.tv_sec = m_receiveTimeoutMs / 1000;
		timeout.tv_usec = (m_receiveTimeoutMs % 1000) * 1000;
		if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1) [[unlikely]] {
			const auto error{ errno };
			LOG_ERROR_NEW(
				"Redirect receive timeout setup failed, errno: {}, error: \"{}\"", error, std::strerror(error));

			(void)close(socket);
			continue;
		}

		bool registered{};
		int32_t pthreadError{};
		{
			const MSAPI::Lock::Atomic::Guard _{ m_workersLock };
			if (m_isRunning.load(std::memory_order_acquire)) {
				// Reserve a worker slot and publish socket ownership under one lock.
				for (uint64_t index{}; index < m_maxConnections; ++index) {
					auto& worker{ m_workers[index] };
					if (worker.IsStarted() && !worker.IsFinished()) {
						continue;
					}

					if (worker.IsStarted()) {
						// Finished workers release this lock before returning and never touch their slot again.
						(void)pthread_join(worker.GetPthread(), nullptr);
						worker.SetStarted(false);
					}

					worker.SetSocket(socket);
					worker.SetEndpoint(endpoint);
					worker.SetFinished(false);

					pthread_t pthread{};
					const auto result{ pthread_create(&pthread, nullptr, RecvPthread, &worker) };
					if (result == 0) {
						worker.SetPthread(pthread);
						worker.SetStarted(true);
						registered = true;
					}
					else {
						worker.SetSocket(-1);
						pthreadError = result;
					}
					break;
				}
			}
		}

		// Format failure logs after releasing the worker lock.
		if (pthreadError != 0) [[unlikely]] {
			LOG_ERROR_NEW(
				"Redirect recv pthread creation failed. Error №{}: {}", pthreadError, std::strerror(pthreadError));
		}

		if (!registered) {
			(void)shutdown(socket, SHUT_RDWR);
			(void)close(socket);
		}
	}
}

void Redirector::HandleConnection(Worker& worker) noexcept
{
	// Retain ownership until the socket is closed and its worker slot is cleared under m_workersLock.
	const int32_t socket{ worker.GetSocket() };
	uint64_t remaining{ 2048 };
	std::unique_ptr<Connection> connection;
	std::shared_ptr<Connection::Data> connectionData;
	try {
		// May throw std::bad_alloc while allocating the Connection.
		connection = std::make_unique<Connection>(socket);

		// End timed-out peeks instead of retrying them, and bound all reads made by the HTTP constructor.
		connection->SetRecv([&remaining](const int32_t descriptor, void* const buffer, const uint64_t size,
								const int32_t flags) noexcept -> int64_t {
			if (remaining == 0) {
				return 0;
			}

			const auto count{ recv(descriptor, buffer, size < remaining ? size : remaining, flags) };
			if (count == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
				return 0;
			}

			if (count > 0 && (flags & MSG_PEEK) == 0) {
				remaining -= static_cast<uint64_t>(count);
			}
			return count;
		});

		// May throw std::bad_alloc while allocating connection data and its shared ownership control block.
		connectionData = std::make_shared<Connection::Data>(std::move(connection), SString<16>{}, /*ip=*/0,
			/*port=*/0, Connection::Type::Income, /*doReconnection=*/false);

		// May throw while formatting allocation or configuration diagnostics.
		MSAPI::RecvBuffer recvBuffer{ connectionData, /*capacityLimit=*/2048, /*toProcessSize=*/1 };

		// May throw while formatting receive diagnostics.
		if (recvBuffer.Recv().GetBufferSize() != 0) {
			while (m_isRunning.load(std::memory_order_acquire) && recvBuffer.GetBufferSize() < 2048) {
				// May throw while formatting buffer growth or receive diagnostics.
				const auto size{ recvBuffer.RecvAdditionalPeek(2048) };
				if (size == 0) {
					break;
				}

				const std::string_view received{ reinterpret_cast<const char*>(recvBuffer.GetData()), size };
				if (received.find("\r\n\r\n") == std::string_view::npos) {
					// Consume this fragment so the next blocking peek waits for new bytes.
					// May throw while formatting receive diagnostics.
					if (!recvBuffer.RecvAdditional(size)) {
						break;
					}
					continue;
				}

				// Leave the final fragment peeked for Data's existing constructor to read and consume.
				// May throw while allocating parsed strings, header entries or body storage, or formatting diagnostics.
				const Data request{ recvBuffer };
				SString<IHandler::DEFAULT_DESTINATION_CAPACITY + 2048> location{};
				SString<4096> response{};
				uint64_t length{};
				if (request.IsValid() && request.IsRequest() && request.GetHTTPType() == "HTTP"
					&& (request.GetVersion() == "1.0" || request.GetVersion() == "1.1")
					&& location.Copy(worker.GetEndpoint().GetDestination()) && location.Concatenate(request.GetUrl())
					&& location.Concatenate(request.GetQuery()) && m_isRunning.load(std::memory_order_acquire)
					&& (length = BuildRedirectResponse(
							{ response.GetBuffer(), response.GetCapacity() }, location.Get(), m_code, CLOSE_CONNECTION))
						!= 0) {

					// Connection handles partial sends; every outcome proceeds to socket closure below.
					// May throw while formatting send diagnostics.
					(void)connectionData->GetConnection().Send(response.GetBuffer(), length, MSG_NOSIGNAL);
				}
				break;
			}
		}
	}
	catch (const std::exception& error) {
		LOG_ERROR_NEW("Redirect request handling failed, error: \"{}\"", error.what());
	}

	// Close and clear together so Stop cannot use a descriptor that another pthread has already reused.
	const MSAPI::Lock::Atomic::Guard _{ m_workersLock };
	if (connectionData != nullptr) {
		connectionData->GetConnection().Close();
	}
	else if (connection != nullptr) {
		connection->Close();
	}
	else {
		(void)shutdown(socket, SHUT_RDWR);
		(void)close(socket);
	}

	worker.SetSocket(-1);
	worker.SetFinished(true);
}

/*---------------------------------------------------------------------------------
Free
---------------------------------------------------------------------------------*/

[[nodiscard]] bool SendRequest(Connection& connection, const std::string_view http)
{
	const auto request{ std::format("{}\r\n\r\n", http) };

	const auto size{ request.length() };
	LOG_PROTOCOL_NEW("Send request size: {} to connection id: {}", size, connection.GetId());

	return connection.Send(request.c_str(), size, MSG_CONFIRM);
}

} // namespace HTTP

} // namespace Protocol

} // namespace MSAPI