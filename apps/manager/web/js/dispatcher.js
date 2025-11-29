/**************************
 * @file        dispatcher.js
 * @version     6.0
 * @date        2024-10-08
 * @author      maks.angels@mail.ru
 * @copyright   © 2021–2025 Maksim Andreevich Leonov
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
 * Required Notice: MSAPI, copyright © 2021–2025 Maksim Andreevich Leonov, maks.angels@mail.ru
 *
 * @brief Dispatcher class for managing hidden views and registered panels. Has its own html without separate view.
 */
class Dispatcher {
	static #privateFields = (() => {
		const m_nodeToHiddenViews = new Map();
		const m_registeredPanelsNames = new Set();
		return { m_nodeToHiddenViews, m_registeredPanelsNames };
	})();

	static #generalTemplate = `
	<div class="dispatcher hidden">
		<div class="hiddenViews">
			<div class="title">Hidden views</div>
			<div class="list"></div>
		</div>
		<div class="registeredPanels">
			<div class="title">Registered panels</div>
			<div class="list"></div>
		</div>
        <div class="control"><span></span></div>
	</div>`;
	static #generalTemplateElement = null;

	constructor()
	{
		this.m_parentNode = document.querySelector('main');
		if (!this.m_parentNode) {
			console.error('Parent node not found');
			return;
		}

		if (!Dispatcher.#generalTemplateElement) {
			const template = document.createElement("template");
			template.innerHTML = Dispatcher.#generalTemplate;
			Dispatcher.#generalTemplateElement = template;
		}

		this.m_view = Dispatcher.#generalTemplateElement.content.cloneNode(true).firstElementChild;
		this.m_parentNode.appendChild(this.m_view);

		this.m_control = this.m_view.querySelector('.control');
		this.m_control.addEventListener('click', () => { this.m_view.classList.toggle('hidden'); });

		this.m_hiddenViews = this.m_view.querySelector('.hiddenViews');
		this.m_registeredPanels = this.m_view.querySelector('.registeredPanels');

		this.RegisterPanel("Dispatcher", () => { this.m_view.classList.remove('hidden'); });
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
		div.addEventListener('click', () => {
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

	RegisterPanel(panelName, creatorFunction)
	{
		if (typeof creatorFunction !== 'function') {
			console.error("Invalid creator function type, function is expected", creatorFunction);
			return;
		}

		if (Dispatcher.#privateFields.m_registeredPanelsNames.has(panelName)) {
			return;
		}

		Dispatcher.#privateFields.m_registeredPanelsNames.add(panelName);
		let div = document.createElement('div');
		let span = document.createElement('span');
		span.innerHTML = panelName;
		div.appendChild(span);

		div.addEventListener('click', () => creatorFunction());

		let registeredPanelsList = this.m_registeredPanels.querySelector('.list');
		registeredPanelsList.appendChild(div);
		if (!this.m_registeredPanels.classList.contains('visible')) {
			this.m_registeredPanels.classList.add('visible');
		}
	}

	UnregisterPanel(panelName)
	{
		if (!Dispatcher.#privateFields.m_registeredPanelsNames.has(panelName)) {
			return;
		}

		Dispatcher.#privateFields.m_registeredPanelsNames.delete(panelName);
		let registeredPanelsList = this.m_registeredPanels.querySelector('.list');
		registeredPanelsList.querySelector('span').remove();
		if (registeredPanelsList.childElementCount === 0) {
			this.m_registeredPanels.classList.remove('visible');
		}
	}
}

if (typeof module !== 'undefined' && typeof module.exports !== 'undefined') {
	module.exports = Dispatcher;
}