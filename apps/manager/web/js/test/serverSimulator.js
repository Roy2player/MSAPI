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

const { WebSocketServer } = require('ws');
const Helper = require("../help/helper");

/**
 * @brief Server's behavior simulator.
 */
class ServerSimulator {
	constructor()
	{
		this.m_createdApps = new Map();
		this.m_installedApps
			= new Set([ { type : "Gateway T" }, { type : "Strategy" }, { type : "Storage" }, { type : "Graph" } ]);
	}

	InitSocket(port)
	{
		this.m_serverSocket = new WebSocketServer({ port });
		this.m_serverSocket.on("connection", (connection) => {
			connection.on("message", (json) => {
				json = Helper.JsonStringToObject(json);

				if (!("uid" in json)) {
					throw new Error(`Message from client does not contain uid ${json}`);
					return;
				}

				if (!("event" in json)) {
					throw new Error(`Message from client does not contain event ${json}`);
					return;
				}

				const event = json["event"];
				if (event == Helper.StringHash32Uint("installedApp")) {
					this.SendInstalledApps(connection, json);
					return;
				}

				if (event == Helper.StringHash32Uint("createApp")) {
					this.CreateApp(connection, json);
					return;
				}

				if (event == Helper.StringHash32Uint("createdApps")) {
					this.SendCreatedApps(connection, json);
					return;
				}

				if (event == Helper.StringHash32Uint("getMetadata")) {
					this.SendMetadata(connection, json);
					return;
				}

				if (event == Helper.StringHash32Uint("parameters")) {
					this.SendParameters(connection, json);
					return;
				}

				throw new Error(`Message from client contains unexpected event`, event);
			});

			connection.on("error", (error) => console.error(`Socket error: ", ${error}`));
			connection.on("close", (code, reason) => console.log(`Socket is closed: ${code} ${reason}`));
		});
	}

	SendInstalledApps(connection, json)
	{
		let response = { "uids" : [ json["uid"] ], "data" : [] };
		this.m_installedApps.forEach((appData) => { response["data"].push(appData); });
		connection.send(Helper.ParametersToJson(response));
	}

	CreateApp(connection, json)
	{
		if (!("appType" in json)) {
			throw new Error(`Message from client does not contain appType ${json}`);
			return;
		}

		const appType = json["appType"];
		let appData = undefined;
		for (const data of this.m_installedApps) {
			if (data["type"] == appType) {
				appData = data;
				break;
			}
		}

		if (appData == undefined) {
			let response
				= { "uids" : [ json["uid"] ], "state" : WebSocketStream.State.Failed, "error" : "Unknown app type" };
			connection.send(Helper.ParametersToJson(response));
		}

		const port = Math.floor(Math.random() * (65535 - 1000 + 1)) + 1000;
		this.m_createdApps.set(
			port, { "type" : appType, "port" : port, "pid" : port, "creation time" : "2026-04-28 16:53:15.460089632" });
		let response = { "uids" : [ json["uid"] ], "data" : { "port" : port } };
		connection.send(Helper.ParametersToJson(response));
	}

	SendCreatedApps(connection, json)
	{
		let response = { "uids" : [ json["uid"] ], "data" : { "created" : [] } };
		this.m_createdApps.forEach((appData) => { response["data"]["created"].push(appData); });
		connection.send(Helper.ParametersToJson(response));
	}

