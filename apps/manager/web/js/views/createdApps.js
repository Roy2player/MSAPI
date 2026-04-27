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
 * @brief View to display grid with created apps.
 *
 * By default contains several columns:
 * - 10 (Open view) to open application view if possible;
 * - 2 (Change state) to run/pause application;
 * - 3 (Modify) to modify application settings;
 * - 4 (Delete) to delete application;
 * - 2000001 (Name) application name;
 * - 5 (Type) application type;
 * - 1000008 (Listening IP) application listening IP;
 * - 1000009 (Port) application port.
 *
 * Delete action also destroys all views related to the deleted application in terms of same port.
 */

if (typeof module !== "undefined" && typeof module.exports !== "undefined") {
	View = require("../view");
	MetadataCollector = require("../views/metadataCollector");
	Dispatcher = require("../views/dispatcher").Dispatcher;
}

class CreatedApps extends View {
	constructor(parameters) { super("Created apps", parameters); }

	Constructor(parameters)
	{
		this.m_portToParametersStream = new Map();
		this.m_grid = new Grid({
			parent : this.m_view,
			indexColumnId : 1000009,
			columns : [ 10, 2, 3, 4, 2000001, 5, 1000008, 1000009 ],
			indexColumn : "Port",
			postAddRowFunction : (rowObject) => {
				let changeStateCell = rowObject.row.querySelector(".cell[parameter-id='2']");
				if (changeStateCell) {
					changeStateCell.addEventListener("click", () => {
						if (changeStateCell.classList.contains("loading")) {
							return;
						}

						const port = rowObject.values[1000009];
						if (port === undefined) {
							console.error("Can't find server port in row values");
							return;
						}

						const state = rowObject.values[2000002];
						if (state === undefined) {
							console.error("Can't find application state in row values");
							return;
						}

						new WebSocketSingle({
							event : Helper.StringHashDjb2(state == 1 ? "run" : "pause"),
							handleResponse : (response) => {},
							handleFailed : (error) => {
								changeStateCell.classList.remove("loading");
								this.DisplayErrorMessage(error);
							},
							viewUid : this.m_uid,
							data : { "port" : port }
						});
						changeStateCell.classList.add("loading");
					});

					changeStateCell.classList.add("action");
				}

				let modifyCell = rowObject.row.querySelector(".cell[parameter-id='3']");
				if (modifyCell) {
					modifyCell.addEventListener("click", () => {
						const port = rowObject.values[1000009];
						if (port === undefined) {
							console.error("Can't find server port in row values");
							return;
						}

						let appType = rowObject.values[5];
						if (appType === undefined) {
							console.error("Can't find application type in row values");
							return;
						}

						new ModifyApp({ appType, port });
					});

					modifyCell.classList.add("action", "modify");
				}

				let deleteCell = rowObject.row.querySelector(".cell[parameter-id='4']");
				if (deleteCell) {
					deleteCell.addEventListener("click", () => {
						const port = rowObject.values[1000009];
						if (port === undefined) {
							console.error("Can't find server port in row values");
							return;
						}

						if (!confirm(`Are you sure you want to delete application ${rowObject.values[2000001]} type ${
								rowObject.values[5]} on port ${port}?`)) {

							return;
						}

						new WebSocketSingle({
							event : Helper.StringHashDjb2("delete"),
							handleResponse : (response) => {
								this.m_grid.RemoveRow({ indexValue : port });
								let destructed = 0;
								// Destruct all views related to the deleted application in terms of same port
								for (let view of View.GetCreatedViews().values()) {
									if (view.m_port == port) {
										view.Destructor();
										destructed++;
									}
								}
							},
							handleFailed : (error) => {
								deleteCell.classList.remove("loading");
								this.DisplayErrorMessage(error);
							},
							viewUid : this.m_uid,
							data : { "port" : port }
						});
						deleteCell.classList.add("loading");
					});

					deleteCell.classList.add("action", "delete");
				}
			},
			postUpdateRowFunction : (rowObject, updatedValues) => {
				if (updatedValues.hasOwnProperty(2000002)) {
					let changeStateCell = rowObject.row.querySelector(".cell[parameter-id='2']");
					if (changeStateCell) {
						if (updatedValues[2000002] == 1) {
							changeStateCell.classList.add("run");
							changeStateCell.classList.remove("pause");
						}
						else {
							changeStateCell.classList.add("pause");
							changeStateCell.classList.remove("run");
						}

						if (changeStateCell.classList.contains("loading")) {
							changeStateCell.classList.remove("loading");
						}
					}
				}

				if (updatedValues.hasOwnProperty(10)) {
					let viewCell = rowObject.row.querySelector(".cell[parameter-id='10']");
					if (viewCell && !viewCell.classList.contains("action", "openAppView")) {
						let appType = rowObject.values[5];
						if (appType === undefined) {
							console.error("Can't find application type in row values", rowObject);
							return;
						}
						const viewPortParameter = MetadataCollector.GetViewPortParameterToAppType(appType);
						if (viewPortParameter != undefined) {
							viewCell.addEventListener("click", () => {
								const port = rowObject.values[viewPortParameter];
								if (port === undefined) {
									console.error("Can't find server port in row values");
									return;
								}
								const ip = rowObject.values[1000008];
								if (ip === undefined) {
									console.error("Can't find server ip in row values");
									return;
								}

								new AppView({ appType, port, ip });
							});

							viewCell.classList.add("action", "openAppView");
						}
					}
				}
			}
		});

		let handleCreatedApps = (data) => {
			if ("deleted" in data) {
				data.deleted.forEach((port) => {
					let stream = this.m_portToParametersStream.get(port);
					if (stream) {
						stream.Close();
						this.m_portToParametersStream.delete(port);
					}
					else {
						console.warn("Unexpectedly absence of parameters stream for app on port", port);
					}
					this.m_grid.RemoveRow({ indexValue : port });
				})
			}

			if ("created" in data) {
				data.created.forEach((app) => {
					this.m_grid.AddOrUpdateRow({
						1000009 : app.port,
						5 : app.type,
						10 : app.viewPortParameter,
					});

					let stream = new WebSocketStream({
						event : Helper.StringHashDjb2("parameters"),
						handleData : (parameters) => {
							if (parameters["1000009"] == undefined) {
								if (Object.keys(parameters).length != 0) {
									log.warn(
										"Update for app row is not empty but does not contain index row", parameters);
								}
								return;
							}
							this.m_grid.AddOrUpdateRow(parameters);
						},
						handleFailed : (error) => { this.DisplayErrorMessage(error); },
						viewUid : this.m_uid,
						data : { "port" : app.port, "filter" : app.port }
					});
					this.m_portToParametersStream.set(app.port, stream);
				})
			}
		};

		new WebSocketStream({
			event : Helper.StringHashDjb2("createdApps"),
			handleData : handleCreatedApps,
			handleFailed : (error) => { this.DisplayErrorMessage(error); },
			viewUid : this.m_uid
		});

		return true;
	}
}

View.AddViewTemplate("Created apps", `<div></div>`);
Dispatcher.RegisterPanel("Created apps", () => new CreatedApps());

if (typeof module !== "undefined" && typeof module.exports !== "undefined") {
	module.exports = CreatedApps;
}