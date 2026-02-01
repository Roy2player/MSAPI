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

const { TestRunner } = require("./testRunner");
const Duration = require('../duration');

let testRunner = new TestRunner();

testRunner.SetPostTestFunction(() => {
	//* Trigger global Durations event to clean up if needed
	document.dispatchEvent(new Event("click"));
});

testRunner.Test('Cast input to duration and check', async () => {
	const testData = [
		{
			type : "Days",
			mask : "d",
			min : 1000000000n * 60n * 60n * 24n,
			max : Duration.DAYS_LIMIT * 1000000000n * 60n * 60n * 24n,
			minText : "1d",
			lessThanMinText : "23h 59m 59s 999l 999u 999n",
			maxText : Duration.DAYS_LIMIT + "d",
			greaterThanMaxText : Duration.DAYS_LIMIT + "d" +
				" 1n",
			overflowText : Duration.DAYS_LIMIT + 1n + "d"
		},
		{
			type : "Hours",
			mask : "dh",
			min : 1000000000n * 60n * 60n,
			max : 1000000000n * 60n * 60n * 23n,
			minText : "1h",
			lessThanMinText : "59m 59s 999l 999u 999n",
			maxText : "23h",
			greaterThanMaxText : "23h 1n",
			overflowText : "24h"
		},
		{
			type : "Minutes",
			mask : "dhm",
			min : 1000000000n * 60n,
			max : 1000000000n * 60n * 59n,
			minText : "1m",
			lessThanMinText : "59s 999l 999u 999n",
			maxText : "59m",
			greaterThanMaxText : "59m 1n",
			overflowText : "60m"
		},
		{
			type : "Seconds",
			mask : "dhms",
			min : 1000000000n,
			max : 1000000000n * 59n,
			minText : "1s",
			lessThanMinText : "999l 999u 999n",
			maxText : "59s",
			greaterThanMaxText : "59s 1n",
			overflowText : "60s"
		},
		{
			type : "Milliseconds",
			mask : "dhmsl",
			min : 1000000n,
			max : 999000000n,
			minText : "1l",
			lessThanMinText : "999u 999n",
			maxText : "999l",
			greaterThanMaxText : "999l 1n",
			overflowText : "1000l"
		},
		{
			type : "Microseconds",
			mask : "dhmslu",
			min : 1000n,
			max : 999000n,
			minText : "1u",
			lessThanMinText : "999n",
			maxText : "999u",
			greaterThanMaxText : "999u 1n",
			overflowText : "1000u"
		},
		{
			type : "Nanoseconds",
			mask : "dhmslun",
			min : 1n,
			max : 999n,
			minText : "1n",
			lessThanMinText : "",
			maxText : "999n",
			greaterThanMaxText : "1u",
			overflowText : "1000n"
		}
	];

	for (let index = 0; index < testData.length; index++) {
		TestRunner.Step(`Cast input to duration with type ${testData[index].type} and check`);
		let input = document.createElement("input");

		body.appendChild(input);
		Duration.Apply(input, testData[index].type);
		testRunner.Assert(Duration.GetDurationsNumber(), index == 0 ? 1 : 2);
		testRunner.Assert(input.type, 'text');
		testRunner.Assert(input.getAttribute('durationType'), testData[index].type);
		testRunner.Assert(input.getAttribute('nanoseconds'), '');
		testRunner.Assert(input.value, '');
		testRunner.Assert(input.classList.contains('invalid'), false);
		testRunner.Assert(getEventListeners(input).input.length, 1);
		testRunner.Assert(getEventListeners(document).click.length, 1);
		testRunner.Assert(input.outerHTML, `<input type="text" durationtype="${testData[index].type}" nanoseconds="">`);

		TestRunner.Step(`Set zero value by method for ${testData[index].type}`);
		Duration.SetValue(input, 0n);
		testRunner.Assert(input.value, '', input.outerHTML);
		testRunner.Assert(input.getAttribute('nanoseconds'), 0);
		testRunner.Assert(input.classList.contains('invalid'), true);
		testRunner.Assert(input.outerHTML,
			`<input type="text" durationtype="${testData[index].type}" nanoseconds="0" class="invalid">`);

		TestRunner.Step(`Set negative value by method for ${testData[index].type}`);
		Duration.SetValue(input, testData[index].min * -1n);
		testRunner.Assert(input.value, "-" + testData[index].minText, input.outerHTML);
		testRunner.Assert(input.getAttribute('nanoseconds'), testData[index].min * -1n);
		testRunner.Assert(input.classList.contains('invalid'), false);
		testRunner.Assert(input.outerHTML,
			`<input type="text" durationtype="${testData[index].type}" nanoseconds="-${
				testData[index].min}" class="">`);

		TestRunner.Step(`Set min value by method for ${testData[index].type}`);
		Duration.SetValue(input, testData[index].min);
		testRunner.Assert(input.value, testData[index].minText);
		testRunner.Assert(input.getAttribute('nanoseconds'), testData[index].min.toString());
		testRunner.Assert(input.classList.contains('invalid'), false);
		testRunner.Assert(input.outerHTML,
			`<input type="text" durationtype="${testData[index].type}" nanoseconds="${testData[index].min}" class="">`);

		TestRunner.Step(`Set value by method less than min for ${testData[index].type}`);
		Duration.SetValue(input, testData[index].min - 1n);
		testRunner.Assert(input.value, '');
		testRunner.Assert(input.getAttribute('nanoseconds'), '0');
		testRunner.Assert(input.classList.contains('invalid'), true);
		testRunner.Assert(input.outerHTML,
			`<input type="text" durationtype="${testData[index].type}" nanoseconds="0" class="invalid">`);

		TestRunner.Step(`Set max value by method for ${testData[index].type}`);
		Duration.SetValue(input, testData[index].max);
		testRunner.Assert(input.value, testData[index].maxText);
		testRunner.Assert(input.getAttribute('nanoseconds'), testData[index].max.toString());
		testRunner.Assert(input.classList.contains('invalid'), false);
		testRunner.Assert(input.outerHTML,
			`<input type="text" durationtype="${testData[index].type}" nanoseconds="${testData[index].max}" class="">`);

		TestRunner.Step(`Set value by method greater than max for ${testData[index].type}`);
		Duration.SetValue(input, testData[index].max + 1n);
		if (testData[index].type == 'Nanoseconds') {
			testRunner.Assert(input.value, '1u');
			testRunner.Assert(input.getAttribute('nanoseconds'), '1000');
			testRunner.Assert(input.classList.contains('invalid'), false);
			testRunner.Assert(input.outerHTML,
				`<input type="text" durationtype="${testData[index].type}" nanoseconds="1000" class="">`);
		}
		else if (testData[index].type == 'Days') {
			testRunner.Assert(input.value, Duration.DAYS_LIMIT + 'd');
			testRunner.Assert(
				input.getAttribute('nanoseconds'), (Duration.DAYS_LIMIT * 1000000000n * 60n * 60n * 24n).toString());
			testRunner.Assert(input.classList.contains('invalid'), false);
			testRunner.Assert(input.outerHTML,
				`<input type="text" durationtype="${testData[index].type}" nanoseconds="${
					(Duration.DAYS_LIMIT * 1000000000n * 60n * 60n * 24n).toString()}" class="">`);
		}
		else {
			testRunner.Assert(input.value, testData[index].maxText);
			testRunner.Assert(input.getAttribute('nanoseconds'), testData[index].max.toString());
			testRunner.Assert(input.classList.contains('invalid'), false);
			testRunner.Assert(input.outerHTML,
				`<input type="text" durationtype="${testData[index].type}" nanoseconds="${
					testData[index].max}" class="">`);
		}

		TestRunner.Step(`Set zero value by method for ${testData[index].type}`);
		Duration.SetValue(input, 0n);
		testRunner.Assert(input.value, '');
		testRunner.Assert(input.getAttribute('nanoseconds'), '0');
		testRunner.Assert(input.classList.contains('invalid'), true);
		testRunner.Assert(input.outerHTML,
			`<input type="text" durationtype="${testData[index].type}" nanoseconds="0" class="invalid">`);

		{
			TestRunner.Step(`Set min and max limits and allow to be empty, set value less than min by method for ${
				testData[index].type}`);
			input.setAttribute('canBeEmpty', 'true');
			const min = testData[index].min * 18n;
			const max = testData[index].min * 22n;
			input.setAttribute('min', min);
			input.setAttribute('max', max);

			Duration.SetValue(input, testData[index].min * 17n);
			testRunner.Assert(input.value, 17 + testData[index].mask[testData[index].mask.length - 1]);
			testRunner.Assert(input.getAttribute('nanoseconds'), (testData[index].min * 17n).toString());
			testRunner.Assert(input.classList.contains('invalid'), true);
			testRunner.Assert(input.outerHTML,
				`<input type="text" durationtype="${testData[index].type}" nanoseconds="${
					(testData[index].min * 17n).toString()}" class="invalid" canbeempty="true" min="${min}" max="${
					max}">`);

			TestRunner.Step(`Set value by method equal to min for ${testData[index].type}`);
			Duration.SetValue(input, testData[index].min * 18n);
			testRunner.Assert(input.value, 18 + testData[index].mask[testData[index].mask.length - 1]);
			testRunner.Assert(input.getAttribute('nanoseconds'), (testData[index].min * 18n).toString());
			testRunner.Assert(input.classList.contains('invalid'), false);
			testRunner.Assert(input.outerHTML,
				`<input type="text" durationtype="${testData[index].type}" nanoseconds="${
					(testData[index].min * 18n).toString()}" class="" canbeempty="true" min="${min}" max="${max}">`);

			TestRunner.Step(`Set value by method equal to max for ${testData[index].type}`);
			Duration.SetValue(input, testData[index].min * 22n);
			testRunner.Assert(input.value, 22 + testData[index].mask[testData[index].mask.length - 1]);
			testRunner.Assert(input.getAttribute('nanoseconds'), (testData[index].min * 22n).toString());
			testRunner.Assert(input.classList.contains('invalid'), false);
			testRunner.Assert(input.outerHTML,
				`<input type="text" durationtype="${testData[index].type}" nanoseconds="${
					(testData[index].min * 22n).toString()}" class="" canbeempty="true" min="${min}" max="${max}">`);

			TestRunner.Step(`Set value by method greater than max for ${testData[index].type}`);
			Duration.SetValue(input, testData[index].min * 23n);
			testRunner.Assert(input.value, 23 + testData[index].mask[testData[index].mask.length - 1]);
			testRunner.Assert(input.getAttribute('nanoseconds'), (testData[index].min * 23n).toString());
			testRunner.Assert(input.classList.contains('invalid'), true);
			testRunner.Assert(input.outerHTML,
				`<input type="text" durationtype="${testData[index].type}" nanoseconds="${
					(testData[index].min * 23n).toString()}" class="invalid" canbeempty="true" min="${min}" max="${
					max}">`);

			TestRunner.Step(`Set zero value by method for ${testData[index].type}`);
			Duration.SetValue(input, 0n);
			testRunner.Assert(input.value, '');
			testRunner.Assert(input.getAttribute('nanoseconds'), '0');
			testRunner.Assert(input.classList.contains('invalid'), true);
			testRunner.Assert(input.outerHTML,
				`<input type="text" durationtype="${
					testData[index].type}" nanoseconds="0" class="invalid" canbeempty="true" min="${min}" max="${
					max}">`);

			TestRunner.Step(`Set value between min and max by method for ${testData[index].type}`);
			Duration.SetValue(input, testData[index].min * 19n);
			testRunner.Assert(input.value, 19 + testData[index].mask[testData[index].mask.length - 1]);
			testRunner.Assert(input.getAttribute('nanoseconds'), (testData[index].min * 19n).toString());
			testRunner.Assert(input.classList.contains('invalid'), false);
			testRunner.Assert(input.outerHTML,
				`<input type="text" durationtype="${testData[index].type}" nanoseconds="${
					(testData[index].min * 19n).toString()}" class="" canbeempty="true" min="${min}" max="${max}">`);
		}

		TestRunner.Step(`Remove limits and set negative value by input for ${testData[index].type}`);
		input.removeAttribute('canBeEmpty');
		input.removeAttribute('min');
		input.removeAttribute('max');

		TestRunner.Step(`Set negative value by input for ${testData[index].type}`);
		input.value = "-" + testData[index].minText;
		input.dispatchEvent(new Event("input"));
		body.dispatchEvent(new Event("click", { bubbles : true }));
		testRunner.Assert(input.value, "-" + testData[index].minText);
		testRunner.Assert(input.getAttribute('nanoseconds'), "-" + testData[index].min.toString());
		testRunner.Assert(input.classList.contains('invalid'), false);
		testRunner.Assert(input.outerHTML,
			`<input type="text" durationtype="${testData[index].type}" nanoseconds="-${
				testData[index].min}" class="">`);

		TestRunner.Step(`Set min value by input for ${testData[index].type}`);
		input.value = testData[index].minText;
		input.dispatchEvent(new Event("input"));
		body.dispatchEvent(new Event("click", { bubbles : true }));
		testRunner.Assert(input.value, testData[index].minText);
		testRunner.Assert(input.getAttribute('nanoseconds'), testData[index].min.toString());
		testRunner.Assert(input.classList.contains('invalid'), false);
		testRunner.Assert(input.outerHTML,
			`<input type="text" durationtype="${testData[index].type}" nanoseconds="${testData[index].min}" class="">`);

		TestRunner.Step(`Set value by input less than min for ${testData[index].type}`);
		input.value = testData[index].lessThanMinText;
		input.dispatchEvent(new Event("input"));
		body.dispatchEvent(new Event("click", { bubbles : true }));
		if (testData[index].type != "Nanoseconds") {
			testRunner.Assert(input.value, testData[index].minText);
			testRunner.Assert(input.getAttribute('nanoseconds'), testData[index].min.toString());
			testRunner.Assert(input.classList.contains('invalid'), false);
			testRunner.Assert(input.outerHTML,
				`<input type="text" durationtype="${testData[index].type}" nanoseconds="${
					testData[index].min}" class="">`);
		}
		else {
			testRunner.Assert(input.value, '');
			testRunner.Assert(input.getAttribute('nanoseconds'), '0');
			testRunner.Assert(input.classList.contains('invalid'), true);
			testRunner.Assert(input.outerHTML,
				`<input type="text" durationtype="${testData[index].type}" nanoseconds="0" class="invalid">`);
		}

		TestRunner.Step(`Set too precise value by input for ${testData[index].type}`);
		for (let i = index; i < testData.length; i++) {
			input.value = testData[index].minText + " " + testData[i].lessThanMinText;
			input.dispatchEvent(new Event("input"));
			body.dispatchEvent(new Event("click", { bubbles : true }));
			testRunner.Assert(input.value, testData[index].minText);
			testRunner.Assert(input.getAttribute('nanoseconds'), testData[index].min.toString());
			testRunner.Assert(input.classList.contains('invalid'), false);
			testRunner.Assert(input.outerHTML,
				`<input type="text" durationtype="${testData[index].type}" nanoseconds="${
					testData[index].min}" class="">`);
		}

		TestRunner.Step(`Set different precise value by input for ${testData[index].type}`);
		if (testData[index].type == 'Nanoseconds') {
			input.value = "1d 1h1m  1s 1l 1u   1n";
			input.dispatchEvent(new Event("input"));
			body.dispatchEvent(new Event("click", { bubbles : true }));
			testRunner.Assert(input.value, "1d 1h 1m 1s 1l 1u 1n");
			testRunner.Assert(input.getAttribute('nanoseconds'), "90061001001001");
			testRunner.Assert(input.classList.contains('invalid'), false);
			testRunner.Assert(input.outerHTML,
				`<input type="text" durationtype="${testData[index].type}" nanoseconds="90061001001001" class="">`);
		}
		else if (testData[index].type == 'Microseconds') {
			input.value = "8d30h65m1373s183l";
			input.dispatchEvent(new Event("input"));
			body.dispatchEvent(new Event("click", { bubbles : true }));
			testRunner.Assert(input.value, "8d 23h 59m 59s 183l");
			testRunner.Assert(input.getAttribute('nanoseconds'), "777599183000000");
			testRunner.Assert(input.classList.contains('invalid'), false);
			testRunner.Assert(input.outerHTML,
				`<input type="text" durationtype="${testData[index].type}" nanoseconds="777599183000000" class="">`);
		}
		else if (testData[index].type == 'Milliseconds') {
			input.value = "8d0s    999l";
			input.dispatchEvent(new Event("input"));
			body.dispatchEvent(new Event("click", { bubbles : true }));
			testRunner.Assert(input.value, "8d 999l");
			testRunner.Assert(input.getAttribute('nanoseconds'), "691200999000000");
			testRunner.Assert(input.classList.contains('invalid'), false);
			testRunner.Assert(input.outerHTML,
				`<input type="text" durationtype="${testData[index].type}" nanoseconds="691200999000000" class="">`);
		}
		else if (testData[index].type == 'Seconds') {
			input.value = "8d0h0m0s";
			input.dispatchEvent(new Event("input"));
			body.dispatchEvent(new Event("click", { bubbles : true }));
			testRunner.Assert(input.value, "8d");
			testRunner.Assert(input.getAttribute('nanoseconds'), "691200000000000");
			testRunner.Assert(input.classList.contains('invalid'), false);
			testRunner.Assert(input.outerHTML,
				`<input type="text" durationtype="${testData[index].type}" nanoseconds="691200000000000" class="">`);
		}
		else if (testData[index].type == 'Minutes') {
			input.value = "8d555m";
			input.dispatchEvent(new Event("input"));
			body.dispatchEvent(new Event("click", { bubbles : true }));
			testRunner.Assert(input.value, "8d 59m");
			testRunner.Assert(input.getAttribute('nanoseconds'), "694740000000000");
			testRunner.Assert(input.classList.contains('invalid'), false);
			testRunner.Assert(input.outerHTML,
				`<input type="text" durationtype="${testData[index].type}" nanoseconds="694740000000000" class="">`);
		}
		else if (testData[index].type == 'Hours' || testData[index].type == 'Days') {
			input.value = "8d";
			input.dispatchEvent(new Event("input"));
			body.dispatchEvent(new Event("click", { bubbles : true }));
			testRunner.Assert(input.value, "8d");
			testRunner.Assert(input.getAttribute('nanoseconds'), "691200000000000");
			testRunner.Assert(input.classList.contains('invalid'), false);
			testRunner.Assert(input.outerHTML,
				`<input type="text" durationtype="${testData[index].type}" nanoseconds="691200000000000" class="">`);
		}
		else {
			testRunner.Assert(false, true, `Unexpected type ${testData[index].type}`);
		}

		TestRunner.Step(`Set max value by input for ${testData[index].type}`);
		input.value = testData[index].maxText;
		input.dispatchEvent(new Event("input"));
		body.dispatchEvent(new Event("click", { bubbles : true }));
		testRunner.Assert(input.value, testData[index].maxText);
		testRunner.Assert(input.getAttribute('nanoseconds'), testData[index].max.toString());
		testRunner.Assert(input.classList.contains('invalid'), false);
		testRunner.Assert(input.outerHTML,
			`<input type="text" durationtype="${testData[index].type}" nanoseconds="${testData[index].max}" class="">`);

		TestRunner.Step(`Set value by input greater than max for ${testData[index].type}`);
		input.value = testData[index].greaterThanMaxText;
		input.dispatchEvent(new Event("input"));
		body.dispatchEvent(new Event("click", { bubbles : true }));
		if (testData[index].type == 'Nanoseconds') {
			testRunner.Assert(input.value, '1u');
			testRunner.Assert(input.getAttribute('nanoseconds'), '1000');
			testRunner.Assert(input.classList.contains('invalid'), false);
			testRunner.Assert(input.outerHTML,
				`<input type="text" durationtype="${testData[index].type}" nanoseconds="1000" class="">`);
		}
		else if (testData[index].type == 'Days') {
			testRunner.Assert(input.value, Duration.DAYS_LIMIT + 'd');
			testRunner.Assert(
				input.getAttribute('nanoseconds'), (Duration.DAYS_LIMIT * 1000000000n * 60n * 60n * 24n).toString());
			testRunner.Assert(input.classList.contains('invalid'), false);
			testRunner.Assert(input.outerHTML,
				`<input type="text" durationtype="${testData[index].type}" nanoseconds="${
					(Duration.DAYS_LIMIT * 1000000000n * 60n * 60n * 24n).toString()}" class="">`);
		}
		else {
			testRunner.Assert(input.value, testData[index].maxText);
			testRunner.Assert(input.getAttribute('nanoseconds'), testData[index].max.toString());
			testRunner.Assert(input.classList.contains('invalid'), false);
			testRunner.Assert(input.outerHTML,
				`<input type="text" durationtype="${testData[index].type}" nanoseconds="${
					testData[index].max}" class="">`);
		}

		TestRunner.Step(`Set overflow value by input for ${testData[index].type}`);
		input.value = testData[index].overflowText;
		input.dispatchEvent(new Event("input"));
		body.dispatchEvent(new Event("click", { bubbles : true }));
		testRunner.Assert(input.value, testData[index].maxText);
		testRunner.Assert(input.getAttribute('nanoseconds'), testData[index].max.toString());
		testRunner.Assert(input.classList.contains('invalid'), false);
		testRunner.Assert(input.outerHTML,
			`<input type="text" durationtype="${testData[index].type}" nanoseconds="${testData[index].max}" class="">`);

		TestRunner.Step(`Set zero and unexpected value by input for ${testData[index].type}`);
		{
			const testValues = [ '', '0' ];

			for (let i = 0; i < testValues.length; i++) {
				input.value = testValues[i];
				input.dispatchEvent(new Event("input"));
				body.dispatchEvent(new Event("click", { bubbles : true }));
				testRunner.Assert(input.value, '');
				testRunner.Assert(input.getAttribute('nanoseconds'), '0');
				testRunner.Assert(input.classList.contains('invalid'), true);
				testRunner.Assert(input.outerHTML,
					`<input type="text" durationtype="${testData[index].type}" nanoseconds="0" class="invalid">`);
			}
		}

		{
			TestRunner.Step(`Set min and max limits and allow to be empty, set value less than min by input for ${
				testData[index].type}`);
			input.setAttribute('canBeEmpty', 'true');
			const min = testData[index].min * 18n;
			const max = testData[index].min * 22n;
			input.setAttribute('min', min);
			input.setAttribute('max', max);

			input.value = "17  " + testData[index].mask[testData[index].mask.length - 1] + "   ()#*$";
			input.dispatchEvent(new Event("input"));
			body.dispatchEvent(new Event("click", { bubbles : true }));
			testRunner.Assert(input.value, 17 + testData[index].mask[testData[index].mask.length - 1]);
			testRunner.Assert(input.getAttribute('nanoseconds'), (testData[index].min * 17n).toString());
			testRunner.Assert(input.classList.contains('invalid'), true);
			testRunner.Assert(input.outerHTML,
				`<input type="text" durationtype="${testData[index].type}" nanoseconds="${
					(testData[index].min * 17n).toString()}" class="invalid" canbeempty="true" min="${min}" max="${
					max}">`);

			TestRunner.Step(`Set value by input equal to min for ${testData[index].type}`);
			input.value = "18  " + testData[index].mask[testData[index].mask.length - 1] + "kjdsfh";
			input.dispatchEvent(new Event("input"));
			body.dispatchEvent(new Event("click", { bubbles : true }));
			testRunner.Assert(input.value, 18 + testData[index].mask[testData[index].mask.length - 1]);
			testRunner.Assert(input.getAttribute('nanoseconds'), (testData[index].min * 18n).toString());
			testRunner.Assert(input.classList.contains('invalid'), false);
			testRunner.Assert(input.outerHTML,
				`<input type="text" durationtype="${testData[index].type}" nanoseconds="${
					(testData[index].min * 18n).toString()}" class="" canbeempty="true" min="${min}" max="${max}">`);

			TestRunner.Step(`Set value by input equal to max for ${testData[index].type}`);
			input.value = "f m22" + testData[index].mask[testData[index].mask.length - 1] + " ";
			input.dispatchEvent(new Event("input"));
			body.dispatchEvent(new Event("click", { bubbles : true }));
			testRunner.Assert(input.value, 22 + testData[index].mask[testData[index].mask.length - 1]);
			testRunner.Assert(input.getAttribute('nanoseconds'), (testData[index].min * 22n).toString());
			testRunner.Assert(input.classList.contains('invalid'), false);
			testRunner.Assert(input.outerHTML,
				`<input type="text" durationtype="${testData[index].type}" nanoseconds="${
					(testData[index].min * 22n).toString()}" class="" canbeempty="true" min="${min}" max="${max}">`);

			TestRunner.Step(`Set negative value by input for ${testData[index].type}`);
			input.value = "-fm22" + testData[index].mask[testData[index].mask.length - 1] + " ";
			input.dispatchEvent(new Event("input"));
			body.dispatchEvent(new Event("click", { bubbles : true }));
			testRunner.Assert(input.value, `-${22n + testData[index].mask[testData[index].mask.length - 1]}`);
			testRunner.Assert(input.getAttribute('nanoseconds'), "-" + (testData[index].min * 22n).toString());
			testRunner.Assert(input.classList.contains('invalid'), true);
			testRunner.Assert(input.outerHTML,
				`<input type="text" durationtype="${testData[index].type}" nanoseconds="-${
					(testData[index].min * 22n).toString()}" class="invalid" canbeempty="true" min="${min}" max="${
					max}">`);

			TestRunner.Step(`Set value by input greater than max for ${testData[index].type}`);
			input.value = " lkdfjkg23" + testData[index].mask[testData[index].mask.length - 1] + "23423";
			input.dispatchEvent(new Event("input"));
			body.dispatchEvent(new Event("click", { bubbles : true }));
			testRunner.Assert(input.value, 23 + testData[index].mask[testData[index].mask.length - 1]);
			testRunner.Assert(input.getAttribute('nanoseconds'), (testData[index].min * 23n).toString());
			testRunner.Assert(input.classList.contains('invalid'), true);
			testRunner.Assert(input.outerHTML,
				`<input type="text" durationtype="${testData[index].type}" nanoseconds="${
					(testData[index].min * 23n).toString()}" class="invalid" canbeempty="true" min="${min}" max="${
					max}">`);

			TestRunner.Step(`Set zero value by input for ${testData[index].type}`);
			input.value = "";
			input.dispatchEvent(new Event("input"));
			body.dispatchEvent(new Event("click", { bubbles : true }));
			testRunner.Assert(input.value, '');
			testRunner.Assert(input.getAttribute('nanoseconds'), '0');
			testRunner.Assert(input.classList.contains('invalid'), true);
			testRunner.Assert(input.outerHTML,
				`<input type="text" durationtype="${
					testData[index].type}" nanoseconds="0" class="invalid" canbeempty="true" min="${min}" max="${
					max}">`);

			TestRunner.Step(`Set value between min and max by input for ${testData[index].type}`);
			input.value = "   19" + testData[index].mask[testData[index].mask.length - 1];
			input.dispatchEvent(new Event("input"));
			body.dispatchEvent(new Event("click", { bubbles : true }));
			testRunner.Assert(input.value, 19 + testData[index].mask[testData[index].mask.length - 1]);
			testRunner.Assert(input.getAttribute('nanoseconds'), (testData[index].min * 19n).toString());
			testRunner.Assert(input.classList.contains('invalid'), false);
			testRunner.Assert(input.outerHTML,
				`<input type="text" durationtype="${testData[index].type}" nanoseconds="${
					(testData[index].min * 19n).toString()}" class="" canbeempty="true" min="${min}" max="${max}">`);

			TestRunner.Step(`Set invalid value by input for ${testData[index].type}`);
			{
				const testValues = [ '48k94l384d834lf834', '58f', testData[index].mask, '48', ' ', 'sdkjf', ' 0.5' ];

				for (let i = 0; i < testValues.length; i++) {
					input.value = testValues[i];
					input.dispatchEvent(new Event("input"));
					body.dispatchEvent(new Event("click", { bubbles : true }));
					testRunner.Assert(input.value, 19 + testData[index].mask[testData[index].mask.length - 1]);
					testRunner.Assert(input.getAttribute('nanoseconds'), (testData[index].min * 19n).toString());
					testRunner.Assert(input.classList.contains('invalid'), false);
					testRunner.Assert(input.outerHTML,
						`<input type="text" durationtype="${testData[index].type}" nanoseconds="${
							(testData[index].min * 19n).toString()}" class="" canbeempty="true" min="${min}" max="${
							max}">`);
				}
			}
		}

		testRunner.Assert(getEventListeners(input).input.length, 1);
		testRunner.Assert(getEventListeners(document).click.length, 1);
		input.remove();
	}
});

