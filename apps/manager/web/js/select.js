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
 * @brief Select static class is a namespace for select related functions. Provides ability to cast input node to
 * select input. Select element has its own view with list of options and parametrized quick search by substring.
 *
 * @brief Can be casted without adding event listener if needed.
 * @brief Attribute "parameter-id" is required to be set.
 * @brief Supports "min" and "max" attributes for validation.
 * @brief Has "type" attribute as "text", "select" attribute to store selected key and "readonly" attribute as true.
 * @brief Has "click" event listener to open select view. If click is outside select view, it will be closed without
 * changes.
 *
 * @test Has unit tests.
 */
class Select {
	static #privateFields = (() => {
		let m_hasGlobalEventListener = false;
		let m_selectsViews = new Set();

		return { m_hasGlobalEventListener, m_selectsViews };
	})();

	/**************************
	 * @return Number of select views. Lazy clearing.
	 */
	static GetSelectViewsNumber() { return Select.#privateFields.m_selectsViews.size; }

	static Apply({ input, setEvent = true })
	{
		input.classList.add("select");
		input.setAttribute("select", "");
		input.type = "text";
		input.readOnly = true;

		if (setEvent) {
			Select.SetEvent(input);
		}
	}

	static SetEvent(input)
	{
		let view = undefined;
		input.addEventListener('click', () => {
			if (view && view.m_parentView.parentNode) {
				return;
			}

			if (!input.hasAttribute("parameter-id")) {
				console.error("Parameter id is not set for select input");
				return;
			}

			const parameterId = +input.getAttribute("parameter-id");
			const stringInterpretation = MetadataCollector.GetStringInterpretation(parameterId);

			if (!stringInterpretation) {
				console.error("No string interpretation is found for parameter id", parameterId);
				return;
			}

			let selectViews = Select.#privateFields.m_selectsViews;
			let viewTitle = ""
			const selectMetadata = MetadataCollector.GetMetadata(parameterId)
			if (selectMetadata)
			{
				viewTitle = "Select: " + selectMetadata.metadata.name + " " + parameterId;
			}
			else
			{
				viewTitle = "Select: " + parameterId;
			}

			view = new SelectView({
				eventTarget : input,
				parameterId : parameterId,
				viewTitle,
				positionUnder : input,
				canBeHidden : false,
				canBeMaximized : false,
				canBeSticked : false,
				canBeClinged : false,
				postCreateFunction : () => {
					let selectItems = view.m_view.querySelector(".options");
					if (!selectItems) {
						console.error(`Select container for options is not found, parameter id: ${parameterId}`);
						return;
					}

					for (let [key, value] of Object.entries(stringInterpretation)) {
						let divElement = document.createElement("div");
						let valueSpan = document.createElement("span");
						let keySpan = document.createElement("span");
						valueSpan.innerHTML = value;
						keySpan.innerHTML = key;
						divElement.appendChild(valueSpan);
						divElement.appendChild(keySpan);

						divElement.addEventListener("click", () => {
							input.value = value;
							input.setAttribute("select", key);
							Select.Validate(input);
							view.Destructor();

							selectViews.forEach(selectView => {
								if (!selectView.m_parentView.parentNode) {
									selectViews.delete(selectView);
								}
							});
						});

						selectItems.appendChild(divElement);
					}
				}
			});

			selectViews.add(view);

			if (!Select.#privateFields.m_hasGlobalEventListener) {
				Select.#privateFields.m_hasGlobalEventListener = true;
				document.addEventListener("click", (event) => {
					selectViews.forEach(selectView => {
						if (!selectView.m_parentView.parentNode) {
							selectViews.delete(selectView);
							return;
						}

						if (!selectView.m_parentView.contains(event.target)) {
							if (event.target == selectView.m_eventTarget) {
								View.UpdateZIndex(selectView);
								return;
							}

							selectView.Destructor();
						}
					});
				});
			}
		});
	}

	static SetValue(input, key)
	{
		if (!input.hasAttribute("parameter-id")) {
			console.error("Parameter id is not set for select input");
			return;
		}

		const currentSelect = input.getAttribute("select");
		if (currentSelect != "" && +currentSelect == key) {
			return;
		}

		const parameterId = +input.getAttribute("parameter-id");
		const stringInterpretation = MetadataCollector.GetStringInterpretation(parameterId);

		if (!stringInterpretation) {
			console.error("No string interpretation is found for parameter id", parameterId);
			return;
		}

		if (!(key in stringInterpretation)) {
			input.value = key + " - not found";
		}
		else {
			input.value = stringInterpretation[key];
		}

		input.setAttribute("select", key);
		Select.Validate(input);
	}

	static Validate(input)
	{
		if (input.value == "") {
			input.classList.add("invalid");
			return false;
		}

		const select = input.getAttribute("select");
		if (select == null) {
			input.classList.add("invalid");
			return false;
		}

		const min = input.getAttribute("min");
		if (min !== null && BigInt(select) < BigInt(min)) {
			input.classList.add("invalid");
			return false;
		}

		const max = input.getAttribute("max");
		if (max !== null && BigInt(select) > BigInt(max)) {
			input.classList.add("invalid");
			return false;
		}

		input.classList.remove("invalid");
		return true;
	}
}

if (typeof module !== 'undefined' && typeof module.exports !== 'undefined') {
	module.exports = Select;
	MetadataCollector = require('./metadataCollector');
	View = require('./view');
	SelectView = require('./views/selectView');
}