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
 *
 * @brief View to display settings of MSAPI application. Only one instance per application port can be created.
 */

if (typeof module !== "undefined" && typeof module.exports !== "undefined") {
	View = require("../view");
	MetadataCollector = require("../views/metadataCollector");
}

class ModifyApp extends View {
	constructor(parameters) { super("ModifyApp", parameters); }

	Constructor(parameters)
	{
		if (View.ShowExistedView(this)) {
			return false;
		}

		this.m_appType = parameters.appType;
		this.m_viewPortParameter = parameters.viewPortParameter;

		let metadata = MetadataCollector.GetAppMetadata(this.m_appType);
		if (!metadata) {
			console.error("Can't get metadata for application", this.m_appType);
			return false;
		}

		let inputs = this.m_view.querySelector(".inputs");
		if (!inputs) {
			console.error("No inputs to fill app parameters");
			return false;
		}

		function parseParameter(parameterId, parameter, parent, mutable, saveThis)
		{
			if (parameter.type == "TableData") {
				let tables = parent.querySelector(".tables");
				if (!tables) {
					tables = document.createElement("div");
					tables.classList.add("tables");
					parent.appendChild(tables);
				}

				saveThis.m_tables.set(+parameterId,
					new Table({ parent : tables, id : parameterId, metadata : parameter, isMutable : mutable }));
				return;
			}

			function getGeneral()
			{
				let general = parent.querySelector(".general");
				if (!general) {
					general = document.createElement("div");
					general.classList.add("general", "items", "horizontal");
					let table = parent.querySelector(".tables");
					if (table) {
						parent.insertBefore(general, table);
					}
					else {
						parent.appendChild(general);
					}
				}
				return general;
			}

			let divElement = document.createElement("div");
			divElement.classList.add("item");
			let inputElement = document.createElement("input");
			inputElement.setAttribute("parameter-id", parameterId);

			if (parameter.type == "Bool") {
				inputElement.setAttribute("type", "checkbox");
				let labelElement = document.createElement("label");
				let spanElement = document.createElement("span");
				spanElement.innerHTML = parameter.name + " (" + parameterId + ") " + parameter.type;
				labelElement.appendChild(inputElement);
				divElement.appendChild(spanElement);
				divElement.appendChild(labelElement);
				getGeneral().appendChild(divElement);

				if (!mutable) {
					inputElement.setAttribute("disabled", true);
				}

				return;
			}

			if (parameter.type == "Timer") {
				Timer.Apply(inputElement);
			}
			else if (parameter.type == "String") {
				inputElement.setAttribute("type", "text");
			}
			else if (parameter.type == "Duration") {
				Duration.Apply(inputElement, parameter.durationType);
			}
			else if (MetadataCollector.IsSelect(parameterId)) {
				Select.Apply({ input : inputElement });
			}
			else {
				inputElement.setAttribute("type", "number");
			}

			let spanElement = document.createElement("span");
			spanElement.innerHTML = parameter.name + " (" + parameterId + ") "
				+ parameter.type + (parameter.type == "Duration" ? ", " + parameter.durationType : "");
			divElement.appendChild(spanElement);

			if (mutable) {
				if (parameter.hasOwnProperty("min") && parameter.min != "null") {
					inputElement.setAttribute("min", parameter.min);
				}
				if (parameter.hasOwnProperty("max") && parameter.max != "null") {
					inputElement.setAttribute("max", parameter.max);
				}
				if (parameter.hasOwnProperty("canBeEmpty")) {
					inputElement.setAttribute("canBeEmpty", parameter.canBeEmpty);
				}
				else {
					inputElement.setAttribute("canBeEmpty", false);
				}

				if (parameter.type != "Duration") {
					inputElement.addEventListener("input", () => { Helper.ValidateLimits(inputElement); });
				}
			}
			else {
				inputElement.setAttribute("readonly", true);
				inputElement.setAttribute("disabled", true);
			}

			divElement.appendChild(inputElement);
			getGeneral().appendChild(divElement);
		}

		function getContainerContent(isMutable)
		{
			let container = isMutable ? inputs.querySelector(".mutable") : inputs.querySelector(".const");
			if (!container) {
				container = document.createElement("div");
				container.classList.add(isMutable ? "mutable" : "const", "collection");
				let containerHeader = document.createElement("div");
				containerHeader.classList.add("header");
				let containerHeaderSpan = document.createElement("span");
				containerHeaderSpan.innerHTML = isMutable ? "Mutable" : "Const";
				containerHeader.appendChild(containerHeaderSpan);
				container.appendChild(containerHeader);
				let containerContent = document.createElement("div");
				containerContent.classList.add("content");
				container.appendChild(containerContent);
				inputs.appendChild(container);
			}
			return container.querySelector("div.content");
		}

		if (metadata.hasOwnProperty("mutable")) {
			for (let [parameterId, parameter] of Object.entries(metadata.mutable)) {
				parseParameter(parameterId, parameter, getContainerContent(true), true, this);
			}
		}
		if (metadata.hasOwnProperty("const")) {
			for (let [parameterId, parameter] of Object.entries(metadata.const)) {
				parseParameter(parameterId, parameter, getContainerContent(false), false, this);
			}
		}

		const containerWithInputs = getContainerContent(true).querySelector(".general");

		function displayParameters(saveThis, parameters)
		{
			for (let [key, value] of Object.entries(parameters)) {
				let parameter = false;
				if (metadata.hasOwnProperty("mutable")) {
					parameter = metadata.mutable.hasOwnProperty(key) ? metadata.mutable[key] : false;
				}
				if (!parameter && metadata.hasOwnProperty("const")) {
					parameter = metadata.const.hasOwnProperty(key) ? metadata.const[key] : false;
				}

				if (!parameter) {
					console.error("Parameter is not found: ", key);
					continue;
				}

				let input = inputs.querySelector("[parameter-id=\"" + key + "\"]");
				if (!input) {
					console.error(`Input not found: ${key}`);
					continue;
				}

				if (parameter.type == "TableData") {
					if ("Rows" in value) {
						let table = saveThis.m_tables.get(+key);
						if (!table) {
							console.error("Table id not found in view:", key);
							continue;
						}
						table.Clear();
						for (let row of value.Rows) {
							table.AddRow(row);
						}
						table.Save();
					}
				}
				else if (parameter.type == "Bool") {
					input.checked = value;
				}
				else if (parameter.type == "Duration") {
					Duration.SetValue(input, value);
				}
				else if (parameter.type == "Timer") {
					Timer.SetValue(input, BigInt(value));
				}
				else if (input.hasAttribute("select")) {
					Select.SetValue(input, value);
				}
				else if (Helper.IsFloat(value)) {
					input.value = Helper.FloatToString(value);
				}
				else {
					input.value = value;
				}
			}

			View.ValidateInputs(containerWithInputs);
		}

		let isTitleUpdated = false;
		let stream = new WebSocketStream({
			event : Helper.StringHash32Uint("parameters"),
			handleData : (parameters) => {
				let name = parameters["2000001"];
				if (!isTitleUpdated && name != undefined) {
					if (name) {
						this.m_title += ": " + name + " on port " + this.m_port;
					}
					else {
						this.m_title += ": " + this.m_appType + " on port " + this.m_port;
					}
					isTitleUpdated = true;
					this.m_parentView.querySelector(".title > span").textContent = this.m_title;
				}

				displayParameters(this, parameters);
			},
			handleFailed : (error) => { this.DisplayErrorMessage(error); },
			viewUid : this.m_uid,
			data : { "port" : this.m_port, "filter" : this.m_port }
		});

		this.m_view.querySelector(".button").addEventListener("click", async () => {
			this.HideErrorMessage();
			if (!View.ValidateInputs(containerWithInputs)) {
				return;
			}

			for (let table of this.m_tables.values()) {
				if (table.IsChanged()) {
					return;
				}
			}

			this.m_parentView.classList.add("loading");
			let newParameters
				= View.ParseInputs(containerWithInputs, false, MetadataCollector.GetAppMetadata(this.m_appType));
			for (const table of this.m_tables.values()) {
				newParameters[table.m_id] = table.GetData();
			}
			new WebSocketSingle({
				event : Helper.StringHash32Uint("modify"),
				handleResponse : (response) => { this.m_parentView.classList.remove("loading"); },
				handleFailed : (error) => {
					this.m_parentView.classList.remove("loading");
					this.DisplayErrorMessage(error);
				},
				viewUid : this.m_uid,
				data : { "parameters" : newParameters, "port" : this.m_port, "filter" : this.m_port }
			});
		});

		return true;
	}
}

View.AddViewTemplate("ModifyApp", `<div class="customView">
    <div class="inputs"></div>
    <div class="button">Modify</div>
</div>`);

if (typeof module !== "undefined" && typeof module.exports !== "undefined") {
	module.exports = ModifyApp;
}