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

const { JSDOM } = require('jsdom');
const Helper = require('../helper.js');

global.createdApps = new Map();

// TODO: That should be defined for each test separately ?
// TODO: It can be solved as container of conditions in stack organization. Default cases can be added here for general
// TODO: usage and it will be posible to override/manage more specific cases in each test.
global.fetch = (url, options) => {
	if (options.headers.Type === 'getInstalledApps') {
		return Promise.resolve({
			ok : true,
			status : 200,
			text : () => Promise.resolve(
				'{"status":true,"apps":[{"type":"Gateway T"},{"type":"Strategy"},{"type":"Storage"},{"type":"Graph"}]}')
		});
	}

	if (options.headers.Type === 'createApp') {
		if (global.failCreateApp) {
			return Promise.resolve({
				ok : true,
				status : 200,
				text : () => Promise.resolve(
					'{"status":false,"message":"Can\'t create app with type: ' + options.headers.AppType + '"}')
			});
		}

		const port = Math.floor(Math.random() * (65535 - 1000 + 1)) + 1000;
		global.createdApps.set(port, options.headers.AppType);

		return Promise.resolve({ text : () => Promise.resolve('{"status":true,"port":' + port + '}') });
	}

	if (options.headers.Type === 'getCreatedApps') {
		return Promise.resolve({
			ok : true,
			status : 200,
			text : () => Promise.resolve('{"status":true,"apps":['
				+ Array.from(global.createdApps.entries())
					  .map(([ port, appType ]) => `{"appType":"${appType}","port":${port}}`)
					  .join(',')
				+ ']}')
		});
	}

	if (options.headers.Type === 'getMetadata') {
		if (options.headers.AppType == 'Strategy') {
			return Promise.resolve({
				ok : true,
				status : 200,
				text : () => Promise.resolve(
					`{"status":true,"metadata":{"mutable":{"30001":{"name":"Price type","type":"Int16","min":1,"max":3,"stringInterpretation":{"0":"Undefined","1":"Fair","2":"Soft"}},"30002":{"name":"Start trading time delay","type":"Duration","canBeEmpty":false,"durationType":"Seconds"},"30003":{"name":"End trading time delay","type":"Duration","canBeEmpty":false,"durationType":"Seconds"},"30004":{"name":"Order hold time","type":"Duration","min":1000000000,"canBeEmpty":false,"durationType":"Seconds"},"30005":{"name":"Active minimum trend strike mode","type":"Bool"},"30006":{"name":"Minimum trend strike","type":"Int64"},"30007":{"name":"Active minimum trend strike price limit","type":"Bool"},"30008":{"name":"Minimum trend strike price limit","type":"Double"},"30009":{"name":"Active target order cost mode","type":"Bool"},"30010":{"name":"Target order cost","type":"Double"},"30011":{"name":"Trend strike barrier","type":"Uint64"},"30012":{"name":"Size buffer soft price change","type":"Uint64","min":1},"30013":{"name":"Gateway id","type":"OptionalInt32","canBeEmpty":false},"30014":{"name":"Commissions","type":"TableData","canBeEmpty":false,"columns":{"0":{"type":"Int16","name":"Instrument type","stringInterpretation":{"0":"Undefined","1":"Bond","2":"Share","3":"Currency","4":"ETF","5":"Futures","6":"StructuralProduct","7":"Option","8":"ClearingCertificate","9":"Index","10":"Commodity"}},"1":{"type":"Double","name":"%"}}},"30015":{"name":"Figis to trade","type":"TableData","canBeEmpty":false,"columns":{"0":{"type":"Uint64","name":"figi"},"1":{"type":"Uint64","name":"Default lots volume"}}},"1000001":{"name":"Seconds between try to connect","type":"Uint32","min":1},"1000002":{"name":"Limit of attempts to connection","type":"Uint64","min":1},"1000003":{"name":"Limit of connections from one IP","type":"Uint64","min":1},"1000004":{"name":"Recv buffer size","type":"Uint64","min":3},"1000005":{"name":"Recv buffer size limit","type":"Uint64","min":1024}},"const":{"1000006":{"name":"Server state","type":"Int16","stringInterpretation":{"0":"Undefined","1":"Initialization","2":"Running","3":"Stopped"}},"1000007":{"name":"Max connections","type":"Int32"},"1000008":{"name":"Listening IP","type":"Uint32"},"1000009":{"name":"Listening port","type":"Uint16"},"2000001":{"name":"Name","type":"String"},"2000002":{"name":"Application state","type":"Int16","stringInterpretation":{"0":"Undefined","1":"Paused","2":"Running"}}}}}`)
			});
		}

		return Promise.resolve(
			{ ok : true, status : 200, text : () => Promise.resolve(`{"status":true,"metadata":{}}`) });
	}

	if (options.headers.Type === 'getParameters') {
		let parameters = [];
		if (global.createdApps.has(options.headers.Port)) {
			let appType = global.createdApps.get(options.headers.Port);
			if (appType === "Strategy") {
				parameters.add({ "2000001" : "Soft scalping" });
			}
		}
		return Promise.resolve(
			{ ok : true, status : 200, text : () => Promise.resolve('{"status":true,"parameters":""}') });
	}

	return Promise.reject(new Error(`Unknown request: ${JSON.stringify(options.headers)}`));
};

