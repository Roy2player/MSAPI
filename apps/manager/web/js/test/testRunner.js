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

const { JSDOM } = require('jsdom');

global.port = 1614;
let jsdom
	= new JSDOM(`<html><body><div class="tables"></div><main><section class="views"></section></main></body></html>`, {
		  url : `http://localhost:${global.port}/`,
	  });
global.document = jsdom.window.document;
global.Node = jsdom.window.Node;
global.Event = jsdom.window.Event;
global.EventTarget = jsdom.window.EventTarget;
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
global.window = jsdom.window;
global.MutationObserver = require("mutation-observer");

require("../views/dispatcher"); // Required for View in Select in Helper
const event = new jsdom.window.Event("DOMContentLoaded", { bubbles : true, cancelable : true });
global.document.dispatchEvent(event);

const Helper = require("../help/helper");

/**
 * @brief Test execution controller, assertions result tracker.
 */
class TestRunner {
	constructor()
	{
		this.m_postTestFunction = null;
		this.m_passedAssertions = 0;
		this.m_totalAssertions = 0;
		this.m_failedTests = 0;
		this.m_totalTests = 0;
		this.m_tests = [];
	}

	SetPostTestFunction(func) { this.m_postTestFunction = func; }

	Test(name, testFunction) { this.m_tests.push({ name, testFunction }); }

	static Step(name) { console.log(`🔹 ${name}`); }

	Assert(actual, expected, message)
	{
		if (!Helper.DeepEqual(expected, actual)) {
			const error = new Error();
			const callerLine = error.stack.split('\n')[2].trim();
			throw new Error(`Actual: ${actual}, expected: ${expected}, ${callerLine}.${message ? ' ' + message : ''}`);
		}

		this.m_passedAssertions++;
	}

	async Run()
	{
		for (const { name, testFunction } of this.m_tests) {
			this.m_totalTests++;
			console.log(`▶️ ${name}`);
			this.m_passedAssertions = 0;
			try {
				await testFunction();
				console.log(`\x1b[32m✔️ ${name}. ${this.m_passedAssertions} passed assertions\x1b[0m`);
			}
			catch (error) {
				this.m_failedTests++;
				console.error(`\x1b[31m❌ ${name}\x1b[0m`);
				console.error(error);
			}
			this.m_totalAssertions += this.m_passedAssertions;
			if (this.m_postTestFunction) {
				this.m_postTestFunction();
			}
		}

		if (this.m_failedTests === 0) {
			if (this.m_totalTests === 0) {
				console.log('✔️ No tests found.');
			}
			else if (this.m_totalTests === 1) {
				console.log(`✔️\x1b[32m Test passed. ${this.m_totalAssertions} passed assertions\x1b[0m`);
			}
			else {
				console.log(`✔️\x1b[32m All ${this.m_totalTests} tests passed. ${
					this.m_totalAssertions} passed assertions\x1b[0m`);
			}

			process.exit(0);
		}

		console.log(`❌\x1b[31m Failed ${this.m_failedTests} tests out of ${this.m_totalTests}. ${
			this.m_totalAssertions} passed assertions\x1b[0m`);
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

		return String(shift).padStart(2, '0');
	};
};

let testRunner = new TestRunner();

module.exports.TestRunner = TestRunner;
module.exports.testRunner = testRunner;