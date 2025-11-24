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
 * @brief View to display installed MSAPI applications.
 */

if (module && module.exports) {
	View = require('../view');
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
					if (!View.GetMetadata(app.type)) {
						View.SendRequest({
							method : "GET",
							mode : "cors",
							headers : { "Accept" : "application/json", "Type" : "getMetadata", "AppType" : app.type }
						});
					}

					if ("viewPortParameter" in app) {
						View.SetViewPortParameterToAppType(app.type, app.viewPortParameter);
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

if (module && module.exports) {
	module.exports = InstalledApps;
}