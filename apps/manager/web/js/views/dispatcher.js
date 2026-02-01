/**************************
 * @file        dispatcher.js
 * @version     6.0
 * @date        2024-10-08
 * @author      maks.angels@mail.ru
 * @copyright   © 2021–2026 Maksim Andreevich Leonov
 *
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
 * @brief Dispatcher interface unit view for managing hidden views and registered panels.
 *
 * @shortcuts
 * - Ctrl+D to toggle visibility.
 */

if (typeof module !== "undefined" && typeof module.exports !== "undefined") {
	View = require("../view");
}

class Dispatcher extends View {
	static #privateFields = (() => {
		const m_nodeToHiddenViews = new Map();
		const m_panelToCreateFunc = new Map();
		return { m_nodeToHiddenViews, m_panelToCreateFunc };
	})();

	constructor() { super("Dispatcher", { "isInterfaceUnit" : true }); }

	async Constructor()
	{
		this.m_control = this.m_view.querySelector('.control');
		this.m_control.addEventListener("click", () => { this.m_view.classList.toggle('hidden'); });
		let savedThis = this;
		document.addEventListener('keydown', function(event) {
			if (event.ctrlKey && (event.key === 'd' || event.key === 'D')) {
				event.preventDefault();
				savedThis.m_control.dispatchEvent(new Event("click", { bubbles : true }));
			}
		});

		this.m_hiddenViews = this.m_view.querySelector('.hiddenViews');
		this.m_registeredPanels = this.m_view.querySelector('.registeredPanels');

		for (let [panelName, creatorFunction] of Dispatcher.#privateFields.m_panelToCreateFunc) {
			this.AddPanel(panelName, creatorFunction);
		}

		return true;
	}

	AddHiddenView(view)
	{
		if (Dispatcher.#privateFields.m_nodeToHiddenViews.has(view)) {
			return;
		}

		let div = document.createElement('div');
		let span = document.createElement('span');
		span.innerHTML = view.m_title || 'Unknown';
		div.appendChild(span);

		let hiddenViewsList = this.m_hiddenViews.querySelector('.list');
		div.addEventListener("click", () => {
			view.Show();
			this.RemoveHiddenView(view);
		});

		hiddenViewsList.appendChild(div);
		Dispatcher.#privateFields.m_nodeToHiddenViews.set(view, div);
		if (!this.m_hiddenViews.classList.contains('visible')) {
			this.m_hiddenViews.classList.add('visible');
		}
		view.m_parentView.classList.add("hidden");
	}

	RemoveHiddenView(view)
	{
		if (!Dispatcher.#privateFields.m_nodeToHiddenViews.has(view)) {
			return;
		}

		Dispatcher.#privateFields.m_nodeToHiddenViews.get(view).remove();
		if (this.m_hiddenViews.querySelector('.list').childElementCount === 0) {
			this.m_hiddenViews.classList.remove('visible');
		}
		Dispatcher.#privateFields.m_nodeToHiddenViews.delete(view);
		view.m_parentView.classList.remove("hidden");
	}

	static RegisterPanel(panelName, creatorFunction)
	{
		if (typeof creatorFunction !== 'function') {
			console.error("Invalid creator function type, function is expected", creatorFunction);
			return;
		}

		if (Dispatcher.#privateFields.m_panelToCreateFunc.has(panelName)) {
			return;
		}

		Dispatcher.#privateFields.m_panelToCreateFunc.set(panelName, creatorFunction);

		if (dispatcher) {
			dispatcher.AddPanel(panelName, creatorFunction);
		}
	}

	AddPanel(panelName, creatorFunction)
	{
		let div = document.createElement('div');
		div.setAttribute('type', panelName);
		let span = document.createElement('span');
		span.innerHTML = panelName;
		div.appendChild(span);

		div.addEventListener("click", () => creatorFunction());

		let registeredPanelsList = this.m_registeredPanels.querySelector('.list');
		registeredPanelsList.appendChild(div);
		if (!this.m_registeredPanels.classList.contains('visible')) {
			this.m_registeredPanels.classList.add('visible');
		}
	}

	UnregisterPanel(panelName)
	{
		if (!Dispatcher.#privateFields.m_panelToCreateFunc.has(panelName)) {
			return;
		}

		Dispatcher.#privateFields.m_panelToCreateFunc.delete(panelName);

		let registeredPanelsList = this.m_registeredPanels.querySelector('.list');
		if (!registeredPanelsList) {
			console.error("Registered panels list not found for unregistering", panelName);
			return;
		}

		let node = registeredPanelsList.querySelector(`div[type="${panelName}"]`);
		if (!node) {
			console.error("Panel node not found for unregistering", panelName);
			return;
		}

		node.remove();
		if (registeredPanelsList.childElementCount === 0) {
			this.m_registeredPanels.classList.remove('visible');
		}
	}
}

View.AddViewTemplate("Dispatcher", `<div class="dispatcher hidden">
		<div class="hiddenViews">
			<div class="title">Hidden views</div>
			<div class="list"></div>
		</div>
		<div class="registeredPanels">
			<div class="title">Registered panels</div>
			<div class="list"></div>
		</div>
        <div class="control"><span></span></div>
	</div>`);

let dispatcher = undefined;
document.addEventListener("DOMContentLoaded", () => {
	// That check is needed for node js tests as there this can be called multiple times
	// Firstly, when it is required to be sure it is defined
	// Secondly, when jsdom dispatches DOMContentLoaded event
	// It is pure hack because of nodejs module system limitations
	if (!dispatcher) {
		dispatcher = new Dispatcher();
	}
});

if (typeof module !== "undefined" && typeof module.exports !== "undefined") {
	module.exports.Dispatcher = Dispatcher;
	document.addEventListener("DOMContentLoaded", () => { global.dispatcher = dispatcher; });
}