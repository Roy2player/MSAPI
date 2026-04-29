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
 *
 * @brief MSAPI Event application layer protocol on top of web socket with json payload format. Supports single and
 * stream event types.
 */

class WebSocketHandler {
	static m_uidToEvent = new Map();
	static m_viewUidToEventUids = new Map();
	static m_queue = [];

	static Type = Object.freeze({
		Undefined : 0,
		Single: 1,
		Stream: 2,
		Max: 3,
	});

	static AddEvent(event)
	{
		if (!(event instanceof WebSocketSingle) && !(event instanceof WebSocketStream)) {
			console.warn("Unexpected event type", event);
			return;
		}

		WebSocketHandler.m_uidToEvent.set(event.m_uid, event);

		let eventUids = WebSocketHandler.m_viewUidToEventUids.get(event.m_viewUid);
		if (!eventUids) {
			let uids = new Set();
			uids.add(event.m_uid);
			WebSocketHandler.m_viewUidToEventUids.set(event.m_viewUid, uids)
			return;
		}

		eventUids.add(event.m_uid);
	}

	static RemoveEvent(event)
	{
		if (!(event instanceof WebSocketSingle) && !(event instanceof WebSocketStream)) {
			console.warn("Unexpected event type", event);
			return;
		}

		if (!WebSocketHandler.m_uidToEvent.has(event.m_uid)) {
			return;
		}
		WebSocketHandler.m_uidToEvent.delete(event.m_uid);

		let eventUids = WebSocketHandler.m_viewUidToEventUids.get(event.m_viewUid);
		if (!eventUids) {
			console.warn("Unexpectedly empty related events to view uid", event.m_viewUid);
			return;
		}

		eventUids.delete(event.m_uid);
	}

	static ClearViewRelatedEvents(viewUid)
	{
		let eventUids = WebSocketHandler.m_viewUidToEventUids.get(viewUid);
		if (eventUids) {
			eventUids.forEach((uid) => {
				let event = WebSocketHandler.GetEvent(uid);
				if (!event) {
					console.warn("Unknown event uid", uid);
					return
				}
				WebSocketHandler.Send(
					`{"uid":${uid},"type":${event.m_type},"event":${event.m_event},"interrupt":true}`);
				WebSocketHandler.m_uidToEvent.delete(uid);
			})

			WebSocketHandler.m_viewUidToEventUids.delete(viewUid);
		}
	}

	static GetEvent(uid) { return WebSocketHandler.m_uidToEvent.get(uid); }

	static Send(json)
	{
		// console.log("Send", json);

		if (!WebSocketHandler.m_serverConnection) {
			WebSocketHandler.OpenWebSocket(json);
			return;
		}

		if (WebSocketHandler.m_serverConnection.readyState == WebSocket.CLOSING
			|| WebSocketHandler.m_serverConnection.readyState == WebSocket.CLOSED) {
			WebSocketHandler.OpenWebSocket(json);
			return;
		}

		if (WebSocketHandler.m_serverConnection.readyState != WebSocket.OPEN) {
			WebSocketHandler.m_queue.push(json);
			return;
		}

		WebSocketHandler.m_serverConnection.send(json);
	}

