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
 * @brief Tests display rounding, metadata tooltips, and validation of unrounded numeric values.
 */

const { testRunner } = require('../test/testRunner');
const Helper = require('../help/helper');

testRunner.Test('Round numeric displays with decimal and arbitrary positive increments', () => {
	const cases = [
		[ null, 0.1, '' ], [ 0, 0.1, '0' ], [ -0, 0.1, '0' ], [ '1.234', 0.1, '1.2' ], [ 1.26, 0.1, '1.3' ],
		[ -1.26, 0.1, '-1.3' ], [ 0.3, 0.1, '0.3' ], [ 1.37, 0.25, '1.25' ], [ 1.38, 0.25, '1.5' ],
		[ 0.125, 0.25, '0.25' ], [ -0.125, 0.25, '0' ], [ -0.375, 0.25, '-0.25' ], [ 149, 100, '100' ],
		[ 150, 100, '200' ], [ 1.2345e-4, 1e-5, '0.00012' ], [ 1e-7, 1e-7, '0.0000001' ], [ 1.26, 1n, '1' ],
		[ 1.234, undefined, '1.234' ], [ NaN, 0.1, 'NaN' ], [ Infinity, 0.1, 'Infinity' ],
		[ -Infinity, 0.1, '-Infinity' ], [ 1.2345678901234567, 1e-17, '1.2345678901234567' ],
		[ 1e308, Number.MIN_VALUE, '1e+308' ]
	];
	for (const [value, accuracy, expected] of cases) {
		testRunner.Assert(Helper.FloatToString(value, accuracy), expected, `Value ${value}, increment ${accuracy}`);
	}
});

testRunner.Test('Reject invalid rounding increments without changing the displayed value', () => {
	const originalError = console.error;
	const errors = [];
	console.error = (...args) => { errors.push(args); };
	try {
		for (const accuracy of [0, -0.1, NaN, Infinity, -Infinity, 'invalid', null]) {
			const previousErrors = errors.length;
			testRunner.Assert(Helper.FloatToString(1.234, accuracy), '1.234');
			testRunner.Assert(errors.length, previousErrors + 1);
		}
	}
	finally {
		console.error = originalError;
	}
});

testRunner.Test('Append descriptions to tooltip titles only when nonempty text is supplied', () => {
	for (const description of [undefined, null, '', 0, false, {}]) {
		testRunner.Assert(Helper.GetCombinedTitle('Price', description), 'Price');
	}
	testRunner.Assert(Helper.GetCombinedTitle('Price', 'In EUR'), 'Price\n\nIn EUR');
	testRunner.Assert(Helper.GetCombinedTitle('Price', 'First line\nSecond line'), 'Price\n\nFirst line\nSecond line');
	testRunner.Assert(Helper.GetCombinedTitle('Price', '<b>Raw text</b>'), 'Price\n\n<b>Raw text</b>');
});

testRunner.Test('Validate numeric limits and emptiness against the raw value', () => {
	const input = document.createElement('input');
	input.type = 'number';
	input.setAttribute('min', '1.24');
	input.setAttribute('max', '1.26');
	input.setAttribute('canBeEmpty', 'false');
	const cases = [
		[ '1.2', '1.245', true ], [ '1.2', '1.239', false ], [ '1.3', '1.261', false ], [ '1.2', '1.24', true ],
		[ '1.3', '1.26', true ], [ '1.2', '', false ]
	];
	for (const [display, rawValue, isValid] of cases) {
		input.value = display;
		input.setAttribute('raw-value', rawValue);
		testRunner.Assert(Helper.ValidateLimits(input), isValid);
		testRunner.Assert(input.classList.contains('invalid'), !isValid);
	}
	input.removeAttribute('raw-value');
	input.value = '1.245';
	testRunner.Assert(Helper.ValidateLimits(input), true);
	input.value = '1.2';
	testRunner.Assert(Helper.ValidateLimits(input), false);
});

testRunner.Run();