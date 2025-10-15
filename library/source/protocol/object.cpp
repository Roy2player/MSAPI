/**************************
 * @file        object.cpp
 * @version     6.0
 * @date        2023-08-29
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

#include "object.h"
#include "../help/autoClearPtr.hpp"
#include "../help/helper.h"
#include "../server/application.h"
#include "../test/test.h"

namespace MSAPI {

namespace ObjectProtocol {

/*---------------------------------------------------------------------------------
StreamData
---------------------------------------------------------------------------------*/

std::string StreamData::ToString() const
{
	return std::format("Stream data:\n{{"
					   "\n\tconnection         : {}"
					   "\n\ttype               : {}"
					   "\n\topen               : {}"
					   "\n\tfilter object hash : {}"
					   "\n\tfilter size        : {}"
					   "\n}}",
		connection, EnumToString(type), open, objectHash, filterSize);
}

/*---------------------------------------------------------------------------------
Data
---------------------------------------------------------------------------------*/

size_t Data::GetHash() const { return m_hash; }

std::string Data::ToString() const
{
	return std::format("Object protocol:\n{{"
					   "\n\tcipher      : {}"
					   "\n\tbuffer size : {}"
					   "\n\thash        : {}"
					   "\n\tstream id   : {}"
					   "\n}}",
		m_cipher, m_bufferSize, m_hash, m_streamId);
}

int Data::GetStreamId() const { return m_streamId; }

void* Data::PackData(const void* data) const
{
	void* buffer{ malloc(m_bufferSize) };
	memcpy(buffer, &m_cipher, sizeof(size_t));
	memcpy(&static_cast<char*>(buffer)[sizeof(size_t)], &m_bufferSize, sizeof(size_t));
	memcpy(&static_cast<char*>(buffer)[sizeof(size_t) * 2], &m_streamId, sizeof(int));
	memcpy(&static_cast<char*>(buffer)[sizeof(size_t) * 2 + sizeof(int)], &m_hash, sizeof(size_t));
	memcpy(&static_cast<char*>(buffer)[sizeof(size_t) * 3 + sizeof(int)], data,
		m_bufferSize - sizeof(size_t) * 3 - sizeof(int));
	// Diagnostic::PrintBinaryDescriptor(buffer, m_bufferSize, "Packed memory");
	return buffer;
}

void Data::UnpackData(void** ptr, void* buffer)
{
	*ptr = &(static_cast<char*>(buffer)[sizeof(size_t) * 3 + sizeof(int)]);
}

bool Data::IsValid() const { return m_cipher == 2666999999 && m_bufferSize >= sizeof(size_t) * 3 + sizeof(int); }

bool Data::UNITTEST()
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
	const auto objectSize{ sizeof(first) };

	Data data{ 1, hashCode, objectSize };
	AutoClearPtr<void> packData{ data.PackData(&first) };

	RETURN_IF_FALSE(t.Assert(data.GetHash(), hashCode, "CustomObject hash code"));
	RETURN_IF_FALSE(t.Assert(data.IsValid(), true, "CustomObject data is valid"));
	RETURN_IF_FALSE(t.Assert(data.GetStreamId(), 1, "CustomObject data stream id"));

	RETURN_IF_FALSE(t.Assert(data == Data{ 2, hashCode, objectSize }, false,
		"Data is not equal to another one, different stream id, operator=="));
	RETURN_IF_FALSE(t.Assert(data != Data{ 2, hashCode, objectSize }, true,
		"Data is not equal to another one, different stream id, operator!="));

	RETURN_IF_FALSE(t.Assert(data == Data{ 1, hashCode + 1, objectSize }, false,
		"Data is not equal to another one, different hash code, operator=="));
	RETURN_IF_FALSE(t.Assert(data != Data{ 1, hashCode + 1, objectSize }, true,
		"Data is not equal to another one, different hash code, operator!="));

	RETURN_IF_FALSE(t.Assert(data == Data{ 1, hashCode, objectSize + 1 }, false,
		"Data is not equal to another one, different object size, operator=="));
	RETURN_IF_FALSE(t.Assert(data != Data{ 1, hashCode, objectSize + 1 }, true,
		"Data is not equal to another one, different object size, operator!="));

	RETURN_IF_FALSE(t.Assert(data.ToString(),
		"Object protocol:\n{"
		"\n\tcipher      : 2666999999"
		"\n\tbuffer size : "
			+ _S(28 + objectSize) + "\n\thash        : " + _S(hashCode)
			+ "\n\tstream id   : 1"
			  "\n}",
		"Data to string is correct"));

	DataHeader header(packData.ptr);
	Data dataUnpacked{ header, packData.ptr };

	RETURN_IF_FALSE(t.Assert(dataUnpacked, data, "Unpacked data is equal to packed one, operator=="));
	RETURN_IF_FALSE(t.Assert(dataUnpacked != data, false, "Unpacked is data equal to packed one, operator!="));

	void* unpackObject;
	Data::UnpackData(&unpackObject, packData.ptr);

	RETURN_IF_FALSE(CustomObject::AreEqual(*reinterpret_cast<const CustomObject*>(unpackObject), first, t));

	return true;
}

