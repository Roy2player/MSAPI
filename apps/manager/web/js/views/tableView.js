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
 * @brief View to display the MSAPI Table.
 */

if (typeof module !== "undefined" && typeof module.exports !== "undefined") {
	View = require("../view");
}

class TableView extends View {
	constructor(parameters) { super("TableView", parameters); }

	async Constructor(parameters)
	{
		this.m_title += ": " + parameters.viewTitle;
		this.m_parentView.querySelector(".title > span").textContent = this.m_title;

		let table = new Table(
			{ parent : this.m_view, id : parameters.tableId, metadata : parameters.metadata, isMutable : false });
		this.m_tables.set(+parameters.tableId, table);

		return true;
	}
}

View.AddViewTemplate("TableView", `<div class="tableView"></div>`);

if (typeof module !== "undefined" && typeof module.exports !== "undefined") {
	module.exports = TableView;
}