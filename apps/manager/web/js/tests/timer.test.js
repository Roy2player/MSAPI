/**************************
 * This file is part of MSAPI.
 * License: see LICENSE.md
 * Contributor terms: see CONTRIBUTING.md
 *
 * This software is licensed under the Polyform Noncommercial License 1.0.0.
 * You may use, copy, modify, and distribute it for noncommercial purposes only.
 *
 * For commercial use, please contact: msapi.info@mail.ru
 *
 * Required Notice: MSAPI, copyright © 2021–2026 Maksim Andreevich Leonov, msapi.info@mail.ru
 */

const { TestRunner } = require("../test/testRunner");
const Timer = require("../core/timer");

let testRunner = new TestRunner();

testRunner.Test('Cast input to timer and check', () => {
	TestRunner.Step('Cast input to timer and check');
	let input = document.createElement("input");
	body.appendChild(input);
	Timer.Apply(input);
	testRunner.Assert(Timer.GetTimersNumber(), 1);
	testRunner.Assert(getEventListeners(document).click.length, 1);
	testRunner.Assert(input.type, 'text');
	testRunner.Assert(input.getAttribute('timestamp'), '');
	testRunner.Assert(input.value, '');
	testRunner.Assert(input.getAttribute('utc'), TestRunner.UTC);
	testRunner.Assert(input.classList.contains('invalid'), false);
	testRunner.Assert(input.outerHTML, `<input type="text" timestamp="" utc="${TestRunner.UTC}">`);

	TestRunner.Step('Set value by method');
	Timer.SetValue(input, 1n);
	testRunner.Assert(input.value,
		`1970-01-01 ${TestRunner.GetHourWithUtcShift(1970, 1, 1, 0)}:00:00.000000001 UTC${TestRunner.UTC}`);
	testRunner.Assert(input.getAttribute('timestamp'), '1');
	testRunner.Assert(input.getAttribute('utc'), TestRunner.UTC);
	testRunner.Assert(input.classList.contains('invalid'), false);

	Timer.SetValue(input, 0n);
	testRunner.Assert(input.value,
		`1970-01-01 ${TestRunner.GetHourWithUtcShift(1970, 1, 1, 0)}:00:00.000000000 UTC${TestRunner.UTC}`);
	testRunner.Assert(input.getAttribute('timestamp'), '0');
	testRunner.Assert(input.getAttribute('utc'), TestRunner.UTC);
	testRunner.Assert(/* cannot be empty by default */ input.classList.contains('invalid'), true);

	Timer.SetValue(input, -13n);
	testRunner.Assert(input.value,
		`1970-01-01 ${TestRunner.GetHourWithUtcShift(1970, 1, 1, 0)}:00:00.000000000 UTC${TestRunner.UTC}`);
	testRunner.Assert(input.getAttribute('timestamp'), '0');
	testRunner.Assert(input.getAttribute('utc'), TestRunner.UTC);
	testRunner.Assert(input.classList.contains('invalid'), true);

	Timer.SetValue(input, 1741120856012345678n);
	testRunner.Assert(input.value,
		`2025-03-04 ${TestRunner.GetHourWithUtcShift(2025, 3, 4, 20)}:40:56.012345678 UTC${TestRunner.UTC}`);
	testRunner.Assert(input.getAttribute('timestamp'), '1741120856012345678');
	testRunner.Assert(input.getAttribute('utc'), TestRunner.UTC);
	testRunner.Assert(input.classList.contains('invalid'), false);

	input.setAttribute('canBeEmpty', 'true');
	Timer.SetValue(input, -12353n);
	testRunner.Assert(input.value,
		`1970-01-01 ${TestRunner.GetHourWithUtcShift(1970, 1, 1, 0)}:00:00.000000000 UTC${TestRunner.UTC}`);
	testRunner.Assert(input.getAttribute('timestamp'), '0');
	testRunner.Assert(input.getAttribute('utc'), TestRunner.UTC);
	testRunner.Assert(input.classList.contains('invalid'), false);

	TestRunner.Step('Set value by input');
	input.value = `1970-01-01 ${TestRunner.GetHourWithUtcShift(1970, 1, 1, 0)}:00:00.000000001 DFJG;KDLJGDLKGJ`;
	input.dispatchEvent(new Event("input", { bubbles : true }));
	body.dispatchEvent(new Event("click", { bubbles : true }));
	testRunner.Assert(input.value,
		`1970-01-01 ${TestRunner.GetHourWithUtcShift(1970, 1, 1, 0)}:00:00.000000001 UTC${TestRunner.UTC}`);
	testRunner.Assert(input.getAttribute('timestamp'), '1');
	testRunner.Assert(input.getAttribute('utc'), TestRunner.UTC);
	testRunner.Assert(input.classList.contains('invalid'), false);
	testRunner.Assert(
		input.outerHTML, `<input type="text" timestamp="1" utc="${TestRunner.UTC}" class="" canbeempty="true">`);

	TestRunner.Step('Create one more input');
	let input2 = document.createElement("input");
	body.appendChild(input2);
	Timer.Apply(input2);
	testRunner.Assert(Timer.GetTimersNumber(), 2);
	testRunner.Assert(getEventListeners(document).click.length, 1);
	testRunner.Assert(input2.type, 'text');
	testRunner.Assert(input2.getAttribute('timestamp'), '');
	testRunner.Assert(input2.value, '');
	testRunner.Assert(input2.getAttribute('utc'), TestRunner.UTC);
	testRunner.Assert(input2.classList.contains('invalid'), false);

	input2.value = `2025_03_04 ${TestRunner.GetHourWithUtcShift(2025, 3, 4, 20)} 40 56-012345678 UTC+3`;
	input2.dispatchEvent(new Event("input", { bubbles : true }));
	body.dispatchEvent(new Event("click", { bubbles : true }));
	testRunner.Assert(input2.value,
		`2025-03-04 ${TestRunner.GetHourWithUtcShift(2025, 3, 4, 20)}:40:56.012345678 UTC${TestRunner.UTC}`);
	testRunner.Assert(input2.getAttribute('timestamp'), '1741120856012345678');
	testRunner.Assert(input2.getAttribute('utc'), TestRunner.UTC);
	testRunner.Assert(input2.classList.contains('invalid'), false);

	input2.value = ``;
	input2.dispatchEvent(new Event("input", { bubbles : true }));
	body.dispatchEvent(new Event("click", { bubbles : true }));
	testRunner.Assert(input2.value,
		`2025-03-04 ${TestRunner.GetHourWithUtcShift(2025, 3, 4, 20)}:40:56.012345678 UTC${TestRunner.UTC}`);
	testRunner.Assert(input2.getAttribute('timestamp'), '1741120856012345678');
	testRunner.Assert(input2.getAttribute('utc'), TestRunner.UTC);
	testRunner.Assert(input2.classList.contains('invalid'), false);

	input2.value = `1969-01-01 00:00:00.000000000 UTC-1`;
	input2.dispatchEvent(new Event("input", { bubbles : true }));
	body.dispatchEvent(new Event("click", { bubbles : true }));
	testRunner.Assert(input2.value,
		`2025-03-04 ${TestRunner.GetHourWithUtcShift(2025, 3, 4, 20)}:40:56.012345678 UTC${TestRunner.UTC}`);
	testRunner.Assert(input2.getAttribute('timestamp'), '1741120856012345678');
	testRunner.Assert(input2.getAttribute('utc'), TestRunner.UTC);
	testRunner.Assert(input2.classList.contains('invalid'), false);

	input2.value = `d`;
	input2.dispatchEvent(new Event("input", { bubbles : true }));
	body.dispatchEvent(new Event("click", { bubbles : true }));
	testRunner.Assert(input2.value,
		`2025-03-04 ${TestRunner.GetHourWithUtcShift(2025, 3, 4, 20)}:40:56.012345678 UTC${TestRunner.UTC}`);
	testRunner.Assert(input2.getAttribute('timestamp'), '1741120856012345678');
	testRunner.Assert(input2.getAttribute('utc'), TestRunner.UTC);
	testRunner.Assert(input2.classList.contains('invalid'), false);

	input2.value = `1970-01-01 00:00:00.000000000 UTC-1`;
	input2.dispatchEvent(new Event("input", { bubbles : true }));
	body.dispatchEvent(new Event("click", { bubbles : true }));
	testRunner.Assert(input2.value,
		`1970-01-01 ${TestRunner.GetHourWithUtcShift(1970, 1, 1, 0)}:00:00.000000000 UTC${TestRunner.UTC}`);
	testRunner.Assert(input2.getAttribute('timestamp'), '0');
	testRunner.Assert(input2.getAttribute('utc'), TestRunner.UTC);
	testRunner.Assert(input2.classList.contains('invalid'), true);

	input.remove();
	input2.remove();
	testRunner.Assert(getEventListeners(document).click.length, 1);
	testRunner.Assert(Timer.GetTimersNumber(), 2);

	TestRunner.Step('Verify timers cleanup');
	let input3 = document.createElement("input");
	body.appendChild(input3);
	Timer.Apply(input3);
	input3.dispatchEvent(new Event("input"));
	document.dispatchEvent(new Event("click"));
	testRunner.Assert(Timer.GetTimersNumber(), 1);
	input3.remove();
});