	static OpenWebSocket(json)
	{
		WebSocketHandler.m_serverConnection = new WebSocket(`ws://${window.location.host}`);

		WebSocketHandler.m_serverConnection.addEventListener("open", () => {
			WebSocketHandler.m_serverConnection.send(json);
			while (WebSocketHandler.m_queue.length > 0) {
				WebSocketHandler.m_serverConnection.send(WebSocketHandler.m_queue.shift());
			}
		});

		WebSocketHandler.m_serverConnection.addEventListener("close", () => {
			this.m_uidToEvent.forEach((event, uid) => {
				if (event.m_handleFailed) {
					event.m_handleFailed(`Web socket is closed, event is interrupted`);
				}
			});

			this.m_uidToEvent.clear();
			this.m_viewUidToEventUids.clear();
		});

		WebSocketHandler.m_serverConnection.addEventListener("error", (error) => {
			console.error("WebSocket is closed with error:", error);

			this.m_uidToEvent.forEach((event, uid) => {
				if (event.m_handleFailed) {
					event.m_handleFailed(`Web socket is closed, event is interrupted, error: ${error}`);
				}
			});

			this.m_uidToEvent.clear();
			this.m_viewUidToEventUids.clear();
		});

		WebSocketHandler.m_serverConnection.addEventListener("message", (e) => {
			let json = e.data;
			// console.log("Receive", json);
			json = Helper.JsonStringToObject(json);

			if (!("uids" in json)) {
				console.warn("Message does not contain event uids", json);
				return;
			}

			const uids = Array.from(json["uids"]);
			let type = WebSocketHandler.Type.Undefined;

			const getEvent = (uid) => {
				uid = Number(uid);
				const event = WebSocketHandler.GetEvent(uid);
				if (!event) {
					console.warn("Message for unknown event is reserved", uid);
					return null;
				}

				return event;
			};

			for (const uid of uids) {
				const event = getEvent(uid);
				if (!event) {
					continue;
				}

				type = event.m_type;
			}

			if (type == WebSocketHandler.Type.Undefined) {
				console.warn("Message does cont contain alive events", json);
				return;
			}

			if (type == WebSocketHandler.Type.Single) {
				if ("state" in json) {
					const state = Number(json["state"]);
					if (state == WebSocketStream.State.Failed) {
						if ("error" in json) {
							Array.from(json["uids"]).forEach((uid) => {
								const event = getEvent(uid);
								if (event) {
									event.m_handleFailed(json["error"]);
									WebSocketHandler.RemoveEvent(event);
								}
							});
							return;
						}

						Array.from(json["uids"]).forEach((uid) => {
							const event = getEvent(uid);
							if (event) {
								event.m_handleFailed();
								WebSocketHandler.RemoveEvent(event);
							}
						});
						return;
					}
				}

				if (!("data" in json)) {
					console.warn("Single event does not contain data", json);
					return;
				}

				Array.from(json["uids"]).forEach((uid) => {
					const event = getEvent(uid);
					if (event) {
						event.m_handleResponse(json["data"]);
						WebSocketHandler.RemoveEvent(event);
					}
				});
				return;
			}

			if (type == WebSocketHandler.Type.Stream) {
				if ("state" in json) {
					let state = Number(json["state"]);
					switch (state) {
					case WebSocketStream.State.Opened:
						Array.from(json["uids"]).forEach((uid) => {
							const event = getEvent(uid);
							if (event) {
								event.m_state = state;
								if (event.m_handleOpened) {
									event.m_handleOpened();
								}
							}
						});
						return;
					case WebSocketStream.State.Done:
						Array.from(json["uids"]).forEach((uid) => {
							const event = getEvent(uid);
							if (event) {
								event.m_state = state;
								if (event.m_handleSnapshotDone) {
									event.m_handleSnapshotDone();
								}
							}
						});
						return;
					case WebSocketStream.State.Failed:
						Array.from(json["uids"]).forEach((uid) => {
							const event = getEvent(uid);
							if (!event) {
								return;
							}

							event.m_state = state;
							WebSocketHandler.RemoveEvent(event);
							if (!event.m_handleFailed) {
								return;
							}

							let error = "";
							if (!("error" in json)) {
								console.warn("Stream event failed update does not contain error");
								error = "No description";
							}
							else {
								error = json["error"];
							}

							event.m_handleFailed(`Stream event failed with error: ${error}`);
						});
						return;
					default:
						console.warn("Unexpected stream event state is reserved", json);
						return;
					}
				}

				if ("data" in json) {
					Array.from(json["uids"]).forEach((uid) => {
						const event = getEvent(uid);
						if (event) {
							event.m_handleData(json["data"]);
						}
					});
					return;
				}

				console.warn("Unexpected stream event message is reserved", json);
				return;
			}

			console.warn("Unexpected event type is reserved", type);
		});
	}
};

