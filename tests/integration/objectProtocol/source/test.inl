/**************************
 * @file        test.inl
 * @date        2026-09-12
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

#ifndef MSAPI_INTEGRATION_TEST_OBJECT_PROTOCOL_INL
#define MSAPI_INTEGRATION_TEST_OBJECT_PROTOCOL_INL

#include "../../../../library/source/test/daemon.inl"
#include "../../../../library/source/test/test.inl"
#include "objectClient.h"
#include "objectDistributor.h"
#include <memory>
#include <sys/resource.h>

namespace MSAPI {

namespace Test {

namespace Integration {

/*---------------------------------------------------------------------------------
Declarations
---------------------------------------------------------------------------------*/

/**************************
 * @brief Integration test for object protocol.
 *
 * @return True if all tests passed and false if something went wrong.
 */
FORCE_INLINE [[nodiscard]] bool ObjectProtocol();

/*---------------------------------------------------------------------------------
Definitions
---------------------------------------------------------------------------------*/

FORCE_INLINE [[nodiscard]] bool ObjectProtocol()
{
	LOG_INFO("MSAPI INTEGRATION TEST: Object protocol");

	// Distributor
	auto distributorPtr{ MSAPI::Daemon<ObjectDistributor>::Create("Distributor") };
	if (distributorPtr == nullptr) [[unlikely]] {
		return false;
	}
	auto& distributor{ distributorPtr->GetApp() };

	// Client
	auto clientPtr{ MSAPI::Daemon<ObjectClient>::Create("Client") };
	if (clientPtr == nullptr) [[unlikely]] {
		return false;
	}
	auto& client{ clientPtr->GetApp() };
	auto clientToDistributorConnectionData{ client.OpenConnection(
		INADDR_LOOPBACK, distributorPtr->GetPort(), /*doReconnection=*/false) };
	if (clientToDistributorConnectionData == nullptr) [[unlikely]] {
		return false;
	}

	// Setup, stream state is closed
	MSAPI::Test::Test t;

	// Stream ids are expected
	auto instrumentsStreamId{ client.GetInstrumentStream().GetId() };
	RETURN_IF_FALSE(t.Assert(instrumentsStreamId, 0, "Instruments stream id is expected"));
	auto orderStreamId{ client.GetOrderStream().GetId() };
	RETURN_IF_FALSE(t.Assert(orderStreamId, 1, "Order stream id is expected"));

	// Attempt to close closed streams has no effect
	instrumentsStreamId = client.GetInstrumentStream().GetId();
	RETURN_IF_FALSE(t.Assert(instrumentsStreamId, 0, "Instruments stream id is expected"));
	orderStreamId = client.GetOrderStream().GetId();
	RETURN_IF_FALSE(t.Assert(orderStreamId, 1, "Order stream id is expected"));

	RETURN_IF_FALSE(t.Assert(client.GetInstrumentStream().Open(), false, "Cannot open stream without connection"));

	RETURN_IF_FALSE(t.Assert(
		client.SetConnectionForStreams(clientToDistributorConnectionData), true, "Set connections to streams"));
	RETURN_IF_FALSE(t.Assert(client.GetInstrumentStream().GetConnectionData()->GetConnectionId(),
		clientToDistributorConnectionData->GetConnectionId(), "Set connection id is expected"));
	RETURN_IF_FALSE(t.Assert(client.GetOrderStream().GetConnectionData()->GetConnectionId(),
		clientToDistributorConnectionData->GetConnectionId(), "Set connection id is expected"));
	RETURN_IF_FALSE(t.Assert(client.GetInstrumentStream().Open(), false, "Cannot open stream without filter"));
	const uint64_t figi1{ 123456789012 };
	InstrumentStructure instrument1{ InstrumentStructure::InstrumentStructureType::First, figi1, 7432435, 998274902,
		34387675464, 1000, 133, InstrumentStructure::Nominal{ 133, 1 }, true, true, true, 133, 0.25, 555666333 };
	distributor.SetInstrument(instrument1);
	MSAPI::Protocol::Object::Filter<FilterStructure> filter{ MSAPI::Protocol::Object::Type::Snapshot };
	RETURN_IF_FALSE(t.Assert(client.GetInstrumentStream().SetFilter(filter), true, "Filter is set"));
	auto streamStateData{ client.GetInstrumentStream().GetStateData() };
	RETURN_IF_FALSE(
		t.Assert(streamStateData.GetState(), MSAPI::Protocol::Object::State::Closed, "Stream state is closed"));
	RETURN_IF_FALSE(t.Assert(streamStateData.IsSnapshotDone(), false, "Snapshot is not done"));
	RETURN_IF_FALSE(t.Assert(client.GetInstrumentStream().Open(), true, "Instrument stream is opened"));

	RETURN_IF_FALSE(t.Assert(client.GetInstrumentStream().Open(), false, "Cannot open already opened stream"));

	// Waiting for HandleStreamSnapshotDone
	RETURN_IF_FALSE(t.Wait<uint64_t>(
		20000, [&client]() { return client.GetActionsNumber(); }, 3, "Client's actions: opened + instrument + done"));
	RETURN_IF_FALSE(t.Assert(client.GetInstruments().size(), 1, "Client got one instrument"));
	RETURN_IF_FALSE(
		t.Assert(std::find_if(client.GetInstruments().begin(), client.GetInstruments().end(),
					 [&instrument1](const auto& instrument) { return instrument1.ToString() == instrument.ToString(); })
				!= client.GetInstruments().end(),
			true, "Client got equal instrument"));
	streamStateData = client.GetInstrumentStream().GetStateData();
	RETURN_IF_FALSE(t.Assert(streamStateData.IsSnapshotDone(), true, "Stream snapshot is done"));

	// Waiting stream Closed state
	RETURN_IF_FALSE(t.Wait<MSAPI::Protocol::Object::State>(
		20000, [&client]() { return client.GetInstrumentStream().GetStateData().GetState(); },
		MSAPI::Protocol::Object::State::Closed, "Instrument stream state is closed"));

	// Close is uncountable action
	RETURN_IF_FALSE(t.Assert(client.GetActionsNumber(), 3, "Client's actions number is still 3"));

	// Setup for next steps
	client.Clear();
	const uint64_t figi2{ 123456789013 };
	InstrumentStructure instrument2{ InstrumentStructure::InstrumentStructureType::Second, figi2, 7432435, 998274902,
		34387675464, 1000, 133, InstrumentStructure::Nominal{ 133, 2 }, true, true, true, 133, 0.25, 555666333 };
	distributor.SetInstrument(instrument2);
	RETURN_IF_FALSE(t.Assert(client.GetInstrumentStream().Open(), true, "Instrument stream is opened"));

	// Waiting for HandleStreamSnapshotDone
	RETURN_IF_FALSE(t.Wait<uint64_t>(
		20000, [&client]() { return client.GetActionsNumber(); }, 4,
		"Client's actions: opened + 2 instruments + done"));
	RETURN_IF_FALSE(t.Assert(client.GetInstruments().size(), 2, "Client got two instruments"));
	RETURN_IF_FALSE(
		t.Assert(std::find_if(client.GetInstruments().begin(), client.GetInstruments().end(),
					 [&instrument1](const auto& instrument) { return instrument1.ToString() == instrument.ToString(); })
				!= client.GetInstruments().end(),
			true, "Client got equal instrument №1"));
	RETURN_IF_FALSE(
		t.Assert(std::find_if(client.GetInstruments().begin(), client.GetInstruments().end(),
					 [&instrument2](const auto& instrument) { return instrument2.ToString() == instrument.ToString(); })
				!= client.GetInstruments().end(),
			true, "Client got equal instrument №2"));
	streamStateData = client.GetInstrumentStream().GetStateData();
	RETURN_IF_FALSE(t.Assert(streamStateData.IsSnapshotDone(), true, "Stream snapshot is done"));

	// Waiting stream Closed state
	RETURN_IF_FALSE(t.Wait<MSAPI::Protocol::Object::State>(
		20000, [&client]() { return client.GetInstrumentStream().GetStateData().GetState(); },
		MSAPI::Protocol::Object::State::Closed, "Instrument stream state is closed"));

	// Close is uncountable action
	RETURN_IF_FALSE(t.Assert(client.GetActionsNumber(), 4, "Client's actions number is still 4"));

	// Setup for next steps
	client.Clear();
	MSAPI::Protocol::Object::Filter<FilterStructure> filter2{ MSAPI::Protocol::Object::Type::SnapshotAndLive };
	RETURN_IF_FALSE(t.Assert(client.GetInstrumentStream().SetFilter(std::move(filter2)), true, "Filter is set"));
	RETURN_IF_FALSE(t.Assert(client.GetInstrumentStream().Open(), true, "Instrument stream is opened"));

	// Waiting for HandleStreamSnapshotDone
	RETURN_IF_FALSE(t.Wait<uint64_t>(
		20000, [&client]() { return client.GetActionsNumber(); }, 4,
		"Client's actions: opened + 2 instruments + done"));
	RETURN_IF_FALSE(t.Assert(client.GetInstruments().size(), 2, "Client got two instruments"));
	RETURN_IF_FALSE(
		t.Assert(std::find_if(client.GetInstruments().begin(), client.GetInstruments().end(),
					 [&instrument1](const auto& instrument) { return instrument1.ToString() == instrument.ToString(); })
				!= client.GetInstruments().end(),
			true, "Client got equal instrument №1"));
	RETURN_IF_FALSE(
		t.Assert(std::find_if(client.GetInstruments().begin(), client.GetInstruments().end(),
					 [&instrument2](const auto& instrument) { return instrument2.ToString() == instrument.ToString(); })
				!= client.GetInstruments().end(),
			true, "Client got equal instrument №2"));
	streamStateData = client.GetInstrumentStream().GetStateData();
	RETURN_IF_FALSE(t.Assert(streamStateData.IsSnapshotDone(), true, "Stream is snapshot done"));

	// Stream still opened
	RETURN_IF_FALSE(
		t.Assert(streamStateData.GetState(), MSAPI::Protocol::Object::State::Opened, "Stream state still is opened"));

	// Waiting one more instrument
	const uint64_t figi3{ 123456789014 };
	InstrumentStructure instrument3{ InstrumentStructure::InstrumentStructureType::Third, figi3, 7432435, 998274902,
		34387675464, 1000, 133, InstrumentStructure::Nominal{ 133, 3 }, true, true, true, 133, 0.25, 555666333 };
	distributor.SetInstrument(instrument3);
	RETURN_IF_FALSE(t.Wait<uint64_t>(
		20000, [&client]() { return client.GetActionsNumber(); }, 5,
		"Client's actions: opened + 3 instruments + done"));
	RETURN_IF_FALSE(t.Assert(client.GetInstruments().size(), 3, "Client got three instruments"));
	RETURN_IF_FALSE(
		t.Assert(std::find_if(client.GetInstruments().begin(), client.GetInstruments().end(),
					 [&instrument3](const auto& instrument) { return instrument3.ToString() == instrument.ToString(); })
				!= client.GetInstruments().end(),
			true, "Client got equal instrument №3"));

	// Stream still snapshot done
	streamStateData = client.GetInstrumentStream().GetStateData();
	RETURN_IF_FALSE(t.Assert(streamStateData.IsSnapshotDone(), true, "Stream still is snapshot done"));

	// Stream still opened
	RETURN_IF_FALSE(
		t.Assert(streamStateData.GetState(), MSAPI::Protocol::Object::State::Opened, "Stream state still is opened"));

	// Check actions number
	RETURN_IF_FALSE(t.Assert(client.GetActionsNumber(), 5, "Client's actions number is still 5"));

	// Close stream manually, action is applied on client side immediately
	client.GetInstrumentStream().Close();
	instrumentsStreamId = client.GetInstrumentStream().GetId();
	RETURN_IF_FALSE(t.Assert(instrumentsStreamId, 2, "Instruments stream changed id on client close"));
	streamStateData = client.GetInstrumentStream().GetStateData();
	RETURN_IF_FALSE(t.Assert(
		streamStateData.GetState(), MSAPI::Protocol::Object::State::Closed, "Instrument stream state is closed"));
	RETURN_IF_FALSE(t.Assert(streamStateData.IsSnapshotDone(), false, "Instrument stream snapshot flag is cleared"));

	// Check actions number
	RETURN_IF_FALSE(t.Assert(client.GetActionsNumber(), 5, "Client's actions number is still 5"));

	// Setup for next steps
	client.Clear();
	MSAPI::Protocol::Object::Filter<FilterStructure> filter3{ MSAPI::Protocol::Object::Type::Snapshot };
	FilterStructure figiFilter3{ figi3 };
	RETURN_IF_FALSE(t.Assert(filter3.SetObject(figiFilter3), 1, "Filters count after setting"));
	RETURN_IF_FALSE(t.Assert(client.GetInstrumentStream().SetFilter(std::move(filter3)), true, "Filter is set"));
	RETURN_IF_FALSE(t.Assert(client.GetInstrumentStream().Open(), true, "Instrument stream is opened"));

	// Waiting for HandleStreamSnapshotDone
	RETURN_IF_FALSE(t.Wait<uint64_t>(
		20000, [&client]() { return client.GetActionsNumber(); }, 3, "Client's actions: opened + instrument + done"));
	RETURN_IF_FALSE(t.Assert(client.GetInstruments().size(), 1, "Client got one instrument"));
	RETURN_IF_FALSE(
		t.Assert(std::find_if(client.GetInstruments().begin(), client.GetInstruments().end(),
					 [&instrument3](const auto& instrument) { return instrument3.ToString() == instrument.ToString(); })
				!= client.GetInstruments().end(),
			true, "Client got equal instrument №3 from stream with filter"));
	streamStateData = client.GetInstrumentStream().GetStateData();
	RETURN_IF_FALSE(t.Assert(streamStateData.IsSnapshotDone(), true, "Stream is snapshot done"));

	// Waiting stream Closed state
	RETURN_IF_FALSE(t.Wait<MSAPI::Protocol::Object::State>(
		20000, [&client]() { return client.GetInstrumentStream().GetStateData().GetState(); },
		MSAPI::Protocol::Object::State::Closed, "Instrument stream state is closed"));

	// Close is uncountable action
	RETURN_IF_FALSE(t.Assert(client.GetActionsNumber(), 3, "Client's actions number is still 3"));

	// Setup for next steps
	client.Clear();
	MSAPI::Protocol::Object::Filter<FilterStructure> filter4{ MSAPI::Protocol::Object::Type::Snapshot };
	FilterStructure figiFilter2{ figi2 };
	RETURN_IF_FALSE(t.Assert(filter4.SetObject(figiFilter2), 1, "Filters count after setting"));
	RETURN_IF_FALSE(t.Assert(filter4.SetObject(figiFilter3), 2, "Filters count after setting"));
	RETURN_IF_FALSE(t.Assert(client.GetInstrumentStream().SetFilter(std::move(filter4)), true, "Filter is set"));
	RETURN_IF_FALSE(t.Assert(client.GetInstrumentStream().Open(), true, "Instrument stream is opened"));

	// Waiting for HandleStreamSnapshotDone
	RETURN_IF_FALSE(t.Wait<uint64_t>(
		20000, [&client]() { return client.GetActionsNumber(); }, 4,
		"Client's actions: opened + 2 instruments + done"));
	RETURN_IF_FALSE(t.Assert(client.GetInstruments().size(), 2, "Client got two instruments"));
	RETURN_IF_FALSE(
		t.Assert(std::find_if(client.GetInstruments().begin(), client.GetInstruments().end(),
					 [&instrument2](const auto& instrument) { return instrument2.ToString() == instrument.ToString(); })
				!= client.GetInstruments().end(),
			true, "Client got equal instrument №2 from stream with filter"));
	RETURN_IF_FALSE(
		t.Assert(std::find_if(client.GetInstruments().begin(), client.GetInstruments().end(),
					 [&instrument3](const auto& instrument) { return instrument3.ToString() == instrument.ToString(); })
				!= client.GetInstruments().end(),
			true, "Client got equal instrument №3 from stream with filter"));
	streamStateData = client.GetInstrumentStream().GetStateData();
	RETURN_IF_FALSE(t.Assert(streamStateData.IsSnapshotDone(), true, "Stream is snapshot done"));

	// Waiting stream Closed state
	RETURN_IF_FALSE(t.Wait<MSAPI::Protocol::Object::State>(
		20000, [&client]() { return client.GetInstrumentStream().GetStateData().GetState(); },
		MSAPI::Protocol::Object::State::Closed, "Instrument stream state is closed"));

	// Close is uncountable action
	RETURN_IF_FALSE(t.Assert(client.GetActionsNumber(), 4, "Client's actions number is still 4"));

	// Setup for next steps
	client.Clear();
	const uint64_t figi4{ 123456789015 };
	const uint64_t figi5{ 123456789016 };
	MSAPI::Protocol::Object::Filter<FilterStructure> filter5{ MSAPI::Protocol::Object::Type::SnapshotAndLive };
	InstrumentStructure instrument4{ InstrumentStructure::InstrumentStructureType::Fourth, figi4, 7432435, 998274902,
		34387675464, 1000, 133, InstrumentStructure::Nominal{ 133, 4 }, true, true, true, 133, 0.25, 555666333 };
	InstrumentStructure instrument5{ InstrumentStructure::InstrumentStructureType::First, figi5, 7432435, 998274902,
		34387675464, 1000, 133, InstrumentStructure::Nominal{ 133, 5 }, true, true, true, 133, 0.25, 555666333 };
	FilterStructure figiFilter4{ figi4 };
	FilterStructure figiFilter5{ figi5 };
	RETURN_IF_FALSE(t.Assert(filter5.SetObject(figiFilter2), 1, "Filters count after setting"));
	RETURN_IF_FALSE(t.Assert(filter5.SetObject(figiFilter3), 2, "Filters count after setting"));
	RETURN_IF_FALSE(t.Assert(filter5.SetObject(figiFilter4), 3, "Filters count after setting"));
	RETURN_IF_FALSE(t.Assert(filter5.SetObject(figiFilter5), 4, "Filters count after setting"));
	RETURN_IF_FALSE(t.Assert(client.GetInstrumentStream().SetFilter(std::move(filter5)), true, "Filter is set"));
	RETURN_IF_FALSE(t.Assert(client.GetInstrumentStream().Open(), true, "Instrument stream is opened"));

	// Waiting for HandleStreamSnapshotDone
	RETURN_IF_FALSE(t.Wait<uint64_t>(
		20000, [&client]() { return client.GetActionsNumber(); }, 4,
		"Client's actions: opened + 2 instruments + done"));
	RETURN_IF_FALSE(t.Assert(client.GetInstruments().size(), 2, "Client got two instruments"));
	RETURN_IF_FALSE(
		t.Assert(std::find_if(client.GetInstruments().begin(), client.GetInstruments().end(),
					 [&instrument2](const auto& instrument) { return instrument2.ToString() == instrument.ToString(); })
				!= client.GetInstruments().end(),
			true, "Client got equal instrument №2"));
	RETURN_IF_FALSE(
		t.Assert(std::find_if(client.GetInstruments().begin(), client.GetInstruments().end(),
					 [&instrument3](const auto& instrument) { return instrument3.ToString() == instrument.ToString(); })
				!= client.GetInstruments().end(),
			true, "Client got equal instrument №3"));
	streamStateData = client.GetInstrumentStream().GetStateData();
	RETURN_IF_FALSE(t.Assert(streamStateData.IsSnapshotDone(), true, "Stream is snapshot done"));

	// Stream still opened
	RETURN_IF_FALSE(
		t.Assert(streamStateData.GetState(), MSAPI::Protocol::Object::State::Opened, "Stream state still is opened"));

	// Waiting one more instrument
	distributor.SetInstrument(instrument4);
	RETURN_IF_FALSE(t.Wait<uint64_t>(
		20000, [&client]() { return client.GetActionsNumber(); }, 5,
		"Client's actions: opened + 3 instruments + done"));
	RETURN_IF_FALSE(t.Assert(client.GetInstruments().size(), 3, "Client got three instruments"));
	RETURN_IF_FALSE(
		t.Assert(std::find_if(client.GetInstruments().begin(), client.GetInstruments().end(),
					 [&instrument4](const auto& instrument) { return instrument4.ToString() == instrument.ToString(); })
				!= client.GetInstruments().end(),
			true, "Client got equal instrument №4"));

	// Stream still snapshot done
	streamStateData = client.GetInstrumentStream().GetStateData();
	RETURN_IF_FALSE(t.Assert(streamStateData.IsSnapshotDone(), true, "Stream still is snapshot done"));

	// Stream still opened
	RETURN_IF_FALSE(
		t.Assert(streamStateData.GetState(), MSAPI::Protocol::Object::State::Opened, "Stream state still is opened"));

	// Check actions number
	RETURN_IF_FALSE(t.Assert(client.GetActionsNumber(), 5, "Client's actions number is still 5"));

	// Waiting one more instrument
	distributor.SetInstrument(instrument5);
	RETURN_IF_FALSE(t.Wait<uint64_t>(
		20000, [&client]() { return client.GetActionsNumber(); }, 6,
		"Client's actions: opened + 4 instruments + done"));
	RETURN_IF_FALSE(t.Assert(client.GetInstruments().size(), 4, "Client got four instruments"));
	RETURN_IF_FALSE(
		t.Assert(std::find_if(client.GetInstruments().begin(), client.GetInstruments().end(),
					 [&instrument5](const auto& instrument) { return instrument5.ToString() == instrument.ToString(); })
				!= client.GetInstruments().end(),
			true, "Client got equal instrument №5"));

	// Stream still snapshot done
	streamStateData = client.GetInstrumentStream().GetStateData();
	RETURN_IF_FALSE(t.Assert(streamStateData.IsSnapshotDone(), true, "Stream still is snapshot done"));

	// Stream still opened
	RETURN_IF_FALSE(
		t.Assert(streamStateData.GetState(), MSAPI::Protocol::Object::State::Opened, "Stream state still is opened"));

	// Check actions number
	RETURN_IF_FALSE(t.Assert(client.GetActionsNumber(), 6, "Client's actions number is still 6"));

	// Close stream manually, action is applied on client side immediately
	client.GetInstrumentStream().Close();
	instrumentsStreamId = client.GetInstrumentStream().GetId();
	RETURN_IF_FALSE(t.Assert(instrumentsStreamId, 3, "Instruments stream changed id on client close"));
	streamStateData = client.GetInstrumentStream().GetStateData();
	RETURN_IF_FALSE(t.Assert(
		streamStateData.GetState(), MSAPI::Protocol::Object::State::Closed, "Instrument stream state is closed"));
	RETURN_IF_FALSE(t.Assert(streamStateData.IsSnapshotDone(), false, "Instrument stream snapshot flag is cleared"));

	// Check actions number
	RETURN_IF_FALSE(t.Assert(client.GetActionsNumber(), 6, "Client's actions number is still 6"));

	// Setup for next steps
	client.Clear();
	distributor.Clear();
	MSAPI::Protocol::Object::Filter<FilterStructure> filter6{ MSAPI::Protocol::Object::Type::SnapshotAndLive };
	RETURN_IF_FALSE(t.Assert(filter6.SetObject(figiFilter2), 1, "Filters count after setting"));
	RETURN_IF_FALSE(t.Assert(filter6.SetObject(figiFilter3), 2, "Filters count after setting"));
	RETURN_IF_FALSE(t.Assert(filter6.SetObject(figiFilter4), 3, "Filters count after setting"));
	RETURN_IF_FALSE(t.Assert(client.GetInstrumentStream().SetFilter(std::move(filter6)), true, "Filter is set"));
	streamStateData = client.GetInstrumentStream().GetStateData();
	RETURN_IF_FALSE(t.Assert(
		streamStateData.GetState(), MSAPI::Protocol::Object::State::Closed, "Instrument stream state is closed"));
	RETURN_IF_FALSE(t.Assert(streamStateData.IsSnapshotDone(), false, "Instrument stream snapshot flag is cleared"));
	streamStateData = client.GetOrderStream().GetStateData();
	RETURN_IF_FALSE(
		t.Assert(streamStateData.GetState(), MSAPI::Protocol::Object::State::Closed, "Order stream state is closed"));
	RETURN_IF_FALSE(t.Assert(streamStateData.IsSnapshotDone(), false, "Order stream snapshot flag is cleared"));
	OrderStructure order1{ figi1, 100.0, 20 };
	OrderStructure order2{ figi2, 100.0, 20 };
	OrderStructure order3{ figi3, 100.0, 20 };
	OrderStructure order4{ figi4, 100.0, 20 };
	OrderStructure order5{ figi5, 100.0, 20 };
	distributor.SetOrder(order1);
	distributor.SetInstrument(instrument1);
	distributor.SetOrder(order2);
	distributor.SetInstrument(instrument2);

	RETURN_IF_FALSE(t.Assert(client.GetInstrumentStream().Open(), true, "Instrument stream is opened"));
	RETURN_IF_FALSE(t.Assert(client.GetOrderStream().Open(), false, "Try open stream without filter"));
	MSAPI::Protocol::Object::Filter<FilterStructure> filter7{ MSAPI::Protocol::Object::Type::SnapshotAndLive };
	RETURN_IF_FALSE(t.Assert(filter7.SetObject(figiFilter2), 1, "Filters count after setting"));
	RETURN_IF_FALSE(t.Assert(filter7.SetObject(figiFilter3), 2, "Filters count after setting"));
	RETURN_IF_FALSE(t.Assert(filter7.SetObject(figiFilter4), 3, "Filters count after setting"));
	RETURN_IF_FALSE(t.Assert(client.GetOrderStream().SetFilter(filter7), true, "Filter is set"));
	RETURN_IF_FALSE(t.Assert(client.GetOrderStream().Open(), true, "Order stream is opened"));

	// Waiting instrument and order stream' HandleStreamSnapshotDone calls
	RETURN_IF_FALSE(t.Wait<uint64_t>(
		20000, [&client]() { return client.GetActionsNumber(); }, 6,
		"Client's actions: 2 opened + 1 instrument + 1 order + 2 done"));
	RETURN_IF_FALSE(t.Assert(client.GetInstruments().size(), 1, "Client got one instrument"));
	RETURN_IF_FALSE(
		t.Assert(std::find_if(client.GetInstruments().begin(), client.GetInstruments().end(),
					 [&instrument2](const auto& instrument) { return instrument2.ToString() == instrument.ToString(); })
				!= client.GetInstruments().end(),
			true, "Client got equal instrument №2"));
	RETURN_IF_FALSE(t.Assert(client.GetOrders().size(), 1, "Client got one order"));
	RETURN_IF_FALSE(t.Assert(std::find_if(client.GetOrders().begin(), client.GetOrders().end(),
								 [&order2](const auto& order) { return order2.ToString() == order.ToString(); })
			!= client.GetOrders().end(),
		true, "Client got equal order №2"));

	// Streams still opened
	streamStateData = client.GetInstrumentStream().GetStateData();
	RETURN_IF_FALSE(t.Assert(
		streamStateData.GetState(), MSAPI::Protocol::Object::State::Opened, "Instrument stream state still is opened"));
	RETURN_IF_FALSE(t.Assert(streamStateData.IsSnapshotDone(), true, "Instrument stream snapshot is done"));
	streamStateData = client.GetOrderStream().GetStateData();
	RETURN_IF_FALSE(t.Assert(
		streamStateData.GetState(), MSAPI::Protocol::Object::State::Opened, "Order stream state still is opened"));
	RETURN_IF_FALSE(t.Assert(streamStateData.IsSnapshotDone(), true, "Order stream snapshot is done"));

	// Check actions number
	RETURN_IF_FALSE(t.Assert(client.GetActionsNumber(), 6, "Client's actions number is still 6"));

	// Waiting one more instrument and order
	distributor.SetInstrument(instrument3);
	distributor.SetOrder(order3);
	RETURN_IF_FALSE(t.Wait<uint64_t>(
		20000, [&client]() { return client.GetActionsNumber(); }, 8,
		"Client's actions: 2 opened + 2 instrument + 2 order + 2 done"));
	RETURN_IF_FALSE(t.Assert(client.GetInstruments().size(), 2, "Client got two instruments"));
	RETURN_IF_FALSE(
		t.Assert(std::find_if(client.GetInstruments().begin(), client.GetInstruments().end(),
					 [&instrument3](const auto& instrument) { return instrument3.ToString() == instrument.ToString(); })
				!= client.GetInstruments().end(),
			true, "Client got equal instrument №3"));
	RETURN_IF_FALSE(t.Assert(client.GetOrders().size(), 2, "Client got two orders"));
	RETURN_IF_FALSE(t.Assert(std::find_if(client.GetOrders().begin(), client.GetOrders().end(),
								 [&order3](const auto& order) { return order3.ToString() == order.ToString(); })
			!= client.GetOrders().end(),
		true, "Client got equal order №3"));

	// Streams still opened and snapshot done
	streamStateData = client.GetInstrumentStream().GetStateData();
	RETURN_IF_FALSE(t.Assert(
		streamStateData.GetState(), MSAPI::Protocol::Object::State::Opened, "Instrument stream state still is opened"));
	RETURN_IF_FALSE(t.Assert(streamStateData.IsSnapshotDone(), true, "Instrument stream snapshot is done"));
	streamStateData = client.GetOrderStream().GetStateData();
	RETURN_IF_FALSE(t.Assert(
		streamStateData.GetState(), MSAPI::Protocol::Object::State::Opened, "Order stream state still is opened"));
	RETURN_IF_FALSE(t.Assert(streamStateData.IsSnapshotDone(), true, "Order stream snapshot is done"));

	// Check actions number
	RETURN_IF_FALSE(t.Assert(client.GetActionsNumber(), 8, "Client's actions number is still 8"));

	// Waiting one more instrument and order
	distributor.SetInstrument(instrument4);
	distributor.SetOrder(order4);
	RETURN_IF_FALSE(t.Wait<uint64_t>(
		20000, [&client]() { return client.GetActionsNumber(); }, 10,
		"Client's actions: 2 opened + 3 instrument + 3 order + 2 done"));
	RETURN_IF_FALSE(t.Assert(client.GetInstruments().size(), 3, "Client got three instruments"));
	RETURN_IF_FALSE(
		t.Assert(std::find_if(client.GetInstruments().begin(), client.GetInstruments().end(),
					 [&instrument4](const auto& instrument) { return instrument4.ToString() == instrument.ToString(); })
				!= client.GetInstruments().end(),
			true, "Client got equal instrument №4"));
	RETURN_IF_FALSE(t.Assert(client.GetOrders().size(), 3, "Client got three orders"));
	RETURN_IF_FALSE(t.Assert(std::find_if(client.GetOrders().begin(), client.GetOrders().end(),
								 [&order4](const auto& order) { return order4.ToString() == order.ToString(); })
			!= client.GetOrders().end(),
		true, "Client got equal order №4"));

	// Streams still opened and snapshot done
	streamStateData = client.GetInstrumentStream().GetStateData();
	RETURN_IF_FALSE(t.Assert(
		streamStateData.GetState(), MSAPI::Protocol::Object::State::Opened, "Instrument stream state still is opened"));
	RETURN_IF_FALSE(t.Assert(streamStateData.IsSnapshotDone(), true, "Instrument stream snapshot is done"));
	streamStateData = client.GetOrderStream().GetStateData();
	RETURN_IF_FALSE(t.Assert(
		streamStateData.GetState(), MSAPI::Protocol::Object::State::Opened, "Order stream state still is opened"));
	RETURN_IF_FALSE(t.Assert(streamStateData.IsSnapshotDone(), true, "Order stream snapshot is done"));

	// Check actions number
	RETURN_IF_FALSE(t.Assert(client.GetActionsNumber(), 10, "Client's actions number is 10"));

	// Set objects what does not match to stream's filter
	distributor.SetInstrument(instrument5);
	distributor.SetOrder(order5);

	// Streams still opened and snapshot done
	streamStateData = client.GetInstrumentStream().GetStateData();
	RETURN_IF_FALSE(t.Assert(
		streamStateData.GetState(), MSAPI::Protocol::Object::State::Opened, "Instrument stream state still is opened"));
	RETURN_IF_FALSE(t.Assert(streamStateData.IsSnapshotDone(), true, "Instrument stream snapshot is done"));
	streamStateData = client.GetOrderStream().GetStateData();
	RETURN_IF_FALSE(t.Assert(
		streamStateData.GetState(), MSAPI::Protocol::Object::State::Opened, "Order stream state still is opened"));
	RETURN_IF_FALSE(t.Assert(streamStateData.IsSnapshotDone(), true, "Order stream snapshot is done"));

	// Check actions number
	RETURN_IF_FALSE(t.Assert(client.GetActionsNumber(), 10, "Client's actions number is still 10"));

	// Close streams manually, action is applied on client side immediately
	client.GetInstrumentStream().Close();
	instrumentsStreamId = client.GetInstrumentStream().GetId();
	RETURN_IF_FALSE(t.Assert(instrumentsStreamId, 4, "Instruments stream changed id on client close"));
	streamStateData = client.GetInstrumentStream().GetStateData();
	RETURN_IF_FALSE(t.Assert(
		streamStateData.GetState(), MSAPI::Protocol::Object::State::Closed, "Instrument stream state is closed"));
	RETURN_IF_FALSE(t.Assert(streamStateData.IsSnapshotDone(), false, "Instrument stream snapshot flag is cleared"));
	client.GetOrderStream().Close();
	orderStreamId = client.GetOrderStream().GetId();
	RETURN_IF_FALSE(t.Assert(orderStreamId, 5, "Order stream changed id on client close"));
	streamStateData = client.GetOrderStream().GetStateData();
	RETURN_IF_FALSE(
		t.Assert(streamStateData.GetState(), MSAPI::Protocol::Object::State::Closed, "Order stream state is closed"));
	RETURN_IF_FALSE(t.Assert(streamStateData.IsSnapshotDone(), false, "Order stream snapshot flag is cleared"));

	// Check actions number
	RETURN_IF_FALSE(t.Assert(client.GetActionsNumber(), 10, "Client's actions number is still 10"));

	// Attempt to close closed streams has no effect
	instrumentsStreamId = client.GetInstrumentStream().GetId();
	RETURN_IF_FALSE(t.Assert(instrumentsStreamId, 4, "Instruments stream id is expected"));
	orderStreamId = client.GetOrderStream().GetId();
	RETURN_IF_FALSE(t.Assert(orderStreamId, 5, "Order stream id is expected"));

	// Check that no other objects are arrived
	RETURN_IF_FALSE(t.Assert(client.GetInstruments().size(), 3, "Client still got three instruments"));
	RETURN_IF_FALSE(t.Assert(client.GetOrders().size(), 3, "Client still got three orders"));

	// Open order stream with snapshot and live, filter and distributor's orders are unchanged since last close
	RETURN_IF_FALSE(t.Assert(client.GetOrderStream().Open(), true, "Order stream is opened"));

	// Waiting for HandleStreamSnapshotDone
	RETURN_IF_FALSE(t.Wait<uint64_t>(
		40000, [&client]() { return client.GetActionsNumber(); }, 15, "Client's actions: opened + 3 orders + done"));
	RETURN_IF_FALSE(t.Assert(client.GetOrders().size(), 3, "Client still got three orders"));
	streamStateData = client.GetOrderStream().GetStateData();
	RETURN_IF_FALSE(
		t.Assert(streamStateData.GetState(), MSAPI::Protocol::Object::State::Opened, "Order stream state is opened"));
	RETURN_IF_FALSE(t.Assert(streamStateData.IsSnapshotDone(), true, "Order stream snapshot is done"));

	// Second distributor
	auto distributor2Ptr{ MSAPI::Daemon<ObjectDistributor>::Create("Distributor2") };
	if (distributor2Ptr == nullptr) [[unlikely]] {
		return false;
	}
	auto& distributor2{ distributor2Ptr->GetApp() };
	const uint64_t figi6{ 123456789017 };
	InstrumentStructure instrument6{ InstrumentStructure::InstrumentStructureType::Second, figi6, 7432435, 998274902,
		34387675464, 1000, 133, InstrumentStructure::Nominal{ 133, 6 }, true, true, true, 133, 0.25, 555666333 };
	distributor2.SetInstrument(instrument6);
	const auto clientToDistributor2ConnectionData{ client.OpenConnection(
		INADDR_LOOPBACK, distributor2Ptr->GetPort(), /*doReconnection=*/false) };
	if (clientToDistributor2ConnectionData == nullptr) [[unlikely]] {
		return false;
	}

	// Reassign instrument stream to second distributor on client side, stream is closed so it is allowed
	RETURN_IF_FALSE(t.Assert(client.GetInstrumentStream().SetConnectionData(clientToDistributor2ConnectionData), true,
		"Instrument stream is reassigned to second distributor"));
	RETURN_IF_FALSE(t.Assert(client.GetInstrumentStream().GetConnectionData()->GetConnectionId(),
		clientToDistributor2ConnectionData->GetConnectionId(), "Set connection id is expected"));
	FilterStructure figiFilter6{ figi6 };
	MSAPI::Protocol::Object::Filter<FilterStructure> filter8{ MSAPI::Protocol::Object::Type::Snapshot };
	RETURN_IF_FALSE(t.Assert(filter8.SetObject(figiFilter6), 1, "Filters count after setting"));
	RETURN_IF_FALSE(t.Assert(client.GetInstrumentStream().SetFilter(std::move(filter8)), true, "Filter is set"));

	// Open instrument stream and check expectations
	RETURN_IF_FALSE(t.Assert(client.GetInstrumentStream().Open(), true, "Instrument stream is opened"));
	RETURN_IF_FALSE(t.Wait<uint64_t>(
		20000, [&client]() { return client.GetActionsNumber(); }, 18, "Client's actions: opened + instrument + done"));
	RETURN_IF_FALSE(t.Assert(client.GetInstruments().size(), 4, "Client got four instruments"));
	RETURN_IF_FALSE(
		t.Assert(std::find_if(client.GetInstruments().begin(), client.GetInstruments().end(),
					 [&instrument6](const auto& instrument) { return instrument6.ToString() == instrument.ToString(); })
				!= client.GetInstruments().end(),
			true, "Client got equal instrument №6 from second distributor"));

	// Waiting stream Closed state
	RETURN_IF_FALSE(t.Wait<MSAPI::Protocol::Object::State>(
		20000, [&client]() { return client.GetInstrumentStream().GetStateData().GetState(); },
		MSAPI::Protocol::Object::State::Closed, "Instrument stream state is closed"));
	streamStateData = client.GetInstrumentStream().GetStateData();
	RETURN_IF_FALSE(t.Assert(streamStateData.IsSnapshotDone(), true, "Instrument stream snapshot is done"));

	// Attempt to reassign order stream while it is opened must fail
	RETURN_IF_FALSE(t.Assert(client.GetOrderStream().SetConnectionData(clientToDistributor2ConnectionData), false,
		"Order stream reassign fails while stream is opened"));
	RETURN_IF_FALSE(t.Assert(client.GetOrderStream().GetConnectionData()->GetConnectionId(),
		clientToDistributorConnectionData->GetConnectionId(), "Associated connection id is not changed"));

	// Close order stream and reassign it once more, now it must succeed
	client.GetOrderStream().Close();
	orderStreamId = client.GetOrderStream().GetId();
	RETURN_IF_FALSE(t.Assert(orderStreamId, 6, "Order stream changed id on client close"));
	streamStateData = client.GetOrderStream().GetStateData();
	RETURN_IF_FALSE(
		t.Assert(streamStateData.GetState(), MSAPI::Protocol::Object::State::Closed, "Order stream state is closed"));
	RETURN_IF_FALSE(t.Assert(streamStateData.IsSnapshotDone(), false, "Order stream snapshot flag is cleared"));
	RETURN_IF_FALSE(t.Assert(client.GetOrderStream().SetConnectionData(clientToDistributor2ConnectionData), true,
		"Order stream is reassigned to second distributor after close"));
	RETURN_IF_FALSE(t.Assert(client.GetOrderStream().GetConnectionData()->GetConnectionId(),
		clientToDistributor2ConnectionData->GetConnectionId(), "Set connection id is expected"));

	// Open reassigned order stream and check it receives orders from second distributor
	OrderStructure order6{ figi6, 100.0, 20 };
	distributor2.SetOrder(order6);
	FilterStructure orderFigiFilter6{ figi6 };
	MSAPI::Protocol::Object::Filter<FilterStructure> filter9{ MSAPI::Protocol::Object::Type::Snapshot };
	RETURN_IF_FALSE(t.Assert(filter9.SetObject(orderFigiFilter6), 1, "Filters count after setting"));
	RETURN_IF_FALSE(t.Assert(client.GetOrderStream().SetFilter(std::move(filter9)), true, "Filter is set"));
	RETURN_IF_FALSE(t.Assert(client.GetOrderStream().Open(), true, "Order stream is opened"));
	RETURN_IF_FALSE(t.Wait<uint64_t>(
		20000, [&client]() { return client.GetActionsNumber(); }, 21, "Client's actions: opened + order + done"));
	RETURN_IF_FALSE(t.Assert(client.GetOrders().size(), 4, "Client got four orders"));
	RETURN_IF_FALSE(t.Assert(std::find_if(client.GetOrders().begin(), client.GetOrders().end(),
								 [&order6](const auto& order) { return order6.ToString() == order.ToString(); })
			!= client.GetOrders().end(),
		true, "Client got equal order №6 from second distributor"));
	RETURN_IF_FALSE(t.Wait<MSAPI::Protocol::Object::State>(
		20000, [&client]() { return client.GetOrderStream().GetStateData().GetState(); },
		MSAPI::Protocol::Object::State::Closed, "Order stream state is closed"));
	streamStateData = client.GetOrderStream().GetStateData();
	RETURN_IF_FALSE(t.Assert(streamStateData.IsSnapshotDone(), true, "Order stream snapshot is done"));

	// Setup for server closes stream scenario, use first distributor for instrument stream once more
	client.Clear();
	RETURN_IF_FALSE(t.Assert(client.GetInstrumentStream().SetConnectionData(clientToDistributorConnectionData), true,
		"Instrument stream is reassigned back to first distributor"));
	RETURN_IF_FALSE(t.Assert(client.GetInstrumentStream().GetConnectionData()->GetConnectionId(),
		clientToDistributorConnectionData->GetConnectionId(), "Set connection id is expected"));
	MSAPI::Protocol::Object::Filter<FilterStructure> filter10{ MSAPI::Protocol::Object::Type::SnapshotAndLive };
	RETURN_IF_FALSE(t.Assert(filter10.SetObject(figiFilter2), 1, "Filters count after setting"));
	RETURN_IF_FALSE(t.Assert(client.GetInstrumentStream().SetFilter(filter10), true, "Filter is set"));
	RETURN_IF_FALSE(t.Assert(client.GetInstrumentStream().Open(), true, "Instrument stream is opened"));
	RETURN_IF_FALSE(t.Wait<uint64_t>(
		20000, [&client]() { return client.GetActionsNumber(); }, 3, "Client's actions: opened + instrument + done"));
	streamStateData = client.GetInstrumentStream().GetStateData();
	RETURN_IF_FALSE(t.Assert(
		streamStateData.GetState(), MSAPI::Protocol::Object::State::Opened, "Instrument stream state is opened"));
	RETURN_IF_FALSE(t.Assert(streamStateData.IsSnapshotDone(), true, "Instrument stream snapshot is done"));

	// Distributor stops, active stream is marked Failed and client is notified
	distributor.StopDistribution();
	RETURN_IF_FALSE(t.Wait<MSAPI::Protocol::Object::State>(
		20000, [&client]() { return client.GetInstrumentStream().GetStateData().GetState(); },
		MSAPI::Protocol::Object::State::Failed, "Instrument stream state is failed after distributor stop"));
	RETURN_IF_FALSE(t.Assert(streamStateData.IsSnapshotDone(), true, "Instrument stream snapshot is remained"));
	RETURN_IF_FALSE(t.Assert(
		client.GetLastFailedIssue(), MSAPI::Protocol::Object::Issue::DistributorStopped, "Failed issue is expected"));

	// Setup for unexpected connection interruption scenario
	client.Clear();
	RETURN_IF_FALSE(t.Assert(client.GetInstrumentStream().SetFilter(filter10), true, "Filter is set"));

	// Distributor unexpectedly closes underlying connection without notifying the client
	distributor.CloseConnection();
	RETURN_IF_FALSE(t.Wait<MSAPI::Application::State>(
		20000, [&client]() { return client.MSAPI::Application::GetState(); }, MSAPI::Application::State::Paused,
		"Client application state is paused"));

	// Client attempts to open stream over the dead connection, send fails and stream state becomes Failed locally
	RETURN_IF_FALSE(
		t.Assert(client.GetInstrumentStream().Open(), false, "Instrument stream open fails over dead connection"));
	streamStateData = client.GetInstrumentStream().GetStateData();
	RETURN_IF_FALSE(t.Assert(
		streamStateData.GetState(), MSAPI::Protocol::Object::State::Failed, "Instrument stream state is remained"));
	RETURN_IF_FALSE(t.Assert(streamStateData.IsSnapshotDone(), true, "Instrument stream snapshot is remained"));
	RETURN_IF_FALSE(t.Assert(client.GetActionsNumber(), 1, "Client's actions: stream failed"));

	// Reconnect client to distributor
	clientToDistributorConnectionData
		= client.OpenConnection(INADDR_LOOPBACK, distributorPtr->GetPort(), /*doReconnection=*/false);
	if (clientToDistributorConnectionData == nullptr) [[unlikely]] {
		return false;
	}

	// Reopen failed stream and check its state reflection
	client.Clear();
	{
		const MSAPI::Lock::AtomicRW::Guard<MSAPI::Lock::read> _{ distributor.GetDistributionLock() };

		RETURN_IF_FALSE(t.Assert(client.GetInstrumentStream().SetConnectionData(clientToDistributorConnectionData),
			true, "Instrument stream is reassigned to distributor again"));
		RETURN_IF_FALSE(t.Assert(client.GetInstrumentStream().Open(), true, "Instrument stream is opened"));
		RETURN_IF_FALSE(t.Wait<MSAPI::Protocol::Object::State>(
			20000, [&client]() { return client.GetInstrumentStream().GetStateData().GetState(); },
			MSAPI::Protocol::Object::State::Opened, "Instrument stream state is opened"));
		streamStateData = client.GetInstrumentStream().GetStateData();
		RETURN_IF_FALSE(t.Assert(streamStateData.IsSnapshotDone(), false, "Instrument stream snapshot is cleared"));
	}
	RETURN_IF_FALSE(t.Wait<uint64_t>(
		20000, [&client]() { return client.GetActionsNumber(); }, 2,
		"Client's actions: opened + done and instrument is dropped as client is paused"));
	streamStateData = client.GetInstrumentStream().GetStateData();
	RETURN_IF_FALSE(t.Assert(
		streamStateData.GetState(), MSAPI::Protocol::Object::State::Opened, "Instrument stream state is opened"));
	RETURN_IF_FALSE(t.Assert(streamStateData.IsSnapshotDone(), true, "Instrument stream snapshot is done"));

	// Distributor unexpectedly closes underlying connection without notifying the client
	distributor.CloseConnection();

	// Active stream is marked Failed and client is notified
	RETURN_IF_FALSE(t.Wait<uint64_t>(
		20000, [&client]() { return client.GetActionsNumber(); }, 3, "Client's actions: stream failed"));
	RETURN_IF_FALSE(t.Wait<MSAPI::Protocol::Object::State>(
		20000, [&client]() { return client.GetInstrumentStream().GetStateData().GetState(); },
		MSAPI::Protocol::Object::State::Failed,
		"Instrument stream state is failed after distributor closes connection"));
	RETURN_IF_FALSE(t.Assert(streamStateData.IsSnapshotDone(), true, "Instrument stream snapshot is remained"));
	RETURN_IF_FALSE(t.Assert(
		client.GetLastFailedIssue(), MSAPI::Protocol::Object::Issue::DistributorStopped, "Failed issue is expected"));

	// Attempt to close failed stream
	client.GetInstrumentStream().Close();
	instrumentsStreamId = client.GetInstrumentStream().GetId();
	RETURN_IF_FALSE(t.Assert(instrumentsStreamId, 4, "Failed instruments stream does not change id on close"));
	streamStateData = client.GetInstrumentStream().GetStateData();
	RETURN_IF_FALSE(t.Assert(streamStateData.GetState(), MSAPI::Protocol::Object::State::Failed,
		"Failed instruments stream state does not change state on close"));
	RETURN_IF_FALSE(t.Assert(
		streamStateData.IsSnapshotDone(), true, "Failed instruments stream snapshot flag is remained on close"));

	// Distributor-side connection cleanup: open stream and receive data
	client.HandleRunRequest();
	client.Clear();
	clientToDistributorConnectionData
		= client.OpenConnection(INADDR_LOOPBACK, distributorPtr->GetPort(), /*doReconnection=*/false);
	if (clientToDistributorConnectionData == nullptr) [[unlikely]] {
		return false;
	}
	RETURN_IF_FALSE(t.Assert(client.GetInstrumentStream().SetConnectionData(clientToDistributorConnectionData), true,
		"Instrument stream is assigned after client reconnect"));
	RETURN_IF_FALSE(t.Assert(client.GetInstrumentStream().SetFilter(filter10), true, "Filter is set"));
	RETURN_IF_FALSE(t.Assert(client.GetInstrumentStream().Open(), true, "Instrument stream is opened"));
	RETURN_IF_FALSE(t.Wait<uint64_t>(
		20000, [&client]() { return client.GetActionsNumber(); }, 3,
		"Client's actions: opened + instrument + done before client close"));
	RETURN_IF_FALSE(t.Assert(client.GetInstruments().size(), 1, "Client got one instrument before client close"));
	RETURN_IF_FALSE(t.Assert(client.HasInstrument(instrument2), true, "Client got instrument before client close"));
	streamStateData = client.GetInstrumentStream().GetStateData();
	RETURN_IF_FALSE(
		t.Assert(streamStateData.GetState(), MSAPI::Protocol::Object::State::Opened, "Instrument stream is opened"));
	RETURN_IF_FALSE(t.Assert(streamStateData.IsSnapshotDone(), true, "Instrument stream snapshot is done"));

	// Distributor-side connection cleanup: close client connection ungracefully
	clientToDistributorConnectionData->GetConnection().Close();
	RETURN_IF_FALSE(t.Wait<MSAPI::Application::State>(
		20000, [&client]() { return client.MSAPI::Application::GetState(); }, MSAPI::Application::State::Paused,
		"Client application is paused after client connection close"));
	RETURN_IF_FALSE(t.Wait<uint64_t>(
		20000, [&client]() { return client.GetActionsNumber(); }, 4,
		"Client's actions: stream failed after client close"));
	streamStateData = client.GetInstrumentStream().GetStateData();
	RETURN_IF_FALSE(
		t.Assert(streamStateData.GetState(), MSAPI::Protocol::Object::State::Failed, "Instrument stream is failed"));
	RETURN_IF_FALSE(t.Assert(
		client.GetLastFailedIssue(), MSAPI::Protocol::Object::Issue::DistributorStopped, "Failed issue is expected"));

	// Distributor-side connection cleanup: reopen the same stream and receive data
	client.HandleRunRequest();
	client.Clear();
	clientToDistributorConnectionData
		= client.OpenConnection(INADDR_LOOPBACK, distributorPtr->GetPort(), /*doReconnection=*/false);
	if (clientToDistributorConnectionData == nullptr) [[unlikely]] {
		return false;
	}
	RETURN_IF_FALSE(t.Assert(client.GetInstrumentStream().SetConnectionData(clientToDistributorConnectionData), true,
		"Instrument stream is reassigned after client connection cleanup"));
	RETURN_IF_FALSE(t.Assert(client.GetInstrumentStream().SetFilter(filter10), true, "Filter is set again"));
	RETURN_IF_FALSE(t.Assert(client.GetInstrumentStream().Open(), true, "Instrument stream is reopened"));
	RETURN_IF_FALSE(t.Wait<uint64_t>(
		20000, [&client]() { return client.GetActionsNumber(); }, 3,
		"Client's actions: reopened + instrument + done after client cleanup"));
	RETURN_IF_FALSE(t.Assert(client.GetInstruments().size(), 1, "Client got one instrument after client cleanup"));
	RETURN_IF_FALSE(t.Assert(client.HasInstrument(instrument2), true, "Client got instrument after client cleanup"));
	streamStateData = client.GetInstrumentStream().GetStateData();
	RETURN_IF_FALSE(t.Assert(
		streamStateData.GetState(), MSAPI::Protocol::Object::State::Opened, "Reopened instrument stream is opened"));
	RETURN_IF_FALSE(t.Assert(streamStateData.IsSnapshotDone(), true, "Reopened instrument stream snapshot is done"));

	// Attempt to set nullptr as connection
	RETURN_IF_FALSE(
		t.Assert(client.GetInstrumentStream().SetConnectionData({}), false, "Failed to set nullptr as connection"));
	RETURN_IF_FALSE(t.Assert(client.GetInstrumentStream().GetConnectionData()->GetConnectionId(),
		clientToDistributorConnectionData->GetConnectionId(), "Associated connection id is not changed"));

	// Verify that client does not have any unhandled actions
	RETURN_IF_FALSE(t.Assert(client.GetUnhandledActions(), 0, "Client does not have unhandled actions"));
	RETURN_IF_FALSE(t.Assert(distributor.GetUnhandledActions(), 0, "Distributor does not have unhandled actions"));
	RETURN_IF_FALSE(t.Assert(distributor2.GetUnhandledActions(), 0, "Distributor2 does not have unhandled actions"));

	distributor2Ptr.reset();
	distributorPtr.reset();
	clientPtr.reset();

	return t.Passed<bool>();
}

} // namespace Integration

} // namespace Test

} // namespace MSAPI

#endif // MSAPI_INTEGRATION_TEST_OBJECT_PROTOCOL_INL