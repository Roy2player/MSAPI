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
 * @brief View to display grid settings.
 */

if (typeof module !== 'undefined' && typeof module.exports !== 'undefined') {
	View = require('../view');
}

class GridSettingsView extends View {
	constructor(parameters) { super("GridSettingsView", parameters); }

	async Constructor(parameters)
	{
		this.m_parentView.querySelector(".title > span").textContent = parameters.viewTitle;
		this.m_eventTarget = parameters.eventTarget;
		this.m_parameterId = parameters.parameterId;
		return true;
	}
}

View.AddViewTemplate("GridSettingsView", `<div class="gridSettingsView customView">
    <div class="group">
        <span class="text">Align:</span>
        <span class="action alignLeft"></span>
        <span class="action alignCenter"></span>
        <span class="action alignRight"></span>
    </div>
    <div class="group">
        <span class="text">Sorting:</span>
        <span class="action ascending"></span>
        <span class="action none"></span>
        <span class="action descending"></span>
    </div>
    <div class="group">
        <span class="text">Filter:</span>
        <span class="action filter"></span>
    </div>
    <div class="filters"></div>
</div>`);

if (typeof module !== 'undefined' && typeof module.exports !== 'undefined') {
	module.exports = GridSettingsView;
}