	SendMetadata(connection, json)
	{
		let response = {};
		if (json["appType"] == Helper.StringHash32Uint("Strategy")) {
			response = {
				"uids" : [ json["uid"] ],
				"data" : {
					"mutable" : {
						"30001" : {
							"name" : "Price type",
							"type" : "Int16",
							"min" : 1,
							"max" : 3,
							"stringInterpretations" : { "0" : "Undefined", "1" : "Fair", "2" : "Soft" }
						},
						"30002" : {
							"name" : "Start trading time delay",
							"type" : "Duration",
							"canBeEmpty" : false,
							"durationType" : "Seconds"
						},
						"30003" : {
							"name" : "End trading time delay",
							"type" : "Duration",
							"canBeEmpty" : false,
							"durationType" : "Seconds"
						},
						"30004" : {
							"name" : "Order hold time",
							"type" : "Duration",
							"min" : 1000000000,
							"canBeEmpty" : false,
							"durationType" : "Seconds"
						},
						"30005" : { "name" : "Active minimum trend strike mode", "type" : "Bool" },
						"30006" : { "name" : "Minimum trend strike", "type" : "Int64" },
						"30007" : { "name" : "Active minimum trend strike price limit", "type" : "Bool" },
						"30008" : { "name" : "Minimum trend strike price limit", "type" : "Double" },
						"30009" : { "name" : "Active target order cost mode", "type" : "Bool" },
						"30010" : { "name" : "Target order cost", "type" : "Double" },
						"30011" : { "name" : "Trend strike barrier", "type" : "Uint64" },
						"30012" : { "name" : "Size buffer soft price change", "type" : "Uint64", "min" : 1 },
						"30013" : { "name" : "Gateway id", "type" : "OptionalInt32", "canBeEmpty" : false },
						"30014" : {
							"name" : "Commissions",
							"type" : "TableData",
							"canBeEmpty" : false,
							"columns" : {
								"0" : {
									"type" : "Int16",
									"name" : "Instrument type",
									"stringInterpretations" : {
										"0" : "Undefined",
										"1" : "Bond",
										"2" : "Share",
										"3" : "Currency",
										"4" : "ETF",
										"5" : "Futures",
										"6" : "StructuralProduct",
										"7" : "Option",
										"8" : "ClearingCertificate",
										"9" : "Index",
										"10" : "Commodity"
									}
								},
								"1" : { "type" : "Double", "name" : "%" }
							}
						},
						"30015" : {
							"name" : "Figis to trade",
							"type" : "TableData",
							"canBeEmpty" : false,
							"columns" : {
								"0" : { "type" : "Uint64", "name" : "figi" },
								"1" : { "type" : "Uint64", "name" : "Default lots volume" }
							}
						},
						"1000001" : { "name" : "Seconds between try to connect", "type" : "Uint32", "min" : 1 },
						"1000002" : { "name" : "Limit of attempts to connection", "type" : "Uint64", "min" : 1 },
						"1000003" : { "name" : "Limit of connections from one IP", "type" : "Uint64", "min" : 1 },
						"1000004" : { "name" : "Recv buffer size", "type" : "Uint64", "min" : 3 },
						"1000005" : { "name" : "Recv buffer size limit", "type" : "Uint64", "min" : 1024 }
					},
					"const" : {
						"1000006" : {
							"name" : "Server state",
							"type" : "Int16",
							"stringInterpretations" :
								{ "0" : "Undefined", "1" : "Initialization", "2" : "Running", "3" : "Stopped" }
						},
						"1000007" : { "name" : "Max connections", "type" : "Int32" },
						"1000008" : { "name" : "Listening IP", "type" : "Uint32" },
						"1000009" : { "name" : "Listening port", "type" : "Uint16" },
						"2000001" : { "name" : "Name", "type" : "String" },
						"2000002" : {
							"name" : "Application state",
							"type" : "Int16",
							"stringInterpretations" : { "0" : "Undefined", "1" : "Paused", "2" : "Running" }
						}
					}
				}
			};
		}
		else {
			response = { "uids" : [ json["uid"] ], "data" : { "mutable" : {}, "const" : {} } };
		}

		connection.send(Helper.ParametersToJson(response));
	}

	SendParameters(connection, json)
	{
		let response = { "uids" : [ json["uid"] ], "data" : {} };
		if (json["appType"] == Helper.StringHash32Uint("Strategy")) {
			response["data"]["response"] = "Soft scalping";
		}

		connection.send(Helper.ParametersToJson(response));
	}
};

module.exports = ServerSimulator;