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
 * @brief View to display installed MSAPI applications. On creation requests metadata for all installed apps if not yet
 * available. Only one instance can be created.
 */

if (typeof module !== "undefined" && typeof module.exports !== "undefined") {
	View = require("../core/view");
	MetadataCollector = require("./metadataCollector");
	Dispatcher = require("./dispatcher").Dispatcher;
	WebSocketSingle = require("../core/webSocketHandler").WebSocketSingle;
}

class InstalledApps extends View {
	constructor(parameters) { super("Installed apps", parameters); }

	Constructor(parameters)
	{
		if (View.ShowExistedView(this)) {
			return false;
		}

		this.m_grid = new Grid({
			parent : this.m_view,
			indexColumnId : 5,
			columns : [ 1, 5 ],
			postAddRowFunction : (rowObject) => {
				let createCell = rowObject.row.querySelector(".cell[parameter-id='1']");
				if (createCell && rowObject.values.hasOwnProperty(5)) {
					createCell.addEventListener("click", () => { new NewApp({ appType : rowObject.values[5] }); });
					createCell.classList.add("action", "create");
				}
			}
		});

		const handleResponse = (response) => {
			response.forEach(app => {
				this.m_grid.AddOrUpdateRow({ 5 : app.type });

				if (!MetadataCollector.GetAppMetadata(app.type)) {
					new WebSocketSingle({
						event : Helper.StringHash32Uint("getMetadata"),
						handleResponse : (metadata) => { MetadataCollector.AddAppMetadata(app.type, metadata); },
						handleFailed : (error) => { this.DisplayErrorMessage(error); },
						viewUid : this.m_uid,
						data : {
							"appType" : Helper.StringHash32Uint(app.type),
							"filter" : Helper.StringHash32Uint(app.type)
						}
					});
				}

				if ("viewPortParameter" in app) {
					MetadataCollector.SetViewPortParameterToAppType(app.type, app.viewPortParameter);
				}
			});
		};

		new WebSocketSingle({
			event : Helper.StringHash32Uint("installedApp"),
			handleResponse : handleResponse,
			handleFailed : (error) => {
				this.m_parentView.classList.remove("loading");
				this.DisplayErrorMessage(error);
			},
			viewUid : this.m_uid
		});

		return true;
	}
}

View.AddViewTemplate("Installed apps", `<div></div>`);
Dispatcher.RegisterPanel("Installed apps", () => new InstalledApps());

if (typeof module !== "undefined" && typeof module.exports !== "undefined") {
	module.exports = InstalledApps;
}