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
 * @brief View to display grid column settings.
 */

if (typeof module !== "undefined" && typeof module.exports !== "undefined") {
	View = require("../core/view");
	Timer = require("../core/timer");
	Select = require("../core/select");
	Helper = require("../help/helper");
	Grid = require("../core/grid");
}

class GridColumnSettingsView extends View {
	constructor(parameters) { super("GridColumnSettingsView", parameters); }

	/**************************
	 * @brief Creates type-specific display accuracy controls and restores invalid edits to the previous selection.
	 * Floating-point controls accept powers of ten within the type's declared accuracy limits.
	 *
	 * @param parameters - Settings view title, event target, parameter identifier, and column object.
	 *
	 * @return True if the column metadata and required controls are available, false otherwise.
	 *
	 * @test Yes.
	 */
	Constructor(parameters)
	{
		this.m_parentView.querySelector(".title > span").textContent = parameters.viewTitle;
		this.m_eventTarget = parameters.eventTarget;
		this.m_parameterId = parameters.parameterId;

		const columnObject = parameters.columnObject;
		if (!columnObject) {
			console.error("Column object is not provided");
			return false;
		}

		const metadata = columnObject?.metadata;
		if (!metadata) {
			console.error("Column metadata is not provided");
			return false;
		}

		if (metadata.type == "Timer") {
			let accuracyNode = this.m_view.querySelector(".group.accuracy");
			if (!accuracyNode) {
				console.error("Accuracy node is not found");
				return false;
			}

			let input = document.createElement("input");
			input.setAttribute("parameter-id", /*Time accuracy*/ 11);
			Select.Apply({ input });
			const value = columnObject?.accuracy;
			if (value) {
				Select.SetValue(input, value);
			}
			accuracyNode.classList.remove("hidden");
			accuracyNode.appendChild(input);

			input.addEventListener("change", () => {
				const accuracy = +input.getAttribute("select");
				const isValid = Object.values(Timer.ACCURACY).includes(accuracy);

				if (!isValid) {
					console.error("Invalid accuracy value", accuracy);
					return;
				}

				Grid.SetColumnAccuracy({ columnObject, accuracy });
			});
		}

		const isFloat = metadata.type.includes("Float");
		const isDouble = metadata.type.includes("Double");
		if (isFloat || isDouble) {
			let accuracyNode = this.m_view.querySelector(".group.accuracy");
			if (!accuracyNode) {
				console.error("Accuracy node is not found");
				return false;
			}

			let input = document.createElement("input");
			input.type = "number";
			input.max = 1;
			input.min = isFloat ? 1e-9 : 1e-17;
			input.step = "any";
			input.title = "Positive rounding increment";
			input.setAttribute("aria-label", "Positive rounding increment");
			input.value = columnObject?.accuracy;
			input.backup = "";

			accuracyNode.classList.remove("hidden");
			accuracyNode.appendChild(input);

			const apply = () => {
				const accuracy = +input.value;
				const extension = Math.log10(accuracy);
				const isValid = Number.isFinite(accuracy) && !Helper.FloatLess(accuracy, input.min, input.min / 10)
					&& !Helper.FloatGreater(accuracy, input.max, input.min)
					&& Helper.FloatEqual(extension, Math.round(extension), input.min / 10);
				if (!isValid) {
					input.value = input.backup;
					return;
				}

				input.backup = input.value;
				Grid.SetColumnAccuracy({ columnObject, accuracy });
			};

			apply();

			input.addEventListener("change", apply);
		}

		return true;
	}
}

View.AddViewTemplate("GridColumnSettingsView", `<div class="gridColumnSettingsView customView">
    <div class="group">
        <span class="text">Align:</span>
        <span class="action alignLeft" title="Align text left"></span>
        <span class="action alignCenter" title="Center text"></span>
        <span class="action alignRight" title="Align text right"></span>
    </div>
    <div class="group">
        <span class="text">Sorting:</span>
        <span class="action ascending" title="Sort ascending"></span>
        <span class="action none" title="Disable sorting"></span>
        <span class="action descending" title="Sort descending"></span>
    </div>
	<div class="group accuracy hidden">
        <span class="text">Accuracy:</span>
    </div>
    <div class="group">
        <span class="text">Filter:</span>
        <span class="action filter" title="Enable or disable column filters"></span>
    </div>
    <div class="filters"></div>
</div>`);

if (typeof module !== "undefined" && typeof module.exports !== "undefined") {
	module.exports = GridColumnSettingsView;
}