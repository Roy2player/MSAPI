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
 * @brief Tests browser script loading and time-accuracy selection without CommonJS globals.
 */

const { readFileSync } = require('node:fs');
const path = require('node:path');
const { runInContext } = require('node:vm');
const { JSDOM } = require('jsdom');
const { TestRunner } = require('../test/testRunner');

const testRunner = new TestRunner();

/**************************
 * @brief Loads the page's scripts in a fresh browser realm without executing the inline server connection.
 *
 * @return Browser DOM with the registered frontend classes and initialized dispatcher.
 *
 * @test Yes.
 */
function CreateBrowser()
{
	const html = readFileSync(path.join(__dirname, '../../html/index.html'), 'utf8');
	const dom = new JSDOM(html, { url : 'http://localhost/', runScripts : 'outside-only' });
	try {
		const browser = dom.window;
		const context = dom.getInternalVMContext();
		for (const script of browser.document.querySelectorAll('script[src]')) {
			const source = readFileSync(path.join(__dirname, '..', script.getAttribute('src')), 'utf8');
			runInContext(source, context, { filename : script.getAttribute('src') });
		}
		browser.document.dispatchEvent(new browser.Event('DOMContentLoaded'));
		return dom;
	}
	catch (error) {
		dom.window.close();
		throw error;
	}
}

testRunner.Test('Load browser scripts in page order and select timer accuracy', () => {
	const dom = CreateBrowser();
	try {
		const browser = dom.window;
		const context = dom.getInternalVMContext();
		testRunner.Assert(runInContext('MetadataCollector.GetMetadata(11).metadata.name', context), 'Time accuracy');
		testRunner.Assert(
			runInContext('Object.keys(MetadataCollector.GetStringInterpretations(11)).length', context), 9);
		runInContext(`
			const timestamp = BigInt(new Date(2026, 10, 1, 12, 34, 56).getTime()) * 1000000n + 123456789n;
			const cell = document.createElement('div');
			const timerInput = document.createElement('input');
			Timer.Apply(timerInput);
			Timer.SetValue(timerInput, timestamp);
			cell.appendChild(timerInput);
			const columnObject = {
				id: 1,
				metadata: {metadata: {name: 'Timestamp', type: 'Timer'}},
				cells: new Map([[{values: {1: timestamp}}, cell]])
			};
			new GridSettingsView({columnObject, viewTitle: 'Accuracy settings'});
		`,
			context);
		const input = browser.document.querySelector('.group.accuracy input');
		testRunner.Assert(input != null, true);
		input.click();
		const option = Array.from(browser.document.querySelectorAll('.selectView .options > div'))
						   .find(node => node.firstElementChild.textContent === 'Milliseconds');
		testRunner.Assert(option != null, true);
		option.click();
		testRunner.Assert(input.value, 'Milliseconds');
		testRunner.Assert(runInContext('columnObject.accuracy === Timer.ACCURACY.Milliseconds', context), true);
		testRunner.Assert(runInContext("timerInput.value.startsWith('2026-11-01 12:34:56.123 UTC')", context), true);
		testRunner.Assert(runInContext("timerInput.getAttribute('timestamp') === timestamp.toString()", context), true);
		const errors = [];
		browser.console.error = (...args) => { errors.push(args); };
		input.setAttribute('select', '5');
		input.dispatchEvent(new browser.Event('change'));
		testRunner.Assert(errors.length, 1);
		testRunner.Assert(runInContext('columnObject.accuracy === Timer.ACCURACY.Milliseconds', context), true);
	}
	finally {
		dom.window.close();
	}
});

testRunner.Test('Apply Float and Double accuracy controls and restore invalid edits', () => {
	for (const type of ['Float', 'Double', 'OptionalFloat', 'OptionalDouble']) {
		const dom = CreateBrowser();
		try {
			const browser = dom.window;
			const context = dom.getInternalVMContext();
			runInContext(`
				const cell = document.createElement('div');
				const columnObject = {
					id: 1, accuracy: 0.01,
					metadata: {metadata: {name: 'Price', type: '${type}', accuracy: 0.01}},
					cells: new Map([[{values: {1: 1.23456}}, cell]])
				};
				let settings = new GridSettingsView({columnObject, viewTitle: 'Price accuracy'});
			`,
				context);
			const input = browser.document.querySelector('.group.accuracy input');
			testRunner.Assert(input.value, '0.01');
			const minimum = type.includes('Float') ? 1e-9 : 1e-17;
			testRunner.Assert(Number(input.min), minimum);
			for (const [value, expected] of [[ '1', '1' ], [ '0.1', '1.2' ], [ String(minimum), '1.23456' ]]) {
				input.value = value;
				input.dispatchEvent(new browser.Event('change'));
				testRunner.Assert(runInContext('columnObject.accuracy', context), Number(value));
				testRunner.Assert(runInContext('cell.textContent', context), expected);
				testRunner.Assert(
					input.checkValidity(), true, 'Accepted accuracy must satisfy native numeric validity');
			}
			for (const value of ['', '0', '-0.1', '2', '0.03', String(minimum / 10), 'NaN', 'Infinity']) {
				input.value = value;
				input.dispatchEvent(new browser.Event('change'));
				testRunner.Assert(Number(input.value), minimum, `Invalid ${type} accuracy ${value} must be restored`);
				testRunner.Assert(runInContext('columnObject.accuracy', context), minimum);
				testRunner.Assert(runInContext('cell.textContent', context), '1.23456');
			}
			runInContext(
				`settings.Destructor(); settings = new GridSettingsView({columnObject, viewTitle: 'Reopened'});`,
				context);
			testRunner.Assert(Number(browser.document.querySelector('.group.accuracy input').value), minimum);
			testRunner.Assert(runInContext('columnObject.metadata.metadata.accuracy', context), 0.01);
		}
		finally {
			dom.window.close();
		}
	}
});

