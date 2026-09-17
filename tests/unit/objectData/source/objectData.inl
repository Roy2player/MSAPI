/**************************
 * @file        objectData.inl
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
#include "../../../../library/source/test/test.inl"

namespace MSAPI {

namespace Test {

namespace Unit {

/*---------------------------------------------------------------------------------
Declarations
---------------------------------------------------------------------------------*/

/**************************
 * @brief Unit test for ObjectData.
 *
 * @return True if all tests passed and false if something went wrong.
 */
FORCE_INLINE [[nodiscard]] bool ObjectData();

/*---------------------------------------------------------------------------------
Definitions
---------------------------------------------------------------------------------*/

FORCE_INLINE [[nodiscard]] bool ObjectData()
{
	static_assert(U(MSAPI::Protocol::Object::Type::Max) == 3, "Absence of stream type enum transcription");
	static_assert(MSAPI::Protocol::Object::EnumToString(MSAPI::Protocol::Object::Type::Undefined) == "Undefined",
		"EnumToString for Type::Undefined");
	static_assert(U(MSAPI::Protocol::Object::Type::Undefined) == 0, "Number for Type::Undefined");
	static_assert(MSAPI::Protocol::Object::EnumToString(MSAPI::Protocol::Object::Type::Snapshot) == "Snapshot",
		"EnumToString for Type::Snapshot");
	static_assert(U(MSAPI::Protocol::Object::Type::Snapshot) == 1, "Number for Type::Snapshot");
	static_assert(
		MSAPI::Protocol::Object::EnumToString(MSAPI::Protocol::Object::Type::SnapshotAndLive) == "Snapshot and live",
		"EnumToString for Type::SnapshotAndLive");
	static_assert(U(MSAPI::Protocol::Object::Type::SnapshotAndLive) == 2, "Number for Type::SnapshotAndLive");
	static_assert(MSAPI::Protocol::Object::EnumToString(MSAPI::Protocol::Object::Type::Max) == "Max",
		"EnumToString for Type::Max");

	static_assert(U(MSAPI::Protocol::Object::State::Max) == 6, "Absence of stream state enum transcription");
	static_assert(MSAPI::Protocol::Object::EnumToString(MSAPI::Protocol::Object::State::Undefined) == "Undefined",
		"EnumToString for State::Undefined");
	static_assert(U(MSAPI::Protocol::Object::State::Undefined) == 0, "Number for State::Undefined");
	static_assert(MSAPI::Protocol::Object::EnumToString(MSAPI::Protocol::Object::State::Pending) == "Pending",
		"EnumToString for State::Pending");
	static_assert(U(MSAPI::Protocol::Object::State::Pending) == 1, "Number for State::Pending");
	static_assert(MSAPI::Protocol::Object::EnumToString(MSAPI::Protocol::Object::State::Opened) == "Opened",
		"EnumToString for State::Opened");
	static_assert(U(MSAPI::Protocol::Object::State::Opened) == 2, "Number for State::Opened");
	static_assert(MSAPI::Protocol::Object::EnumToString(MSAPI::Protocol::Object::State::Done) == "Done",
		"EnumToString for State::Done");
	static_assert(U(MSAPI::Protocol::Object::State::Done) == 3, "Number for State::Done");
	static_assert(MSAPI::Protocol::Object::EnumToString(MSAPI::Protocol::Object::State::Failed) == "Failed",
		"EnumToString for State::Failed");
	static_assert(U(MSAPI::Protocol::Object::State::Failed) == 4, "Number for State::Failed");
	static_assert(MSAPI::Protocol::Object::EnumToString(MSAPI::Protocol::Object::State::Closed) == "Closed",
		"EnumToString for State::Closed");
	static_assert(U(MSAPI::Protocol::Object::State::Closed) == 5, "Number for State::Closed");
	static_assert(MSAPI::Protocol::Object::EnumToString(MSAPI::Protocol::Object::State::Max) == "Max",
		"EnumToString for State::Max");

	static_assert(U(MSAPI::Protocol::Object::Issue::Max) == 9, "Absence of stream issue enum transcription");
	static_assert(MSAPI::Protocol::Object::EnumToString(MSAPI::Protocol::Object::Issue::Undefined) == "Undefined",
		"EnumToString for Issue::Undefined");
	static_assert(U(MSAPI::Protocol::Object::Issue::Undefined) == 0, "Number for Issue::Undefined");
	static_assert(MSAPI::Protocol::Object::EnumToString(MSAPI::Protocol::Object::Issue::Empty) == "Empty",
		"EnumToString for Issue::Empty");
	static_assert(U(MSAPI::Protocol::Object::Issue::Empty) == 1, "Number for Issue::Empty");
	static_assert(MSAPI::Protocol::Object::EnumToString(MSAPI::Protocol::Object::Issue::StreamIsAlreadyOpened)
			== "Stream is already opened",
		"EnumToString for Issue::StreamIsAlreadyOpened");
	static_assert(
		U(MSAPI::Protocol::Object::Issue::StreamIsAlreadyOpened) == 2, "Number for Issue::StreamIsAlreadyOpened");
	static_assert(MSAPI::Protocol::Object::EnumToString(MSAPI::Protocol::Object::Issue::StreamDoesNotExist)
			== "Stream does not exist",
		"EnumToString for Issue::StreamDoesNotExist");
	static_assert(U(MSAPI::Protocol::Object::Issue::StreamDoesNotExist) == 3, "Number for Issue::StreamDoesNotExist");
	static_assert(MSAPI::Protocol::Object::EnumToString(MSAPI::Protocol::Object::Issue::FilterObjectHashMismatch)
			== "Filter object hash mismatch",
		"EnumToString for Issue::FilterObjectHashMismatch");
	static_assert(
		U(MSAPI::Protocol::Object::Issue::FilterObjectHashMismatch) == 4, "Number for Issue::FilterObjectHashMismatch");
	static_assert(
		MSAPI::Protocol::Object::EnumToString(MSAPI::Protocol::Object::Issue::FilterNotFound) == "Filter not found",
		"EnumToString for Issue::FilterNotFound");
	static_assert(U(MSAPI::Protocol::Object::Issue::FilterNotFound) == 5, "Number for Issue::FilterNotFound");
	static_assert(MSAPI::Protocol::Object::EnumToString(MSAPI::Protocol::Object::Issue::FilterSizeExceeded)
			== "Filter size exceeded",
		"EnumToString for Issue::FilterSizeExceeded");
	static_assert(U(MSAPI::Protocol::Object::Issue::FilterSizeExceeded) == 6, "Number for Issue::FilterSizeExceeded");
	static_assert(MSAPI::Protocol::Object::EnumToString(MSAPI::Protocol::Object::Issue::UnknownHash) == "Unknown hash",
		"EnumToString for Issue::UnknownHash");
	static_assert(U(MSAPI::Protocol::Object::Issue::UnknownHash) == 7, "Number for Issue::UnknownHash");
	static_assert(MSAPI::Protocol::Object::EnumToString(MSAPI::Protocol::Object::Issue::DistributorStopped)
			== "Distributor stopped",
		"EnumToString for Issue::DistributorStopped");
	static_assert(U(MSAPI::Protocol::Object::Issue::DistributorStopped) == 8, "Number for Issue::DistributorStopped");
	static_assert(MSAPI::Protocol::Object::EnumToString(MSAPI::Protocol::Object::Issue::Max) == "Max",
		"EnumToString for Issue::Max");

	static_assert(sizeof(MSAPI::Protocol::Object::Data) % 16 == 0, "In buffer object alignment is correct");

	static_assert(MSAPI::Protocol::Object::Data::CIPHER == 2666999999, "Data cipher is expected");

	static_assert(MSAPI::Protocol::Object::CLEANUP_INSIDE, "CLEANUP_INSIDE values is expected");
	static_assert(!MSAPI::Protocol::Object::CLEANUP_OUTSIDE, "CLEANUP_OUTSIDE values is expected");

	LOG_INFO("MSAPI UNIT TEST Object protocol Data");
	MSAPI::Test::Test t;

	// StreamStateResponse
	{
		MSAPI::Protocol::Object::StreamStateResponse response;
		RETURN_IF_FALSE(t.Assert(response.state, MSAPI::Protocol::Object::State::Undefined,
			"Default state of stream state response is expected"));
		RETURN_IF_FALSE(t.Assert(response.issue, MSAPI::Protocol::Object::Issue::Empty,
			"Default issue of stream state response is expected"));
	}

	// Filter
	{
		MSAPI::Protocol::Object::FilterBase filter{ MSAPI::Protocol::Object::Type::SnapshotAndLive };

		struct TestStruct {
			uint16_t field{};

			FORCE_INLINE [[nodiscard]] bool operator==(const TestStruct&) const noexcept = default;
		};

		TestStruct object1{ 11 };
		TestStruct object2{ 441 };

		MSAPI::Protocol::Object::Filter<TestStruct> filter2{ std::move(filter) };
		RETURN_IF_FALSE(t.Assert(filter2.SetObject(object1), 1, "Expected number of objects"));
		RETURN_IF_FALSE(t.Assert(filter2.SetObject(object2), 2, "Expected number of objects"));

		RETURN_IF_FALSE(t.Assert(filter2.GetObjects().size(), 2, "Filter objects are expected"));
		RETURN_IF_FALSE(t.Assert(filter2.GetObjects()[0], object1, "Filter objects are expected"));
		RETURN_IF_FALSE(t.Assert(filter2.GetObjects()[1], object2, "Filter objects are expected"));

		RETURN_IF_FALSE(t.Assert(filter2.ToString(),
			"Filter special:\n{"
			"\n\tfilter object hash : "
				+ _S(typeid(TestStruct).hash_code())
				+ "\n\tfilter size        : 2"
				  "\n\t                   : Filter base:\n{"
				  "\n\ttype               : Snapshot and live"
				  "\n\tstream object hash : 0"
				  "\n\ttotal filter size  : 0"
				  "\n}\n}",
			"Filter to string is expected"));
	}

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

		[[nodiscard]] static bool AreEqual(const CustomObject& a, const CustomObject& b, MSAPI::Test::Test& t)
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
	MSAPI::Protocol::Object::Data::GetPointerToObjectInBuffer(&unpackObject, packData.Get());

	RETURN_IF_FALSE(CustomObject::AreEqual(*reinterpret_cast<const CustomObject*>(unpackObject), first, t));

	RETURN_IF_FALSE(t.Assert(MSAPI::Protocol::Object::Data{ header, dataSpan.subspan(0, 27) }.GetObjectHash(), 0,
		"Hash of empty data is expected"));
	RETURN_IF_FALSE(t.Assert(MSAPI::Protocol::Object::Data{ header, dataSpan.subspan(0, 27) }.GetStreamId(), 0,
		"Stream id of empty data is expected"));
	RETURN_IF_FALSE(t.Assert(MSAPI::Protocol::Object::Data{ header, dataSpan.subspan(0, 27) }.GetCipher(),
		MSAPI::Protocol::Object::Data::CIPHER, "Cipher is expected"));
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
	RETURN_IF_FALSE(t.Assert(
		MSAPI::Protocol::Object::Data{ MSAPI::DataHeader{ MSAPI::Protocol::Object::Data::CIPHER + 1 }, dataSpan }
			.IsValid(),
		false, "Object is invalid because of cipher"));
	std::array<uint64_t, 2> data1{ UINT64(MSAPI::Protocol::Object::Data::CIPHER), UINT64(15) };
	std::span<const uint8_t> data1span{ reinterpret_cast<const uint8_t*>(data1.data()),
		sizeof(uint64_t) * data1.size() };
	RETURN_IF_FALSE(t.Assert(MSAPI::Protocol::Object::Data{ MSAPI::DataHeader{ data1span }, dataSpan }.IsValid(), false,
		"Object is invalid because of buffer size"));

	return t.Passed<bool>();
}

} // namespace Unit

} // namespace Test

} // namespace MSAPI

#endif // MSAPI_UNIT_TEST_OBJECT_DATA_INL