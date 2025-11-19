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
const Dispatcher = require('../dispatcher');
const Grid = require('../grid');
const View = require('../application');
const viewTemplates = require('../viewTemplates');

global.Grid = Grid;
global.View = View;

let testRunner = new TestRunner();

function DestroyViews()
{
	document.querySelectorAll('body > main > section.views .viewHeader .close')
		.forEach((button) => button.dispatchEvent(new Event('click', { bubbles : true })));
	testRunner.Assert(View.GetCreatedApplications().size, 0, 'Unexpected created applications count');
}

function CheckHiddenViewsList(expectedSize)
{
	let hiddenViewsList = global.dispatcher.m_hiddenViews.querySelector('.hiddenViews > .list');
	testRunner.Assert(hiddenViewsList.childElementCount, expectedSize, 'Unexpected hidden views list length');
	if (expectedSize === 0) {
		testRunner.Assert(
			global.dispatcher.m_hiddenViews.classList.contains('visible'), false, 'Unexpected hidden views visibility');
	}
	else {
		testRunner.Assert(
			global.dispatcher.m_hiddenViews.classList.contains('visible'), true, 'Unexpected hidden views visibility');
	}
	testRunner.Assert(global.dispatcher.m_registeredPanels.querySelector('.list').childElementCount, 0,
		'Unexpected registered panels list length');
	testRunner.Assert(global.dispatcher.m_registeredPanels.classList.contains('visible'), false,
		'Unexpected registered panels visibility');
}

function CheckClassList(element, expectedClasses)
{
	const classListArray = Array.from(element.classList);
	testRunner.Assert(JSON.stringify(classListArray), JSON.stringify(expectedClasses), 'Unexpected class list');
}

testRunner.SetPostTestFunction(DestroyViews);
global.dispatcher = new Dispatcher();

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

testRunner.Test('Manage list of hidden views via dispatcher and directly via view objects\' callbacks', async () => {
	testRunner.Assert(
		global.dispatcher.m_parentNode, document.querySelector('main'), 'Dispatcher parent node not found');
	testRunner.Assert(
		global.dispatcher.m_view, document.querySelector('main > .dispatcher'), 'Dispatcher view not found');
	testRunner.Assert(global.dispatcher.m_view.classList.contains('hidden'), true, 'Unexpected dispatcher visibility');
	testRunner.Assert(global.dispatcher.m_control, document.querySelector('main > .dispatcher > .control'),
		'Dispatcher control not found');
	testRunner.Assert(global.dispatcher.m_hiddenViews, document.querySelector('main > .dispatcher > .hiddenViews'),
		'Dispatcher hidden views not found');
	testRunner.Assert(
		global.dispatcher.m_hiddenViews.classList.contains('visible'), false, 'Unexpected hidden views visibility');
	testRunner.Assert(global.dispatcher.m_registeredPanels,
		document.querySelector('main > .dispatcher > .registeredPanels'), 'Dispatcher registered panels not found');
	testRunner.Assert(global.dispatcher.m_registeredPanels.classList.contains('visible'), false,
		'Unexpected registered panels visibility');

	let view1 = new View('InstalledApps');
	await TestRunner.WaitFor(() => view1.m_created, 'view1 created');
	CheckClassList(view1.m_parentView, [ 'view' ]);

	let hiddenViewsList = global.dispatcher.m_hiddenViews.querySelector('.hiddenViews > .list');

	//* Add hidden view
	global.dispatcher.AddHiddenView(view1);
	CheckHiddenViewsList(1);
	testRunner.Assert(hiddenViewsList.childNodes[0].querySelector('span').innerHTML, 'InstalledApps',
		'Unexpected name on hidden view');
	CheckClassList(view1.m_parentView, [ 'view', 'hidden' ]);

	//* Click to show
	hiddenViewsList.firstElementChild.dispatchEvent(new Event('click', { bubbles : true }));
	CheckHiddenViewsList(0);
	CheckClassList(view1.m_parentView, [ 'view' ]);

	//* Add hidden view
	global.dispatcher.AddHiddenView(view1);
	CheckHiddenViewsList(1);
	CheckClassList(view1.m_parentView, [ 'view', 'hidden' ]);

	//* Try to add the same view again
	global.dispatcher.AddHiddenView(view1);
	CheckHiddenViewsList(1);
	CheckClassList(view1.m_parentView, [ 'view', 'hidden' ]);

	//* Add another hidden view
	let view2 = new View('InstalledApps');
	await TestRunner.WaitFor(() => view2.m_created, 'view2 created');
	CheckClassList(view2.m_parentView, [ 'view' ]);
	global.dispatcher.AddHiddenView(view2);
	CheckHiddenViewsList(2);
	CheckClassList(view2.m_parentView, [ 'view', 'hidden' ]);

	//* Add another hidden view
	let view3 = new View('CreatedApps');
	await TestRunner.WaitFor(() => view3.m_created, 'view3 created');
	CheckClassList(view3.m_parentView, [ 'view' ]);
	global.dispatcher.AddHiddenView(view3);
	CheckHiddenViewsList(3);
	testRunner.Assert(
		hiddenViewsList.childNodes[2].querySelector('span').innerHTML, 'CreatedApps', 'Unexpected name on hidden view');
	CheckClassList(view3.m_parentView, [ 'view', 'hidden' ]);

	//* Call show on app
	view1.Show();
	CheckHiddenViewsList(2);
	CheckClassList(view1.m_parentView, [ 'view' ]);

	//* Call maximize on app
	view2.Maximize();
	CheckHiddenViewsList(1);
	CheckClassList(view2.m_parentView, [ 'view', 'maximized' ]);

	//* Call hide on apps
	view1.Hide();
	view2.Hide();
	CheckHiddenViewsList(3);
	CheckClassList(view1.m_parentView, [ 'view', 'hidden' ]);
	CheckClassList(view2.m_parentView, [ 'view', 'maximized', 'hidden' ]);

	//* Click to show to all
	hiddenViewsList.childNodes[2].dispatchEvent(new Event('click', { bubbles : true }));
	hiddenViewsList.childNodes[1].dispatchEvent(new Event('click', { bubbles : true }));
	hiddenViewsList.childNodes[0].dispatchEvent(new Event('click', { bubbles : true }));
	CheckHiddenViewsList(0);
	CheckClassList(view1.m_parentView, [ 'view' ]);
	CheckClassList(view2.m_parentView, [ 'view', 'maximized' ]);
	CheckClassList(view3.m_parentView, [ 'view' ]);

	//* Add hidden views
	global.dispatcher.AddHiddenView(view1);
	global.dispatcher.AddHiddenView(view2);
	global.dispatcher.AddHiddenView(view3);
	CheckHiddenViewsList(3);
	CheckClassList(view1.m_parentView, [ 'view', 'hidden' ]);
	CheckClassList(view2.m_parentView, [ 'view', 'maximized', 'hidden' ]);
	CheckClassList(view3.m_parentView, [ 'view', 'hidden' ]);

	//* Remove hidden view
	global.dispatcher.RemoveHiddenView(view1);
	CheckHiddenViewsList(2);
	CheckClassList(view1.m_parentView, [ 'view' ]);

	//* Remove hidden views
	global.dispatcher.RemoveHiddenView(view2);
	global.dispatcher.RemoveHiddenView(view3);
	CheckHiddenViewsList(0);
	CheckClassList(view2.m_parentView, [ 'view', 'maximized' ]);
	CheckClassList(view3.m_parentView, [ 'view' ]);
});