testRunner.Test('Hide accuracy controls for unsupported types and preserve unspecified defaults', () => {
	const dom = CreateBrowser();
	try {
		const browser = dom.window;
		const context = dom.getInternalVMContext();
		runInContext(`
			const columnObject = {metadata: {metadata: {type: 'String'}}, cells: new Map()};
			let settings = new GridSettingsView({columnObject, viewTitle: 'No accuracy'});
		`,
			context);
		testRunner.Assert(browser.document.querySelector('.group.accuracy input'), null);
		runInContext(`
			settings.Destructor();
			columnObject.metadata.metadata.type = 'Double';
			settings = new GridSettingsView({columnObject, viewTitle: 'Default accuracy'});
		`,
			context);
		const input = browser.document.querySelector('.group.accuracy input');
		testRunner.Assert(input.value, '');
		testRunner.Assert(runInContext('columnObject.accuracy', context), undefined);
		input.value = 'invalid';
		input.dispatchEvent(new browser.Event('change'));
		testRunner.Assert(input.value, '');
		testRunner.Assert(runInContext('columnObject.accuracy', context), undefined);
	}
	finally {
		dom.window.close();
	}
});

testRunner.Test('Display descriptions and timer accuracy in Modify App while submitting full timestamps', () => {
	const dom = CreateBrowser();
	try {
		const browser = dom.window;
		const context = dom.getInternalVMContext();
		runInContext(`
			const sent = [];
			// Keep the real event registration and parsing; capture transport instead of opening a socket.
			WebSocketHandler.Send = json => sent.push(Helper.JsonStringToObject(json));
			MetadataCollector.AddAppMetadata('Presentation test', {
				mutable: {
					66001: {name: 'Timestamp', type: 'Timer', accuracy: Timer.ACCURACY.Microseconds, description: 'Full precision'},
					66002: {name: 'Plain', type: 'String', description: ''},
					66003: {name: 'Enabled', type: 'Bool', description: '<b>Switch</b>'},
					66006: {name: 'Prices', type: 'TableData', description: 'Table details', columns: {
						0: {name: 'Price', type: 'Double', accuracy: 0.1, description: 'Unrounded source'}
					}}
				},
				const: {
					66004: {name: 'Fixed label', type: 'String', description: 'Constant details'},
					66005: {name: 'Date', type: 'Timer', accuracy: Timer.ACCURACY.Days, description: 'Day display'}
				}
			});
			const timestamp = BigInt(new Date(2026, 10, 1, 12, 34, 56).getTime()) * 1000000n + 123456789n;
			const view = new ModifyApp({appType: 'Presentation test', port: 16614});
			WebSocketHandler.GetEvent(Number(sent[0].uid)).m_handleData({
				66001: timestamp, 66002: 'Label', 66003: true, 66004: 'Constant', 66005: timestamp,
				66006: {Rows: [[1.234]]}
			});
		`,
			context);
		const input = browser.document.querySelector('input[parameter-id="66001"]');
		testRunner.Assert(input.closest('.item').title, 'Timestamp (66001) Timer\n\nFull precision');
		testRunner.Assert(input.value.startsWith('2026-11-01 12:34:56.123456 UTC'), true);
		testRunner.Assert(input.getAttribute('accuracy'), '26');
		testRunner.Assert(browser.document.querySelector('input[parameter-id="66002"]').closest('.item').title, '');
		testRunner.Assert(browser.document.querySelector('input[parameter-id="66003"]').closest('.item').title,
			'Enabled (66003) Bool\n\n<b>Switch</b>');
		const constant = browser.document.querySelector('input[parameter-id="66004"]');
		testRunner.Assert(constant.closest('.item').title, 'Fixed label (66004) String\n\nConstant details');
		testRunner.Assert(constant.disabled, true);
		const date = browser.document.querySelector('input[parameter-id="66005"]');
		testRunner.Assert(date.value.startsWith('2026-11-01 UTC'), true);
		testRunner.Assert(date.readOnly, true);
		testRunner.Assert(
			browser.document.querySelector('.tableWrapper .header').title, 'Prices (66006)\n\nTable details');
		browser.document.querySelector('.viewContent .button').click();
		testRunner.Assert(runInContext('sent.length', context), 2);
		testRunner.Assert(runInContext('sent[1].parameters[66001] === timestamp', context), true);
		testRunner.Assert(runInContext('sent[1].parameters[66006][0][0]', context), 1.234);
	}
	finally {
		dom.window.close();
	}
});

testRunner.Run();