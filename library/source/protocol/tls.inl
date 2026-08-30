/**************************
 * @file        tls.inl
 * @version     6.0
 * @date        2026-04-29
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
 * @brief Implementation of Transport Layer Security (TLS) for 1.3 version (RFC 8446).
 */

#ifndef MSAPI_PROTOCOL_TLS_INL
#define MSAPI_PROTOCOL_TLS_INL

#include "../help/diagnostic.inl"
#include "../server/recvBuffer.inl"

namespace MSAPI {

namespace Protocol {

namespace TLS {

/*---------------------------------------------------------------------------------
Declarations
---------------------------------------------------------------------------------*/

enum class Version : uint16_t {
	Invalid = 0,
	SSL_3_0 = 0x0300,
	TLS_1_0 = 0x0301,
	TLS_1_1 = 0x0302,
	TLS_1_2 = 0x0303,
	TLS_1_3 = 0x0304
};

FORCE_INLINE [[nodiscard]] std::string_view EnumToString(const Version version)
{
	switch (version) {
	case Version::Invalid:
		return "Invalid";
	case Version::SSL_3_0:
		return "SSL 3.0";
	case Version::TLS_1_0:
		return "TLS 1.0";
	case Version::TLS_1_1:
		return "TLS 1.1";
	case Version::TLS_1_2:
		return "TLS 1.2";
	case Version::TLS_1_3:
		return "TLS 1.3";
	default:
		LOG_WARNING_NEW("Unknown TLS version {}", U(version));
		return "Unknown";
	}
}

enum class DataType : uint8_t { Invalid = 0, ChangeCipherSpec = 20, Alert = 21, Handshake = 22, ApplicationData = 23 };

FORCE_INLINE [[nodiscard]] std::string_view EnumToString(const DataType value)
{
	switch (value) {
	case DataType::Invalid:
		return "Invalid";
	case DataType::ChangeCipherSpec:
		return "Change cipher spec";
	case DataType::Alert:
		return "Alert";
	case DataType::Handshake:
		return "Handshake";
	case DataType::ApplicationData:
		return "Application data";
	default:
		LOG_WARNING_NEW("Unknown TLS data type {}", U(value));
		return "Unknown";
	}
}

enum class Chipher : uint16_t {
	TLS_EMPTY_RENEGOTIATION_INFO_SCSV = 0x00FF,
	TLS_AES_128_GCM_SHA256 = 0x1301,
	TLS_AES_256_GCM_SHA384 = 0x1302,
	TLS_CHACHA20_POLY1305_SHA256 = 0x1303,
	TLS_AES_128_CCM_SHA256 = 0x1304,
	TLS_AES_128_CCM_8_SHA256 = 0x1305,
	TLS_FALLBACK_SCSV = 0x5600
};

FORCE_INLINE [[nodiscard]] std::string_view EnumToString(const Chipher value)
{
	switch (value) {
	case Chipher::TLS_EMPTY_RENEGOTIATION_INFO_SCSV:
		return "TLS EMPTY RENEGOTIATION INFO SCSV";
	case Chipher::TLS_AES_128_GCM_SHA256:
		return "TLS AES 128 GCM SHA256";
	case Chipher::TLS_AES_256_GCM_SHA384:
		return "TLS AES 256 GCM SHA384";
	case Chipher::TLS_CHACHA20_POLY1305_SHA256:
		return "TLS CHACHA20 POLY1305 SHA256";
	case Chipher::TLS_AES_128_CCM_SHA256:
		return "TLS AES 128 CCM SHA256";
	case Chipher::TLS_AES_128_CCM_8_SHA256:
		return "TLS AES 128 CCM 8 SHA256";
	case Chipher::TLS_FALLBACK_SCSV:
		return "TLS FALLBACK SCSV";
	default:
		LOG_WARNING_NEW("Unknown TLS cipher {}", U(value));
		return "Unknown";
	}
}

enum class FragmentType : uint8_t {
	HelloRequest = 0,
	ClientHello = 1,
	SeverHello = 2,
	NewSessionTicket = 4,
	EncryptedExtension = 8,
	Certificate = 11,
	ServerKeyExchange = 12,
	CertificateRequest = 13,
	ServerHelloDone = 14,
	CertificateVerify = 15,
	Finished = 20
};

FORCE_INLINE [[nodiscard]] std::string_view EnumToString(const FragmentType value)
{
	switch (value) {
	case FragmentType::HelloRequest:
		return "Hello request";
	case FragmentType::ClientHello:
		return "Client hello";
	case FragmentType::SeverHello:
		return "Sever hello";
	case FragmentType::NewSessionTicket:
		return "New session ticket";
	case FragmentType::EncryptedExtension:
		return "Encrypted extension";
	case FragmentType::Certificate:
		return "Certificate";
	case FragmentType::ServerKeyExchange:
		return "Server key exchange";
	case FragmentType::CertificateRequest:
		return "Certificate request";
	case FragmentType::ServerHelloDone:
		return "Server hello done";
	case FragmentType::CertificateVerify:
		return "Certificate verify";
	case FragmentType::Finished:
		return "Finished";
	default:
		LOG_WARNING_NEW("Unknown TLS fragment type {}", U(value));
		return "Unknown";
	}
}

class Cursor {
private:
	uint8_t* const m_pointer;
	size_t m_offset{};

public:
	FORCE_INLINE Cursor(uint8_t* const pointer) noexcept
		: m_pointer{ pointer }
	{
	}

