/**************************
 * @file        main.cpp
 * @version     6.0
 * @date        2024-05-01
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

#include "../../../library/source/help/bin.h"
#include "../../../library/source/test/daemon.hpp"
#include "../../../library/source/test/test.h"
#include "objectClient.h"
#include "objectDistributor.h"
#include <memory>
#include <sys/mman.h>
#include <sys/resource.h>

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
	MSAPI_MLOCKALL_CURRENT_FUTURE

	std::string path;
	path.resize(512);
	MSAPI::Helper::GetExecutableDir(path);
	if (path.empty()) [[unlikely]] {
		return 1;
	}
	path += "../";
	MSAPI::logger.SetParentPath(path);

	path += "logs/";
	{

		auto logs{ MSAPI::Bin::ListFiles<MSAPI::Bin::FileType::Regular, std::vector<std::string>>(path) };
		for (const auto& file : logs) {
			MSAPI::Bin::Remove(path + file);
		}
	}

	MSAPI::logger.SetLevelSave(MSAPI::Log::Level::INFO);
	MSAPI::logger.SetName("TestOP");
	MSAPI::logger.SetToFile(true);
	MSAPI::logger.SetToConsole(true);
	MSAPI::logger.Start();

	//* Distributor
	const int distributorId{ 1 };
	auto distributorPtr{ MSAPI::Daemon<ObjectDistributor>::Create("Distributor") };
	if (distributorPtr == nullptr) {
		return 1;
	}
	auto distributor{ static_cast<ObjectDistributor*>(distributorPtr->GetApp()) };

	//* Client
	auto clientPtr{ MSAPI::Daemon<ObjectClient>::Create("Client") };
	if (clientPtr == nullptr) {
		return 1;
	}
	auto client{ static_cast<ObjectClient*>(clientPtr->GetApp()) };
	if (!client->OpenConnect(distributorId, INADDR_LOOPBACK, distributorPtr->GetPort(), false)) {
		return 1;
	}

	//* Setup, stream state is undefined
	client->SetConnectionForStreams(distributorId);
	MSAPI::Test test;
	const size_t figi1{ 123456789012 };
	InstrumentStructure instrument1{ InstrumentStructure::InstrumentStructureType::First, figi1, 7432435, 998274902,
		34387675464, 1000, 133, InstrumentStructure::Nominal{ 133, 1 }, true, true, true, 133, 0.25, 555666333 };
	distributor->SetInstrument(instrument1);
	MSAPI::ObjectProtocol::Filter<FilterStructure> filter{ MSAPI::ObjectProtocol::Type::Snapshot };
	client->GetInstrumentStream().SetFilter(filter);
	test.Assert(static_cast<short>(client->GetInstrumentStream().GetState()),
		static_cast<short>(MSAPI::ObjectProtocol::State::Undefined), "Stream state is undefined");
	client->GetInstrumentStream().Open();

	//* Waiting for HandleStreamSnapshotDone
	const auto& actions{ client->GetActionsNumber() };
	client->WaitActionsNumber(test, 50000, 3 /* opened + instrument + done */);
	test.Assert(actions, size_t{ 3 }, "Client's actions number is 3");
	test.Assert(client->GetInstruments().size(), size_t{ 1 }, "Client got one instrument");
	test.Assert(std::find_if(client->GetInstruments().begin(), client->GetInstruments().end(),
					[&instrument1](const auto& instrument) { return instrument1.ToString() == instrument.ToString(); })
			!= client->GetInstruments().end(),
		true, "Client got equal instrument");
	test.Assert(client->GetInstrumentStream().IsSnapshotDone(), true, "Stream snapshot is done");

	//* Waiting stream Closed state
	client->WaitActionsNumber(test, 5000);
	test.Assert(static_cast<short>(client->GetInstrumentStream().GetState()),
		static_cast<short>(MSAPI::ObjectProtocol::State::Closed), "Stream state is closed");

	//* Close is uncountable action
	test.Assert(actions, size_t{ 3 }, "Client's actions number is still 3");

	//* Setup for next steps
	client->Clear();
	const size_t figi2{ 123456789013 };
	InstrumentStructure instrument2{ InstrumentStructure::InstrumentStructureType::Second, figi2, 7432435, 998274902,
		34387675464, 1000, 133, InstrumentStructure::Nominal{ 133, 2 }, true, true, true, 133, 0.25, 555666333 };
	distributor->SetInstrument(instrument2);
	client->GetInstrumentStream().Open();

	//* Waiting for HandleStreamSnapshotDone
	client->WaitActionsNumber(test, 5000, 4 /* opened + 2 instruments + done */);
	test.Assert(actions, size_t{ 4 }, "Client's actions number is 4");
	test.Assert(client->GetInstruments().size(), size_t{ 2 }, "Client got two instruments");
	test.Assert(std::find_if(client->GetInstruments().begin(), client->GetInstruments().end(),
					[&instrument1](const auto& instrument) { return instrument1.ToString() == instrument.ToString(); })
			!= client->GetInstruments().end(),
		true, "Client got equal instrument №1");
	test.Assert(std::find_if(client->GetInstruments().begin(), client->GetInstruments().end(),
					[&instrument2](const auto& instrument) { return instrument2.ToString() == instrument.ToString(); })
			!= client->GetInstruments().end(),
		true, "Client got equal instrument №2");
	test.Assert(client->GetInstrumentStream().IsSnapshotDone(), true, "Stream snapshot is done");

	//* Waiting stream Closed state
	client->WaitActionsNumber(test, 5000);
	test.Assert(static_cast<short>(client->GetInstrumentStream().GetState()),
		static_cast<short>(MSAPI::ObjectProtocol::State::Closed), "Stream state is closed");

	//* Close is uncountable action
	test.Assert(actions, size_t{ 4 }, "Client's actions number is still 4");

	//* Setup for next steps
	client->Clear();
	MSAPI::ObjectProtocol::Filter<FilterStructure> filter2{ MSAPI::ObjectProtocol::Type::SnapshotAndLive };
	client->GetInstrumentStream().SetFilter(filter2);
	client->GetInstrumentStream().Open();

	//* Waiting for HandleStreamSnapshotDone
	client->WaitActionsNumber(test, 5000, 4 /* opened + 2 instruments + done */);
	test.Assert(actions, size_t{ 4 }, "Client's actions number is 4");
	test.Assert(client->GetInstruments().size(), size_t{ 2 }, "Client got two instruments");
	test.Assert(std::find_if(client->GetInstruments().begin(), client->GetInstruments().end(),
					[&instrument1](const auto& instrument) { return instrument1.ToString() == instrument.ToString(); })
			!= client->GetInstruments().end(),
		true, "Client got equal instrument №1");
	test.Assert(std::find_if(client->GetInstruments().begin(), client->GetInstruments().end(),
					[&instrument2](const auto& instrument) { return instrument2.ToString() == instrument.ToString(); })
			!= client->GetInstruments().end(),
		true, "Client got equal instrument №2");
	test.Assert(client->GetInstrumentStream().IsSnapshotDone(), true, "Stream is snapshot done");

	//* Stream still opened
	test.Assert(static_cast<short>(client->GetInstrumentStream().GetState()),
		static_cast<short>(MSAPI::ObjectProtocol::State::Opened), "Stream state still is opened");

	//* Waiting one more instrument
	const size_t figi3{ 123456789014 };
	InstrumentStructure instrument3{ InstrumentStructure::InstrumentStructureType::Third, figi3, 7432435, 998274902,
		34387675464, 1000, 133, InstrumentStructure::Nominal{ 133, 3 }, true, true, true, 133, 0.25, 555666333 };
	distributor->SetInstrument(instrument3);
	client->WaitActionsNumber(test, 5000, 5);
	test.Assert(actions, size_t{ 5 }, "Client's actions number is 5");
	test.Assert(client->GetInstruments().size(), size_t{ 3 }, "Client got three instruments");
	test.Assert(std::find_if(client->GetInstruments().begin(), client->GetInstruments().end(),
					[&instrument3](const auto& instrument) { return instrument3.ToString() == instrument.ToString(); })
			!= client->GetInstruments().end(),
		true, "Client got equal instrument №3");

	//* Stream still snapshot done
	test.Assert(client->GetInstrumentStream().IsSnapshotDone(), true, "Stream still is snapshot done");

	//* Stream still opened
	test.Assert(static_cast<short>(client->GetInstrumentStream().GetState()),
		static_cast<short>(MSAPI::ObjectProtocol::State::Opened), "Stream state still is opened");

	//* Check actions number
	test.Assert(actions, size_t{ 5 }, "Client's actions number is still 5");

	//* Close stream manually
	client->GetInstrumentStream().Close();
	client->WaitActionsNumber(test, 5000);
	test.Assert(static_cast<short>(client->GetInstrumentStream().GetState()),
		static_cast<short>(MSAPI::ObjectProtocol::State::Closed), "Stream state is closed");

	//* Check actions number
	test.Assert(actions, size_t{ 5 }, "Client's actions number is still 5");

	//* Setup for next steps
	client->Clear();
	MSAPI::ObjectProtocol::Filter<FilterStructure> filter3{ MSAPI::ObjectProtocol::Type::Snapshot };
	FilterStructure figiFilter3{ figi3 };
	filter3.SetObject(figiFilter3);
	client->GetInstrumentStream().SetFilter(filter3);
	client->GetInstrumentStream().Open();

	//* Waiting for HandleStreamSnapshotDone
	client->WaitActionsNumber(test, 5000, 3 /* opened + instrument + done */);
	test.Assert(actions, size_t{ 3 }, "Client's actions number is 3");
	test.Assert(client->GetInstruments().size(), size_t{ 1 }, "Client got one instrument");
	test.Assert(std::find_if(client->GetInstruments().begin(), client->GetInstruments().end(),
					[&instrument3](const auto& instrument) { return instrument3.ToString() == instrument.ToString(); })
			!= client->GetInstruments().end(),
		true, "Client got equal instrument №3 from stream with filter");
	test.Assert(client->GetInstrumentStream().IsSnapshotDone(), true, "Stream is snapshot done");

	//* Waiting stream Closed state
	client->WaitActionsNumber(test, 5000);
	test.Assert(static_cast<short>(client->GetInstrumentStream().GetState()),
		static_cast<short>(MSAPI::ObjectProtocol::State::Closed), "Stream state is closed");

	//* Close is uncountable action
	test.Assert(actions, size_t{ 3 }, "Client's actions number is still 3");

	//* Setup for next steps
	client->Clear();
	MSAPI::ObjectProtocol::Filter<FilterStructure> filter4{ MSAPI::ObjectProtocol::Type::Snapshot };
	FilterStructure figiFilter2{ figi2 };
	filter4.SetObject(figiFilter2);
	filter4.SetObject(figiFilter3);
	client->GetInstrumentStream().SetFilter(filter4);
	client->GetInstrumentStream().Open();

	//* Waiting for HandleStreamSnapshotDone
	client->WaitActionsNumber(test, 5000, 4 /* opened + 2 instruments + done */);
	test.Assert(actions, size_t{ 4 }, "Client's actions number is 4");
	test.Assert(client->GetInstruments().size(), size_t{ 2 }, "Client got two instruments");
	test.Assert(std::find_if(client->GetInstruments().begin(), client->GetInstruments().end(),
					[&instrument2](const auto& instrument) { return instrument2.ToString() == instrument.ToString(); })
			!= client->GetInstruments().end(),
		true, "Client got equal instrument №2 from stream with filter");
	test.Assert(std::find_if(client->GetInstruments().begin(), client->GetInstruments().end(),
					[&instrument3](const auto& instrument) { return instrument3.ToString() == instrument.ToString(); })
			!= client->GetInstruments().end(),
		true, "Client got equal instrument №3 from stream with filter");
	test.Assert(client->GetInstrumentStream().IsSnapshotDone(), true, "Stream is snapshot done");

	//* Waiting stream Closed state
	client->WaitActionsNumber(test, 5000);
	test.Assert(static_cast<short>(client->GetInstrumentStream().GetState()),
		static_cast<short>(MSAPI::ObjectProtocol::State::Closed), "Stream state is closed");

	//* Close is uncountable action
	test.Assert(actions, size_t{ 4 }, "Client's actions number is still 4");

	//* Setup for next steps
	client->Clear();
	const size_t figi4{ 123456789015 };
	const size_t figi5{ 123456789016 };
	MSAPI::ObjectProtocol::Filter<FilterStructure> filter5{ MSAPI::ObjectProtocol::Type::SnapshotAndLive };
	InstrumentStructure instrument4{ InstrumentStructure::InstrumentStructureType::Fourth, figi4, 7432435, 998274902,
		34387675464, 1000, 133, InstrumentStructure::Nominal{ 133, 4 }, true, true, true, 133, 0.25, 555666333 };
	InstrumentStructure instrument5{ InstrumentStructure::InstrumentStructureType::First, figi5, 7432435, 998274902,
		34387675464, 1000, 133, InstrumentStructure::Nominal{ 133, 5 }, true, true, true, 133, 0.25, 555666333 };
	FilterStructure figiFilter4{ figi4 };
	FilterStructure figiFilter5{ figi5 };
	filter5.SetObject(figiFilter2);
	filter5.SetObject(figiFilter3);
	filter5.SetObject(figiFilter4);
	filter5.SetObject(figiFilter5);
	client->GetInstrumentStream().SetFilter(filter5);
	client->GetInstrumentStream().Open();

	//* Waiting for HandleStreamSnapshotDone
	client->WaitActionsNumber(test, 5000, 4 /* opened + 2 instruments + done */);
	test.Assert(actions, size_t{ 4 }, "Client's actions number is 4");
	test.Assert(client->GetInstruments().size(), size_t{ 2 }, "Client got two instruments");
	test.Assert(std::find_if(client->GetInstruments().begin(), client->GetInstruments().end(),
					[&instrument2](const auto& instrument) { return instrument2.ToString() == instrument.ToString(); })
			!= client->GetInstruments().end(),
		true, "Client got equal instrument №2");
	test.Assert(std::find_if(client->GetInstruments().begin(), client->GetInstruments().end(),
					[&instrument3](const auto& instrument) { return instrument3.ToString() == instrument.ToString(); })
			!= client->GetInstruments().end(),
		true, "Client got equal instrument №3");
	test.Assert(client->GetInstrumentStream().IsSnapshotDone(), true, "Stream is snapshot done");

	//* Stream still opened
	test.Assert(static_cast<short>(client->GetInstrumentStream().GetState()),
		static_cast<short>(MSAPI::ObjectProtocol::State::Opened), "Stream state still is opened");

	//* Waiting one more instrument
	distributor->SetInstrument(instrument4);
	client->WaitActionsNumber(test, 5000, 5);
	test.Assert(actions, size_t{ 5 }, "Client's actions number is 5");
	test.Assert(client->GetInstruments().size(), size_t{ 3 }, "Client got three instruments");
	test.Assert(std::find_if(client->GetInstruments().begin(), client->GetInstruments().end(),
					[&instrument4](const auto& instrument) { return instrument4.ToString() == instrument.ToString(); })
			!= client->GetInstruments().end(),
		true, "Client got equal instrument №4");

	//* Stream still snapshot done
	test.Assert(client->GetInstrumentStream().IsSnapshotDone(), true, "Stream still is snapshot done");

	//* Stream still opened
	test.Assert(static_cast<short>(client->GetInstrumentStream().GetState()),
		static_cast<short>(MSAPI::ObjectProtocol::State::Opened), "Stream state still is opened");

	//* Check actions number
	test.Assert(actions, size_t{ 5 }, "Client's actions number is still 5");

	//* Waiting one more instrument
	distributor->SetInstrument(instrument5);
	client->WaitActionsNumber(test, 5000, 6);
	test.Assert(actions, size_t{ 6 }, "Client's actions number is 6");
	test.Assert(client->GetInstruments().size(), size_t{ 4 }, "Client got four instruments");
	test.Assert(std::find_if(client->GetInstruments().begin(), client->GetInstruments().end(),
					[&instrument5](const auto& instrument) { return instrument5.ToString() == instrument.ToString(); })
			!= client->GetInstruments().end(),
		true, "Client got equal instrument №5");

	//* Stream still snapshot done
	test.Assert(client->GetInstrumentStream().IsSnapshotDone(), true, "Stream still is snapshot done");

	//* Stream still opened
	test.Assert(static_cast<short>(client->GetInstrumentStream().GetState()),
		static_cast<short>(MSAPI::ObjectProtocol::State::Opened), "Stream state still is opened");

	//* Check actions number
	test.Assert(actions, size_t{ 6 }, "Client's actions number is still 6");

	//* Close stream manually
	client->GetInstrumentStream().Close();
	client->WaitActionsNumber(test, 5000);
	test.Assert(static_cast<short>(client->GetInstrumentStream().GetState()),
		static_cast<short>(MSAPI::ObjectProtocol::State::Closed), "Stream state is closed");

	//* Check actions number
	test.Assert(actions, size_t{ 6 }, "Client's actions number is still 6");

	//* Setup for next steps
	client->Clear();
	distributor->Clear();
	MSAPI::ObjectProtocol::Filter<FilterStructure> filter6{ MSAPI::ObjectProtocol::Type::SnapshotAndLive };
	filter6.SetObject(figiFilter2);
	filter6.SetObject(figiFilter3);
	filter6.SetObject(figiFilter4);
	client->GetInstrumentStream().SetFilter(filter6);
	test.Assert(static_cast<short>(client->GetInstrumentStream().GetState()),
		static_cast<short>(MSAPI::ObjectProtocol::State::Closed), "Instrument stream state is closed");
	test.Assert(static_cast<short>(client->GetOrderStream().GetState()),
		static_cast<short>(MSAPI::ObjectProtocol::State::Undefined), "Order stream state is undefined");
	OrderStructure order1{ figi1, 100.0, 20 };
	OrderStructure order2{ figi2, 100.0, 20 };
	OrderStructure order3{ figi3, 100.0, 20 };
	OrderStructure order4{ figi4, 100.0, 20 };
	OrderStructure order5{ figi5, 100.0, 20 };
	distributor->SetOrder(order1);
	distributor->SetInstrument(instrument1);
	distributor->SetOrder(order2);
	distributor->SetInstrument(instrument2);

	client->GetInstrumentStream().Open();
	test.Assert(client->GetOrderStream().Open(), false, "Try open stream without filter");
	client->GetOrderStream().SetFilter(filter6);
	client->GetOrderStream().Open();

	//* Waiting instrument and order stream' HandleStreamSnapshotDone calls
	client->WaitActionsNumber(test, 5000, 6 /* 2 opened + 1 instrument + 1 order + 2 done*/);
	test.Assert(actions, size_t{ 6 }, "Client's actions number is 6");
	test.Assert(client->GetInstruments().size(), size_t{ 1 }, "Client got one instrument");
	test.Assert(std::find_if(client->GetInstruments().begin(), client->GetInstruments().end(),
					[&instrument2](const auto& instrument) { return instrument2.ToString() == instrument.ToString(); })
			!= client->GetInstruments().end(),
		true, "Client got equal instrument №2");
	test.Assert(client->GetOrders().size(), size_t{ 1 }, "Client got one order");
	test.Assert(std::find_if(client->GetOrders().begin(), client->GetOrders().end(),
					[&order2](const auto& order) { return order2.ToString() == order.ToString(); })
			!= client->GetOrders().end(),
		true, "Client got equal order №2");
	test.Assert(client->GetInstrumentStream().IsSnapshotDone(), true, "Instrument stream snapshot is done");
	test.Assert(client->GetOrderStream().IsSnapshotDone(), true, "Order stream snapshot is done");

	//* Streams still opened
	test.Assert(static_cast<short>(client->GetInstrumentStream().GetState()),
		static_cast<short>(MSAPI::ObjectProtocol::State::Opened), "Instrument stream state still is opened");
	test.Assert(static_cast<short>(client->GetOrderStream().GetState()),
		static_cast<short>(MSAPI::ObjectProtocol::State::Opened), "Order stream state still is opened");

	//* Check actions number
	test.Assert(actions, size_t{ 6 }, "Client's actions number is still 6");

	//* Waiting one more instrument and order
	distributor->SetInstrument(instrument3);
	distributor->SetOrder(order3);
	client->WaitActionsNumber(test, 5000, 8);
	test.Assert(actions, size_t{ 8 }, "Client's actions number is 8");
	test.Assert(client->GetInstruments().size(), size_t{ 2 }, "Client got two instruments");
	test.Assert(std::find_if(client->GetInstruments().begin(), client->GetInstruments().end(),
					[&instrument3](const auto& instrument) { return instrument3.ToString() == instrument.ToString(); })
			!= client->GetInstruments().end(),
		true, "Client got equal instrument №3");
	test.Assert(client->GetOrders().size(), size_t{ 2 }, "Client got two orders");
	test.Assert(std::find_if(client->GetOrders().begin(), client->GetOrders().end(),
					[&order3](const auto& order) { return order3.ToString() == order.ToString(); })
			!= client->GetOrders().end(),
		true, "Client got equal order №3");

	//* Streams still snapshot done
	test.Assert(client->GetInstrumentStream().IsSnapshotDone(), true, "Instrument stream snapshot is done");
	test.Assert(client->GetOrderStream().IsSnapshotDone(), true, "Order stream snapshot is done");

	//* Streams still opened
	test.Assert(static_cast<short>(client->GetInstrumentStream().GetState()),
		static_cast<short>(MSAPI::ObjectProtocol::State::Opened), "Instrument stream state still is opened");
	test.Assert(static_cast<short>(client->GetOrderStream().GetState()),
		static_cast<short>(MSAPI::ObjectProtocol::State::Opened), "Order stream state still is opened");

	//* Check actions number
	test.Assert(actions, size_t{ 8 }, "Client's actions number is still 8");

	//* Waiting one more instrument and order
	distributor->SetInstrument(instrument4);
	distributor->SetOrder(order4);
	client->WaitActionsNumber(test, 5000, 10);
	test.Assert(client->GetInstruments().size(), size_t{ 3 }, "Client got three instruments");
	test.Assert(std::find_if(client->GetInstruments().begin(), client->GetInstruments().end(),
					[&instrument4](const auto& instrument) { return instrument4.ToString() == instrument.ToString(); })
			!= client->GetInstruments().end(),
		true, "Client got equal instrument №4");
	test.Assert(client->GetOrders().size(), size_t{ 3 }, "Client got three orders");
	test.Assert(std::find_if(client->GetOrders().begin(), client->GetOrders().end(),
					[&order4](const auto& order) { return order4.ToString() == order.ToString(); })
			!= client->GetOrders().end(),
		true, "Client got equal order №4");

	//* Streams still snapshot done
	test.Assert(client->GetInstrumentStream().IsSnapshotDone(), true, "Instrument stream snapshot is done");
	test.Assert(client->GetOrderStream().IsSnapshotDone(), true, "Order stream snapshot is done");

	//* Streams still opened
	test.Assert(static_cast<short>(client->GetInstrumentStream().GetState()),
		static_cast<short>(MSAPI::ObjectProtocol::State::Opened), "Instrument stream state still is opened");
	test.Assert(static_cast<short>(client->GetOrderStream().GetState()),
		static_cast<short>(MSAPI::ObjectProtocol::State::Opened), "Order stream state still is opened");

	//* Check actions number
	test.Assert(actions, size_t{ 10 }, "Client's actions number is 10");

	//* Set objects what does not match to stream's filter
	distributor->SetInstrument(instrument5);
	distributor->SetOrder(order5);
	client->WaitActionsNumber(test, 5000);
	test.Assert(client->GetInstruments().size(), size_t{ 3 }, "Client still got three instruments");
	test.Assert(client->GetOrders().size(), size_t{ 3 }, "Client still got three orders");

	//* Streams still snapshot done
	test.Assert(client->GetInstrumentStream().IsSnapshotDone(), true, "Instrument stream snapshot is done");
	test.Assert(client->GetOrderStream().IsSnapshotDone(), true, "Order stream snapshot is done");

	//* Streams still opened
	test.Assert(static_cast<short>(client->GetInstrumentStream().GetState()),
		static_cast<short>(MSAPI::ObjectProtocol::State::Opened), "Instrument stream state still is opened");
	test.Assert(static_cast<short>(client->GetOrderStream().GetState()),
		static_cast<short>(MSAPI::ObjectProtocol::State::Opened), "Order stream state still is opened");

	//* Check actions number
	test.Assert(actions, size_t{ 10 }, "Client's actions number is still 10");

	//* Close streams manually
	client->GetInstrumentStream().Close();
	client->GetOrderStream().Close();
	client->WaitActionsNumber(test, 5000);
	test.Assert(static_cast<short>(client->GetInstrumentStream().GetState()),
		static_cast<short>(MSAPI::ObjectProtocol::State::Closed), "Instrument stream state is closed");
	test.Assert(static_cast<short>(client->GetOrderStream().GetState()),
		static_cast<short>(MSAPI::ObjectProtocol::State::Closed), "Order stream state is closed");

	//* Check actions number
	test.Assert(actions, size_t{ 10 }, "Client's actions number is still 10");

	distributorPtr.reset();
	clientPtr.reset();

	return test.Passed<int32_t>();
}