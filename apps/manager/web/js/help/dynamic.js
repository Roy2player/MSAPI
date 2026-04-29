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

/**
 * @brief Helpers to add interface dynamic.
 */
class Dynamic {

	/**
	 * @brief Add clicking to data-form-item='button' by pressing enter in any data-form-item='input' inside provided
	 * node.
	 */
	static InitForm(node)
	{
		let inputs = node.querySelectorAll("[data-form-item='input']");
		if (inputs.length == 0) {
			console.warn("No inputs in form node");
			return;
		}

		let buttons = node.querySelectorAll("[data-form-item='button']");
		if (buttons.length == 0) {
			console.warn("No buttons in form node");
			return;
		}

		if (buttons.length > 1) {
			console.warn("More than one button in form node");
			return;
		}

		const button = buttons[0];

		inputs.forEach(element => {
			element.addEventListener("keydown", (event) => {
				if (event.key === 'Enter') {
					button.click();
				}
			});
		});
	}

	/**
	 * @brief Clear value for all data-form-item='input' inside provided node.
	 */
	static ClearForm(node)
	{
		let inputs = node.querySelectorAll("[data-form-item='input']");
		if (inputs.length == 0) {
			console.warn("No inputs in form node");
			return;
		}

		inputs.forEach(element => { element.value = ""; });
	}

	/**
	 * @brief Add data-trigger-[toggle|remove|add]='id1,id2' action to each mentioned id and invoke with "active" class
	 * to each data-trigger-id='id' inside provided node.
	 */
	static InitTriggers(node)
	{
		let getDrivenLists = (trigger, action) => {
			if (trigger.hasAttribute(`data-trigger-${action}`)) {
				let drivenIds = trigger.getAttribute(`data-trigger-${action}`).replace(/ /g, '').split(',');
				let drivenLists = new Set;

				drivenIds.forEach((id) => {
					let nodes = node.querySelectorAll(`[data-trigger-id=${id}]`);
					if (nodes.length != 0) {
						drivenLists.add(nodes);
					}
				});

				return drivenLists;
			}

			return [];
		};

		let addEventListener = (action, func) => {
			node.querySelectorAll(`[data-trigger-${action}]`).forEach((trigger) => {
				let drivenLists = getDrivenLists(trigger, action);

				trigger.addEventListener("click", () => { drivenLists.forEach((list) => { list.forEach(func); }); });
			});
		};

		addEventListener("toggle", (driven) => { driven.classList.toggle("active") });
		addEventListener("remove", (driven) => { driven.classList.remove("active") });
		addEventListener("add", (driven) => { driven.classList.add("active") });
	}
};

if (typeof module !== "undefined" && typeof module.exports !== "undefined") {
	module.exports = Dynamic;
}