testRunner.Test('Show the timer mask when an empty editable input receives focus', () => {
	const input = document.createElement('input');
	body.appendChild(input);
	Timer.Apply(input);
	const mask = Timer.ToString(0n, input.getAttribute('utc'));

	input.focus();
	testRunner.Assert(document.activeElement, input);
	testRunner.Assert(input.value, mask);
	testRunner.Assert(input.getAttribute('timestamp'), '');
	testRunner.Assert(input.classList.contains('invalid'), false);
	testRunner.Assert(input.selectionStart, 0);
	testRunner.Assert(input.selectionEnd, 4);
	input.click();
	testRunner.Assert(input.value, mask);
	input.blur();
	testRunner.Assert(input.value, '');
	testRunner.Assert(input.getAttribute('timestamp'), '');

	input.setAttribute('canBeEmpty', 'true');
	input.focus();
	testRunner.Assert(input.value, mask);
	input.blur();
	testRunner.Assert(input.value, '');

	input.readOnly = true;
	input.focus();
	testRunner.Assert(input.value, '');
	input.blur();
	input.readOnly = false;
	input.disabled = true;
	input.dispatchEvent(new Event('focus'));
	testRunner.Assert(input.value, '');
	input.disabled = false;

	input.focus();
	const timestamp = 1741120856012345678n;
	Timer.SetValue(input, timestamp);
	const value = input.value;
	input.blur();
	testRunner.Assert(input.value, value);
	input.focus();
	testRunner.Assert(input.value, value);
	input.blur();
	testRunner.Assert(input.getAttribute('timestamp'), timestamp.toString());

	const editable = document.createElement('input');
	body.appendChild(editable);
	Timer.Apply(editable);
	editable.focus();
	editable.value = value;
	editable.dispatchEvent(new Event('input', { bubbles : true }));
	editable.blur();
	body.click();
	testRunner.Assert(editable.value, value);
	testRunner.Assert(editable.getAttribute('timestamp'), timestamp.toString());
	input.remove();
	editable.remove();
});

