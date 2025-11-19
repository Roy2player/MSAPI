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
 * @brief Template registrations for different view types.
 * This file contains all the HTML templates and parameter templates used by various views.
 */

// Import View class if in Node.js environment
if (typeof module !== 'undefined' && typeof module.exports !== 'undefined') {
	View = require("./application");
}

// Parameter templates
View.AddParametersTemplate("NewApp", "default", [
	{ "inputs" : [ { "key" : "ip", "value" : 0 } ] }, { "selects" : [ { "key" : "LogLevel", "value" : 5 } ] },
	{ "checkboxes" : [ { "key" : "lofInFile", "value" : true }, { "key" : "separateDaysLogging", "value" : true } ] }
]);

// View templates
View.AddViewTemplate("AppView", `<div class="appView"></div>`);
View.AddViewTemplate("InstalledApps", `<div></div>`);
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
View.AddViewTemplate("ModifyApp", `<div class="customView">
        <div class="inputs"></div>
        <div class="button">Modify</div>
    </div>`);
View.AddViewTemplate("CreatedApps", `<div></div>`);
View.AddViewTemplate("TableView", `<div class="tableView"></div>`);
View.AddViewTemplate("GridSettingsView", `<div class="gridSettingsView customView">
		<div class="group">
			<span class="text">Align:</span>
			<span class="action alignLeft"></span>
			<span class="action alignCenter"></span>
			<span class="action alignRight"></span>
		</div>
		<div class="group">
			<span class="text">Sorting:</span>
			<span class="action ascending"></span>
			<span class="action none"></span>
			<span class="action descending"></span>
		</div>
		<div class="group">
			<span class="text">Filter:</span>
			<span class="action filter"></span>
		</div>
		<div class="filters"></div>
	</div>`);
View.AddViewTemplate("SelectView", `<div class="selectView customView">
		<div class="search">
			<div class="searchIco"></div>
			<input type="text" placeholder="Search">
			<div class="caseSensitive"></div>
		</div>
		<div class="options"></div>
	</div>`);

if (typeof module !== 'undefined' && typeof module.exports !== 'undefined') {
	module.exports = View;
}