	FORCE_INLINE [[nodiscard]] uint8_t Get8() noexcept { return m_pointer[m_offset++]; }

	FORCE_INLINE [[nodiscard]] uint16_t Get16() noexcept
	{
		uint16_t value{ static_cast<uint16_t>(m_pointer[m_offset++]) << 8 };
		value |= m_pointer[m_offset++];
		return value;
	}

	FORCE_INLINE [[nodiscard]] uint32_t Get32() noexcept
	{
		uint32_t value{ static_cast<uint32_t>(m_pointer[m_offset++]) << 24 };
		value |= (static_cast<uint32_t>(m_pointer[m_offset++]) << 16);
		value |= (static_cast<uint32_t>(m_pointer[m_offset++]) << 8);
		value |= m_pointer[m_offset++];
		return value;
	}

	FORCE_INLINE [[nodiscard]] uint64_t Get64() noexcept
	{
		uint64_t value{ static_cast<uint64_t>(m_pointer[m_offset++]) << 56 };
		value |= (static_cast<uint64_t>(m_pointer[m_offset++]) << 48);
		value |= (static_cast<uint64_t>(m_pointer[m_offset++]) << 40);
		value |= (static_cast<uint64_t>(m_pointer[m_offset++]) << 32);
		value |= (static_cast<uint32_t>(m_pointer[m_offset++]) << 24);
		value |= (static_cast<uint32_t>(m_pointer[m_offset++]) << 16);
		value |= (static_cast<uint32_t>(m_pointer[m_offset++]) << 8);
		value |= m_pointer[m_offset++];
		return value;
	}

	FORCE_INLINE [[nodiscard]] std::span<const uint8_t> GetArray(const size_t size) noexcept
	{
		m_offset += size;
		return { m_pointer, size };
	}
}

class Controller {
public:
	enum class CheckingResult : int8_t { Undefined, Failed, Confirmed, Unconfirmed, Max };

	FORCE_INLINE [[nodiscard]] std::string_view EnumToString(const CheckingResult value)
	{
		switch (value) {
		case CheckingResult::Undefined:
			return "Undefined";
		case CheckingResult::Failed:
			return "Failed";
		case CheckingResult::Confirmed:
			return "Confirmed";
		case CheckingResult::Unconfirmed:
			return "Unconfirmed";
		case CheckingResult::Max:
			return "Max";
		default:
			LOG_WARNING_NEW("Unknown TLS controller checking result {}", U(value));
			return "Unknown";
		}
	}

private:
	RecvBuffer& m_recvBuffer;
	DataType m_dataType;
	FragmentType m_fragmentType;

public:
	FORCE_INLINE Controller(RecvBuffer& recvBuffer) noexcept
		: m_recvBuffer{ recvBuffer }
	{
	}

	FORCE_INLINE [[nodiscard]] CheckingResult CheckTls() { }
};

/**************************
 * Data frame format (all unsigned):
 *
 * 1 byte       Type
 * 2 bytes      Protocol Version
 * 2 bytes      Fragment size
 * N-M bytes    Fragment
 */
class Data {
public:
public:
	/**************************
	 * Fragment frame format (all unsigned):
	 *
	 * 1 byte       Type
	 * 3 bytes      Size
	 * N-M bytes    Data
	 */
	class Fragment {
	private:
		std::vector<uint8_t>& m_buffer;
		bool m_isValid;

	public:
		static inline constexpr uint8_t FRAGMENT_DATA_OFFSET{ sizeof(uint8_t) + sizeof(uint8_t[24]) };

