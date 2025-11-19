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
 */

const { TestRunner } = require('./testRunner');
const View = require('../application');
const viewTemplates = require('../viewTemplates');
const Table = require('../table');
const Grid = require('../grid');

global.Grid = Grid;
global.Table = Table;
global.dispatcher = undefined;

MetadataCollector.AddMetadata(1, { name : "Create", type : "system" }, true);
MetadataCollector.AddMetadata(2, { name : "Change state", type : "system" }, true);
MetadataCollector.AddMetadata(3, { name : "Modify", type : "system" }, true);
MetadataCollector.AddMetadata(4, { name : "Delete", type : "system" }, true);
MetadataCollector.AddMetadata(5, { name : "Type", type : "String" }, true);
MetadataCollector.AddMetadata(6, {
	name : "Table template with boolean operator",
	type : "TableData",
	id : 6,
	columns : [ { name : "Boolean operator", type : "Int8", stringInterpretation : { 0 : "Equal" } } ]
},
	true);
MetadataCollector.AddMetadata(7, {
	name : "Table template with number operator",
	type : "TableData",
	id : 7,
	columns : [ {
		name : "Number operator",
		type : "Int8",
		stringInterpretation : {
			0 : "Equal",
			1 : "Not equal",
			2 : "Less than",
			3 : "Greater than",
			4 : "Less than or equal to",
			5 : "Greater than or equal to"
		}
	} ]
},
	true);
MetadataCollector.AddMetadata(8, {
	name : "Table template with string operator",
	type : "TableData",
	id : 8,
	columns : [ {
		name : "String operator",
		type : "Int8",
		stringInterpretation : {
			0 : "Equal case sensitive",
			1 : "Equal case insensitive",
			2 : "Not equal case sensitive",
			3 : "Not equal case insensitive",
			4 : "Contains case sensitive",
			5 : "Contains case insensitive"
		}
	} ]
},
	true);
MetadataCollector.AddMetadata(9, {
	name : "Table template with optional number operator",
	type : "TableData",
	id : 9,
	columns : [ {
		name : "Optional number operator",
		type : "Int8",
		stringInterpretation : {
			0 : "Equal",
			1 : "Not equal",
			2 : "Less than",
			3 : "Greater than",
			4 : "Less than or equal to",
			5 : "Greater than or equal to"
		}
	} ]
},
	true);

function DestroyViews()
{
	document.querySelectorAll('body > main > section.views .viewHeader .close')
		.forEach((button) => button.dispatchEvent(new Event('click', { bubbles : true })));
	testRunner.Assert(View.GetCreatedApplications().size, 0, 'Unexpected created applications count');
}

let testRunner = new TestRunner();
testRunner.SetPostTestFunction(DestroyViews);

testRunner.Test('Create InstalledApps panel', async () => {
	const app = new View("InstalledApps");

	testRunner.Assert(app.m_viewType, 'InstalledApps', 'View type is unexpected');
	testRunner.Assert(app.m_title, 'InstalledApps', 'Title is unexpected');
	testRunner.Assert(app.m_appType, undefined, 'App type is unexpected');
	testRunner.Assert(app.m_parameters, null, 'Parameters are unexpected');
	testRunner.Assert(app.m_parentNode, body.querySelector('main > section.views'), 'Parent node is unexpected');
	testRunner.Assert(
		app.m_parentView, body.querySelector('main > section.views > .view'), 'Parent view node is unexpected');

	testRunner.Assert(View.GetViewTemplate('InstalledApps') !== null, true, 'View template is unexpected');
	testRunner.Assert(app.m_grid !== null, true, 'Grid is not created');

	await TestRunner.WaitFor(() => app.m_grid.m_view.children.length == 5, "Installed apps are loaded");
	testRunner.Assert(app.m_grid.m_view.childNodes.length, 5, 'Grid has unexpected number of children');
	testRunner.Assert(app.m_grid.m_columnById.size, 2, 'Columns size is unexpected');
	testRunner.Assert(app.m_grid.m_columnByOrder.size, 2, 'Columns size is unexpected');
	testRunner.Assert(app.m_grid.m_columnByOrder.get(0).headerCell.querySelector("span.text").innerHTML, 'Create',
		'First column name is unexpected');
	testRunner.Assert(app.m_grid.m_columnByOrder.get(1).headerCell.querySelector("span.text").innerHTML, 'Type',
		'Second column name is unexpected');
});

