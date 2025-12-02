/**************************
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
 * @brief Class for creating a table inside node with class "tables" in given parent node for the given parameter. The
 * table can be mutable or immutable. Mutable table has buttons for adding, removing, reverting and saving rows. Columns
 * can be any MSAPI type except another table.
 *
 * Expected parameter structure (example for Duration column):
 * {
 * 		id: string,
 *  	name: string,
 *  	columns: [
 * 			{
 * 				id: string,
 * 				name: string,
 * 				type: string,
 * 				min: number,
 * 				max: number,
 * 				canBeEmpty: boolean,
 * 				durationType: string
 *  		},
 * 			...
 * 		]
 * }
 *
 * @test Has unit tests.
 */
class Table {
	static #template = `
	<div class="tableWrapper">
		<div>
			<div class="header"></div>
			<div class="table">
				<div class="row header"></div>
				<div class="row new"></div>
			</div>
			<div class="buttons">
				<div class="clear"><span>Clear</span></div>
				<div class="revert"><span>Revert</span></div>
				<div class="save"><span>Save</span></div>
			</div>
		</div>
	</div>`;
	static #templateElement = null;

	m_newRowsReferences = [];

	constructor({ parent, id, metadata, isMutable, postSaveFunction })
	{
		this.m_parent = parent;
		this.m_id = id;
		this.m_metadata = metadata;
		this.m_isMutable = isMutable;
		this.m_data = null;
		this.m_postSaveFunction = postSaveFunction;

		if (!Table.#templateElement) {
			const template = document.createElement("template");
			template.innerHTML = Table.#template;
			Table.#templateElement = template;
		}

		const clone = Table.#templateElement.content.cloneNode(true);
		const tempContainer = document.createElement("div");
		tempContainer.appendChild(clone);
		tempContainer.querySelector(".tableWrapper > div > .header").innerHTML
			= this.m_metadata.name + " (" + this.m_id + ")";

		let rowHeader = tempContainer.querySelector(".row.header");
		let newRow = tempContainer.querySelector(".table > .new");

		for (let [columnId, columnMetadata] of Object.entries(this.m_metadata.columns)) {
			let divElement = document.createElement("div");
			let spanElement = document.createElement("span");
			if (columnMetadata.hasOwnProperty("name")) {
				spanElement.innerHTML = columnMetadata.name + " (" + columnId + ")" +
					" " + columnMetadata.type;
			}
			else {
				spanElement.innerHTML = "undefined (" + columnId + ")" +
					" " + columnMetadata.type;
			}

			divElement.appendChild(spanElement);
			rowHeader.appendChild(divElement);

			function setMin(inputElement, metadata, min)
			{
				if (metadata.min != undefined && metadata.min >= min) {
					inputElement.setAttribute("min", metadata.min.toString());
				}
				else {
					inputElement.setAttribute("min", min.toString());
				}
			}

			function setMax(inputElement, metadata, max)
			{
				if (metadata.max != undefined && metadata.max <= max) {
					inputElement.setAttribute("max", metadata.max.toString());
				}
				else {
					inputElement.setAttribute("max", max.toString());
				}
			}

			let inputElement = document.createElement("input");

			if (this.m_isMutable) {
				inputElement.addEventListener("input", () => { Helper.ValidateLimits(inputElement); });
			}
			else {
				inputElement.setAttribute("readonly", true);
			}

			if (columnMetadata.type == "Bool") {
				inputElement.setAttribute("type", "checkbox");
				let labelElement = document.createElement("label");
				labelElement.appendChild(inputElement);
				newRow.appendChild(document.createElement("div").appendChild(labelElement));
				continue;
			}

			if (columnMetadata.type == "Timer") {
				inputElement.setAttribute("canBeEmpty",
					columnMetadata.canBeEmpty == undefined ? "true" : columnMetadata.canBeEmpty.toString());
				Timer.Apply(inputElement);
			}
			else if (columnMetadata.type == "String") {
				inputElement.setAttribute("canBeEmpty",
					columnMetadata.canBeEmpty == undefined ? "true" : columnMetadata.canBeEmpty.toString());
				inputElement.setAttribute("type", "text");
			}
			else if (columnMetadata.type == "Duration") {
				inputElement.setAttribute("canBeEmpty",
					columnMetadata.canBeEmpty == undefined ? "true" : columnMetadata.canBeEmpty.toString());

				if (columnMetadata.hasOwnProperty("durationType")) {
					spanElement.innerHTML += ", " + columnMetadata.durationType;
					Duration.Apply(inputElement, columnMetadata.durationType);
				}
				else {
					spanElement.innerHTML += ", Nanoseconds";
					Duration.Apply(inputElement, "Nanoseconds");
				}

				setMin(inputElement, columnMetadata, Helper.TYPES_LIMITS.int_64.min);
				setMax(inputElement, columnMetadata, Helper.TYPES_LIMITS.int_64.max);
			}
			else {
				inputElement.setAttribute("canBeEmpty",
					columnMetadata.type.includes("Optional")
						? (columnMetadata.canBeEmpty == undefined ? "true" : columnMetadata.canBeEmpty.toString())
						: "false");

				const tableColumnHash = Helper.StringHash(id + "-" + columnId);
				if (MetadataCollector.IsSelect(tableColumnHash)) {
					inputElement.setAttribute("parameter-id", tableColumnHash);
					Select.Apply({ input : inputElement });
				}
				else {
					inputElement.setAttribute("type", "number");
				}

				if (columnMetadata.type.includes("Int8")) {
					inputElement.setAttribute("step", "1");
					setMin(inputElement, columnMetadata, Helper.TYPES_LIMITS.int_8.min);
					setMax(inputElement, columnMetadata, Helper.TYPES_LIMITS.int_8.max);
				}
				else if (columnMetadata.type.includes("Int16")) {
					inputElement.setAttribute("step", "1");
					setMin(inputElement, columnMetadata, Helper.TYPES_LIMITS.int_16.min);
					setMax(inputElement, columnMetadata, Helper.TYPES_LIMITS.int_16.max);
				}
				else if (columnMetadata.type.includes("Int32")) {
					inputElement.setAttribute("step", "1");
					setMin(inputElement, columnMetadata, Helper.TYPES_LIMITS.int_32.min);
					setMax(inputElement, columnMetadata, Helper.TYPES_LIMITS.int_32.max);
				}
				else if (columnMetadata.type.includes("Int64")) {
					inputElement.setAttribute("step", "1");
					setMin(inputElement, columnMetadata, Helper.TYPES_LIMITS.int_64.min);
					setMax(inputElement, columnMetadata, Helper.TYPES_LIMITS.int_64.max);
				}
				else if (columnMetadata.type.includes("Uint8")) {
					inputElement.setAttribute("step", "1");
					setMin(inputElement, columnMetadata, Helper.TYPES_LIMITS.uint_8.min);
					setMax(inputElement, columnMetadata, Helper.TYPES_LIMITS.uint_8.max);
				}
				else if (columnMetadata.type.includes("Float")) {
					inputElement.setAttribute("step", "0.01");
					setMin(inputElement, columnMetadata, Helper.TYPES_LIMITS.float_32.min);
					setMax(inputElement, columnMetadata, Helper.TYPES_LIMITS.float_32.max);
				}
				else if (columnMetadata.type.includes("Uint16")) {
					inputElement.setAttribute("step", "1");
					setMin(inputElement, columnMetadata, Helper.TYPES_LIMITS.uint_16.min);
					setMax(inputElement, columnMetadata, Helper.TYPES_LIMITS.uint_16.max);
				}
				else if (columnMetadata.type.includes("Uint32")) {
					inputElement.setAttribute("step", "1");
					setMin(inputElement, columnMetadata, Helper.TYPES_LIMITS.uint_32.min);
					setMax(inputElement, columnMetadata, Helper.TYPES_LIMITS.uint_32.max);
				}
				else if (columnMetadata.type.includes("Uint64")) {
					inputElement.setAttribute("step", "1");
					setMin(inputElement, columnMetadata, Helper.TYPES_LIMITS.uint_64.min);
					setMax(inputElement, columnMetadata, Helper.TYPES_LIMITS.uint_64.max);
				}
				else if (columnMetadata.type.includes("Double")) {
					inputElement.setAttribute("step", "0.01");
					setMin(inputElement, columnMetadata, Helper.TYPES_LIMITS.float_64.min);
					setMax(inputElement, columnMetadata, Helper.TYPES_LIMITS.float_64.max);
				}
				else {
					console.error("Unknown type: " + columnMetadata.type);
				}
			}

			newRow.appendChild(document.createElement("div").appendChild(inputElement));
		}

		rowHeader.appendChild(document.createElement("div"));
		this.m_wrapper = tempContainer.querySelector(".tableWrapper");
		if (this.m_isMutable) {
			let addNewRow = document.createElement("div");
			addNewRow.classList.add("add");
			newRow.appendChild(addNewRow);
			newRow.querySelector(".add").addEventListener("click", () => this.AddRow());

			this.m_wrapper.querySelector(".buttons > .revert").addEventListener("click", this.Revert.bind(this));
			this.m_wrapper.querySelector(".buttons > .save").addEventListener("click", this.Save.bind(this));
			this.m_wrapper.querySelector(".buttons > .clear").addEventListener("click", this.Clear.bind(this));
		}
		else {
			let addNewRow = document.createElement("div");
			newRow.appendChild(addNewRow);
			this.m_wrapper.querySelector(".buttons").remove();
			this.m_wrapper.classList.add("const");
		}

		this.m_wrapper.setAttribute("parameter-id", this.m_id);
		this.m_parent.appendChild(this.m_wrapper);
	}

	AddRow(values)
	{
		let newRow = this.m_wrapper.querySelector(".table > .new");
		let inputs = newRow.querySelectorAll("input");
		if (values) {
			if (values.length != inputs.length) {
				console.error("Values length does not match inputs length. values: " + values.length
					+ ", inputs: " + inputs.length);
			}

			for (let index = 0; index < values.length; index++) {
				if (inputs[index].type == "checkbox") {
					inputs[index].checked = values[index];
					continue;
				}

				if (inputs[index].hasAttribute("timestamp")) {
					Timer.SetValue(inputs[index], BigInt(values[index]));
					continue;
				}

				if (inputs[index].hasAttribute("nanoseconds")) {
					Duration.SetValue(inputs[index], BigInt(values[index]));
					continue;
				}

				if (MetadataCollector.IsSelect(inputs[index].getAttribute("parameter-id"))) {
					Select.SetValue(inputs[index], values[index]);
					continue;
				}

				if (Helper.IsFloat(values[index])) {
					inputs[index].value = Helper.FloatToString(values[index]);
					continue;
				}

				inputs[index].value = values[index];
			}
		}

		inputs.forEach(input => { Helper.ValidateLimits(input); });

		if (!values && Array.from(inputs).some(input => input.classList.contains("invalid"))) {
			return;
		}

		let newRowCopy = newRow.cloneNode(true);
		let inputsCopy = newRowCopy.querySelectorAll("input");
		for (let index = 0; index < inputs.length; index++) {
			if (inputs[index].type == "checkbox") {
				inputsCopy[index].checked = inputs[index].checked;
				inputs[index].checked = false;
				continue;
			}

			if (this.m_isMutable) {
				if (inputsCopy[index].hasAttribute("nanoseconds")) {
					Duration.SetEvent(inputsCopy[index]);
					inputs[index].setAttribute("nanoseconds", "");
				}
				else if (inputsCopy[index].hasAttribute("timestamp")) {
					Timer.SetEvent(inputsCopy[index]);
					inputs[index].setAttribute("timestamp", "");
				}
				else if (inputsCopy[index].hasAttribute("select")) {
					Select.SetEvent(inputsCopy[index]);
					inputs[index].setAttribute("select", "");
				}
			}

			inputsCopy[index].value = inputs[index].value;
			inputs[index].classList.remove("invalid");
			inputs[index].value = "";
		}

		let table = this.m_wrapper.querySelector(".table");
		table.insertBefore(newRowCopy, newRow);

		newRowCopy.classList.remove("new");
		if (!this.m_isMutable) {
			return;
		}

		newRowCopy.classList.add("added");
		newRowCopy.querySelector(".add").remove();
		let remove = document.createElement("div");
		remove.classList.add("remove");
		newRowCopy.appendChild(remove);

		remove.addEventListener("click", () => {
			if (this.m_newRowsReferences.includes(newRowCopy)) {
				this.m_newRowsReferences = this.m_newRowsReferences.filter(row => row != newRowCopy);

				newRowCopy.remove();

				if (this.m_newRowsReferences.length == 0
					&& this.m_wrapper.querySelectorAll(".table > .added").length == 0
					&& this.m_wrapper.querySelectorAll(".table > .removed").length == 0
					&& this.m_wrapper.querySelectorAll(".table > .row input.changed").length == 0) {

					this.m_wrapper.classList.remove("changed");
				}
				return;
			}

			newRowCopy.classList.add("removed");
			this.m_wrapper.classList.add("changed");
		});

		newRowCopy.querySelectorAll("input").forEach(input => {
			if (input.hasAttribute("select")) {
				if (!this.m_mutationObserver) {
					this.m_mutationObserver = new MutationObserver((mutationsList) => {
						for (const mutation of mutationsList) {
							if (mutation.type != "attributes" || mutation.attributeName != "select") {
								continue;
							}

							if (mutation.target.value == mutation.target.getAttribute("backup")) {
								mutation.target.classList.remove("changed");
								if (this.m_wrapper.querySelectorAll(".table > .added").length == 0
									&& this.m_wrapper.querySelectorAll(".table > .removed").length == 0
									&& this.m_wrapper.querySelectorAll(".table > .row input.changed").length == 0) {

									this.m_wrapper.classList.remove("changed");
								}
							}
							else {
								mutation.target.classList.add("changed");
								this.m_wrapper.classList.add("changed");
							}

							Helper.ValidateLimits(mutation.target);
						}
					});
				}

				this.m_mutationObserver.observe(input, { attributes : true })
			}
			else {
				input.addEventListener("change", () => {
					if ((input.type == "checkbox" && input.checked == (input.getAttribute("backup") == "true"))
						|| input.value == input.getAttribute("backup")) {

						input.classList.remove("changed");
						if (this.m_wrapper.querySelectorAll(".table > .added").length == 0
							&& this.m_wrapper.querySelectorAll(".table > .removed").length == 0
							&& this.m_wrapper.querySelectorAll(".table > .row input.changed").length == 0) {

							this.m_wrapper.classList.remove("changed");
						}
					}
					else {
						input.classList.add("changed");
						this.m_wrapper.classList.add("changed");
					}

					Helper.ValidateLimits(input);
				});
			}
		});

		this.m_newRowsReferences.push(newRowCopy);
		this.m_wrapper.classList.add("changed");
	}

	RemoveRow({ index })
	{
		if (this.m_isMutable) {
			return;
		}

		let rows = this.m_wrapper.querySelectorAll(".table > .row:not(.header):not(.new)");
		if (rows.length < index) {
			return;
		}

		rows[index - 1].remove();
	}

	Revert()
	{
		if (!this.m_wrapper.classList.contains("changed")) {
			return;
		}

		this.m_newRowsReferences.forEach(row => { row.remove(); });
		this.m_newRowsReferences = [];

		let removed = this.m_wrapper.querySelectorAll(".table > .removed");
		if (removed.length != 0) {
			removed.forEach(row => { row.classList.remove("removed"); });
		}

		let changed = this.m_wrapper.querySelectorAll(".table > .row input.changed");
		if (changed.length != 0) {
			changed.forEach(input => {
				if (input.type == "checkbox") {
					input.checked = input.getAttribute("backup") == "true";
				}
				else {
					input.value = input.getAttribute("backup");
				}
				input.classList.remove("changed", "invalid");
			});
		}

		this.m_wrapper.classList.remove("changed");
	}

	IsChanged() { return this.m_wrapper.classList.contains("changed"); }

	Save()
	{
		if (!this.m_wrapper.classList.contains("changed")) {
			return;
		}

		this.m_data = null;

		let allRows = this.m_wrapper.querySelectorAll(".table > .row:not(.header):not(.new)");
		if (allRows.length != 0
			&& Array.from(allRows).some(
				row => Array.from(row.querySelectorAll("input")).some(input => input.classList.contains("invalid")))) {

			if (this.m_postSaveFunction) {
				this.m_postSaveFunction();
			}
			return;
		}

		let changed = this.m_wrapper.querySelectorAll(".table > .row input.changed");
		if (changed.length != 0) {
			changed.forEach(input => {
				if (input.type == "checkbox") {
					input.setAttribute("backup", input.checked);
				}
				else {
					input.setAttribute("backup", input.value);
				}
				input.classList.remove("changed");
			});
		}

		let added = this.m_wrapper.querySelectorAll(".table > .added");
		if (added.length != 0) {
			added.forEach(row => {
				row.classList.remove("added");
				row.querySelectorAll("input").forEach(input => {
					input.classList.remove("changed");
					if (input.type == "checkbox") {
						input.setAttribute("backup", input.checked);
					}
					else {
						input.setAttribute("backup", input.value);
					}
				});
			});
		}

		let removed = this.m_wrapper.querySelectorAll(".table > .removed");
		if (removed.length != 0) {
			removed.forEach(row => { row.remove(); });
		}

		this.m_newRowsReferences = [];
		this.m_wrapper.classList.remove("changed");
		if (this.m_postSaveFunction) {
			this.m_postSaveFunction();
		}
	}

	Clear()
	{
		let rows = this.m_wrapper.querySelectorAll(".table > .row.added");
		rows.forEach(row => { row.remove(); });

		rows = this.m_wrapper.querySelectorAll(".table > .row.new");
		rows.forEach(row => {
			row.querySelectorAll("input").forEach(input => {
				if (input.type == "checkbox") {
					input.checked = false;
				}
				else {
					input.value = "";
				}
				input.classList.remove("invalid");
			});
		});

		rows = this.m_wrapper.querySelectorAll(".table > .row:not(.header):not(.new)");
		rows.forEach(row => { row.classList.add("removed"); });

		if (rows.length != 0) {
			this.m_wrapper.classList.add("changed");
		}
		else {
			this.m_wrapper.classList.remove("changed");
		}
	}

	Destructor()
	{
		this.m_wrapper.remove();
		if (this.m_mutationObserver) {
			this.m_mutationObserver.disconnect();
			this.m_mutationObserver = null;
		}
	}

	GetData()
	{
		if (this.m_data) {
			return this.m_data;
		}

		this.m_data = [];

		const rows = this.m_wrapper.querySelectorAll(".table > .row:not(.header):not(.new)");

		rows.forEach(row => {
			let data = [];
			row.querySelectorAll("input").forEach(input => {
				if (input.type == "checkbox") {
					data.push(input.checked);
				}
				else if (input.type == "number") {
					if (input.value.includes(".")) {
						data.push(+input.value);
					}
					else if (input.value == "") {
						data.push(null);
					}
					else {
						const columnType = this.m_metadata.columns[1].type;
						if (columnType.includes("Double") || columnType.includes("Float")) {
							data.push(+input.value);
						}
						else {
							data.push(BigInt(input.value));
						}
					}
				}
				else if (input.hasAttribute("timestamp")) {
					data.push(BigInt(input.getAttribute("timestamp")));
				}
				else if (input.hasAttribute("select")) {
					data.push(BigInt(input.getAttribute("select")));
				}
				else if (input.hasAttribute("durationType")) {
					data.push(BigInt(input.getAttribute("nanoseconds")));
				}
				else {
					data.push(input.value);
				}
			});

			this.m_data.push(data);
		});

		return this.m_data;
	}
}

if (typeof module !== "undefined" && typeof module.exports !== "undefined") {
	module.exports = Table;
	Helper = require("./helper");
	Timer = require("./timer");
	Select = require("./select");
	Duration = require("./duration");
	MetadataCollector = require("./metadataCollector");
}