testRunner.Test('Use the timestamp date for daylight-saving expectations', () => {
	const timezone = process.env.TZ;
	const currentUTC = TestRunner.currentUTC;
	try {
		process.env.TZ = 'Europe/Stockholm';
		const cases = [
			{ month : 1, day : 21, hour : 0, expected : '01' }, { month : 4, day : 21, hour : 0, expected : '02' },
			{ month : 3, day : 30, hour : 0, expected : '01' }, { month : 3, day : 30, hour : 1, expected : '03' },
			{ month : 10, day : 26, hour : 0, expected : '02' }, { month : 10, day : 26, hour : 1, expected : '02' }
		];
		// Expectations must be identical whether the tests run during standard time or daylight-saving time.
		for (const offset of [1, 2]) {
			TestRunner.currentUTC = offset;
			for (const { month, day, hour, expected } of cases) {
				testRunner.Assert(TestRunner.GetHourWithUtcShift(2025, month, day, hour), expected,
					`UTC date 2025-${month}-${day} at ${hour}:00 with current offset ${offset}`);
			}
		}
	}
	finally {
		TestRunner.currentUTC = currentUTC;
		if (timezone === undefined) {
			delete process.env.TZ;
		}
		else {
			process.env.TZ = timezone;
		}
	}
});

testRunner.Test('Display each timer accuracy without changing the stored timestamp', () => {
	const input = document.createElement('input');
	body.appendChild(input);
	Timer.Apply(input);
	const timestamp = BigInt(new Date(2026, 10, 1, 12, 34, 56).getTime()) * 1000000n + 123456789n;
	const suffix = ` UTC${TestRunner.UTC}`;
	const cases = [
		[ Timer.ACCURACY.Years, '2026' ], [ Timer.ACCURACY.Months, '2026-11' ], [ Timer.ACCURACY.Days, '2026-11-01' ],
		[ Timer.ACCURACY.Hours, '2026-11-01 12' ], [ Timer.ACCURACY.Minutes, '2026-11-01 12:34' ],
		[ Timer.ACCURACY.Seconds, '2026-11-01 12:34:56' ], [ Timer.ACCURACY.Milliseconds, '2026-11-01 12:34:56.123' ],
		[ Timer.ACCURACY.Microseconds, '2026-11-01 12:34:56.123456' ],
		[ Timer.ACCURACY.Nanoseconds, '2026-11-01 12:34:56.123456789' ]
	];
	for (const [accuracy, text] of cases) {
		input.setAttribute('accuracy', accuracy);
		Timer.SetValue(input, timestamp);
		testRunner.Assert(input.value, text + suffix);
		testRunner.Assert(input.getAttribute('timestamp'), timestamp.toString());
		Timer.Normalize(input);
		testRunner.Assert(input.getAttribute('timestamp'), timestamp.toString(),
			'Normalizing the unchanged display must retain hidden precision');
		input.focus();
		testRunner.Assert(input.value, '2026-11-01 12:34:56.123456789' + suffix);
		input.blur();
		testRunner.Assert(input.value, text + suffix);
		testRunner.Assert(input.getAttribute('timestamp'), timestamp.toString());
		input.readOnly = true;
		input.focus();
		testRunner.Assert(input.value, text + suffix, 'Read-only fields must retain their display accuracy on focus');
		input.blur();
		input.readOnly = false;
	}
	input.remove();
});

