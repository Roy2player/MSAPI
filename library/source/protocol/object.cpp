/**************************
 * @file        object.cpp
 * @version     6.0
 * @date        2023-08-29
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

#include "object.h"
#include "../help/autoClearPtr.inl"
#include "../help/diagnostic.inl"
#include "../help/helper.h"
#include "../server/application.h"

namespace MSAPI {

namespace Protocol {

namespace Object {

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

uint64_t Data::GetHash() const { return m_hash; }

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

int32_t Data::GetStreamId() const { return m_streamId; }

void* Data::PackData(const void* data) const
{
	void* buffer{ malloc(m_bufferSize) };
	memcpy(buffer, &m_cipher, sizeof(uint64_t));
	memcpy(&static_cast<char*>(buffer)[sizeof(uint64_t)], &m_bufferSize, sizeof(uint64_t));
	memcpy(&static_cast<char*>(buffer)[sizeof(uint64_t) * 2], &m_streamId, sizeof(int32_t));
	memcpy(&static_cast<char*>(buffer)[sizeof(uint64_t) * 2 + sizeof(int32_t)], &m_hash, sizeof(uint64_t));
	memcpy(&static_cast<char*>(buffer)[sizeof(uint64_t) * 3 + sizeof(int32_t)], data,
		m_bufferSize - sizeof(uint64_t) * 3 - sizeof(int32_t));
	// Diagnostic::PrintBinaryDescriptor<Diagnostic::binary>(buffer, m_bufferSize, "Packed object data");
	return buffer;
}

void Data::UnpackData(const void** ptr, const void* buffer)
{
	*ptr = &(static_cast<const char*>(buffer)[sizeof(uint64_t) * 3 + sizeof(int32_t)]);
}

bool Data::IsValid() const
{
	return m_cipher == 2666999999 && m_bufferSize >= sizeof(uint64_t) * 3 + sizeof(int32_t) && m_hash != 0
		&& m_streamId != 0;
}

/*---------------------------------------------------------------------------------
IHandlerBase
---------------------------------------------------------------------------------*/

const std::map<int32_t, StreamBase*>& IHandlerBase::GetStreamsContainer() const { return m_streamToId; }

void IHandlerBase::SetStream(const int32_t streamId, StreamBase* stream)
{
	if (!m_streamToId.emplace(streamId, stream).second) {
		LOG_ERROR("Duplicate a stream id: " + _S(streamId));
	}
}

void IHandlerBase::RemoveStream(const int32_t streamId) { m_streamToId.erase(streamId); }

void IHandlerBase::CollectStreamState(const int32_t streamId, const StreamStateResponse* state)
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

uint64_t FilterBase::GetFilterSize() const { return m_filterSize; }

bool FilterBase::Empty() const { return m_filterSize == 0; }

void FilterBase::IncrementFilterSize() { ++m_filterSize; }

uint64_t FilterBase::GetStreamObjectHash() const { return m_streamObjectHash; }

void FilterBase::SetStreamObjectHash(const uint64_t streamObjectHash) { m_streamObjectHash = streamObjectHash; }

uint64_t FilterBase::GetFilterObjectHash() const
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

StreamBase::StreamBase()
	: m_id{ m_streamCounter.fetch_add(1, std::memory_order_relaxed) }
{
}

State StreamBase::GetState() const { return m_state; }

int32_t StreamBase::GetId() const noexcept { return m_id; }

int32_t StreamBase::GetConnection() const { return m_connection; }

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

void StreamBase::SetConnection(const int32_t connection) { m_connection = connection; }

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

void Send(const int32_t connection, const Data& data, const void* object)
{
	LOG_PROTOCOL("Send data: " + data.ToString() + ", to connection: " + _S(connection));
	AutoClearPtr<void> packData{ data.PackData(object) };

	if (send(connection, packData.Get(), data.GetBufferSize(), MSG_NOSIGNAL) == -1) {
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

} // namespace Object

} // namespace Protocol

} // namespace MSAPI