	public:
		FORCE_INLINE Fragment(RecvBuffer& recvBuffer, std::vector<uint8_t>& b, const uint16_t dataSize) noexcept
			: m_buffer{ b }
		{
			// Check required buffer size
			if (recvBuffer.GetReadDataSize() < DATA_DATA_OFFSET + FRAGMENT_DATA_OFFSET) [[unlikely]] {
				LOG_ERROR_NEW("TLS data size is too low {}", recvBuffer.GetReadDataSize());
				m_isValid = false;
				return;
			}

			const uint8_t* const buffer{ static_cast<const uint8_t*>(*recvBuffer->buffer) + DATA_DATA_OFFSET };

			static_assert(std::is_same_v<std::underlying_type_t<Type>, uint8_t>, "Type of fragment type enum");
			const auto type{ static_cast<Type>(buffer[0]) };

			uint32_t fragmentSize [[gnu::uninitialized]];
			memcpy(&fragmentSize, buffer + 1, sizeof(uint32_t));
			fragmentSize = be32toh(fragmentSize);
			fragmentSize &= 0x00FFFFFF;

			int64_t restFragmentSize{ fragmentSize - dataSize + FRAGMENT_DATA_OFFSET };
			if (restFragmentSize < 0) [[unlikely]] {
				LOG_ERROR_NEW("Rest fragment size is negative. Data size {}, fragment size {}", dataSize, fragmentSize);
				m_isValid = false;
				return;
			}

			LOG_PROTOCOL_NEW("Income TLS fragment:\n{{"
							 "\n\ttype    : {}"
							 "\n\tsize    : {} + 4 fragment header"
							 "\n}}",
				EnumToString(type), fragmentSize);

			if (restFragmentSize > 0) {
				LOG_ERROR_NEW("Read fragment size {}. Fetching is not supported yet", restFragmentSize);
				// Each next continuation TLS data structure is: Data layout + fragment data
				if (!recvBuffer.RecvAdditional(static_cast<uint64_t>(restFragmentSize))) {
					return;
				}
				return;
			}

			switch (type) {
			case Type::HelloRequest:
				LOG_ERROR_NEW("Unsupported yet TLS fragment type {}", EnumToString(type));
				m_isValid = false;
				return;
			case Type::ClientHello:
				// ClientHello fragment{ recvBuffer, m_buffer };
				// if (!fragment.IsValid()) [[unlikely]] {
				//     m_isValid = false;
				// }

				if (HandleClientHello(recvBuffer, fragmentSize)) {
					m_isValid = false;
				}
				m_isValid = HandleClientHello

					// m_isValid = true;
					return;
			case Type::SeverHello:
				LOG_ERROR_NEW("Unsupported yet TLS fragment type {}", EnumToString(type));
				m_isValid = false;
				return;
			case Type::NewSessionTicket:
				LOG_ERROR_NEW("Unsupported yet TLS fragment type {}", EnumToString(type));
				m_isValid = false;
				return;
			case Type::EncryptedExtension:
				LOG_ERROR_NEW("Unsupported yet TLS fragment type {}", EnumToString(type));
				m_isValid = false;
				return;
			case Type::Certificate:
				LOG_ERROR_NEW("Unsupported yet TLS fragment type {}", EnumToString(type));
				m_isValid = false;
				return;
			case Type::ServerKeyExchange:
				LOG_ERROR_NEW("Unsupported yet TLS fragment type {}", EnumToString(type));
				m_isValid = false;
				return;
			case Type::CertificateRequest:
				LOG_ERROR_NEW("Unsupported yet TLS fragment type {}", EnumToString(type));
				m_isValid = false;
				return;
			case Type::ServerHelloDone:
				LOG_ERROR_NEW("Unsupported yet TLS fragment type {}", EnumToString(type));
				m_isValid = false;
				return;
			case Type::CertificateVerify:
				LOG_ERROR_NEW("Unsupported yet TLS fragment type {}", EnumToString(type));
				m_isValid = false;
				return;
			case Type::Finished:
				LOG_ERROR_NEW("Unsupported yet TLS fragment type {}", EnumToString(type));
				m_isValid = false;
				return;
			default:
				LOG_ERROR_NEW("TLS fragment with unexpected type {}", EnumToString(type));
				m_isValid = false;
				return;
			}
		}

		FORCE_INLINE [[nodiscard]] Type GetType() const noexcept
		{
			return static_cast<Type>(m_buffer[DATA_DATA_OFFSET]);
		}