class TestRunner {
	constructor(
		html = '<html><body><div class="tables"></div><main><section class="views"></section></main></body></html>')
	{
		this.m_dom = new JSDOM(html, { 
			resources: "usable",
			pretendToBeVisual: true,
			includeNodeLocations: true,
			storageQuota: 10000000,
		});
		global.document = this.m_dom.window.document;
		global.Node = this.m_dom.window.Node;
		global.Event = this.m_dom.window.Event;
		global.EventTarget = this.m_dom.window.EventTarget;
		global.body = document.querySelector('body');

		const originalAddEventListener = EventTarget.prototype.addEventListener;
		const eventListeners = new WeakMap();

		EventTarget.prototype.addEventListener = function(type, listener, options) {
			if (!eventListeners.has(this)) {
				eventListeners.set(this, {});
			}
			if (!eventListeners.get(this)[type]) {
				eventListeners.get(this)[type] = [];
			}

			eventListeners.get(this)[type].push({ listener, options });
			originalAddEventListener.call(this, type, listener, options);
		};

		EventTarget.prototype.removeEventListener = function(type, listener, options) {
			if (!eventListeners.has(this) || !eventListeners.get(this)[type]) {
				return;
			}

			const index = eventListeners.get(this)[type].findIndex(
				({ listener : registeredListener, options : registeredOptions }) => listener === registeredListener
					&& options === registeredOptions);

			if (index !== -1) {
				eventListeners.get(this)[type].splice(index, 1);
			}
		};

		global.getEventListeners = function(target) { return eventListeners.get(target) || {}; };

		this.m_postTestFunction = null;
		this.m_failedAssertions = 0;
		this.m_failedTests = 0;
		this.m_totalTests = 0;
		this.m_tests = [];

		global.window = this.m_dom.window;
		global.MutationObserver = require('mutation-observer');
	}

	SetPostTestFunction(func) { this.m_postTestFunction = func; }

	Test(name, testFunction) { this.m_tests.push({ name, testFunction }); }

	static Step(name) { console.log(`🔹 ${name}`); }

	Assert(actual, expected, message)
	{
		if (!Helper.DeepEqual(expected, actual)) {
			const error = new Error();
			const callerLine = error.stack.split('\n')[2].trim();
			console.error(`Actual: ${actual}, expected: ${expected}, ${callerLine}.${message ? ' ' + message : ''}`);
			this.m_failedAssertions++;
		}
	}

	async Run()
	{
		for (const { name, testFunction } of this.m_tests) {
			this.m_totalTests++;
			try {
				console.log(`▶️ ${name}`);
				this.m_failedAssertions = 0;
				await testFunction();
				if (this.m_failedAssertions == 0) {
					console.log(`\x1b[32m✔️ ${name}\x1b[0m`);
				}
				else {
					this.m_failedTests++;
					console.error(`\x1b[31m❌ ${name}\x1b[0m`);
				}
			}
			catch (error) {
				this.m_failedTests++;
				console.error(`\x1b[31m❌ ${name}\x1b[0m`);
				console.error(error);
			}

			if (this.m_postTestFunction !== null) {
				this.m_postTestFunction();
			}
		}

		if (this.m_failedTests === 0) {
			if (this.m_totalTests === 0) {
				console.log('✔️ No tests found.');
			}
			else if (this.m_totalTests === 1) {
				console.log('✔️\x1b[32m Test passed.\x1b[0m');
			}
			else {
				console.log(`✔️\x1b[32m All ${this.m_totalTests} tests passed.\x1b[0m`);
			}

			process.exit(0);
		}

		console.log(`❌\x1b[31m Failed ${this.m_failedTests} tests out of ${this.m_totalTests}.\x1b[0m`);
		process.exit(1);
	}

	static async WaitFor(conditionFunction, name, millisecondsToWait = 1000)
	{
		const startTime = Date.now();
		while (true) {
			if (conditionFunction()) {
				return;
			}
			if (Date.now() - startTime > millisecondsToWait) {
				throw new Error(`Timeout "${name}" ${millisecondsToWait} milliseconds waiting for condition`);
			}
			await new Promise(resolve => setTimeout(resolve, 1));
		}
	}

	static async Wait(milliseconds = 1000) { await new Promise(resolve => setTimeout(resolve, milliseconds)); }

	static currentUTC = (new Date).getTimezoneOffset() / 60 * -1;
	static UTC = TestRunner.currentUTC >= 0 ? `+${TestRunner.currentUTC}` : `${TestRunner.currentUTC}`;
	static GetHourWithUtcShift = (year, month, utcHour) => {
		let shift = utcHour + TestRunner.currentUTC;

		if ((new Date(year, month - 1, 4)).getTimezoneOffset() / 60 * -1 < TestRunner.currentUTC) {
			shift -= 1;
		}

		return String(shift).padStart(2, '0')
	};
}

class TableChecker {
	static CheckValues(test, inputs, values)
	{
		test.Assert(inputs.length, values.length);

		for (let i = 0; i < values.length; i++) {
			if (typeof values[i] === 'boolean') {
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
		let inputs = newRow.querySelectorAll('input');
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
			addButton.dispatchEvent(new Event('click'));
			table.Save();
			return;
		}
		test.Assert(false, true, "Add button not found");
	}
}

module.exports.TestRunner = TestRunner;
module.exports.TableChecker = TableChecker;