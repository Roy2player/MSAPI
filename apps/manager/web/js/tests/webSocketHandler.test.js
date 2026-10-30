/**************************
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

const { TestRunner, testRunner } = require("../test/testRunner");
const { WebSocketServer } = require("ws");
const { WebSocketHandler, WebSocketStream } = require("../core/webSocketHandler");

// Exercise the unchanged client against the exact lifecycle sequences emitted by the server.
const server = new WebSocketServer({ port : global.port });
server.on("connection", connection => {
	connection.on("message", message => {
		const request = JSON.parse(message);
		if (request.interrupt) {
			return;
		}
		const send = data => connection.send(JSON.stringify({ uids : [ request.uid ], ...data }));
		send({ state : WebSocketStream.State.Opened });
		if (request.event === 1) {
			send({ data : [ 1, 2 ] });
		}
		else if (request.event !== 3) {
			send({ data : [ 10 ] });
			send({ data : [ 20 ] });
		}
		if (request.event >= 3) {
			send({ state : WebSocketStream.State.Failed, error : "snapshot failed" });
		}
		else {
			send({ state : WebSocketStream.State.Done });
		}
	});
});

const activeStreams = [];
let previousDataCalls = 0;
for (const scenario of [1, 2, 3, 4]) {
	testRunner.Test(`Stream initialization sequence ${scenario}`, async () => {
		const calls = [];
		let finished = false;
		const previousCount = previousDataCalls;

		let stream = new WebSocketStream({
			event : scenario,
			viewUid : scenario,
			handleOpened : () => {
				testRunner.Assert(stream.m_state, WebSocketStream.State.Opened, "Opened sets state before callback");
				calls.push("Opened");
			},
			handleData : data => {
				testRunner.Assert(stream.m_state, WebSocketStream.State.Opened, "Data arrives while snapshot is open");
				calls.push(data);
				if (scenario === 1) {
					++previousDataCalls;
				}
			},
			handleSnapshotDone : () => {
				testRunner.Assert(stream.m_state, WebSocketStream.State.Done);
				calls.push("Done");
				finished = true;
			},
			handleFailed : error => {
				testRunner.Assert(stream.m_state, WebSocketStream.State.Failed);
				testRunner.Assert(error.includes("snapshot failed"), true);
				calls.push("Failed");
				finished = true;
			}
		});

		await TestRunner.WaitFor(() => finished, "Snapshot finishes");
		const expected = [ "Opened" ];
		if (scenario === 1) {
			expected.push([ 1, 2 ]);
		}
		else if (scenario !== 3) {
			expected.push([ 10 ], [ 20 ]);
		}
		expected.push(scenario >= 3 ? "Failed" : "Done");
		testRunner.Assert(calls, expected, "Lifecycle ordering, including partial failure");

		if (scenario !== 1) {
			testRunner.Assert(previousDataCalls, previousCount, "Existing subscriber receives no new stream chunks");
		}

		if (scenario >= 3) {
			testRunner.Assert(WebSocketHandler.GetEvent(stream.m_uid), undefined, "Failed stream is removed");
		}
		else {
			testRunner.Assert(WebSocketHandler.GetEvent(stream.m_uid), stream, "Successful stream remains registered");
			activeStreams.push(stream);
		}
	});
}

testRunner.Test("Close successful streams after snapshot completion", () => {
	for (const stream of activeStreams) {
		stream.Close();
		testRunner.Assert(stream.m_state, WebSocketStream.State.Closed);
		testRunner.Assert(WebSocketHandler.GetEvent(stream.m_uid), undefined);
	}

	for (const connection of server.clients) {
		connection.terminate();
	}

	server.close();
});

testRunner.Run();