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
 * @brief View to display pre-creation settings of MSAPI application.
 */

if (typeof module !== 'undefined' && typeof module.exports !== 'undefined') {
	View = require('../view');
}

class NewApp extends View {
	constructor(parameters) { super("NewApp", parameters); }

	async Constructor(parameters)
	{
		this.m_appType = parameters.appType;
		this.m_title += ": " + this.m_appType;
		this.m_parentView.querySelector(".title > span").textContent = this.m_title;

		this.m_view.querySelector(".button").addEventListener("click", async () => {
			this.HideErrorMessage();
			if (!View.ValidateInputs(this.m_view)) {
				return;
			}

			this.m_parentView.classList.add("loading");
			let headers = View.ParseInputs(this.m_view, true);
			headers["Accept"] = "application/json";
			headers["Type"] = "createApp";
			headers["AppType"] = this.m_appType;
			let result = await View.SendRequest({ method : "GET", mode : "cors", headers });
			if (result.status) {
				this.Destructor();
			}
			else {
				this.m_parentView.classList.remove("loading");
				this.DisplayErrorMessage(result.message);
			}
		});

		return true;
	}
}

View.AddParametersTemplate("NewApp", "default", [
	{ "inputs" : [ { "key" : "ip", "value" : 0 } ] }, { "selects" : [ { "key" : "LogLevel", "value" : 5 } ] },
	{ "checkboxes" : [ { "key" : "logInFile", "value" : true }, { "key" : "separateDaysLogging", "value" : true } ] }
]);

View.AddViewTemplate("NewApp", `<div class="customView">
        <div class="items vertical">
			<div class="item"><input name="name" type="text" placeholder="Name"/></div>
			<div class="item"><input value="127.0.0.1" name="ip" type="text" placeholder="Listening IP" canBeEmpty="false" /></div>
            <div class="item"><input name="port" type="number" placeholder="Listening Port" /></div>
            <div class="item">
				<label>
					<input name="logInConsole" type="checkbox" />
					<span>Log in console</span>
				</label>
			</div>
            <div class="item">
				<label>
					<input name="logInFile" type="checkbox" />
					<span>Log in file</span>
				</label>
			</div>
			<div class="item">
				<span class="title">Log level:</span>
				<select name="logLevel" canBeEmpty="false">
					<option value="1">ERROR</option>
					<option value="2">WARNING</option>
					<option value="3">INFO</option>
					<option value="4">DEBUG</option>
					<option value="5">PROTOCOL</option>
				</select>
			</div>
            <div class="item">
				<label>
					<input name="separateDaysLogging" type="checkbox" />
					<span>Separate days logging</span>
				</label>
			</div>
        </div>
        <div class="button">Create</div>
    </div>`);

if (typeof module !== 'undefined' && typeof module.exports !== 'undefined') {
	module.exports = NewApp;
}