testRunner.Test('Commit decimal fractions before notifying timer change listeners', () => {
	const base = BigInt(new Date(2026, 10, 1, 12, 35, 56).getTime()) * 1000000n;
	const cases = [
		[ Timer.ACCURACY.Milliseconds, '123', 123000000n ], [ Timer.ACCURACY.Microseconds, '123456', 123456000n ],
		[ Timer.ACCURACY.Nanoseconds, '123456789', 123456789n ],
		[ Timer.ACCURACY.Milliseconds, '123456789', 123456789n ]
	];
	for (const [accuracy, fraction, nanoseconds] of cases) {
		const input = document.createElement('input');
		body.appendChild(input);
		Timer.Apply(input);
		input.setAttribute('accuracy', accuracy);
		let observedTimestamp;
		input.addEventListener('change', () => { observedTimestamp = input.getAttribute('timestamp'); });
		input.value = `2026-11-01 12:35:56.${fraction} UTC${TestRunner.UTC}`;
		input.dispatchEvent(new Event('input', { bubbles : true }));
		input.dispatchEvent(new Event('change', { bubbles : true }));
		testRunner.Assert(observedTimestamp, (base + nanoseconds).toString());
		body.click();
		testRunner.Assert(input.getAttribute('timestamp'), observedTimestamp,
			'The outside click must not parse the rounded display again');
		input.remove();
	}
});

testRunner.Test('Retain hidden timestamp precision when editing a coarse display', () => {
	const original = BigInt(new Date(2026, 10, 1, 12, 34, 56).getTime()) * 1000000n + 123456789n;
	const expected = original + 60000000000n;
	for (const accuracy of Object.values(Timer.ACCURACY)) {
		const input = document.createElement('input');
		body.appendChild(input);
		Timer.Apply(input);
		input.setAttribute('accuracy', accuracy);
		Timer.SetValue(input, original);
		input.focus();
		input.value = input.value.replace(':34:', ':35:');
		input.dispatchEvent(new Event('input', { bubbles : true }));
		input.dispatchEvent(new Event('change', { bubbles : true }));
		input.blur();
		body.click();
		testRunner.Assert(input.getAttribute('timestamp'), expected.toString());
		testRunner.Assert(input.value, Timer.ToString(expected, input.getAttribute('utc'), accuracy));
		input.remove();
	}
});

testRunner.Test('Preserve the caret and selection when focusing a timestamp with display accuracy', () => {
	const input = document.createElement('input');
	body.appendChild(input);
	Timer.Apply(input);
	const timestamp = BigInt(new Date(2026, 10, 1, 12, 34, 56).getTime()) * 1000000n + 123456789n;
	try {
		for (const accuracy of Object.values(Timer.ACCURACY)) {
			input.setAttribute('accuracy', accuracy);
			Timer.SetValue(input, timestamp);
			const display = input.value;
			const selections = [
				[ 0, 0, 'none' ], [ 2, 2, 'none' ], [ 0, 4, 'forward' ], [ 1, 4, 'backward' ],
				[ display.length, display.length, 'none' ]
			];
			for (const [start, end, direction] of selections) {
				input.setSelectionRange(start, end, direction);
				input.focus();
				testRunner.Assert(input.value, Timer.ToString(timestamp, input.getAttribute('utc')));
				testRunner.Assert(
					input.selectionStart, start, `Caret start must survive focus at accuracy ${accuracy}`);
				testRunner.Assert(input.selectionEnd, end, `Selection end must survive focus at accuracy ${accuracy}`);
				testRunner.Assert(input.selectionDirection, direction);
				testRunner.Assert(input.getAttribute('timestamp'), timestamp.toString());
				input.blur();
				testRunner.Assert(input.value, display);
			}
		}
	}
	finally {
		input.remove();
	}
});

testRunner.Run();