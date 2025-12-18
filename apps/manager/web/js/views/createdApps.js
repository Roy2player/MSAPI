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
 * Required Notice: MSAPI, copyright © 2021–2025 Maksim Andreevich Leonov, maks.angels@mail.ru
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
	constructor(parameters) { super("CreatedApps", parameters); }

	async Constructor(parameters)
	{
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

						View.SendRequest({
							method : "GET",
							mode : "cors",
							headers : {
								"Accept" : "application/json",
								"Type" : state == 1 ? "run" : "pause",
								"Port" : rowObject.values[1000009]
							}
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
						View.SendRequest({
							method : "GET",
							mode : "cors",
							headers : { "Accept" : "application/json", "Type" : "delete", "Port" : port }
						});
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
								const port = rowObject.values[1000009];
								if (port === undefined) {
									console.error("Can't find server port in row values");
									return;
								}

								new AppView({ appType, port, viewPortParameter });
							});

							viewCell.classList.add("action", "openAppView");
						}
					}
				}
			}
		});
		this.AddCallback("getCreatedApps", (response) => {
			if ("apps" in response) {
				this.m_grid.m_rowByIndexValue.forEach((rowObject, indexValue) => {
					if (!response.apps.some(app => app.port == indexValue)) {
						this.m_grid.RemoveRow({ indexValue : indexValue });
					}
				});

				response.apps.forEach(app => {
					this.m_grid.AddOrUpdateRow({
						1000009 : app.port,
						5 : app.type,
						10 : app.viewPortParameter,
					});
					View.SendRequest({
						method : "GET",
						mode : "cors",
						headers : { "Accept" : "application/json", "Type" : "getParameters", "Port" : app.port }
					});
				});
			}
		});

		this.AddCallback("run", (response, extraParameters) => {
			if (response.status && response.result) {
				(async () => {
					await View.SendRequest({
						method : "GET",
						mode : "cors",
						headers :
							{ "Accept" : "application/json", "Type" : "getParameters", "Port" : extraParameters.port }
					});
				})();
			}
			let rowObject = this.m_grid.m_rowByIndexValue.get(extraParameters.port);
			if (rowObject) {
				let actionCell = rowObject.row.querySelector(".cell[parameter-id='2']");
				if (actionCell) {
					actionCell.classList.remove("loading");
				}
			}
			else {
				console.error("Can't find row for port", extraParameters.port);
			}
		});

		this.AddCallback("pause", (response, extraParameters) => {
			if (response.status && response.result) {
				(async () => {
					await View.SendRequest({
						method : "GET",
						mode : "cors",
						headers :
							{ "Accept" : "application/json", "Type" : "getParameters", "Port" : extraParameters.port }
					});
				})();
			}
			let rowObject = this.m_grid.m_rowByIndexValue.get(extraParameters.port);
			if (rowObject) {
				let actionCell = rowObject.row.querySelector(".cell[parameter-id='2']");
				if (actionCell) {
					actionCell.classList.remove("loading");
				}
			}
			else {
				console.error("Can't find row for port", extraParameters.port);
			}
		});

		this.AddCallback("delete", (response, extraParameters) => {
			this.m_grid.RemoveRow({ indexValue : extraParameters.port });
			let destructed = 0;
			// Destruct all views related to the deleted application in terms of same port
			for (let view of View.GetCreatedViews().values()) {
				if (view.m_port == extraParameters.port) {
					view.Destructor();
					destructed++;
				}
			}
			return destructed > 0;
		});

		this.AddCallback("getParameters", (response, extraParameters) => {
			if ("parameters" in response && "port" in extraParameters) {
				this.m_grid.AddOrUpdateRow(response.parameters);
			}
		});

		View.SendRequest(
			{ method : "GET", mode : "cors", headers : { "Accept" : "application/json", "Type" : "getCreatedApps" } });

		return true;
	}
}

View.AddViewTemplate("CreatedApps", `<div></div>`);
Dispatcher.RegisterPanel("CreatedApps", () => new CreatedApps());

if (typeof module !== "undefined" && typeof module.exports !== "undefined") {
	module.exports = CreatedApps;
}