/*---------------------------------------------------------------------------------
IHandlerBase
---------------------------------------------------------------------------------*/

const std::map<int, StreamBase*>& IHandlerBase::GetStreamsContainer() const { return m_streamToId; }

void IHandlerBase::SetStream(const int streamId, StreamBase* stream)
{
	if (!m_streamToId.emplace(streamId, stream).second) {
		LOG_ERROR("Duplicate a stream id: " + _S(streamId));
	}
}

void IHandlerBase::RemoveStream(const int streamId) { m_streamToId.erase(streamId); }

void IHandlerBase::CollectStreamState(const int streamId, const StreamStateResponse* state)
{
	const auto it = m_streamToId.find(streamId);
	if (it == m_streamToId.end()) {
		LOG_WARNING_NEW("Got state for unknown stream id: {}, state: {}", streamId, EnumToString(state->state));
		return;
	}

	LOG_PROTOCOL_NEW("Client got stream state: {}, for stream id: {}", EnumToString(state->state), streamId);
	it->second->SetState(state->state);
	switch (state->state) {
	case State::Opened:
		HandleStreamOpened(streamId);
		return;
	case State::Done:
		HandleStreamSnapshotDone(streamId);
		return;
	case State::Failed:
		HandleStreamFailed(streamId);
		return;
	case State::Closed:
		return;
	default:
		LOG_ERROR_NEW("Unknown state for stream id: {}, state: {}", streamId, EnumToString(state->state));
		return;
	}
}

/*---------------------------------------------------------------------------------
FilterBase
---------------------------------------------------------------------------------*/

FilterBase::FilterBase(const Type type)
	: m_type(type)
{
}

Type FilterBase::GetType() const { return m_type; }

void FilterBase::SetType(const Type type) { m_type = type; }

size_t FilterBase::GetFilterSize() const { return m_filterSize; }

bool FilterBase::Empty() const { return m_filterSize == 0; }

void FilterBase::IncrementFilterSize() { ++m_filterSize; }

size_t FilterBase::GetStreamObjectHash() const { return m_streamObjectHash; }

void FilterBase::SetStreamObjectHash(const size_t streamObjectHash) { m_streamObjectHash = streamObjectHash; }

size_t FilterBase::GetFilterObjectHash() const
{
	LOG_ERROR("Call unexpected method by FilterBase class");
	return 0;
}

std::string FilterBase::ToString() const
{
	return std::format("Filter base:\n{{"
					   "\n\ttype               : {}"
					   "\n\tstream object hash : {}"
					   "\n\tfilter size        : {}"
					   "\n}}",
		EnumToString(m_type), m_streamObjectHash, m_filterSize);
}

/*---------------------------------------------------------------------------------
StreamBase
---------------------------------------------------------------------------------*/

