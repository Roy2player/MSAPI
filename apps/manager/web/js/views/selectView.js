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
 * @brief View to display a select view.
 */

if (typeof module !== 'undefined' && typeof module.exports !== 'undefined') {
	View = require('../view');
}

class SelectView extends View {
	constructor(parameters) { super("SelectView", parameters); }

	async Constructor(parameters)
	{
		this.m_title = parameters.viewTitle;
		this.m_parentView.querySelector(".title > span").textContent = this.m_title;
		this.m_eventTarget = parameters.eventTarget;

		let caseSensitiveElement = this.m_view.querySelector(".search > .caseSensitive");
		let isCaseSensitive = false;
		caseSensitiveElement.classList.add("disabled");

		let search = () => {
			let items = this.m_view.querySelectorAll(".options>*");
			items.forEach((item) => {
				if ((isCaseSensitive && item.innerHTML.includes(searchElement.value))
					|| (!isCaseSensitive && item.innerHTML.toLowerCase().includes(searchElement.value.toLowerCase()))) {

					item.style.display = "";
				}
				else {
					item.style.display = "none";
				}
			});
		};

		caseSensitiveElement.addEventListener("click", () => {
			isCaseSensitive = !isCaseSensitive;
			if (isCaseSensitive) {
				caseSensitiveElement.classList.remove("disabled");
			}
			else {
				caseSensitiveElement.classList.add("disabled");
			}

			search();
		});

		let searchElement = this.m_view.querySelector(".search > input");
		searchElement.addEventListener("input", search);

		return true;
	}
}

View.AddViewTemplate("SelectView", `<div class="selectView customView">
    <div class="search">
        <div class="searchIco"></div>
        <input type="text" placeholder="Search">
        <div class="caseSensitive"></div>
    </div>
    <div class="options"></div>
</div>`);

if (typeof module !== 'undefined' && typeof module.exports !== 'undefined') {
	module.exports = SelectView;
}