testRunner.Test('Manage list of registered panels', async () => {
	testRunner.Assert(
		global.dispatcher.m_parentNode, document.querySelector('main'), 'Dispatcher parent node not found');
	testRunner.Assert(
		global.dispatcher.m_view, document.querySelector('main > .dispatcher'), 'Dispatcher view not found');
	testRunner.Assert(global.dispatcher.m_view.classList.contains('hidden'), true, 'Unexpected dispatcher visibility');
	testRunner.Assert(global.dispatcher.m_control, document.querySelector('main > .dispatcher > .control'),
		'Dispatcher control not found');
	testRunner.Assert(global.dispatcher.m_hiddenViews, document.querySelector('main > .dispatcher > .hiddenViews'),
		'Dispatcher hidden views not found');
	testRunner.Assert(
		global.dispatcher.m_hiddenViews.classList.contains('visible'), false, 'Unexpected hidden views visibility');
	testRunner.Assert(global.dispatcher.m_registeredPanels,
		document.querySelector('main > .dispatcher > .registeredPanels'), 'Dispatcher registered panels not found');
	testRunner.Assert(global.dispatcher.m_registeredPanels.classList.contains('visible'), false,
		'Unexpected registered panels visibility');

	let registeredPanelsList = global.dispatcher.m_registeredPanels.querySelector('.list');
	let hiddenViewsList = global.dispatcher.m_hiddenViews.querySelector('.list');
	let checkRegisteredPanelsList = (registered, hidden) => {
		testRunner.Assert(
			registeredPanelsList.childElementCount, registered, 'Unexpected registered panels list length');
		if (registered === 0) {
			testRunner.Assert(global.dispatcher.m_registeredPanels.classList.contains('visible'), false,
				'Unexpected registered panels visibility');
		}
		else {
			testRunner.Assert(global.dispatcher.m_registeredPanels.classList.contains('visible'), true,
				'Unexpected registered panels visibility');
		}

		testRunner.Assert(hiddenViewsList.childElementCount, hidden, 'Unexpected hidden views list length');
		if (hidden === 0) {
			testRunner.Assert(global.dispatcher.m_hiddenViews.classList.contains('visible'), false,
				'Unexpected hidden views visibility');
		}
		else {
			testRunner.Assert(global.dispatcher.m_hiddenViews.classList.contains('visible'), true,
				'Unexpected hidden views visibility');
		}
	};

	//* Register panel
	global.dispatcher.RegisterPanel('InstalledApps');
	checkRegisteredPanelsList(1, 0);
	testRunner.Assert(registeredPanelsList.childNodes[0].querySelector('span').innerHTML, 'InstalledApps',
		'Unexpected name on registered panel');

	//* Register panel
	global.dispatcher.RegisterPanel('InstalledApps');
	checkRegisteredPanelsList(1, 0);

	//* Register panel
	global.dispatcher.RegisterPanel('CreatedApps');
	checkRegisteredPanelsList(2, 0);

	//* Click to create panel
	registeredPanelsList.childNodes[0].dispatchEvent(new Event('click', { bubbles : true }));
	await TestRunner.WaitFor(() => View.GetCreatedApplications().size === 1, 'view is created');
	checkRegisteredPanelsList(2, 0);

	testRunner.Assert(View.GetCreatedApplications().size, 1, 'Unexpected created views count');
	testRunner.Assert(Array.from(View.GetCreatedApplications().values())[0].m_title, 'InstalledApps',
		'Unexpected name on created view');

	//* Click to create panel
	registeredPanelsList.childNodes[1].dispatchEvent(new Event('click', { bubbles : true }));
	await TestRunner.WaitFor(() => View.GetCreatedApplications().size === 2, 'view is created');
	checkRegisteredPanelsList(2, 0);
	testRunner.Assert(View.GetCreatedApplications().size, 2, 'Unexpected created views count');
	testRunner.Assert(Array.from(View.GetCreatedApplications().values())[1].m_title, 'CreatedApps',
		'Unexpected name on created view');

	//* Click to create panel
	registeredPanelsList.childNodes[1].dispatchEvent(new Event('click', { bubbles : true }));
	await TestRunner.WaitFor(() => View.GetCreatedApplications().size === 3, 'view is created');
	checkRegisteredPanelsList(2, 0);
	testRunner.Assert(View.GetCreatedApplications().size, 3, 'Unexpected created views count');
	testRunner.Assert(Array.from(View.GetCreatedApplications().values())[2].m_title, 'CreatedApps',
		'Unexpected name on created view');

	//* Hide views
	let views = Array.from(View.GetCreatedApplications().values());
	views[0].Hide();
	views[1].Hide();
	checkRegisteredPanelsList(2, 2);

	//* Destroy views
	DestroyViews();
	checkRegisteredPanelsList(2, 0);
});

