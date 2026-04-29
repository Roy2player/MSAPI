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

const Duration = require("../core/duration");
const Timer = require("../core/timer");
const Select = require("../core/select");

class TableChecker {
	static CheckValues(test, inputs, values)
	{
		test.Assert(inputs.length, values.length);

		for (let i = 0; i < values.length; i++) {
			if (typeof values[i] === "boolean") {
				test.Assert(inputs[i].checked, values[i], `Child ${i + 1} has incorrect value`);
			}
			else {
				test.Assert(inputs[i].value, values[i], `Child ${i + 1} has incorrect value`);
			}
		}
	}

	static AddRow(test, table, values)
	{
		let newRow = table.m_wrapper.querySelector(".row.new");
		let inputs = newRow.querySelectorAll("input");
		test.Assert(inputs.length, values.length);

		for (let i = 0; i < inputs.length; i++) {
			let input = inputs[i];
			if (input.type == 'checkbox') {
				input.checked = values[i];
				continue;
			}

			if (input.hasAttribute("nanoseconds")) {
				Duration.SetValue(input, values[i]);
				continue;
			}

			if (input.hasAttribute("timestamp")) {
				Timer.SetValue(input, values[i]);
				continue;
			}

			if (input.hasAttribute("select")) {
				Select.SetValue(input, values[i]);
				continue;
			}

			input.value = values[i];
		}

		let addButton = table.m_wrapper.querySelector(".add");
		if (addButton) {
			addButton.dispatchEvent(new Event("click"));
			table.Save();
			return;
		}
		test.Assert(false, true, "Add button not found");
	}
};

module.exports = TableChecker;