std::set<int> StreamBase::m_streamIds;

StreamBase::StreamBase()
{
	int id;
	do {
		id = static_cast<int>(Identifier::mersenne());
	} while (m_streamIds.find(id) != m_streamIds.end());
	Identifier::SetId(id);
	m_streamIds.emplace(id);
}

StreamBase::~StreamBase() { m_streamIds.erase(GetId()); }

State StreamBase::GetState() const { return m_state; }

int StreamBase::GetConnection() const { return m_connection; }

bool StreamBase::Empty() const { return m_connection == 0; }

void StreamBase::SetState(const State state)
{
	if (state == State::Done) {
		m_snapshotDone = true;
		return;
	}

	m_state = state;
}

bool StreamBase::IsSnapshotDone() const { return m_snapshotDone; }

void StreamBase::SetConnection(const int connection) { m_connection = connection; }

/*---------------------------------------------------------------------------------
ApplicationStateChecker
---------------------------------------------------------------------------------*/

ApplicationStateChecker::ApplicationStateChecker(const MSAPI::Application* application)
	: m_application(application)
{
}

bool ApplicationStateChecker::CheckApplicationState() const { return m_application->Application::IsRunning(); }

/*---------------------------------------------------------------------------------
Another
---------------------------------------------------------------------------------*/

void Send(const int connection, const Data& data, const void* object)
{
	LOG_PROTOCOL("Send data: " + data.ToString() + ", to connection: " + _S(connection));
	AutoClearPtr<void> packData{ data.PackData(object) };

	if (send(connection, packData.ptr, data.GetBufferSize(), MSG_NOSIGNAL) == -1) {
		if (errno == 104) {
			LOG_DEBUG("Send returned error №104: Connection reset by peer");
			return;
		}
		LOG_ERROR("Send event failed, connection: " + _S(connection) + ", data: " + data.ToString() + ". Error №"
			+ _S(errno) + ": " + std::strerror(errno));
	}
}

std::string_view EnumToString(const Type value)
{
	static_assert(U(Type::Max) == 3, "You need to add new stream type enum description");

	switch (value) {
	case Type::Undefined:
		return "Undefined";
	case Type::Snapshot:
		return "Snapshot";
	case Type::SnapshotAndLive:
		return "Snapshot and live";
	case Type::Max:
		return "Max";
	default:
		LOG_ERROR("Unknown stream type enum: " + _S(U(value)));
		return "Unknown";
	}
}

std::string_view EnumToString(const State value)
{
	static_assert(U(State::Max) == 7, "You need to add new stream state enum description");

	switch (value) {
	case State::Undefined:
		return "Undefined";
	case State::Pending:
		return "Pending";
	case State::Opened:
		return "Opened";
	case State::Done:
		return "Done";
	case State::Failed:
		return "Failed";
	case State::Closed:
		return "Closed";
	case State::Removed:
		return "Removed";
	case State::Max:
		return "Max";
	default:
		LOG_ERROR("Unknown stream state enum: " + _S(U(value)));
		return "Unknown";
	}
}

std::string_view EnumToString(const Issue value)
{
	static_assert(U(Issue::Max) == 8, "You need to add new stream issue enum description");

	switch (value) {
	case Issue::Undefined:
		return "Undefined";
	case Issue::Empty:
		return "Empty";
	case Issue::NotUniqueFilter:
		return "Not unique filter";
	case Issue::ReservedFilterObjectWithoutFilter:
		return "Reserved filter object without filter";
	case Issue::UnknownFilterObjectHash:
		return "Unknown filter object hash";
	case Issue::UnknownHash:
		return "Unknown hash";
	case Issue::BadVariantAccess:
		return "Bad variant access";
	case Issue::ExtraFilterObject:
		return "Extra filter object";
	case Issue::Max:
		return "Max";
	default:
		LOG_ERROR("Unknown stream issue enum: " + _S(U(value)));
		return "Unknown";
	}
}

}; //* namespace ObjectProtocol

}; //* namespace MSAPI