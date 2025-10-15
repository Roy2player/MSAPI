/**************************
 * @file        http.cpp
 * @version     6.0
 * @date        2023-09-02
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
 */

#include "http.h"
#include "../help/helper.h"
#include "../help/log.h"
#include "../server/server.h"
#include <charconv>
#include <cstring>
#include <iomanip>
#include <sys/socket.h>

namespace MSAPI {

namespace HTTP {

/*---------------------------------------------------------------------------------
Data
---------------------------------------------------------------------------------*/

Data::Data(MSAPI::RecvBufferInfo* recvBufferInfo)
{
	bool isHeaders{ false };
	bool isHtmlFormat{ true };
	bool isUrl{ false };
	bool isHTTPtype{ false };
	bool isVersion{ false };
	bool isCode{ false };
	bool isCodeText{ false };
	bool isKeyLine{ true };
	std::string value{ "" };
	std::string key{ "" };

	size_t readSize{ 2048 };
	if (!MSAPI::Server::LookForAdditionalData(recvBufferInfo, readSize)) {
		return;
	}
	const void* buffer{ *recvBufferInfo->buffer };
	readSize += sizeof(size_t) * 2;

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
		bool hasSpace{ false };
		bool validity{ false };
		for (size_t index{ 0 }; index < readSize; ++index) {
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
			LOG_ERROR("Invalid HTTP message format, connection: " + _S(recvBufferInfo->connection)
				+ ", id: " + _S(recvBufferInfo->id));
			return;
		}
	}