testRunner.Test('Verify negative minimum of days', async () => {
	TestRunner.Step(`Cast input to duration with type Milliseconds and check`);
	let input = document.createElement("input");

	body.appendChild(input);
	Duration.Apply(input, "Milliseconds");
	testRunner.Assert(Duration.GetDurationsNumber(), 1);
	testRunner.Assert(input.type, 'text');
	testRunner.Assert(input.getAttribute('durationType'), "Milliseconds");
	testRunner.Assert(input.getAttribute('nanoseconds'), '');
	testRunner.Assert(input.value, '');
	testRunner.Assert(input.classList.contains('invalid'), false);
	testRunner.Assert(getEventListeners(input).input.length, 1);
	testRunner.Assert(getEventListeners(document).click.length, 1);
	testRunner.Assert(input.outerHTML, `<input type="text" durationtype="Milliseconds" nanoseconds="">`);

	TestRunner.Step(`Set negative value by method`);
	Duration.SetValue(input, -1000000000n * 60n * 60n * 24n);
	testRunner.Assert(input.value, '-1d');
	testRunner.Assert(input.getAttribute('nanoseconds'), '-86400000000000');
	testRunner.Assert(input.classList.contains('invalid'), false);
	testRunner.Assert(input.outerHTML, `<input type="text" durationtype="Milliseconds" nanoseconds="-86400000000000">`);

	TestRunner.Step(`Set minimum negative value by method`);
	Duration.SetValue(input, -1000000000n * 60n * 60n * 24n * Duration.DAYS_LIMIT - 1000000000n);
	testRunner.Assert(input.value, '-' + Duration.DAYS_LIMIT + 'd');
	testRunner.Assert(
		input.getAttribute('nanoseconds'), '-' + (Duration.DAYS_LIMIT * 1000000000n * 60n * 60n * 24n).toString());
	testRunner.Assert(input.classList.contains('invalid'), false);
	testRunner.Assert(input.outerHTML,
		`<input type="text" durationtype="Milliseconds" nanoseconds="${
			- (Duration.DAYS_LIMIT * 1000000000n * 60n * 60n * 24n).toString()}">`);

	TestRunner.Step(`Set zero value directly`);
	input.value = '';
	input.dispatchEvent(new Event("input"));
	document.dispatchEvent(new Event("click"));
	testRunner.Assert(input.value, '');
	testRunner.Assert(input.getAttribute('nanoseconds'), '0');
	testRunner.Assert(input.classList.contains('invalid'), true);
	testRunner.Assert(
		input.outerHTML, `<input type="text" durationtype="Milliseconds" nanoseconds="0" class="invalid">`);

	TestRunner.Step(`Set minimum negative value by input`);
	input.value = '-' + Duration.DAYS_LIMIT + 'd 1l';
	input.dispatchEvent(new Event("input"));
	document.dispatchEvent(new Event("click"));
	testRunner.Assert(input.value, '-' + Duration.DAYS_LIMIT + 'd');
	testRunner.Assert(input.getAttribute('nanoseconds'), `-${Duration.DAYS_LIMIT * 1000000000n * 60n * 60n * 24n}`);
	testRunner.Assert(input.classList.contains('invalid'), false);
	testRunner.Assert(input.outerHTML,
		`<input type="text" durationtype="Milliseconds" nanoseconds="${
			- (Duration.DAYS_LIMIT * 1000000000n * 60n * 60n * 24n).toString()}" class="">`);
});

testRunner.Run();