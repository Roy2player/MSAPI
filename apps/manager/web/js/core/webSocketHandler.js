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
	static m_idToEvent = new Map();
	static m_viewIdToEventIds = new Map();
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

		WebSocketHandler.m_idToEvent.set(event.m_id, event);

		let eventIds = WebSocketHandler.m_viewIdToEventIds.get(event.m_viewId);
		if (!eventIds) {
			let ids = new Set();
			ids.add(event.m_id);
			WebSocketHandler.m_viewIdToEventIds.set(event.m_viewId, ids)
			return;
		}

		eventIds.add(event.m_id);
	}

	static RemoveEvent(event)
	{
		if (!(event instanceof WebSocketSingle) && !(event instanceof WebSocketStream)) {
			console.warn("Unexpected event type", event);
			return;
		}

		if (!WebSocketHandler.m_idToEvent.has(event.m_id)) {
			return;
		}
		WebSocketHandler.m_idToEvent.delete(event.m_id);

		let eventIds = WebSocketHandler.m_viewIdToEventIds.get(event.m_viewId);
		if (!eventIds) {
			console.warn("Unexpectedly empty related events to view id", event.m_viewId);
			return;
		}

		eventIds.delete(event.m_id);
	}

	static ClearViewRelatedEvents(viewId)
	{
		let eventIds = WebSocketHandler.m_viewIdToEventIds.get(viewId);
		if (eventIds) {
			eventIds.forEach((id) => {
				let event = WebSocketHandler.GetEvent(id);
				if (!event) {
					console.warn("Unknown event id", id);
					return
				}
				WebSocketHandler.Send(`{"id":${id},"type":${event.m_type},"event":${event.m_event},"interrupt":true}`);
				WebSocketHandler.m_idToEvent.delete(id);
			})

			WebSocketHandler.m_viewIdToEventIds.delete(viewId);
		}
	}

	static GetEvent(id) { return WebSocketHandler.m_idToEvent.get(id); }

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
			this.m_idToEvent.forEach((event, id) => {
				if (event.m_handleFailed) {
					event.m_handleFailed(`Web socket is closed, event is interrupted`);
				}
			});

			this.m_idToEvent.clear();
			this.m_viewIdToEventIds.clear();
		});

		WebSocketHandler.m_serverConnection.addEventListener("error", (error) => {
			console.error("WebSocket is closed with error:", error);

			this.m_idToEvent.forEach((event, id) => {
				if (event.m_handleFailed) {
					event.m_handleFailed(`Web socket is closed, event is interrupted, error: ${error}`);
				}
			});

			this.m_idToEvent.clear();
			this.m_viewIdToEventIds.clear();
		});

		WebSocketHandler.m_serverConnection.addEventListener("message", (e) => {
			let json = e.data;
			// console.log("Receive", json);
			json = Helper.JsonStringToObject(json);

			if (!("ids" in json)) {
				console.warn("Message does not contain event ids", json);
				return;
			}

			const ids = Array.from(json["ids"]);
			let type = WebSocketHandler.Type.Undefined;

			const getEvent = (id) => {
				id = Number(id);
				const event = WebSocketHandler.GetEvent(id);
				if (!event) {
					console.warn("Message for unknown event is reserved", id);
					return null;
				}

				return event;
			};

			for (const id of ids) {
				const event = getEvent(id);
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
							Array.from(json["ids"]).forEach((id) => {
								const event = getEvent(id);
								if (event) {
									event.m_handleFailed(json["error"]);
									WebSocketHandler.RemoveEvent(event);
								}
							});
							return;
						}

						Array.from(json["ids"]).forEach((id) => {
							const event = getEvent(id);
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

				Array.from(json["ids"]).forEach((id) => {
					const event = getEvent(id);
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
						Array.from(json["ids"]).forEach((id) => {
							const event = getEvent(id);
							if (event) {
								event.m_state = state;
								if (event.m_handleOpened) {
									event.m_handleOpened();
								}
							}
						});
						return;
					case WebSocketStream.State.Done:
						Array.from(json["ids"]).forEach((id) => {
							const event = getEvent(id);
							if (event) {
								event.m_state = state;
								if (event.m_handleSnapshotDone) {
									event.m_handleSnapshotDone();
								}
							}
						});
						return;
					case WebSocketStream.State.Failed:
						Array.from(json["ids"]).forEach((id) => {
							const event = getEvent(id);
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
					Array.from(json["ids"]).forEach((id) => {
						const event = getEvent(id);
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
		if (typeof args.viewId !== "number") {
			console.warn("View id must be number type", args.viewId);
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
		this.m_id = Helper.GenerateId();
		this.m_viewId = args.viewId;
		this.m_type = WebSocketHandler.Type.Single;

		let data = {};
		if (args.data) {
			data = args.data;
		}

		data.id = this.m_id;
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
		if (typeof args.viewId !== "number") {
			console.warn("View id must be number type", args.viewId);
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
		this.m_id = Helper.GenerateId();
		this.m_viewId = args.viewId;
		this.m_type = WebSocketHandler.Type.Stream;
		this.m_state = WebSocketStream.State.Pending;

		let data = {};
		if (args.data) {
			data = args.data;
		}
		data.id = this.m_id;
		data.event = this.m_event;
		data.type = WebSocketHandler.Type.Stream;

		WebSocketHandler.AddEvent(this);
		WebSocketHandler.Send(Helper.ParametersToJson(data));
	}

	Close()
	{
		this.m_state = WebSocketStream.State.Closed;
		WebSocketHandler.Send(
			`{"id":${this.m_id},"type":${WebSocketHandler.Type.Stream},"event":${this.m_event},"interrupt":true}`);
		WebSocketHandler.RemoveEvent(this);
	}
};

if (typeof module !== "undefined" && typeof module.exports !== "undefined") {
	WebSocket = require('ws');
	Helper = require("../help/helper");
	module.exports.WebSocketHandler = WebSocketHandler;
	module.exports.WebSocketStream = WebSocketStream;
	module.exports.WebSocketSingle = WebSocketSingle;
}