	if (m_isRequest) {
		bool isMessageType{ true };
		for (size_t index{ 0 }; index < readSize; ++index) {
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

								do {
									++index;
								} while (static_cast<const char*>(buffer)[index] != ' ');

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

				LOG_ERROR("Unexpected symbol inside request start line, connection: " + _S(recvBufferInfo->connection));
				return;
			}

			if (static_cast<const char*>(buffer)[index] != '\n') {
				if (static_cast<const char*>(buffer)[index] == ':'
					&& index + 2 /* 1 + 1 because of can be used below */ < readSize
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

			if (static_cast<const char*>(buffer)[index - 1] == '\r' && index + 2 < readSize
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

			LOG_ERROR(
				"Invalid HTTP message type: " + m_messageType + ", connection: " + _S(recvBufferInfo->connection));
			return;
		}
		if (m_HTTPtype != "HTTP" && m_HTTPtype != "HTTPS") {
			LOG_ERROR("Invalid HTTP type: " + m_HTTPtype + ", connection: " + _S(recvBufferInfo->connection));
			return;
		}
		if (m_version.empty()) {
			LOG_ERROR("Empty HTTP version, connection: " + _S(recvBufferInfo->connection));
			return;
		}
		if (m_url.empty()) {
			LOG_ERROR("Empty HTTP url, connection: " + _S(recvBufferInfo->connection));
			return;
		}

		if (m_format.empty()) {
			m_format = "html";
		}
		m_isValid = true;
	}
	else {
		isHTTPtype = true;
		for (size_t index{ 0 }; index < readSize; ++index) {
			if (isHeaders) {
				if (static_cast<const char*>(buffer)[index] != '\n') {
					if (static_cast<const char*>(buffer)[index] == ':'
						&& index + 2 /* 1 + 1 because of can be used below */ < readSize
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

				if (index + 1 < readSize && static_cast<const char*>(buffer)[index + 1] == '\r') {
					if (index + 2 < readSize && static_cast<const char*>(buffer)[index + 2] == '\n') {
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
			LOG_ERROR("Invalid HTTP type: " + m_HTTPtype + ", connection: " + _S(recvBufferInfo->connection)
				+ ", id: " + _S(recvBufferInfo->id));
			return;
		}
		if (m_version.empty()) {
			LOG_ERROR("Empty HTTP version, connection: " + _S(recvBufferInfo->connection)
				+ ", id: " + _S(recvBufferInfo->id));
			return;
		}
		if (m_code.empty()) {
			LOG_ERROR(
				"Empty HTTP code, connection: " + _S(recvBufferInfo->connection) + ", id: " + _S(recvBufferInfo->id));
			return;
		}
		if (m_codeText.empty()) {
			LOG_ERROR("Empty HTTP code text, connection: " + _S(recvBufferInfo->connection)
				+ ", id: " + _S(recvBufferInfo->id));
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
		if (!MSAPI::Server::ReadAdditionalData(recvBufferInfo, httpMessageSize)) {
			return;
		}

		m_body = std::move(
			std::string{ &static_cast<const char*>(*recvBufferInfo->buffer)[m_messageSize], contentLength });
		m_messageSize = httpMessageSize;
	}
	else {
		MSAPI::Server::ReadAdditionalData(recvBufferInfo, m_messageSize);
	}
}

const std::map<std::string, std::string, std::less<>>& Data::GetHeadersMap() const noexcept { return m_headersMap; }

size_t Data::GetSizeHeadersMap() const noexcept { return m_headersMap.size(); }

const std::string* Data::GetValue(const std::string& key) const
{
	if (const auto it = m_headersMap.find(key); it != m_headersMap.end()) {
		return &it->second;
	}
	return nullptr;
}

bool Data::IsValid() const noexcept { return m_isValid; }

const std::string& Data::GetVersion() const noexcept { return m_version; }

const std::string& Data::GetTypeMessage() const noexcept { return m_messageType; }

const std::string& Data::GetHTTPType() const noexcept { return m_HTTPtype; }

size_t Data::GetMessageSize() const noexcept { return m_messageSize; }

bool Data::IsRequest() const noexcept { return m_isRequest; }

void Data::GetResponseHeader200(const size_t length, std::string& result) const noexcept
{
	std::string format;
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
		LOG_WARNING("Format is not supported: " + m_format);
		format = "text/" + m_format;
	}

	result = std::move(m_HTTPtype + "/" + m_version + " 200 OK\r\nContent-Type: " + format
		+ "; charset=utf-8\r\nConnection: keep-alive\r\nKeep-Alive: timeout=0,max=0\r\nContent-Length: " + _S(length)
		+ "\r\n\r\n");
}

const std::string& Data::GetUrl() const noexcept { return m_url; }

const std::string& Data::GetFormat() const noexcept { return m_format; }

const std::string& Data::GetCode() const noexcept { return m_code; }

const std::string& Data::GetCodeText() const noexcept { return m_codeText; }

bool Data::SendResponse(const int connection, const std::string& body, const std::string& contentType) const
{
	std::string response;
	if (contentType.empty()) {
		GetResponseHeader200(body.length(), response);
		response += body;
	}
	else {
		response = std::move(std::string{ m_HTTPtype + "/" + m_version + " 200 OK\r\nContent-Type: " + contentType
			+ "; charset=utf-8\r\nConnection: keep-alive\r\nKeep-Alive: timeout=0,max=0\r\nContent-Length: "
			+ _S(body.length()) + "\r\n\r\n" + body });
	}
	const auto result{ send(connection, response.c_str(), response.length(), MSG_CONFIRM) };
	if (result == -1) {
		if (errno == 104) {
			LOG_DEBUG("Send returned error №104: Connection reset by peer");
			return false;
		}
		LOG_ERROR("Message not be sended. Error №" + _S(errno) + ": " + std::strerror(errno));
		return false;
	}
	LOG_PROTOCOL("Size of message: " + _S(result) + ", connection: " + _S(connection));
	return true;
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
	stream << "HTTP message:\n{" << std::fixed << std::setprecision(16) << "\n\t" << format << "is valid"
		   << " : " << _S(m_isValid) << "\n\t" << format << "type"
		   << " : " << (m_isRequest ? "Request" : "Response") << "\n\t" << format << "message type"
		   << " : " << m_messageType << "\n\t" << format << "url"
		   << " : " << m_url << "\n\t" << format << "HTTP type"
		   << " : " << m_HTTPtype << "\n\t" << format << "version"
		   << " : " << m_version << "\n\t" << format << "format"
		   << " : " << m_format << "\n\t" << format << "code"
		   << " : " << m_code << "\n\t" << format << "code text"
		   << " : " << m_codeText << "\n\t" << format << "message size"
		   << " : " << m_messageSize << "\n\t" << format << "Headers"
		   << " :\n{";
	for (const auto& [key, value] : m_headersMap) {
		stream << "\n\t" << format << key << " : " << value;
	}
	stream << "\n}\n}";

#undef format

	return stream.str();
}

void Data::SendSource(const int connection, const std::string& path, const std::string& contentType) const
{
	std::fstream file(path, std::ios::binary | std::ios::in);
	if (!file.is_open()) {
		LOG_WARNING("File do not open, path: " + path);
		Send404(connection);
		return;
	}

	file.seekg(0, file.end);
	size_t length{ UINT64(file.tellg()) };
	file.seekg(0, file.beg);

	std::string response;
	std::istream is(file.rdbuf());
	if (contentType.empty()) {
		GetResponseHeader200(length, response);
		response += std::string{ std::istreambuf_iterator<char>{ is }, {} };
	}
	else {
		const std::string source{ std::istreambuf_iterator<char>{ is }, {} };
		response = std::move(std::string{ m_HTTPtype + "/" + m_version + " 200 OK\r\nContent-Type: " + contentType
			+ "; charset=utf-8\r\nConnection: keep-alive\r\nKeep-Alive: timeout=0,max=0\r\nContent-Length: "
			+ _S(source.length()) + "\r\n\r\n" + source });
	}

	const auto result{ send(connection, response.c_str(), response.length(), MSG_CONFIRM) };
	if (result == -1) {
		if (errno == 104) {
			LOG_DEBUG("Send returned error №104: Connection reset by peer");
			file.close();
			return;
		}
		LOG_ERROR("Fail to send message, patch: " + path + ". Error №" + _S(errno) + ": " + std::strerror(errno));
		file.close();
		return;
	}
	file.close();
	LOG_PROTOCOL("Size of message: " + _S(result) + ", path: " + path + ", connection: " + _S(connection));
}

void Data::Send404(const int connection, const std::string& body, const std::string& contentType) const
{
	const std::string response{ m_HTTPtype + "/" + m_version
		+ " 404 Not Found\r\nContent-Type: " + (contentType.empty() ? "text/html" : contentType)
		+ "; charset=utf-8\r\nConnection: keep-alive\r\nKeep-Alive: timeout=0,max=0\r\n"
		+ (body.empty() ? "\r\n" : "Content-Length: " + _S(body.length()) + "\r\n\r\n" + body) };

	const auto result{ send(connection, response.c_str(), response.length(), MSG_CONFIRM) };
	if (result == -1) {
		if (errno == 104) {
			LOG_DEBUG("Send returned error №104: Connection reset by peer");
			return;
		}
		LOG_ERROR("Fail to send message. Error №" + _S(errno) + ": " + std::strerror(errno));
		return;
	}
	LOG_PROTOCOL("Size of message: " + _S(result) + ", connection: " + _S(connection));
}

/*---------------------------------------------------------------------------------
IHandler
---------------------------------------------------------------------------------*/

IHandler::IHandler(const MSAPI::Application* application)
	: m_application(application)
{
}

void IHandler::Collect(const int connection, const Data& data)
{
	if (m_application->IsRunning()) {
		LOG_PROTOCOL(data.ToString());
		HandleHttp(connection, data);
		return;
	}
	LOG_PROTOCOL("Application is not running. " + data.ToString());
}

/*---------------------------------------------------------------------------------
Another
---------------------------------------------------------------------------------*/

void SendRequest(const int connection, const std::string& HTTP)
{
	std::stringstream request;
	request << HTTP;
	request << "\r\n\r\n";
	const auto result{ send(connection, request.str().c_str(), request.str().length(), MSG_CONFIRM) };
	if (result == -1) {
		if (errno == 104) {
			LOG_DEBUG("Send returned error №104: Connection reset by peer");
			return;
		}
		LOG_ERROR("Fail to send message. Error №" + _S(errno) + ": " + std::strerror(errno));
		return;
	}
	LOG_PROTOCOL("Size of message: " + _S(result) + ", connection: " + _S(connection));
}

}; //* namespace HTTP

}; //* namespace MSAPI