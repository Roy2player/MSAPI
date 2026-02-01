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
 * available.
 */

if (typeof module !== "undefined" && typeof module.exports !== "undefined") {
	View = require("../view");
	MetadataCollector = require("../views/metadataCollector");
	Dispatcher = require("../views/dispatcher").Dispatcher;
}

class InstalledApps extends View {
	constructor(parameters) { super("InstalledApps", parameters); }

	async Constructor(parameters)
	{
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

		this.AddCallback("getInstalledApps", (response) => {
			if ("apps" in response) {
				response.apps.forEach(app => { this.m_grid.AddOrUpdateRow({ 5 : app.type }); });

				response.apps.forEach(app => {
					if (!MetadataCollector.GetAppMetadata(app.type)) {
						View.SendRequest({
							method : "GET",
							mode : "cors",
							headers : { "Accept" : "application/json", "Type" : "getMetadata", "AppType" : app.type }
						});
					}

					if ("viewPortParameter" in app) {
						MetadataCollector.SetViewPortParameterToAppType(app.type, app.viewPortParameter);
					}
				});
			}
		});

		void View.SendRequest({
			method : "GET",
			mode : "cors",
			headers : { "Accept" : "application/json", "Type" : "getInstalledApps" }
		});

		return true;
	}
}

View.AddViewTemplate("InstalledApps", `<div></div>`);
Dispatcher.RegisterPanel("InstalledApps", () => new InstalledApps());

if (typeof module !== "undefined" && typeof module.exports !== "undefined") {
	module.exports = InstalledApps;
}