/**************************
 * @file        objectData.inl
 * @version     6.0
 * @date        2025-11-20
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

#ifndef MSAPI_UNIT_TEST_OBJECT_DATA_INL
#define MSAPI_UNIT_TEST_OBJECT_DATA_INL

#include "../../../../library/source/protocol/object.inl"
#include "../../../../library/source/test/test.h"

namespace MSAPI {

namespace Tests {

namespace Unit {

/*---------------------------------------------------------------------------------
Declarations
---------------------------------------------------------------------------------*/

/**************************
 * @brief Unit test for ObjectData.
 *
 * @return True if all tests passed and false if something went wrong.
 */
[[nodiscard]] bool ObjectData();

/*---------------------------------------------------------------------------------
Definitions
---------------------------------------------------------------------------------*/

bool ObjectData()
{
	LOG_INFO_UNITTEST("MSAPI Object protocol Data");
	MSAPI::Test t;

	struct CustomObject {
	private:
		int m_param1;
		uint m_param2;
		double m_param3;
		uint64_t m_param4;

	public:
		CustomObject();
		CustomObject(const int param1, const uint param2, const double param3, const uint64_t param4)
			: m_param1(param1)
			, m_param2(param2)
			, m_param3(param3)
			, m_param4(param4) {};

		[[nodiscard]] static bool AreEqual(const CustomObject& a, const CustomObject& b, MSAPI::Test& t)
		{
			RETURN_IF_FALSE(t.Assert(a.m_param1, b.m_param1, "CustomObject param1"));
			RETURN_IF_FALSE(t.Assert(a.m_param2, b.m_param2, "CustomObject param2"));
			RETURN_IF_FALSE(t.Assert(Helper::FloatEqual(a.m_param3, b.m_param3), true, "CustomObject param3"));
			RETURN_IF_FALSE(t.Assert(a.m_param4, b.m_param4, "CustomObject param4"));
			return true;
		}
	};

	CustomObject first{ 1, 2, 3.369, 9009008001 };

	const auto hashCode{ typeid(CustomObject).hash_code() };
	constexpr auto objectSize{ sizeof(CustomObject) };

	MSAPI::Protocol::Object::Data data{ 1, hashCode, objectSize };
	AutoClearPtr<void> packData{ data.PackData(&first) };

	RETURN_IF_FALSE(t.Assert(data.GetObjectHash(), hashCode, "CustomObject hash code"));
	RETURN_IF_FALSE(t.Assert(data.IsValid(), true, "CustomObject data is valid"));
	RETURN_IF_FALSE(t.Assert(data.GetStreamId(), 1, "CustomObject data stream id"));

	RETURN_IF_FALSE(t.Assert(data == MSAPI::Protocol::Object::Data{ 2, hashCode, objectSize }, false,
		"Data is not equal to another one, different stream id, operator=="));
	RETURN_IF_FALSE(t.Assert(data != MSAPI::Protocol::Object::Data{ 2, hashCode, objectSize }, true,
		"Data is not equal to another one, different stream id, operator!="));

	RETURN_IF_FALSE(t.Assert(data == MSAPI::Protocol::Object::Data{ 1, hashCode + 1, objectSize }, false,
		"Data is not equal to another one, different hash code, operator=="));
	RETURN_IF_FALSE(t.Assert(data != MSAPI::Protocol::Object::Data{ 1, hashCode + 1, objectSize }, true,
		"Data is not equal to another one, different hash code, operator!="));

	RETURN_IF_FALSE(t.Assert(data == MSAPI::Protocol::Object::Data{ 1, hashCode, objectSize + 1 }, false,
		"Data is not equal to another one, different object size, operator=="));
	RETURN_IF_FALSE(t.Assert(data != MSAPI::Protocol::Object::Data{ 1, hashCode, objectSize + 1 }, true,
		"Data is not equal to another one, different object size, operator!="));

	RETURN_IF_FALSE(t.Assert(data.ToString(),
		"Object protocol:\n{"
		"\n\tcipher      : 2666999999"
		"\n\tbuffer size : "
			+ _S(32 + objectSize) + "\n\tobject hash : " + _S(hashCode)
			+ "\n\tstream id   : 1"
			  "\n}",
		"Data to string is correct"));

	const std::span<const uint8_t> dataSpan{ static_cast<const uint8_t*>(packData.Get()), 32 + objectSize };
	MSAPI::DataHeader header(dataSpan);
	MSAPI::Protocol::Object::Data dataUnpacked{ header, dataSpan };

	RETURN_IF_FALSE(t.Assert(dataUnpacked, data, "Unpacked data is equal to packed one, operator=="));
	RETURN_IF_FALSE(t.Assert(dataUnpacked != data, false, "Unpacked is data equal to packed one, operator!="));

	const void* unpackObject;
	MSAPI::Protocol::Object::Data::UnpackData(&unpackObject, packData.Get());

	RETURN_IF_FALSE(CustomObject::AreEqual(*reinterpret_cast<const CustomObject*>(unpackObject), first, t));

	RETURN_IF_FALSE(t.Assert(MSAPI::Protocol::Object::Data{ header, dataSpan.subspan(0, 27) }.GetObjectHash(), 0,
		"Hash of empty data is expected"));
	RETURN_IF_FALSE(t.Assert(MSAPI::Protocol::Object::Data{ header, dataSpan.subspan(0, 27) }.GetStreamId(), 0,
		"Stream id of empty data is expected"));
	RETURN_IF_FALSE(t.Assert(MSAPI::Protocol::Object::Data{ header, dataSpan.subspan(0, 27) }.GetCipher(), 2666999999,
		"Cipher is expected"));
	RETURN_IF_FALSE(t.Assert(MSAPI::Protocol::Object::Data{ header, dataSpan.subspan(0, 27) }.GetBufferSize(),
		32 + objectSize, "Buffer size is expected"));
	RETURN_IF_FALSE(t.Assert(
		MSAPI::Protocol::Object::Data{ header, dataSpan.subspan(0, 27) }.IsValid(), false, "Empty data is invalid"));

	RETURN_IF_FALSE(t.Assert(
		MSAPI::Protocol::Object::Data{ MSAPI::DataHeader{ std::span<const uint8_t>{} }, std::span<const uint8_t>{} }
			.GetObjectHash(),
		0, "Hash of empty data is expected"));
	RETURN_IF_FALSE(t.Assert(
		MSAPI::Protocol::Object::Data{ MSAPI::DataHeader{ std::span<const uint8_t>{} }, std::span<const uint8_t>{} }
			.GetStreamId(),
		0, "Stream id of empty data is expected"));
	RETURN_IF_FALSE(t.Assert(
		MSAPI::Protocol::Object::Data{ MSAPI::DataHeader{ std::span<const uint8_t>{} }, std::span<const uint8_t>{} }
			.GetCipher(),
		0, "Cipher of empty data is expected"));
	RETURN_IF_FALSE(t.Assert(
		MSAPI::Protocol::Object::Data{ MSAPI::DataHeader{ std::span<const uint8_t>{} }, std::span<const uint8_t>{} }
			.GetBufferSize(),
		0, "Buffer of empty data size is expected"));
	RETURN_IF_FALSE(t.Assert(
		MSAPI::Protocol::Object::Data{ MSAPI::DataHeader{ std::span<const uint8_t>{} }, std::span<const uint8_t>{} }
			.IsValid(),
		false, "Empty data is invalid"));

	RETURN_IF_FALSE(t.Assert(MSAPI::Protocol::Object::Data{ 1, 1, 0 }.IsValid(), true, "Object is valid"));
	RETURN_IF_FALSE(
		t.Assert(MSAPI::Protocol::Object::Data{ 0, 1, 0 }.IsValid(), false, "Object is invalid because of stream id"));
	RETURN_IF_FALSE(
		t.Assert(MSAPI::Protocol::Object::Data{ 1, 0, 0 }.IsValid(), false, "Object is invalid because of hash"));
	RETURN_IF_FALSE(t.Assert(
		MSAPI::Protocol::Object::Data{ MSAPI::DataHeader{ dataSpan }, dataSpan }.IsValid(), true, "Object is valid"));
	RETURN_IF_FALSE(t.Assert(MSAPI::Protocol::Object::Data{ MSAPI::DataHeader{ 2666999999 + 1 }, dataSpan }.IsValid(),
		false, "Object is invalid because of cipher"));
	std::array<uint64_t, 2> data1{ UINT64(2666999999), UINT64(15) };
	std::span<const uint8_t> data1span{ reinterpret_cast<const uint8_t*>(data1.data()),
		sizeof(uint64_t) * data1.size() };
	RETURN_IF_FALSE(t.Assert(MSAPI::Protocol::Object::Data{ MSAPI::DataHeader{ data1span }, dataSpan }.IsValid(), false,
		"Object is invalid because of buffer size"));

	return true;
}

} // namespace Unit

} // namespace Tests

} // namespace MSAPI

#endif // MSAPI_UNIT_TEST_OBJECT_DATA_INL