testRunner.Test('Create NewApp and Modify panels', () => {
	testRunner.Assert(View.GetParametersTemplate('NewApp', "default") !== null, true,
		'Default parameters template is unexpected');

	let parameters = { appType : "Strategy" };
	const app = new View("NewApp", parameters);

	testRunner.Assert(app.m_viewType, 'NewApp', 'View type is unexpected');
	testRunner.Assert(app.m_title, 'NewApp: Strategy', 'Title is unexpected');
	testRunner.Assert(app.m_appType, "Strategy", 'Alias is unexpected');
	testRunner.Assert(app.m_parameters, parameters, 'Parameters are unexpected');
	testRunner.Assert(app.m_parentNode, body.querySelector('main > section.views'), 'Parent node is unexpected');
	testRunner.Assert(
		app.m_parentView, body.querySelector('main > section.views > .view'), 'Parent view node is unexpected');

	testRunner.Assert(View.GetViewTemplate('NewApp') !== null, true, 'View template is unexpected');
});

testRunner.Test('Add app to CreatedApps panel', () => {
	const app = new View("CreatedApps");

	testRunner.Assert(app.m_viewType, 'CreatedApps', 'View type is unexpected');
	testRunner.Assert(app.m_title, 'CreatedApps', 'Title is unexpected');
	testRunner.Assert(app.m_appType, undefined, 'App type is unexpected');
	testRunner.Assert(app.m_parameters, null, 'Parameters are unexpected');
	testRunner.Assert(app.m_parentNode, body.querySelector('main > section.views'), 'Parent node is unexpected');
	testRunner.Assert(
		app.m_parentView, body.querySelector('main > section.views > .view'), 'Parent view node is unexpected');

	testRunner.Assert(View.GetViewTemplate('CreatedApps') !== null, true, 'View template is unexpected');

	testRunner.Assert(app.m_grid !== null, true, 'Grid not created');
	testRunner.Assert(app.m_grid.m_view.childNodes.length, 1, 'Grid has unexpected number of children');
	testRunner.Assert(app.m_grid.m_columnById.size, 7, 'Columns size is unexpected');
	testRunner.Assert(app.m_grid.m_columnByOrder.size, 7, 'Columns size is unexpected');
	testRunner.Assert(app.m_grid.m_columnByOrder.get(0).headerCell.querySelector("span.text").innerHTML, 'Change state',
		'Column name is unexpected');
	testRunner.Assert(app.m_grid.m_columnByOrder.get(1).headerCell.querySelector("span.text").innerHTML, 'Modify',
		'Column name is unexpected');
	testRunner.Assert(app.m_grid.m_columnByOrder.get(2).headerCell.querySelector("span.text").innerHTML, 'Delete',
		'Column name is unexpected');
	testRunner.Assert(app.m_grid.m_columnByOrder.get(3).headerCell.querySelector("span.text").innerHTML, 'Name',
		'Column name is unexpected');
	testRunner.Assert(app.m_grid.m_columnByOrder.get(4).headerCell.querySelector("span.text").innerHTML, 'Type',
		'Column name is unexpected');
	testRunner.Assert(app.m_grid.m_columnByOrder.get(5).headerCell.querySelector("span.text").innerHTML, 'Listening IP',
		'Column name is unexpected');
	testRunner.Assert(app.m_grid.m_columnByOrder.get(6).headerCell.querySelector("span.text").innerHTML,
		'Listening port', 'Column name is unexpected');
});

testRunner.Run();