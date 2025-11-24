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
 * @brief View to display settings of MSAPI application.
 */

if (module && module.exports) {
	View = require('../view');
}

class ModifyApp extends View {
	constructor(parameters) { super("ModifyApp", parameters); }

	async Constructor(parameters)
	{
		let existedApp = View.GetViewByPort(parameters.port);
		if (existedApp) {
			existedApp.Show();
			return false;
		}

		this.m_appType = parameters.appType;
		this.m_viewPortParameter = parameters.viewPortParameter;

		let metadata = View.GetMetadata(this.m_appType);
		if (!metadata) {
			let result = await View.SendRequest({
				method : "GET",
				mode : "cors",
				headers : { "Accept" : "application/json", "Type" : "getMetadata", "AppType" : this.m_appType }
			});
			if (!("status" in result) || !result.status) {
				console.error("Can't get metadata for application", this.m_appType);
				return false;
			}
		}
		metadata = View.GetMetadata(this.m_appType);

		this.m_port = parameters.port;
		let result = await View.SendRequest({
			method : "GET",
			mode : "cors",
			headers : { "Accept" : "application/json", "Type" : "getParameters", "Port" : this.m_port }
		});
		if (!("status" in result) || !result.status) {
			console.error("Can't get parameters for application", this.m_appType);
			return false;
		}

		let inputs = this.m_view.querySelector(".inputs");

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

		const parametersToPort = View.GetParameters(this.m_port);
		if (parametersToPort) {
			displayParameters(this, parametersToPort);

			parametersToPort[5] = this.m_appType;
			parametersToPort[10] = this.m_viewPortParameter;
		}
		if (parametersToPort && parametersToPort.hasOwnProperty(2000001)) {
			this.m_title += ": " + parametersToPort[2000001];
		}
		else {
			this.m_title += ": " + this.m_appType;
		}
		this.m_parentView.querySelector(".title > span").textContent = this.m_title;

		this.AddCallback("getParameters", (response, extraParameters) => {
			if ("parameters" in response && "port" in extraParameters && extraParameters.port == this.m_port) {
				displayParameters(this);
			}
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
			let headers = { "Accept" : "application/json", "Type" : "modify", "Port" : this.m_port };
			let newParameters = View.ParseInputs(containerWithInputs, false, View.GetMetadata(this.m_appType));

			for (const table of this.m_tables.values()) {
				newParameters[table.m_id] = table.GetData();
			}

			headers["Parameters"] = Helper.ParametersToJson(newParameters);

			let result = await View.SendRequest({ method : "GET", mode : "cors", headers });
			if ("status" in result) {
				this.m_parentView.classList.remove("loading");
				if (!result.status) {
					this.DisplayErrorMessage(result.message);
				}

				View.SendRequest({
					method : "GET",
					mode : "cors",
					headers : { "Accept" : "application/json", "Type" : "getParameters", "Port" : this.m_port }
				});
			}
			else {
				this.DisplayErrorMessage("Error: No status in response.");
			}
		});

		return true;
	}
}

View.AddViewTemplate("ModifyApp", `<div class="customView">
    <div class="inputs"></div>
    <div class="button">Modify</div>
</div>`);

if (module && module.exports) {
	module.exports = ModifyApp;
}