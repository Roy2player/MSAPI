/**************************
 * This file is part of MSAPI.
 * License: see LICENSE.md
 * Contributor terms: see CONTRIBUTING.md
 *
 * This software is licensed under the Polyform Noncommercial License 1.0.0.
 * You may use, copy, modify, and distribute it for noncommercial purposes only.
 *
 * For commercial use, please contact: msapi.info@mail.ru
 *
 * Required Notice: MSAPI, copyright © 2021–2026 Maksim Andreevich Leonov, msapi.info@mail.ru
 *
 * @brief View to display the MSAPI Table.
 */

if (typeof module !== "undefined" && typeof module.exports !== "undefined") {
	View = require("../core/view");
}

class TableView extends View {
	constructor(parameters) { super("TableView", parameters); }

	Constructor(parameters)
	{
		this.SetTitleInfo(parameters.viewTitle);
		this.m_eventTarget = parameters.eventTarget;

		let table = new Table(
			{ parent : this.m_viewNode, id : parameters.tableId, metadata : parameters.metadata, isMutable : false });
		this.m_tableIdToTable.set(+parameters.tableId, table);

		return true;
	}
}

View.AddViewTemplate("TableView", `<div class="tableView"></div>`);

if (typeof module !== "undefined" && typeof module.exports !== "undefined") {
	module.exports = TableView;
}