		FORCE_INLINE [[nodiscard]] uint32_t GetFragmentSize() const noexcept
		{
			uint32_t size [[gnu::uninitialized]];
			memcpy(&size, m_buffer.data() + DATA_DATA_OFFSET, sizeof(uint32_t));
			size = be32toh(size);
			// size &= 0x00FFFFFF;
			LOG_INFO_NEW("DATA_DATA_OFFSET {}", DATA_DATA_OFFSET);
			Diagnostic::PrintBinaryDescriptor<Diagnostic::hex>(
				m_buffer.data() + DATA_DATA_OFFSET, 4, "TLS Fragment size in buffer");
			Diagnostic::PrintBinaryDescriptor<Diagnostic::hex>(&size, 4, "TLS Fragment size");
			return size;
		}

		FORCE_INLINE [[nodiscard]] bool IsCompleted() const noexcept
		{
			return m_buffer.size() == GetFragmentSize() + DATA_DATA_OFFSET + FRAGMENT_DATA_OFFSET;
		}

		// FORCE_INLINE [[nodiscard]] std::span<const uint8_t> GetData() const noexcept
		// {
		//     return m_buffer.subspan(FRAGMENT_DATA_OFFSET);
		// }

		FORCE_INLINE [[nodiscard]] bool IsValid() const noexcept { return m_isValid; }
	};

	/**************************
	 * Extension frame format (all unsigned):
	 *
	 * 2 bytes      Type
	 * 2 bytes      Data size
	 * 2-N bytes    Data
	 */
	class Extension {
	public:
		enum class Type : uint16_t {
			SERVER_NAME = 0x0000,
			MAX_FRAGMENT_LENGTH = 0x0001,
			CLIENT_CERTIFICATE_URL = 0x0002,
			TRUSTED_CA_KEYS = 0x0003,
			TRUNCATED_HMAC = 0x0004,
			STATUS_REQUEST = 0x0005,
			USER_MAPPING = 0x0006,
			CLIENT_AUTHZ = 0x0007,
			SERVER_AUTHZ = 0x0008,
			CERT_TYPE = 0x0009,
			SUPPORTED_GROUPS = 0x000A,
			EC_POINT_FORMATS = 0x000B,
			SRP = 0x000C,
			SIGNATURE_ALGORITHMS = 0x000D,
			USE_SRTP = 0x000E,
			HEARTBEAT = 0x000F,
			APPLICATION_LAYER_PROTOCOL_NEGOTIATION = 0x0010,
			STATUS_REQUEST_V2 = 0x0011,
			SIGNED_CERTIFICATE_TIMESTAMP = 0x0012,
			CLIENT_CERTIFICATE_TYPE = 0x0013,
			SERVER_CERTIFICATE_TYPE = 0x0014,
			PADDING = 0x0015,
			ENCRYPT_THEN_MAC = 0x0016,
			EXTENDED_MASTER_SECRET = 0x0017,
			TOKEN_BINDING = 0x0018,
			CACHED_INFO = 0x0019,
			TLS_LTS = 0x001A,
			COMPRESS_CERTIFICATE = 0x001B,
			RECORD_SIZE_LIMIT = 0x001C,
			PWD_PROTECT = 0x001D,
			PWD_CLEAR = 0x001E,
			PASSWORD_SALT = 0x001F,
			TICKET_PINNING = 0x0020,
			TLS_CERT_WITH_EXTERN_PSK = 0x0021,
			DELEGATED_CREDENTIALS = 0x0022,
			SESSION_TICKET = 0x0023,
			TLMSP = 0x0024,
			TLMSP_PROXYING = 0x0025,
			TLMSP_DELEGATE = 0x0026,
			SUPPORTED_EKT_CIPHERS = 0x0027,
			PRE_SHARED_KEY = 0x0029,
			EARLY_DATA = 0x002A,
			SUPPORTED_VERSIONS = 0x002B,
			COOKIE = 0x002C,
			PSK_KEY_EXCHANGE_MODES = 0x002D,
			CERTIFICATE_AUTHORITIES = 0x002F,
			OID_FILTERS = 0x0030,
			POST_HANDSHAKE_AUTH = 0x0031,
			SIGNATURE_ALGORITHMS_CERT = 0x0032,
			KEY_SHARE = 0x0033,
			TRANSPARENCY_INFO = 0x0034,
			CONNECTION_ID = 0x0035,
			EXTERNAL_ID_HASH = 0x0036,
			EXTERNAL_SESSION_ID = 0x0037,
			QUIC_TRANSPORT_PARAMETERS = 0x0039,
			TICKET_REQUEST = 0x003A,
			DNSSEC_CHAIN = 0x003B,
			SEQUENCE_NUMBER_ENCRYPTION_ALGORITHMS = 0x003C,
			RENEGOTIATION_INFO = 0xFF01
		};

