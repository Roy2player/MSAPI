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
const Timer = require('../timer');

let testRunner = new TestRunner();

testRunner.Test('Cast input to timer and check', () => {
	TestRunner.Step('Cast input to timer and check');
	let input = document.createElement('input');
	body.appendChild(input);
	Timer.Apply(input);
	testRunner.Assert(Timer.GetTimersNumber(), 1);
	testRunner.Assert(getEventListeners(document).click.length, 1)
	testRunner.Assert(input.type, 'text');
	testRunner.Assert(input.getAttribute('timestamp'), '');
	testRunner.Assert(input.value, '');
	testRunner.Assert(input.getAttribute('utc'), TestRunner.UTC);
	testRunner.Assert(input.classList.contains('invalid'), false);
	testRunner.Assert(input.outerHTML, `<input type="text" timestamp="" utc="${TestRunner.UTC}">`);

	TestRunner.Step('Set value by method');
	Timer.SetValue(input, 1n);
	testRunner.Assert(
		input.value, `1970-01-01 ${TestRunner.GetHourWithUtcShift(1970, 1, 0)}:00:00.000000001 UTC${TestRunner.UTC}`);
	testRunner.Assert(input.getAttribute('timestamp'), '1');
	testRunner.Assert(input.getAttribute('utc'), TestRunner.UTC);
	testRunner.Assert(input.classList.contains('invalid'), false);

	Timer.SetValue(input, 0n);
	testRunner.Assert(
		input.value, `1970-01-01 ${TestRunner.GetHourWithUtcShift(1970, 1, 0)}:00:00.000000000 UTC${TestRunner.UTC}`);
	testRunner.Assert(input.getAttribute('timestamp'), '0');
	testRunner.Assert(input.getAttribute('utc'), TestRunner.UTC);
	testRunner.Assert(/* cannot be empty by default */ input.classList.contains('invalid'), true);

	Timer.SetValue(input, -13n);
	testRunner.Assert(
		input.value, `1970-01-01 ${TestRunner.GetHourWithUtcShift(1970, 1, 0)}:00:00.000000000 UTC${TestRunner.UTC}`);
	testRunner.Assert(input.getAttribute('timestamp'), '0');
	testRunner.Assert(input.getAttribute('utc'), TestRunner.UTC);
	testRunner.Assert(input.classList.contains('invalid'), true);

	Timer.SetValue(input, 1741120856012345678n);
	testRunner.Assert(
		input.value, `2025-03-04 ${TestRunner.GetHourWithUtcShift(2025, 3, 20)}:40:56.012345678 UTC${TestRunner.UTC}`);
	testRunner.Assert(input.getAttribute('timestamp'), '1741120856012345678');
	testRunner.Assert(input.getAttribute('utc'), TestRunner.UTC);
	testRunner.Assert(input.classList.contains('invalid'), false);

	input.setAttribute('canBeEmpty', 'true');
	Timer.SetValue(input, -12353n);
	testRunner.Assert(
		input.value, `1970-01-01 ${TestRunner.GetHourWithUtcShift(1970, 1, 0)}:00:00.000000000 UTC${TestRunner.UTC}`);
	testRunner.Assert(input.getAttribute('timestamp'), '0');
	testRunner.Assert(input.getAttribute('utc'), TestRunner.UTC);
	testRunner.Assert(input.classList.contains('invalid'), false);

	TestRunner.Step('Set value by input');
	input.value = `1970-01-01 ${TestRunner.GetHourWithUtcShift(1970, 1, 0)}:00:00.000000001 DFJG;KDLJGDLKGJ`;
	input.dispatchEvent(new Event('input', { bubbles : true }));
	body.dispatchEvent(new Event('click', { bubbles : true }));
	testRunner.Assert(
		input.value, `1970-01-01 ${TestRunner.GetHourWithUtcShift(1970, 1, 0)}:00:00.000000001 UTC${TestRunner.UTC}`);
	testRunner.Assert(input.getAttribute('timestamp'), '1');
	testRunner.Assert(input.getAttribute('utc'), TestRunner.UTC);
	testRunner.Assert(input.classList.contains('invalid'), false);
	testRunner.Assert(
		input.outerHTML, `<input type="text" timestamp="1" utc="${TestRunner.UTC}" class="" canbeempty="true">`);

	TestRunner.Step('Create one more input');
	let input2 = document.createElement('input');
	body.appendChild(input2);
	Timer.Apply(input2);
	testRunner.Assert(Timer.GetTimersNumber(), 2);
	testRunner.Assert(getEventListeners(document).click.length, 1)
	testRunner.Assert(input2.type, 'text');
	testRunner.Assert(input2.getAttribute('timestamp'), '');
	testRunner.Assert(input2.value, '');
	testRunner.Assert(input2.getAttribute('utc'), TestRunner.UTC);
	testRunner.Assert(input2.classList.contains('invalid'), false);

	input2.value = `2025_03_04 ${TestRunner.GetHourWithUtcShift(2025, 3, 20)} 40 56-012345678 UTC+3`;
	input2.dispatchEvent(new Event('input', { bubbles : true }));
	body.dispatchEvent(new Event('click', { bubbles : true }));
	testRunner.Assert(
		input2.value, `2025-03-04 ${TestRunner.GetHourWithUtcShift(2025, 3, 20)}:40:56.012345678 UTC${TestRunner.UTC}`);
	testRunner.Assert(input2.getAttribute('timestamp'), '1741120856012345678');
	testRunner.Assert(input2.getAttribute('utc'), TestRunner.UTC);
	testRunner.Assert(input2.classList.contains('invalid'), false);

	input2.value = ``;
	input2.dispatchEvent(new Event('input', { bubbles : true }));
	body.dispatchEvent(new Event('click', { bubbles : true }));
	testRunner.Assert(
		input2.value, `2025-03-04 ${TestRunner.GetHourWithUtcShift(2025, 3, 20)}:40:56.012345678 UTC${TestRunner.UTC}`);
	testRunner.Assert(input2.getAttribute('timestamp'), '1741120856012345678');
	testRunner.Assert(input2.getAttribute('utc'), TestRunner.UTC);
	testRunner.Assert(input2.classList.contains('invalid'), false);

	input2.value = `1969-01-01 00:00:00.000000000 UTC-1`;
	input2.dispatchEvent(new Event('input', { bubbles : true }));
	body.dispatchEvent(new Event('click', { bubbles : true }));
	testRunner.Assert(
		input2.value, `2025-03-04 ${TestRunner.GetHourWithUtcShift(2025, 3, 20)}:40:56.012345678 UTC${TestRunner.UTC}`);
	testRunner.Assert(input2.getAttribute('timestamp'), '1741120856012345678');
	testRunner.Assert(input2.getAttribute('utc'), TestRunner.UTC);
	testRunner.Assert(input2.classList.contains('invalid'), false);

	input2.value = `d`;
	input2.dispatchEvent(new Event('input', { bubbles : true }));
	body.dispatchEvent(new Event('click', { bubbles : true }));
	testRunner.Assert(
		input2.value, `2025-03-04 ${TestRunner.GetHourWithUtcShift(2025, 3, 20)}:40:56.012345678 UTC${TestRunner.UTC}`);
	testRunner.Assert(input2.getAttribute('timestamp'), '1741120856012345678');
	testRunner.Assert(input2.getAttribute('utc'), TestRunner.UTC);
	testRunner.Assert(input2.classList.contains('invalid'), false);

	input2.value = `1970-01-01 00:00:00.000000000 UTC-1`;
	input2.dispatchEvent(new Event('input', { bubbles : true }));
	body.dispatchEvent(new Event('click', { bubbles : true }));
	testRunner.Assert(
		input2.value, `1970-01-01 ${TestRunner.GetHourWithUtcShift(1970, 1, 0)}:00:00.000000000 UTC${TestRunner.UTC}`);
	testRunner.Assert(input2.getAttribute('timestamp'), '0');
	testRunner.Assert(input2.getAttribute('utc'), TestRunner.UTC);
	testRunner.Assert(input2.classList.contains('invalid'), true);

	input.remove();
	input2.remove();
	testRunner.Assert(getEventListeners(document).click.length, 1);
	testRunner.Assert(Timer.GetTimersNumber(), 2);

	TestRunner.Step('Verify timers cleanup');
	let input3 = document.createElement('input');
	body.appendChild(input3);
	Timer.Apply(input3);
	input3.dispatchEvent(new Event('input'));
	document.dispatchEvent(new Event('click'));
	testRunner.Assert(Timer.GetTimersNumber(), 1);
	input3.remove();
});

testRunner.Run();