class WebSocketSingle {
	constructor(args)
	{
		if (typeof args.event !== "number") {
			console.warn("Event must be number type", args.event);
			return;
		}
		if (typeof args.viewUid !== "number") {
			console.warn("View uid must be number type", args.viewUid);
			return;
		}
		if (!args.handleResponse || typeof args.handleResponse !== "function") {
			console.warn("HandleResponse must be defined and must be function type", args.handleResponse);
			return;
		}
		if (args.handleFailed && typeof args.handleFailed !== "function") {
			console.warn("HandleFailed must be function type", args.handleFailed);
			return;
		}

		this.m_event = args.event;
		this.m_handleResponse = args.handleResponse;
		this.m_handleFailed = args.handleFailed;
		this.m_uid = Helper.GenerateUid();
		this.m_viewUid = args.viewUid;
		this.m_type = WebSocketHandler.Type.Single;

		let data = {};
		if (args.data) {
			data = args.data;
		}

		data.uid = this.m_uid;
		data.event = this.m_event;
		data.type = WebSocketHandler.Type.Single;

		WebSocketHandler.AddEvent(this);
		WebSocketHandler.Send(Helper.ParametersToJson(data));
	}
};

class WebSocketStream {
	static State
		= Object.freeze({ Undefined : 0, Pending: 1, Opened: 2, Done: 3, Failed: 4, Closed: 5, Removed: 6, Max: 7 });

	constructor(args)
	{
		if (typeof args.event !== "number") {
			console.warn("Event must be number type", args.event);
			return;
		}
		if (typeof args.viewUid !== "number") {
			console.warn("View uid must be number type", args.viewUid);
			return;
		}
		if (!args.handleData || typeof args.handleData !== "function") {
			console.warn("HandleData must be defined and be function type", args.handleData);
			return;
		}
		if (args.handleOpened && typeof args.handleOpened !== "function") {
			console.warn("HandleOpened must be function type", args.handleOpened);
			return;
		}
		if (args.handleSnapshotDone && typeof args.handleSnapshotDone !== "function") {
			console.warn("HandleSnapshotDone must be function type", args.handleSnapshotDone);
			return;
		}
		if (args.handleFailed && typeof args.handleFailed !== "function") {
			console.warn("HandleFailed must be function type", args.handleFailed);
			return;
		}

		this.m_event = args.event;
		this.m_handleData = args.handleData;
		this.m_handleOpened = args.handleOpened;
		this.m_handleSnapshotDone = args.handleSnapshotDone;
		this.m_handleFailed = args.handleFailed;
		this.m_uid = Helper.GenerateUid();
		this.m_viewUid = args.viewUid;
		this.m_type = WebSocketHandler.Type.Stream;
		this.m_state = WebSocketStream.State.Pending;

		let data = {};
		if (args.data) {
			data = args.data;
		}
		data.uid = this.m_uid;
		data.event = this.m_event;
		data.type = WebSocketHandler.Type.Stream;

		WebSocketHandler.AddEvent(this);
		WebSocketHandler.Send(Helper.ParametersToJson(data));
	}

	Close()
	{
		this.m_state = WebSocketStream.State.Closed;
		WebSocketHandler.Send(
			`{"uid":${this.m_uid},"type":${WebSocketHandler.Type.Stream},"event":${this.m_event},"interrupt":true}`);
		WebSocketHandler.RemoveEvent(this);
	}
};

if (typeof module !== "undefined" && typeof module.exports !== "undefined") {
	WebSocket = require('ws');
	Helper = require("./helper");
	module.exports.WebSocketHandler = WebSocketHandler;
	module.exports.WebSocketStream = WebSocketStream;
	module.exports.WebSocketSingle = WebSocketSingle;
}