	private:
		const std::span<const uint8_t> m_buffer;

	public:
		FORCE_INLINE Extension(std::span<uint8_t>&& buffer) noexcept
			: m_buffer(std::move(buffer))
		{
		}

		FORCE_INLINE [[nodiscard]] Type GetType() const noexcept
		{
			static_assert(std::is_same_v<std::underlying_type_t<Type>, uint16_t>, "Type of extension enum");
			uint16_t type [[gnu::uninitialized]];
			memcpy(&type, m_buffer.data(), sizeof(uint16_t));
			type = be16toh(type);
			return static_cast<Type>(type);
		}

		FORCE_INLINE [[nodiscard]] uint16_t GetSize() const noexcept
		{
			uint16_t size [[gnu::uninitialized]];
			memcpy(&size, m_buffer.data() + sizeof(uint16_t), sizeof(uint16_t));
			size = be16toh(size);
			return size;
		}

		FORCE_INLINE [[nodiscard]] std::span<const uint8_t> GetData() const noexcept
		{
			return m_buffer.subspan(sizeof(uint16_t) * 2);
		}
	};

private:
	// Type m_type;
	// Version m_protocolVersion;
	// //! Type and versions are part of the buffer
	// uint16_t m_size;
	std::vector<uint8_t> m_buffer;
	bool m_isValid;

public:
	static inline constexpr uint8_t DATA_DATA_OFFSET{ sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint16_t) };

public:
	// TODO: Data structure failing is the connection comprocentaction, what is the logical reason to close it
	// immediately.
	FORCE_INLINE Data(RecvBuffer& recvBuffer)
	{
		// Below code assumes that MSAPI minimum buffer size is 16 bytes

		const uint8_t* const buffer{ static_cast<const uint8_t*>(*recvBuffer->buffer) };

		static_assert(std::is_same_v<std::underlying_type_t<Type>, uint8_t>, "Type of data type enum");
		const auto type{ static_cast<Type>(buffer[0]) };

		if (U(type) < U(Type::ChangeCipherSpec) || U(type) > U(Type::ApplicationData)) [[unlikely]] {
			LOG_ERROR_NEW("TLS Data with unexpected type {}", EnumToString(type));
			m_isValid = false;
			return;
		}

		static_assert(std::is_same_v<std::underlying_type_t<Version>, uint16_t>, "Type of data type enum");
		uint16_t version [[gnu::uninitialized]];
		memcpy(&version, buffer + sizeof(uint8_t), sizeof(uint16_t));
		version = be16toh(version);

		/* Data level TLS version does not make any sense
			if (version != U(Version::TLS_1_2)) [[unlikely]] {
				LOG_ERROR_NEW("TLS Data with unexpected version {}", TLS::EnumToString(static_cast<Version>(version)));
				m_isValid = false;
				return;
			}
		*/

		LOG_PROTOCOL(EnumToString(type));
		Diagnostic::PrintBinaryDescriptor<Diagnostic::hex>(buffer, 16, "TLS Data");

		uint16_t dataSize [[gnu::uninitialized]];
		memcpy(&dataSize, buffer + sizeof(uint8_t) + sizeof(uint16_t), sizeof(uint16_t));
		dataSize = be16toh(dataSize);

		const auto totalSize{ static_cast<size_t>(DATA_DATA_OFFSET) + dataSize };
		if (!recvBuffer.RecvAdditional(totalSize)) {
			return;
		}
		m_buffer.resize(totalSize);
		LOG_PROTOCOL_NEW("Income TLS data:\n{{"
						 "\n\ttype    : {}"
						 "\n\tversion : {}"
						 "\n\tsize    : {} + 3 data header"
						 "\n}}",
			EnumToString(type), TLS::EnumToString(static_cast<Version>(version)), dataSize);

		Fragment fragment{ recvBuffer, m_buffer, dataSize };
		if (!fragment.IsValid()) [[unlikely]] {
			m_isValid = false;
		}
		m_isValid = true;
	}
};

/**************************
 * ClientHello frame format (all unsigned):
 *
 * 2 bytes      TLS version
 * 32 bytes     Random
 * 1 byte       Session id size
 * 0-32 bytes   Session id
 * 2 bytes      Cipher suites size
 * 2-N bytes    Cipher suites, each 2 bytes
 * 1 byte       Compression methods size
 * 1-N bytes    Compression methods (must include 0x00)
 * 2 bytes      Extensions size
 * 6-N bytes    Extensions
 */
FORCE_INLINE bool HandleClientHello(RecvBuffer& recvBuffer, const uint32_t fragmentSize)
{
	const uint8_t* const buffer{ static_cast<const uint8_t*>(*recvBuffer->buffer) + DATA_DATA_OFFSET
		+ FRAGMENT_DATA_OFFSET };

	static_assert(std::is_same_v<std::underlying_type_t<Version>, uint16_t>, "Type of data type enum");
	uint16_t version [[gnu::uninitialized]];
	memcpy(&version, buffer, sizeof(uint16_t));
	version = be16toh(version);

	/* Fragment level TLS version does not make any sense
		if (version != U(Version::TLS_1_2)) [[unlikely]] {
			LOG_ERROR_NEW("TLS Fragment with unexpected version {}", TLS::EnumToString(static_cast<Version>(version)));
			m_isValid = false;
			return;
		}
	*/

	const uint8_t sessionIdSize{ buffer + sizeof(uint16_t) };
}

// class ClientHello
// {
// private:
//     std::vector<uint8_t> m_buffer;
//     uint16_t m_cipherSuitesSize;
//     uint8_t m_sessionSize;
//     bool m_valid;

// public:
//     /**************************
//      * @brief Extract and save dynamic size parts to further data filed access. Validate buffer.
//      *
//      * @param buffer
//      */
//     FORCE_INLINE ClientHello(RecvBuffer& recvBuffer, std::vector<uint8_t>& buffer)
//         : m_buffer{buffer}
//     {

//         // Verify minimum buffer allocation
//         if (m_buffer.size() <= static_cast<size_t>(DATA_DATA_OFFSET))

//         const auto version{GetVersion()};
//         // For TLS 1.3 the version in handshake message layout is fixed to TLS 1.2
//         if (version != U(Version::TLS_1_2)) [[unlikely]] {
//             LOG_ERROR_NEW("TLS handshake message with unexpected version {}",
//             EnumToString(static_cast<Version>(version))); m_isValid = false; return;
//         }

//         m_sessionSize = m_buffer[FRAGMENT_DATA_OFFSET + sizeof(uint16_t) + 32];

//         m_isValid = true;
//     }

//     FORCE_INLINE [[nodiscard]] Version GetVersion() const noexcept
//     {
//         static_assert(std::is_same_v<std::underlying_type_t<Version>, uint16_t>, "Type of TLS version enum");
//         uint16_t version [[gnu::uninitialized]];
//         memcpy(&version, m_buffer.data() + FRAGMENT_DATA_OFFSET, sizeof(uint16_t));
//         be16toh(version);
//         return static_cast<Version>(version);
//     }

//     FORCE_INLINE [[nodiscard]] std::span<const uint8_t> GetRandom() const noexcept
//     {
//         return { m_buffer.data() + FRAGMENT_DATA_OFFSET + sizeof(uint16_t), 32 };
//     }

//     FORCE_INLINE [[nodiscard]] std::span<const uint8_t> GetSession() const noexcept
//     {
//         if (m_sessionSize == 0) {
//             return {};
//         }

//         return {m_buffer.data() + FRAGMENT_DATA_OFFSET + sizeof(uint16_t) + 32 + sizeof(uint8_t), m_sessionIdSize};
//     }

//     class CipherSuites {
//     private:
//         std::span<uint8_t> m_buffer;

//     public:
//         FORCE_INLINE CipherSuites(const std::span<uint8_t> suitesBuffer) noexcept
//         {
//             memcpy(&m_cipherSuitesSize, suitesBuffer.data(), sizeof(uint16_t));
//             be16toh(m_cipherSuitesSize);
//             if (m_cipherSuitesSize < 2) [[unlikely]] {
//                 LOG_ERROR_NEW("Cipher suites size cannot be less than 2 bytes, {} is provided", m_cipherSuitesSize);
//                 return;
//             }
//         }
//     };
// };

/*---------------------------------------------------------------------------------
Definitions
---------------------------------------------------------------------------------*/

} // namespace TLS

} // namespace Protocol

} // namespace MSAPI

#endif // MSAPI_PROTOCOL_TLS_INL