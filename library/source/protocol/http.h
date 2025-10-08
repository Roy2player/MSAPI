/**************************
 * @file        http.h
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
 *
 * @brief Protocol for reserving and sending HTTP messages.
 * @brief Request can recognize and prepare response message with correct content type for next formats: js, dat, ogg,
 * pdf, xhtml, json, ldjson, xml, zip, mp3, wma, wav, gif, jpeg, jpg, png, tiff, ico, djvu, svg, bmp, webp, css, csv,
 * html, txt, mpeg, mp4, mov, wmv, avi, webm.
 * @brief Response can be 200 OK or 404 Not Found only.
 *
 * MSAPI_HANDLER_HTTP_PRESET macro is used to reserve and collect HTTP message.
 */

#ifndef MSAPI_HTTP_H
#define MSAPI_HTTP_H

#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace MSAPI {

class Application;
class RecvBufferInfo;

namespace HTTP {

/**************************
 * @brief Object for parsing and collecting information about a HTTP message.
 */
class Data {
private:
	std::map<std::string, std::string, std::less<>> m_headersMap;
	std::string m_messageType{ "" };
	std::string m_url{ "" };
	std::string m_HTTPtype{ "" };
	std::string m_version{ "" };
	std::string m_format{ "" };
	std::string m_code{ "" };
	std::string m_codeText{ "" };
	std::string m_body{ "" };
	bool m_isRequest{ false };
	bool m_isValid{ false };
	size_t m_messageSize{ 0 };

public:
	/**************************
	 * @brief Construct a new Data object, parse buffer and collect information about HTTP message. Response does not
	 * contain format. If buffer does not contain HTTP message, object will be invalid and additional data will not be
	 * read from socket.
	 *
	 * @attention HTTP header section cannot be longer than 2048 bytes.
	 * @attention If request does not contain format, it will be set as html.
	 *
	 * @param recvBufferInfo Pointer to recv buffer info object with allocated memory.
	 */
	Data(MSAPI::RecvBufferInfo* recvBufferInfo);

	/**************************
	 * @return Readable link to maps with headers like { header, value }.
	 */
	const std::map<std::string, std::string, std::less<>>& GetHeadersMap() const noexcept;

	/**************************
	 * @return Size of headers map.
	 */
	size_t GetSizeHeadersMap() const noexcept;

	/**************************
	 * @return Readable pointer to value from headers map by key, nullptr if does not exist.
	 */
	const std::string* GetValue(const std::string& key) const;

	/**************************
	 * @return Version of HTTP message.
	 */
	const std::string& GetVersion() const noexcept;

	/**************************
	 * @return Type of HTTP message, GET/POST/PUT/DELETE/HEAD/CONNECT/OPTIONS/TRACE/PATCH.
	 */
	const std::string& GetTypeMessage() const noexcept;

	/**************************
	 * @return Type of HTTP message, HTTP or HTTPS.
	 */
	const std::string& GetHTTPType() const noexcept;

	/**************************
	 * @return Code of HTTP message.
	 */
	const std::string& GetCode() const noexcept;

	/**************************
	 * @return Code text of HTTP message.
	 */
	const std::string& GetCodeText() const noexcept;

	/**************************
	 * @return Whole size of HTTP message, including headers and body.
	 */
	size_t GetMessageSize() const noexcept;

	/**************************
	 * @return Request url of HTTP message.
	 */
	const std::string& GetUrl() const noexcept;

	/**************************
	 * @return Format of HTTP request, html as default.
	 */
	const std::string& GetFormat() const noexcept;

	/**************************
	 * @return Body of HTTP message.
	 */
	const std::string& GetBody() const noexcept;

	/**************************
	 * @return True if HTTP message is request.
	 */
	bool IsRequest() const noexcept;

	/**************************
	 * @return True if HTTP message is valid.
	 */
	bool IsValid() const noexcept;

	/**************************
	 * @brief Distributor function for sending HTTP 200 OK response message. Providing content type manually is faster.
	 *
	 * @param connection Socket connection to which send HTTP message.
	 * @param body Body of HTTP message.
	 * @param contentType Content type of HTTP message, if empty, will be used format of data.
	 *
	 * @return True if HTTP message was sent successfully, false otherwise.
	 */
	bool SendResponse(int connection, const std::string& body, const std::string& contentType = "") const;

	/**************************
	 * @brief Distributor function for sending source file by path in HTTP message. Providing content type manually is
	 * faster.
	 *
	 * @param connection Socket connection to which send HTTP message.
	 * @param path Full path to source file.
	 * @param contentType Content type of HTTP message, if empty, will be used format of data.
	 */
	void SendSource(int connection, const std::string& path, const std::string& contentType = "") const;

	/**************************
	 * @brief Distributor function for sending HTTP 404 response message.
	 *
	 * @param connection Socket connection to which send HTTP message.
	 * @param body Body of HTTP message, empty as default.
	 * @param contentType Content type of HTTP message, if empty, will be used text/html. Used only if not empty body.
	 */
	void Send404(int connection, const std::string& body = "", const std::string& contentType = "") const;

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
	 */
	std::string ToString() const;

	/**************************
	 * @brief Call ToString() inside.
	 */
	friend std::ostream& operator<<(std::ostream& out, Data& data);

	/**************************
	 * @brief Call ToString() inside.
	 */
	operator std::string() const;

private:
	/**************************
	 * @return Prepared 200 OK HTTP response header with specific length.
	 *
	 * @todo String as parameter to fill.
	 */
	void GetResponseHeader200(size_t length, std::string& result) const noexcept;
};

/**************************
 * @brief Object for reserving HTTP messages. Use MSAPI_HANDLER_HTTP_PRESET macro to reserve and collect HTTP message.
 */
class IHandler {
private:
	const MSAPI::Application* m_application;

public:
	/**************************
	 * @brief Construct a new IHandler object, empty constructor.
	 *
	 * @param application Readable pointer to application object.
	 */
	IHandler(const MSAPI::Application* application);

	/**************************
	 * @brief Default destructor.
	 */
	virtual ~IHandler() = default;

	/**************************
	 * @brief Collect HTTP message from socket connection and call Handler function if Application is running.
	 *
	 * @param connection Socket connection from which reserved message.
	 * @param data Reserved HTTP message.
	 *
	 * @todo Data can be transferred by rvalue reference.
	 */
	void Collect(int connection, const Data& data);

	/**************************
	 * @brief Handler function for HTTP message.
	 *
	 * @param connection Socket connection from which reserved message.
	 * @param data Reserved HTTP message.
	 */
	virtual void HandleHttp(int connection, const Data& data) = 0;
};

/**************************
 * @brief Send HTTP message to socket connection. End of request \r\n\r\n will be added inside. Cannot include body.
 *
 * @param connection Socket connection to which send HTTP message.
 * @param HTTP HTTP message.
 */
void SendRequest(int connection, const std::string& HTTP);

}; //* namespace HTTP

}; //* namespace MSAPI

#define MSAPI_HANDLER_HTTP_PRESET                                                                                      \
	if (MSAPI::HTTP::Data http(recvBufferInfo); http.IsValid()) {                                                      \
		MSAPI::HTTP::IHandler::Collect(recvBufferInfo->connection, http);                                              \
		return;                                                                                                        \
	}

#endif //* MSAPI_HTTP_H