testRunner.Test('Hide and show dispatcher', () => {
	testRunner.Assert(
		global.dispatcher.m_parentNode, document.querySelector('main'), 'Dispatcher parent node not found');
	testRunner.Assert(
		global.dispatcher.m_view, document.querySelector('main > .dispatcher'), 'Dispatcher view not found');
	testRunner.Assert(global.dispatcher.m_view.classList.contains('hidden'), true, 'Unexpected dispatcher visibility');
	testRunner.Assert(global.dispatcher.m_control, document.querySelector('main > .dispatcher > .control'),
		'Dispatcher control not found');
	testRunner.Assert(global.dispatcher.m_hiddenViews, document.querySelector('main > .dispatcher > .hiddenViews'),
		'Dispatcher hidden views not found');
	testRunner.Assert(
		!global.dispatcher.m_hiddenViews.classList.contains('visible'), true, 'Unexpected hidden views visibility');
	testRunner.Assert(global.dispatcher.m_registeredPanels,
		document.querySelector('main > .dispatcher > .registeredPanels'), 'Dispatcher registered panels not found');
	//* Was initialized in previous tests
	testRunner.Assert(global.dispatcher.m_registeredPanels.classList.contains('visible'), true,
		'Unexpected registered panels visibility');

	global.dispatcher.m_control.dispatchEvent(new Event('click'));
	testRunner.Assert(!global.dispatcher.m_view.classList.contains('hidden'), true, 'Unexpected dispatcher visibility');

	global.dispatcher.m_control.dispatchEvent(new Event('click'));
	testRunner.Assert(global.dispatcher.m_view.classList.contains('hidden'), true, 'Unexpected dispatcher visibility');
});

testRunner.Run();