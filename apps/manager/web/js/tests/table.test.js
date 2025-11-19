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
const Table = require('../table');
const Application = require('../application');
const viewTemplates = require('../viewTemplates');
const MetadataCollector = require('../metadataCollector');

global.dispatcher = undefined;
let testRunner = new TestRunner();

class RowChecking {
	static CheckInvalidStates(inputs, expectedStates)
	{
		if (inputs.length != expectedStates.length) {
			testRunner.Assert(false, true, `Incorrect number of inputs: ${expectedStates.length}`);
		}

		for (let i = 0; i < expectedStates.length; i++) {
			testRunner.Assert(inputs[i].classList.contains("invalid"), expectedStates[i],
				`Child ${i + 1} invalid state should be ${expectedStates[i]}. Node: ${inputs[i].outerHTML}`);
		}
	}

	static SetValues(inputs, values)
	{
		if (inputs.length != values.length) {
			testRunner.Assert(false, true, `Incorrect number of inputs: ${values.length}`);
		}

		for (let i = 0; i < values.length; i++) {
			if (typeof values[i] === 'boolean') {
				inputs[i].checked = values[i];
			}
			else {
				inputs[i].value = values[i];
			}

			if (getEventListeners(inputs[i]).change != undefined) {
				inputs[i].dispatchEvent(new Event('change'));

				if (inputs[i].hasAttribute("nanoseconds") || inputs[i].hasAttribute("timestamp")) {
					inputs[i].dispatchEvent(new Event('input'));
				}
				else {
					testRunner.Assert(getEventListeners(inputs[i]).input == undefined, true,
						`Table input cell cannot contain input event listener if change event listener is available`);
				}
			}
			else if (getEventListeners(inputs[i]).input != undefined) {
				inputs[i].dispatchEvent(new Event('input'));
			}

			//* Trigger global event listeners in case if input type has it
			document.dispatchEvent(new Event('click'));
		}
	}

	static CheckValues(inputs, values)
	{
		if (inputs.length != values.length) {
			testRunner.Assert(false, true, `Incorrect number of children: ${values.length}`);
		}

		for (let i = 0; i < values.length; i++) {
			if (typeof values[i] === 'boolean') {
				if (inputs[i].checked.toString() != values[i].toString()) {
					testRunner.Assert(false, true,
						`Child ${i + 1} should have the correct value: ${values[i]}, but has ${inputs[i].checked}`);
				}
			}
			else {
				if (inputs[i].value.toString() != values[i].toString()) {
					testRunner.Assert(false, true,
						`Child ${i + 1} should have the correct value: ${values[i]}, but has ${inputs[i].value}`);
				}
			}
		}
	}

	static CheckAttributes(inputs, values)
	{
		if (inputs.length != values.length) {
			testRunner.Assert(false, true, `Incorrect number of children: ${values.length}`);
		}

		for (let i = 0; i < values.length; i++) {
			if (values[i].type !== undefined) {
				testRunner.Assert(inputs[i].getAttribute("type"), values[i].type,
					`Input ${i} should have the correct type ${inputs[i].outerHTML}`);
			}

			if (values[i].canBeEmpty !== undefined) {
				if (values[i].type == "checkbox") {
					testRunner.Assert(false, true, `Input ${i} should not have canBeEmpty attribute`);
				}

				testRunner.Assert(inputs[i].getAttribute("canBeEmpty"), values[i].canBeEmpty.toString(),
					`Input ${i} should have the correct canBeEmpty attribute ${inputs[i].outerHTML}`);
			}

			if (values[i].step !== undefined) {
				testRunner.Assert(inputs[i].getAttribute("step"), values[i].step,
					`Input ${i} should have the correct step attribute`);
			}

			if (values[i].min !== undefined) {
				testRunner.Assert(
					inputs[i].getAttribute("min"), values[i].min, `Input ${i} should have the correct min attribute`);
			}

			if (values[i].max !== undefined) {
				testRunner.Assert(
					inputs[i].getAttribute("max"), values[i].max, `Input ${i} should have the correct max attribute`);
			}

			if (values[i].durationType !== undefined) {
				testRunner.Assert(inputs[i].getAttribute("durationType"), values[i].durationType,
					`Input ${i} should have the correct durationType attribute`);
			}

			if (values[i].readonly !== undefined) {
				testRunner.Assert(inputs[i].getAttribute("readonly"), values[i].readonly.toString(),
					`Input ${i} should have the correct readonly attribute`);
			}
		}
	}
}

function DestroyTables()
{
	const tables = body.querySelectorAll('.tableWrapper');
	for (let table of tables) {
		table.remove();
	}
}

testRunner.SetPostTestFunction(DestroyTables);

testRunner.Test('Create mutable and immutable tables', () => {
	const columns = {
		"0" : { type : 'Int8', name : 'Name one', min : 0, max : 100 },
		"1" : { type : 'Int16', name : 'Name two', min : 0 },
		"2" : { type : 'Int32', name : 'Name three', max : 100 },
		"3" : { type : 'Int64', name : 'Name four' },
		"4" : { type : 'Uint8', name : 'Name five' },
		"5" : { type : 'Uint16', name : 'Name six' },
		"6" : { type : 'Uint32', name : 'Name seven' },
		"7" : { type : 'Uint64', name : 'Name eight' },
		"8" : { type : 'Float', name : 'Name nine' },
		"9" : { type : 'Double', name : 'Name ten' },
		"10" : { type : 'Double', name : 'Name eleven' },
		"11" : { type : 'OptionalInt8', name : 'Name twelve', min : 0, max : 100, canBeEmpty : true },
		"12" : { type : 'OptionalInt16', name : 'Name thirteen', min : 0, canBeEmpty : true },
		"13" : { type : 'OptionalInt32', name : 'Name fourteen', max : 100, canBeEmpty : true },
		"14" : { type : 'OptionalInt64', name : 'Name fifteen', canBeEmpty : true },
		"15" : { type : 'OptionalUint8', name : 'Name sixteen', canBeEmpty : false },
		"16" : { type : 'OptionalUint16', name : 'Name seventeen', canBeEmpty : false },
		"17" : { type : 'OptionalUint32', name : 'Name eighteen' },
		"18" : { type : 'OptionalUint64', name : 'Name nineteen' },
		"19" : { type : 'OptionalFloat', name : 'Name twenty' },
		"20" : { type : 'OptionalDouble', name : 'Name twenty one' },
		"21" : { type : 'OptionalDouble', name : 'Name twenty two' },
		"22" : { type : 'Bool', name : 'Name twenty three' },
		"23" : { type : 'String', name : 'Name twenty four' },
		"24" : { type : 'Timer', name : 'Name twenty five' },
		"25" : { type : 'Duration', name : 'Name twenty six', canBeEmpty : false, durationType : 'Seconds' }
	};

	//* Table with all columns, mutable
	const parameter1 = { name : 'Important data', type : 'TableData', canBeEmpty : false, columns };

	const table1 = new Table({ parent : body, id : 30015, metadata : parameter1, isMutable : true });
	const tableNode1 = body.querySelectorAll('.tableWrapper')[0];
	testRunner.Assert(tableNode1 != null, true, "Table node should not be null");

	testRunner.Assert(table1.m_parent, body, "Parent should be set correctly");
	testRunner.Assert(table1.m_metadata, parameter1, "Metadata should be set correctly");
	testRunner.Assert(table1.m_isMutable, true, "isMutable should be set correctly");

	testRunner.Assert(
		tableNode1.getAttribute("parameter-id"), "30015", "Table wrapper should have the correct parameter-id");

	const tableHeader1 = tableNode1.querySelector("div > .header");
	testRunner.Assert(tableHeader1 != null, true, "Table header should not be null");
	testRunner.Assert(tableHeader1.textContent, "Important data (30015)", "Table header should have the correct text");

	const headerRow1 = tableNode1.querySelector(".header.row");
	testRunner.Assert(headerRow1 != null, true, "Header row should not be null");
	const headerColumns1 = headerRow1.querySelectorAll("span");
	testRunner.Assert(headerColumns1.length, 26, "Header row should have the correct number of columns");

	const expectedHeaderTexts = [
		"Name one (0) Int8", "Name two (1) Int16", "Name three (2) Int32", "Name four (3) Int64", "Name five (4) Uint8",
		"Name six (5) Uint16", "Name seven (6) Uint32", "Name eight (7) Uint64", "Name nine (8) Float",
		"Name ten (9) Double", "Name eleven (10) Double", "Name twelve (11) OptionalInt8",
		"Name thirteen (12) OptionalInt16", "Name fourteen (13) OptionalInt32", "Name fifteen (14) OptionalInt64",
		"Name sixteen (15) OptionalUint8", "Name seventeen (16) OptionalUint16", "Name eighteen (17) OptionalUint32",
		"Name nineteen (18) OptionalUint64", "Name twenty (19) OptionalFloat", "Name twenty one (20) OptionalDouble",
		"Name twenty two (21) OptionalDouble", "Name twenty three (22) Bool", "Name twenty four (23) String",
		"Name twenty five (24) Timer", "Name twenty six (25) Duration, Seconds"
	];

	for (let i = 0; i < headerColumns1.length; i++) {
		testRunner.Assert(
			headerColumns1[i].textContent, expectedHeaderTexts[i], `Header column ${i} should have the correct text`);
	}

	let rows1 = tableNode1.querySelectorAll(".row");
	testRunner.Assert(rows1.length, 2, "Table should have the correct number of rows: " + 2);

	const newRow1 = tableNode1.querySelector(".new.row");
	testRunner.Assert(newRow1 != null, true, "New row should not be null");
	const newInputs1 = newRow1.querySelectorAll("input");
	testRunner.Assert(newInputs1.length, 26, "New row should have the correct number of inputs");

	const expectedInputValues = [
		{ type : "number", canBeEmpty : false, step : 1, min : 0, max : 100 },
		{ type : "number", canBeEmpty : false, step : 1, min : 0, max : Helper.TYPES_LIMITS.int_16.max },
		{ type : "number", canBeEmpty : false, step : 1, min : Helper.TYPES_LIMITS.int_32.min, max : 100 }, {
			type : "number",
			canBeEmpty : false,
			step : 1,
			min : Helper.TYPES_LIMITS.int_64.min,
			max : Helper.TYPES_LIMITS.int_64.max
		},
		{
			type : "number",
			canBeEmpty : false,
			step : 1,
			min : Helper.TYPES_LIMITS.uint_8.min,
			max : Helper.TYPES_LIMITS.uint_8.max
		},
		{
			type : "number",
			canBeEmpty : false,
			step : 1,
			min : Helper.TYPES_LIMITS.uint_16.min,
			max : Helper.TYPES_LIMITS.uint_16.max
		},
		{
			type : "number",
			canBeEmpty : false,
			step : 1,
			min : Helper.TYPES_LIMITS.uint_32.min,
			max : Helper.TYPES_LIMITS.uint_32.max
		},
		{
			type : "number",
			canBeEmpty : false,
			step : 1,
			min : Helper.TYPES_LIMITS.uint_64.min,
			max : Helper.TYPES_LIMITS.uint_64.max
		},
		{
			type : "number",
			canBeEmpty : false,
			step : 0.01,
			min : Helper.TYPES_LIMITS.float_32.min,
			max : Helper.TYPES_LIMITS.float_32.max
		},
		{
			type : "number",
			canBeEmpty : false,
			step : 0.01,
			min : Helper.TYPES_LIMITS.float_64.min,
			max : Helper.TYPES_LIMITS.float_64.max
		},
		{
			type : "number",
			canBeEmpty : false,
			step : 0.01,
			min : Helper.TYPES_LIMITS.float_64.min,
			max : Helper.TYPES_LIMITS.float_64.max
		},
		{ type : "number", canBeEmpty : true, step : 1, min : 0, max : 100 },
		{ type : "number", canBeEmpty : true, step : 1, min : 0, max : Helper.TYPES_LIMITS.int_16.max },
		{ type : "number", canBeEmpty : true, step : 1, min : Helper.TYPES_LIMITS.int_32.min, max : 100 }, {
			type : "number",
			canBeEmpty : true,
			step : 1,
			min : Helper.TYPES_LIMITS.int_64.min,
			max : Helper.TYPES_LIMITS.int_64.max
		},
		{
			type : "number",
			canBeEmpty : false,
			step : 1,
			min : Helper.TYPES_LIMITS.uint_8.min,
			max : Helper.TYPES_LIMITS.uint_8.max
		},
		{
			type : "number",
			canBeEmpty : false,
			step : 1,
			min : Helper.TYPES_LIMITS.uint_16.min,
			max : Helper.TYPES_LIMITS.uint_16.max
		},
		{
			type : "number",
			canBeEmpty : true,
			step : 1,
			min : Helper.TYPES_LIMITS.uint_32.min,
			max : Helper.TYPES_LIMITS.uint_32.max
		},
		{
			type : "number",
			canBeEmpty : true,
			step : 1,
			min : Helper.TYPES_LIMITS.uint_64.min,
			max : Helper.TYPES_LIMITS.uint_64.max
		},
		{
			type : "number",
			canBeEmpty : true,
			step : 0.01,
			min : Helper.TYPES_LIMITS.float_32.min,
			max : Helper.TYPES_LIMITS.float_32.max
		},
		{
			type : "number",
			canBeEmpty : true,
			step : 0.01,
			min : Helper.TYPES_LIMITS.float_64.min,
			max : Helper.TYPES_LIMITS.float_64.max
		},
		{
			type : "number",
			canBeEmpty : true,
			step : 0.01,
			min : Helper.TYPES_LIMITS.float_64.min,
			max : Helper.TYPES_LIMITS.float_64.max
		},
		{ type : "checkbox" }, { type : "text", canBeEmpty : true }, { type : "text", canBeEmpty : true },
		{ type : "text", canBeEmpty : false, durationType : "Seconds" }
	];

	RowChecking.CheckAttributes(newInputs1, expectedInputValues);

	const newButton1 = newRow1.querySelector(".add");
	testRunner.Assert(newButton1 != null, true, "New row should have a button");

	const buttons1 = tableNode1.querySelector(".buttons");
	testRunner.Assert(buttons1 != null, true, "Buttons should not be null");
	testRunner.Assert(buttons1.children.length, 3, "Buttons should have the correct number of children");
	testRunner.Assert(buttons1.children[0].textContent, "Clear", "First button should have the correct text");
	testRunner.Assert(buttons1.children[1].textContent, "Revert", "Second button should have the correct text");
	testRunner.Assert(buttons1.children[2].textContent, "Save", "Third button should have the correct text");

	//* Table with all columns, immutable
	const parameter2 = { name : 'Important data', type : 'TableData', canBeEmpty : false, columns };

	const table2 = new Table({ parent : body, id : 30016, metadata : parameter2, isMutable : false });
	const tableNode2 = body.querySelectorAll('.tableWrapper')[1];
	testRunner.Assert(tableNode2 != null, true, "Table node should not be null");

	testRunner.Assert(table2.m_parent, body, "Parent should be set correctly");
	testRunner.Assert(table2.m_metadata, parameter2, "Metadata should be set correctly");
	testRunner.Assert(table2.m_isMutable, false, "isMutable should be set correctly");

	testRunner.Assert(
		tableNode2.getAttribute("parameter-id"), "30016", "Table wrapper should have the correct parameter-id");

	const tableHeader2 = tableNode2.querySelector("div > .header");
	testRunner.Assert(tableHeader2 != null, true, "Table header should not be null");
	testRunner.Assert(tableHeader2.textContent, "Important data (30016)", "Table header should have the correct text");

	const headerRow2 = tableNode2.querySelector(".header.row");
	testRunner.Assert(headerRow2 != null, true, "Header row should not be null");
	const headerColumns2 = headerRow2.querySelectorAll("span");
	testRunner.Assert(headerColumns2.length, 26, "Header row should have the correct number of columns");

	for (let i = 0; i < headerColumns2.length; i++) {
		testRunner.Assert(
			headerColumns2[i].textContent, expectedHeaderTexts[i], `Header column ${i} should have the correct text`);
	}

	let rows2 = tableNode2.querySelectorAll(".row");
	testRunner.Assert(rows2.length, 2, "Table should have the correct number of rows: " + 2);

	const newRow2 = tableNode2.querySelector(".new.row");
	testRunner.Assert(newRow2 != null, true, "New row should not be null");
	const newInputs2 = newRow2.querySelectorAll("input");
	testRunner.Assert(newInputs2.length, 26, "New row should have the correct number of inputs");

	RowChecking.CheckAttributes(newInputs2, expectedInputValues);
	RowChecking.CheckAttributes(newInputs2, [
		{ readonly : true }, { readonly : true }, { readonly : true }, { readonly : true }, { readonly : true },
		{ readonly : true }, { readonly : true }, { readonly : true }, { readonly : true }, { readonly : true },
		{ readonly : true }, { readonly : true }, { readonly : true }, { readonly : true }, { readonly : true },
		{ readonly : true }, { readonly : true }, { readonly : true }, { readonly : true }, { readonly : true },
		{ readonly : true }, { readonly : true }, { readonly : true }, { readonly : true }, { readonly : true },
		{ readonly : true }
	]);

	const newButton2 = newRow2.querySelector(".add");
	testRunner.Assert(newButton2, null, "New row should not have a button");

	const buttons2 = tableNode2.querySelector(".buttons");
	testRunner.Assert(buttons2, null, "Buttons should be null");
});

testRunner.Test('Add row via button and directly', () => {
	const parameter = {
		name : 'Commissions',
		type : 'TableData',
		canBeEmpty : false,
		columns : {
			"0" : { type : 'Int8' },
			"1" : { type : 'Int16' },
			"2" : { type : 'Int32' },
			"3" : { type : 'Int64' },
			"4" : { type : 'Uint8' },
			"5" : { type : 'Uint16' },
			"6" : { type : 'Uint32' },
			"7" : { type : 'Uint64' },
			"8" : { type : 'Float' },
			"9" : { type : 'Double' },
			"10" : { type : 'Double' },
			"11" : { type : 'OptionalInt8' },
			"12" : { type : 'OptionalInt16' },
			"13" : { type : 'OptionalInt32' },
			"14" : { type : 'OptionalInt64' },
			"15" : { type : 'OptionalUint8' },
			"16" : { type : 'OptionalUint16' },
			"17" : { type : 'OptionalUint32' },
			"18" : { type : 'OptionalUint64' },
			"19" : { type : 'OptionalFloat' },
			"20" : { type : 'OptionalDouble' },
			"21" : { type : 'OptionalDouble', canBeEmpty : false },
			"22" : { type : 'Bool' },
			"23" : { type : 'String', canBeEmpty : false },
			"24" : { type : 'Timer', canBeEmpty : false },
			"25" : { type : 'Duration', canBeEmpty : false },
		}
	};

	const table = new Table({ parent : body, id : 30014, metadata : parameter, isMutable : true });
	const tableNode = body.querySelector('.tableWrapper');
	testRunner.Assert(tableNode != null, true, "Table node should not be null");

	let rows = tableNode.querySelectorAll(".row");
	testRunner.Assert(rows.length, 2, "Table should have the correct number of rows: " + 2);

	//* Try to add row with empty values
	table.AddRow();
	testRunner.Assert(tableNode.classList.contains("changed"), false, "Table should not be marked as changed");
	rows = tableNode.querySelectorAll(".row");
	testRunner.Assert(rows.length, 2, "Table should have the correct number of rows: " + 2);
	let newRow = tableNode.querySelector(".row.new");
	testRunner.Assert(newRow != null, true, "New row should not be null");
	let newInputs = newRow.querySelectorAll("input");
	testRunner.Assert(newInputs.length, 26, "New row should have the correct number of inputs");
	RowChecking.CheckInvalidStates(newInputs, [
		true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false,
		false, false, false, false, true, false, true, true, true
	]);

	//* Try to add row with invalid values
	RowChecking.SetValues(newInputs, [
		"3123fd", -2, 3, 4, 5, 6, 7, 8, 9, 10, -11, 12, 13, 14, 15, 16, 17, 18, 19, 20.003, -21.002, 22.001, true,
		"text", -532564, "2"
	]);
	table.AddRow();
	testRunner.Assert(tableNode.classList.contains("changed"), false, "Table should not be marked as changed");
	RowChecking.CheckInvalidStates(newInputs, [
		true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false, true, true
	]);
	rows = tableNode.querySelectorAll(".row");
	testRunner.Assert(rows.length, 2, "Table should have the correct number of rows: " + 2);

	//* Try to add row with valid values
	RowChecking.SetValues(newInputs, [
		1, -2, 3, 4, 5, 6, 7, 8, 9, 10, -11, 12, 13, 14, 15, 16, 17, 18, 19, 20.003, -21.002, 22.001, true, "text",
		"2021-01-01", "44m"
	]);
	newRow.querySelector(".add").dispatchEvent(new Event('click', { bubbles : true }));
	testRunner.Assert(tableNode.classList.contains("changed"), true, "Table should be marked as changed");

	RowChecking.CheckInvalidStates(newInputs, [
		false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false, false, false
	]);

	rows = tableNode.querySelectorAll(".row");
	testRunner.Assert(rows.length, 3, "Table should have the correct number of rows: " + 3);
	testRunner.Assert(rows[1].classList.contains("added"), true, "New row should be marked as added");
	testRunner.Assert(rows[1].childNodes.length, 27, "New row should have the correct number of children");
	let addedInputs = rows[1].querySelectorAll("input");
	testRunner.Assert(addedInputs.length, 26, "New row should have the correct number of inputs");
	RowChecking.CheckValues(addedInputs, [
		1, -2, 3, 4, 5, 6, 7, 8, 9, 10, -11, 12, 13, 14, 15, 16, 17, 18, 19, 20.003, -21.002, 22.001, true, "text",
		`2021-01-01 00:00:00.000000000 UTC${TestRunner.UTC}`, "44m"
	]);

	const expectedValues = [
		{
			type : "number",
			canBeEmpty : false,
			step : 1,
			min : Helper.TYPES_LIMITS.int_8.min,
			max : Helper.TYPES_LIMITS.int_8.max
		},
		{
			type : "number",
			canBeEmpty : false,
			step : 1,
			min : Helper.TYPES_LIMITS.int_16.min,
			max : Helper.TYPES_LIMITS.int_16.max
		},
		{
			type : "number",
			canBeEmpty : false,
			step : 1,
			min : Helper.TYPES_LIMITS.int_32.min,
			max : Helper.TYPES_LIMITS.int_32.max
		},
		{
			type : "number",
			canBeEmpty : false,
			step : 1,
			min : Helper.TYPES_LIMITS.int_64.min,
			max : Helper.TYPES_LIMITS.int_64.max
		},
		{
			type : "number",
			canBeEmpty : false,
			step : 1,
			min : Helper.TYPES_LIMITS.uint_8.min,
			max : Helper.TYPES_LIMITS.uint_8.max
		},
		{
			type : "number",
			canBeEmpty : false,
			step : 1,
			min : Helper.TYPES_LIMITS.uint_16.min,
			max : Helper.TYPES_LIMITS.uint_16.max
		},
		{
			type : "number",
			canBeEmpty : false,
			step : 1,
			min : Helper.TYPES_LIMITS.uint_32.min,
			max : Helper.TYPES_LIMITS.uint_32.max
		},
		{
			type : "number",
			canBeEmpty : false,
			step : 1,
			min : Helper.TYPES_LIMITS.uint_64.min,
			max : Helper.TYPES_LIMITS.uint_64.max
		},
		{
			type : "number",
			canBeEmpty : false,
			step : 0.01,
			min : Helper.TYPES_LIMITS.float_32.min,
			max : Helper.TYPES_LIMITS.float_32.max
		},
		{
			type : "number",
			canBeEmpty : false,
			step : 0.01,
			min : Helper.TYPES_LIMITS.float_64.min,
			max : Helper.TYPES_LIMITS.float_64.max
		},
		{
			type : "number",
			canBeEmpty : false,
			step : 0.01,
			min : Helper.TYPES_LIMITS.float_64.min,
			max : Helper.TYPES_LIMITS.float_64.max
		},
		{
			type : "number",
			canBeEmpty : true,
			step : 1,
			min : Helper.TYPES_LIMITS.int_8.min,
			max : Helper.TYPES_LIMITS.int_8.max
		},
		{
			type : "number",
			canBeEmpty : true,
			step : 1,
			min : Helper.TYPES_LIMITS.int_16.min,
			max : Helper.TYPES_LIMITS.int_16.max
		},
		{
			type : "number",
			canBeEmpty : true,
			step : 1,
			min : Helper.TYPES_LIMITS.int_32.min,
			max : Helper.TYPES_LIMITS.int_32.max
		},
		{
			type : "number",
			canBeEmpty : true,
			step : 1,
			min : Helper.TYPES_LIMITS.int_64.min,
			max : Helper.TYPES_LIMITS.int_64.max
		},
		{
			type : "number",
			canBeEmpty : true,
			step : 1,
			min : Helper.TYPES_LIMITS.uint_8.min,
			max : Helper.TYPES_LIMITS.uint_8.max
		},
		{
			type : "number",
			canBeEmpty : true,
			step : 1,
			min : Helper.TYPES_LIMITS.uint_16.min,
			max : Helper.TYPES_LIMITS.uint_16.max
		},
		{
			type : "number",
			canBeEmpty : true,
			step : 1,
			min : Helper.TYPES_LIMITS.uint_32.min,
			max : Helper.TYPES_LIMITS.uint_32.max
		},
		{
			type : "number",
			canBeEmpty : true,
			step : 1,
			min : Helper.TYPES_LIMITS.uint_64.min,
			max : Helper.TYPES_LIMITS.uint_64.max
		},
		{
			type : "number",
			canBeEmpty : true,
			step : 0.01,
			min : Helper.TYPES_LIMITS.float_32.min,
			max : Helper.TYPES_LIMITS.float_32.max
		},
		{
			type : "number",
			canBeEmpty : true,
			step : 0.01,
			min : Helper.TYPES_LIMITS.float_64.min,
			max : Helper.TYPES_LIMITS.float_64.max
		},
		{
			type : "number",
			canBeEmpty : false,
			step : 0.01,
			min : Helper.TYPES_LIMITS.float_64.min,
			max : Helper.TYPES_LIMITS.float_64.max
		},
		{ type : "checkbox" }, { type : "text", canBeEmpty : false }, { type : "text", canBeEmpty : false }, {
			type : "text",
			canBeEmpty : false,
		}
	];
	RowChecking.CheckAttributes(newInputs, expectedValues);

	//* Try to add row with valid values
	table.AddRow([
		1, -2, 3, 4, 5, 6, 7, 8, 9, 10, -11, 12, 13, 14, 15, 16, 17, 18, 19, 20.003, -21.002, 22.001, true, "text",
		1609459200000000000n, 130000000000
	]);
	testRunner.Assert(tableNode.classList.contains("changed"), true, "Table should be marked as changed");
	rows = tableNode.querySelectorAll(".row");
	testRunner.Assert(rows.length, 4, "Table should have the correct number of rows: " + 4);
	testRunner.Assert(rows[2].classList.contains("added"), true, "New row should be marked as added");
	addedInputs = rows[2].querySelectorAll("input");
	testRunner.Assert(addedInputs.length, 26, "New row should have the correct number of inputs");
	RowChecking.CheckValues(addedInputs, [
		1, -2, 3, 4, 5, 6, 7, 8, 9, 10, -11, 12, 13, 14, 15, 16, 17, 18, 19, 20.003, -21.002, 22.001, true, "text",
		`2021-01-01 ${TestRunner.GetHourWithUtcShift(2021, 0, 0)}:00:00.000000000 UTC${TestRunner.UTC}`, "2m 10s"
	]);
	RowChecking.CheckAttributes(newInputs, expectedValues);
	//* Add row with invalid values directly
	table.AddRow([
		1, -2, 3, 4, 5, 6, 7, 8, 9, 10, -11, 12, 13, 14, 15, 16, 17, 18, 19, 20.003, -21.002, 22.001, "text", "text",
		-532564, 33
	]);
	testRunner.Assert(tableNode.classList.contains("changed"), true, "Table should be marked as changed");
	rows = tableNode.querySelectorAll(".row");
	testRunner.Assert(rows.length, 5, "Table should have the correct number of rows: " + 5);
});

testRunner.Test('Remove row', () => {
	const parameter = {
		name : 'Commissions',
		type : 'TableData',
		canBeEmpty : false,
		columns : {
			"0" : { name : "Type", type : 'Int16' },
			"1" : { name : "%", type : 'Double' },
		}
	};

	const table = new Table({ parent : body, id : 30014, metadata : parameter, isMutable : true });
	const tableNode = body.querySelector('.tableWrapper');
	testRunner.Assert(tableNode != null, true, "Table node should not be null");

	let rows = tableNode.querySelectorAll(".row");
	testRunner.Assert(rows.length, 2, "Table should have the correct number of rows: " + 2);

	//* Add row
	table.AddRow([ 1, 2.5 ]);
	testRunner.Assert(tableNode.classList.contains("changed"), true, "Table should be marked as changed");
	rows = tableNode.querySelectorAll(".row");
	testRunner.Assert(rows.length, 3, "Table should have the correct number of rows: " + 3);
	testRunner.Assert(rows[1].classList.contains("added"), true, "New row should be marked as added");

	//* Remove row
	rows[1].querySelector(".remove").dispatchEvent(new Event('click', { bubbles : true }));
	testRunner.Assert(tableNode.classList.contains("changed"), false, "Table should not be marked as changed");
	rows = tableNode.querySelectorAll(".row");
	testRunner.Assert(rows.length, 2, "Table should have the correct number of rows: " + 2);

	//* Add two rows
	table.AddRow([ 1, 2.5 ]);
	table.RemoveRow({ index : 1 });
	table.AddRow([ 2, 3.5 ]);
	table.RemoveRow({ index : 2 });
	testRunner.Assert(tableNode.classList.contains("changed"), true, "Table should be marked as changed");
	rows = tableNode.querySelectorAll(".row");
	testRunner.Assert(rows.length, 4, "Table should have the correct number of rows: " + 4);
	testRunner.Assert(rows[1].classList.contains("added"), true, "New row should be marked as added");
	testRunner.Assert(rows[2].classList.contains("added"), true, "New row should be marked as added");

	//* Remove first row
	rows[1].querySelector(".remove").dispatchEvent(new Event('click', { bubbles : true }));
	testRunner.Assert(tableNode.classList.contains("changed"), true, "Table should be marked as changed");
	rows = tableNode.querySelectorAll(".row");
	testRunner.Assert(rows.length, 3, "Table should have the correct number of rows: " + 3);
	testRunner.Assert(rows[1].classList.contains("added"), true, "New row should be marked as added");

	//* Remove second row
	rows[1].querySelector(".remove").dispatchEvent(new Event('click', { bubbles : true }));
	testRunner.Assert(tableNode.classList.contains("changed"), false, "Table should not be marked as changed");
	rows = tableNode.querySelectorAll(".row");
	testRunner.Assert(rows.length, 2, "Table should have the correct number of rows: " + 2);
});

testRunner.Test('Clear, save and revert table via buttons and directly', () => {
	const parameter = {
		name : 'Commissions',
		type : 'TableData',
		canBeEmpty : false,
		columns : {
			"0" : { name : "Type", type : 'Int16' },
			"1" : { name : "%", type : 'Double' },
		}
	};

	const table = new Table({ parent : body, id : 30014, metadata : parameter, isMutable : true });
	const tableNode = body.querySelector('.tableWrapper');
	testRunner.Assert(tableNode != null, true, "Table node should not be null");

	let rows = tableNode.querySelectorAll(".row");
	testRunner.Assert(rows.length, 2, "Table should have the correct number of rows: " + 2);

	//* Add row
	table.AddRow([ 1, 2.5 ]);
	testRunner.Assert(tableNode.classList.contains("changed"), true, "Table should be marked as changed");
	rows = tableNode.querySelectorAll(".row");
	testRunner.Assert(rows.length, 3, "Table should have the correct number of rows: " + 3);
	testRunner.Assert(rows[1].classList.contains("added"), true, "New row should be marked as added");

	//* Clear table
	table.Clear();
	testRunner.Assert(tableNode.classList.contains("changed"), false, "Table should not be marked as changed");
	rows = tableNode.querySelectorAll(".row");
	testRunner.Assert(rows.length, 2, "Table should have the correct number of rows: " + 2);

	//* Add row
	table.AddRow([ 1, 2.5 ]);
	testRunner.Assert(tableNode.classList.contains("changed"), true, "Table should be marked as changed");
	rows = tableNode.querySelectorAll(".row");
	testRunner.Assert(rows.length, 3, "Table should have the correct number of rows: " + 3);
	testRunner.Assert(rows[1].classList.contains("added"), true, "New row should be marked as added");

	//* Save table
	table.Save();
	testRunner.Assert(tableNode.classList.contains("changed"), false, "Table should not be marked as changed");
	rows = tableNode.querySelectorAll(".row");
	testRunner.Assert(rows.length, 3, "Table should have the correct number of rows: " + 3);
	testRunner.Assert(rows[1].classList.contains("added"), false, "New row should not be marked as added");

	//* Clear table
	tableNode.querySelector(".clear").dispatchEvent(new Event('click', { bubbles : true }));
	testRunner.Assert(tableNode.classList.contains("changed"), true, "Table should be marked as changed");
	rows = tableNode.querySelectorAll(".row");
	testRunner.Assert(rows.length, 3, "Table should have the correct number of rows: " + 3);
	testRunner.Assert(rows[1].classList.contains("added"), false, "New row should not be marked as added");
	testRunner.Assert(rows[1].classList.contains("removed"), true, "New row should be marked as removed");

	//* Revert table
	table.Revert();
	testRunner.Assert(tableNode.classList.contains("changed"), false, "Table should not be marked as changed");
	rows = tableNode.querySelectorAll(".row");
	testRunner.Assert(rows.length, 3, "Table should have the correct number of rows: " + 3);
	testRunner.Assert(rows[1].classList.contains("added"), false, "New row should not be marked as added");
	testRunner.Assert(rows[1].classList.contains("removed"), false, "New row should not be marked as removed");

	//* Add row
	table.AddRow([ 1, 2.5 ]);
	testRunner.Assert(tableNode.classList.contains("changed"), true, "Table should be marked as changed");
	rows = tableNode.querySelectorAll(".row");
	testRunner.Assert(rows.length, 4, "Table should have the correct number of rows: " + 4);
	testRunner.Assert(rows[2].classList.contains("added"), true, "New row should be marked as added");

	//* Clear table
	table.Clear();
	testRunner.Assert(tableNode.classList.contains("changed"), true, "Table should be marked as changed");
	rows = tableNode.querySelectorAll(".row");
	testRunner.Assert(rows.length, 3, "Table should have the correct number of rows: " + 3);
	testRunner.Assert(rows[1].classList.contains("added"), false, "New row should not be marked as added");
	testRunner.Assert(rows[1].classList.contains("removed"), true, "New row should be marked as removed");

	//* Add row
	table.AddRow([ 1, 2.5 ]);
	testRunner.Assert(tableNode.classList.contains("changed"), true, "Table should be marked as changed");
	rows = tableNode.querySelectorAll(".row");
	testRunner.Assert(rows.length, 4, "Table should have the correct number of rows: " + 4);
	testRunner.Assert(rows[2].classList.contains("added"), true, "New row should be marked as added");

	//* Revert table
	tableNode.querySelector(".revert").dispatchEvent(new Event('click', { bubbles : true }));
	testRunner.Assert(tableNode.classList.contains("changed"), false, "Table should not be marked as changed");
	rows = tableNode.querySelectorAll(".row");
	testRunner.Assert(rows.length, 3, "Table should have the correct number of rows: " + 3);
	testRunner.Assert(rows[1].classList.contains("added"), false, "New row should not be marked as added");
	testRunner.Assert(rows[1].classList.contains("removed"), false, "New row should not be marked as removed");

	//* Set invalid values
	RowChecking.SetValues(rows[1].querySelectorAll("input"), [ "text", 2.5 ]);
	testRunner.Assert(tableNode.classList.contains("changed"), true, "Table should be marked as changed");
	RowChecking.CheckInvalidStates(rows[1].querySelectorAll("input"), [ true, false ]);

	//* Add row
	table.AddRow([ 1, 2.5 ]);
	testRunner.Assert(tableNode.classList.contains("changed"), true, "Table should be marked as changed");
	rows = tableNode.querySelectorAll(".row");
	testRunner.Assert(rows.length, 4, "Table should have the correct number of rows: " + 4);
	RowChecking.CheckInvalidStates(rows[1].querySelectorAll("input"), [ true, false ]);
	testRunner.Assert(rows[2].classList.contains("added"), true, "New row should be marked as added");

	//* Save table
	table.Save();
	testRunner.Assert(tableNode.classList.contains("changed"), true, "Table should be marked as changed");
	rows = tableNode.querySelectorAll(".row");
	testRunner.Assert(rows.length, 4, "Table should have the correct number of rows: " + 4);
	RowChecking.CheckInvalidStates(rows[1].querySelectorAll("input"), [ true, false ]);
	testRunner.Assert(rows[2].classList.contains("added"), true, "New row should be marked as added");

	//* Revert table
	table.Revert();
	testRunner.Assert(tableNode.classList.contains("changed"), false, "Table should not be marked as changed");
	rows = tableNode.querySelectorAll(".row");
	testRunner.Assert(rows.length, 3, "Table should have the correct number of rows: " + 3);
	RowChecking.CheckInvalidStates(rows[1].querySelectorAll("input"), [ false, false ]);
	testRunner.Assert(rows[1].classList.contains("added"), false, "New row should not be marked as added");

	//* Add row
	table.AddRow([ 1, 2.5 ]);
	testRunner.Assert(tableNode.classList.contains("changed"), true, "Table should be marked as changed");
	rows = tableNode.querySelectorAll(".row");
	testRunner.Assert(rows.length, 4, "Table should have the correct number of rows: " + 4);
	testRunner.Assert(rows[2].classList.contains("added"), true, "New row should be marked as added");

	//* Save table
	tableNode.querySelector(".save").dispatchEvent(new Event('click', { bubbles : true }));
	testRunner.Assert(tableNode.classList.contains("changed"), false, "Table should not be marked as changed");
	rows = tableNode.querySelectorAll(".row");
	testRunner.Assert(rows.length, 4, "Table should have the correct number of rows: " + 4);
	testRunner.Assert(rows[2].classList.contains("added"), false, "New row should not be marked as added");
});

testRunner.Test('Min and max limits, step attribute', () => {
	const parameter = {
		name : 'Commissions',
		type : 'TableData',
		canBeEmpty : false,
		columns : {
			"0" : { type : 'Int8', min : -10, max : 10 },
			"1" : { type : 'Int16', min : -10, max : 10 },
			"2" : { type : 'Int32', min : -10, max : 10 },
			"3" : { type : 'Int64', min : -10, max : 10 },
			"4" : { type : 'Uint8', min : 0, max : 10 },
			"5" : { type : 'Uint16', min : 0, max : 10 },
			"6" : { type : 'Uint32', min : 0, max : 10 },
			"7" : { type : 'Uint64', min : 0, max : 10 },
			"8" : { type : 'Float', min : -10, max : 10 },
			"9" : { type : 'Double', min : -10, max : 10 },
			"10" : { type : 'Double', min : -10, max : 10 },
			"11" : { type : 'OptionalInt8', min : -10, max : 10 },
			"12" : { type : 'OptionalInt16', min : -10, max : 10 },
			"13" : { type : 'OptionalInt32', min : -10, max : 10 },
			"14" : { type : 'OptionalInt64', min : -10, max : 10 },
			"15" : { type : 'OptionalUint8', min : 0, max : 10 },
			"16" : { type : 'OptionalUint16', min : 0, max : 10 },
			"17" : { type : 'OptionalUint32', min : 0, max : 10 },
			"18" : { type : 'OptionalUint64', min : 0, max : 10 },
			"19" : { type : 'OptionalFloat', min : -10, max : 10 },
			"20" : { type : 'OptionalDouble', min : -10, max : 10 },
			"21" : { type : 'OptionalDouble', min : -10, max : 10 },
			"22" : { type : 'Duration', min : -10, max : 10 }
		}
	};

	const table = new Table({ parent : body, id : 30014, metadata : parameter, isMutable : true });
	const tableNode = body.querySelector('.tableWrapper');
	testRunner.Assert(tableNode != null, true, "Table node should not be null");

	let rows = tableNode.querySelectorAll(".row");
	testRunner.Assert(rows.length, 2, "Table should have the correct number of rows: " + 2);

	const newRow = tableNode.querySelector(".row.new");
	testRunner.Assert(newRow != null, true, "New row should not be null");

	const newInputs = newRow.querySelectorAll("input");
	testRunner.Assert(newInputs.length, 23, "New row should have the correct number of inputs");

	//* Set all values to min - step
	RowChecking.SetValues(newInputs, [
		parameter.columns[0].min - +newInputs[0].step, parameter.columns[1].min - +newInputs[1].step,
		parameter.columns[2].min - +newInputs[2].step, parameter.columns[3].min - +newInputs[3].step,
		parameter.columns[4].min - +newInputs[4].step, parameter.columns[5].min - +newInputs[5].step,
		parameter.columns[6].min - +newInputs[6].step, parameter.columns[7].min - +newInputs[7].step,
		parameter.columns[8].min - +newInputs[8].step, parameter.columns[9].min - +newInputs[9].step,
		parameter.columns[10].min - +newInputs[10].step, parameter.columns[11].min - +newInputs[11].step,
		parameter.columns[12].min - +newInputs[12].step, parameter.columns[13].min - +newInputs[13].step,
		parameter.columns[14].min - +newInputs[14].step, parameter.columns[15].min - +newInputs[15].step,
		parameter.columns[16].min - +newInputs[16].step, parameter.columns[17].min - +newInputs[17].step,
		parameter.columns[18].min - +newInputs[18].step, parameter.columns[19].min - +newInputs[19].step,
		parameter.columns[20].min - +newInputs[20].step, parameter.columns[21].min - +newInputs[21].step,
		`${parameter.columns[22].min - 1}n`
	]);
	RowChecking.CheckInvalidStates(newInputs, [
		true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true,
		true, true, true, true, true
	]);
	RowChecking.CheckValues(newInputs, [
		parameter.columns[0].min - +newInputs[0].step, parameter.columns[1].min - +newInputs[1].step,
		parameter.columns[2].min - +newInputs[2].step, parameter.columns[3].min - +newInputs[3].step,
		parameter.columns[4].min - +newInputs[4].step, parameter.columns[5].min - +newInputs[5].step,
		parameter.columns[6].min - +newInputs[6].step, parameter.columns[7].min - +newInputs[7].step,
		parameter.columns[8].min - +newInputs[8].step, parameter.columns[9].min - +newInputs[9].step,
		parameter.columns[10].min - +newInputs[10].step, parameter.columns[11].min - +newInputs[11].step,
		parameter.columns[12].min - +newInputs[12].step, parameter.columns[13].min - +newInputs[13].step,
		parameter.columns[14].min - +newInputs[14].step, parameter.columns[15].min - +newInputs[15].step,
		parameter.columns[16].min - +newInputs[16].step, parameter.columns[17].min - +newInputs[17].step,
		parameter.columns[18].min - +newInputs[18].step, parameter.columns[19].min - +newInputs[19].step,
		parameter.columns[20].min - +newInputs[20].step, parameter.columns[21].min - +newInputs[21].step,
		`${parameter.columns[22].min - 1}n`
	]);

	//* Try to add row with invalid values
	table.AddRow();
	testRunner.Assert(tableNode.classList.contains("changed"), false, "Table should not be marked as changed");
	rows = tableNode.querySelectorAll(".row");
	testRunner.Assert(rows.length, 2, "Table should have the correct number of rows: " + 2);

	//* Increment all values by their steps
	newInputs.forEach(input => {
		if (input.hasAttribute("nanoseconds")) {
			RowChecking.SetValues([ input ], [ `${parameter.columns[22].min}n` ]);
		}
		else {
			input.stepUp();
		}

		input.dispatchEvent(new Event('input', { bubbles : true }));
	});
	RowChecking.CheckInvalidStates(newInputs, [
		false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false
	]);
	RowChecking.CheckValues(newInputs, [
		parameter.columns[0].min, parameter.columns[1].min, parameter.columns[2].min, parameter.columns[3].min,
		parameter.columns[4].min, parameter.columns[5].min, parameter.columns[6].min, parameter.columns[7].min,
		parameter.columns[8].min, parameter.columns[9].min, parameter.columns[10].min, parameter.columns[11].min,
		parameter.columns[12].min, parameter.columns[13].min, parameter.columns[14].min, parameter.columns[15].min,
		parameter.columns[16].min, parameter.columns[17].min, parameter.columns[18].min, parameter.columns[19].min,
		parameter.columns[20].min, parameter.columns[21].min, `${parameter.columns[22].min}n`
	]);

	//* Try to add row with valid values
	table.AddRow();
	testRunner.Assert(tableNode.classList.contains("changed"), true, "Table should be marked as changed");
	rows = tableNode.querySelectorAll(".row");
	testRunner.Assert(rows.length, 3, "Table should have the correct number of rows: " + 3);
	testRunner.Assert(rows[1].classList.contains("added"), true, "New row should be marked as added");
	let addedInputs = rows[1].querySelectorAll("input");
	testRunner.Assert(addedInputs.length, 23, "New row should have the correct number of inputs");
	RowChecking.CheckValues(addedInputs, [
		parameter.columns[0].min, parameter.columns[1].min, parameter.columns[2].min, parameter.columns[3].min,
		parameter.columns[4].min, parameter.columns[5].min, parameter.columns[6].min, parameter.columns[7].min,
		parameter.columns[8].min, parameter.columns[9].min, parameter.columns[10].min, parameter.columns[11].min,
		parameter.columns[12].min, parameter.columns[13].min, parameter.columns[14].min, parameter.columns[15].min,
		parameter.columns[16].min, parameter.columns[17].min, parameter.columns[18].min, parameter.columns[19].min,
		parameter.columns[20].min, parameter.columns[21].min, `${parameter.columns[22].min}n`
	]);
	RowChecking.CheckInvalidStates(newInputs, [
		false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false
	]);
	RowChecking.CheckValues(
		newInputs, [ "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "" ]);

	//* Set all values to max + step
	RowChecking.SetValues(addedInputs, [
		parameter.columns[0].max + +addedInputs[0].step, parameter.columns[1].max + +addedInputs[1].step,
		parameter.columns[2].max + +addedInputs[2].step, parameter.columns[3].max + +addedInputs[3].step,
		parameter.columns[4].max + +addedInputs[4].step, parameter.columns[5].max + +addedInputs[5].step,
		parameter.columns[6].max + +addedInputs[6].step, parameter.columns[7].max + +addedInputs[7].step,
		parameter.columns[8].max + +addedInputs[8].step, parameter.columns[9].max + +addedInputs[9].step,
		parameter.columns[10].max + +addedInputs[10].step, parameter.columns[11].max + +addedInputs[11].step,
		parameter.columns[12].max + +addedInputs[12].step, parameter.columns[13].max + +addedInputs[13].step,
		parameter.columns[14].max + +addedInputs[14].step, parameter.columns[15].max + +addedInputs[15].step,
		parameter.columns[16].max + +addedInputs[16].step, parameter.columns[17].max + +addedInputs[17].step,
		parameter.columns[18].max + +addedInputs[18].step, parameter.columns[19].max + +addedInputs[19].step,
		parameter.columns[20].max + +addedInputs[20].step, parameter.columns[21].max + +addedInputs[21].step,
		`${parameter.columns[22].max + 1}n`
	]);
	testRunner.Assert(tableNode.classList.contains("changed"), true, "Table should be marked as changed");
	RowChecking.CheckInvalidStates(addedInputs, [
		true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true,
		true, true, true, true, true
	]);
	RowChecking.CheckValues(addedInputs, [
		parameter.columns[0].max + +addedInputs[0].step, parameter.columns[1].max + +addedInputs[1].step,
		parameter.columns[2].max + +addedInputs[2].step, parameter.columns[3].max + +addedInputs[3].step,
		parameter.columns[4].max + +addedInputs[4].step, parameter.columns[5].max + +addedInputs[5].step,
		parameter.columns[6].max + +addedInputs[6].step, parameter.columns[7].max + +addedInputs[7].step,
		parameter.columns[8].max + +addedInputs[8].step, parameter.columns[9].max + +addedInputs[9].step,
		parameter.columns[10].max + +addedInputs[10].step, parameter.columns[11].max + +addedInputs[11].step,
		parameter.columns[12].max + +addedInputs[12].step, parameter.columns[13].max + +addedInputs[13].step,
		parameter.columns[14].max + +addedInputs[14].step, parameter.columns[15].max + +addedInputs[15].step,
		parameter.columns[16].max + +addedInputs[16].step, parameter.columns[17].max + +addedInputs[17].step,
		parameter.columns[18].max + +addedInputs[18].step, parameter.columns[19].max + +addedInputs[19].step,
		parameter.columns[20].max + +addedInputs[20].step, parameter.columns[21].max + +addedInputs[21].step,
		`${parameter.columns[22].max + 1}n`
	]);

	//* Try to save table
	table.Save();
	testRunner.Assert(tableNode.classList.contains("changed"), true, "Table should be marked as changed");
	rows = tableNode.querySelectorAll(".row");
	testRunner.Assert(rows.length, 3, "Table should have the correct number of rows: " + 3);
	testRunner.Assert(rows[1].classList.contains("added"), true, "New row should be marked as added");
	addedInputs = rows[1].querySelectorAll("input");
	testRunner.Assert(addedInputs.length, 23, "New row should have the correct number of inputs");
	RowChecking.CheckValues(addedInputs, [
		parameter.columns[0].max + +addedInputs[0].step, parameter.columns[1].max + +addedInputs[1].step,
		parameter.columns[2].max + +addedInputs[2].step, parameter.columns[3].max + +addedInputs[3].step,
		parameter.columns[4].max + +addedInputs[4].step, parameter.columns[5].max + +addedInputs[5].step,
		parameter.columns[6].max + +addedInputs[6].step, parameter.columns[7].max + +addedInputs[7].step,
		parameter.columns[8].max + +addedInputs[8].step, parameter.columns[9].max + +addedInputs[9].step,
		parameter.columns[10].max + +addedInputs[10].step, parameter.columns[11].max + +addedInputs[11].step,
		parameter.columns[12].max + +addedInputs[12].step, parameter.columns[13].max + +addedInputs[13].step,
		parameter.columns[14].max + +addedInputs[14].step, parameter.columns[15].max + +addedInputs[15].step,
		parameter.columns[16].max + +addedInputs[16].step, parameter.columns[17].max + +addedInputs[17].step,
		parameter.columns[18].max + +addedInputs[18].step, parameter.columns[19].max + +addedInputs[19].step,
		parameter.columns[20].max + +addedInputs[20].step, parameter.columns[21].max + +addedInputs[21].step,
		`${parameter.columns[22].max + 1}n`
	]);
	RowChecking.CheckInvalidStates(addedInputs, [
		true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true,
		true, true, true, true, true
	]);

	//* Decrement all values by their steps
	addedInputs.forEach(input => {
		if (input.hasAttribute("nanoseconds")) {
			RowChecking.SetValues([ input ], [ `${parameter.columns[22].max}n` ]);
		}
		else {
			input.stepDown();
		}

		input.dispatchEvent(new Event('change', { bubbles : true }));
	});
	RowChecking.CheckInvalidStates(addedInputs, [
		false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false
	]);
	RowChecking.CheckValues(addedInputs, [
		parameter.columns[0].max, parameter.columns[1].max, parameter.columns[2].max, parameter.columns[3].max,
		parameter.columns[4].max, parameter.columns[5].max, parameter.columns[6].max, parameter.columns[7].max,
		parameter.columns[8].max, parameter.columns[9].max, parameter.columns[10].max, parameter.columns[11].max,
		parameter.columns[12].max, parameter.columns[13].max, parameter.columns[14].max, parameter.columns[15].max,
		parameter.columns[16].max, parameter.columns[17].max, parameter.columns[18].max, parameter.columns[19].max,
		parameter.columns[20].max, parameter.columns[21].max, `${parameter.columns[22].max}n`
	]);
	testRunner.Assert(tableNode.classList.contains("changed"), true, "Table should be marked as changed");

	//* Try to save table
	table.Save();
	testRunner.Assert(tableNode.classList.contains("changed"), false, "Table should not be marked as changed");
	rows = tableNode.querySelectorAll(".row");
	testRunner.Assert(rows.length, 3, "Table should have the correct number of rows: " + 3);
	testRunner.Assert(rows[1].classList.contains("added"), false, "New row should not be marked as added");
});

testRunner.Test('Validation out of range custom limits', () => {
	const parameter = {
		name : 'Commissions',
		type : 'TableData',
		canBeEmpty : false,
		columns : {
			"0" : { type : 'Int8', min : Helper.TYPES_LIMITS.int_8.min - 1, max : Helper.TYPES_LIMITS.int_8.max + 1 },
			"1" :
				{ type : 'Int16', min : Helper.TYPES_LIMITS.int_16.min - 1, max : Helper.TYPES_LIMITS.int_16.max + 1 },
			"2" :
				{ type : 'Int32', min : Helper.TYPES_LIMITS.int_32.min - 1, max : Helper.TYPES_LIMITS.int_32.max + 1 },
			"3" :
				{ type : 'Int64', min : Helper.TYPES_LIMITS.int_64.min - 1, max : Helper.TYPES_LIMITS.int_64.max + 1 },
			"4" :
				{ type : 'Uint8', min : Helper.TYPES_LIMITS.uint_8.min - 1, max : Helper.TYPES_LIMITS.uint_8.max + 1 },
			"5" : {
				type : 'Uint16',
				min : Helper.TYPES_LIMITS.uint_16.min - 1,
				max : Helper.TYPES_LIMITS.uint_16.max + 1
			},
			"6" : {
				type : 'Uint32',
				min : Helper.TYPES_LIMITS.uint_32.min - 1,
				max : Helper.TYPES_LIMITS.uint_32.max + 1
			},
			"7" : {
				type : 'Uint64',
				min : Helper.TYPES_LIMITS.uint_64.min - 1,
				max : Helper.TYPES_LIMITS.uint_64.max + 1
			},
			"8" : {
				type : 'Float',
				min : Helper.TYPES_LIMITS.float_32.min - 1,
				max : Helper.TYPES_LIMITS.float_32.max + 1
			},
			"9" : {
				type : 'Double',
				min : Helper.TYPES_LIMITS.float_64.min - 1,
				max : Helper.TYPES_LIMITS.float_64.max + 1
			},
			"10" : {
				type : 'Double',
				min : Helper.TYPES_LIMITS.float_64.min - 1,
				max : Helper.TYPES_LIMITS.float_64.max + 1
			},
			"11" : {
				type : 'OptionalInt8',
				min : Helper.TYPES_LIMITS.int_8.min - 1,
				max : Helper.TYPES_LIMITS.int_8.max + 1
			},
			"12" : {
				type : 'OptionalInt16',
				min : Helper.TYPES_LIMITS.int_16.min - 1,
				max : Helper.TYPES_LIMITS.int_16.max + 1
			},
			"13" : {
				type : 'OptionalInt32',
				min : Helper.TYPES_LIMITS.int_32.min - 1,
				max : Helper.TYPES_LIMITS.int_32.max + 1
			},
			"14" : {
				type : 'OptionalInt64',
				min : Helper.TYPES_LIMITS.int_64.min - 1,
				max : Helper.TYPES_LIMITS.int_64.max + 1
			},
			"15" : {
				type : 'OptionalUint8',
				min : Helper.TYPES_LIMITS.uint_8.min - 1,
				max : Helper.TYPES_LIMITS.uint_8.max + 1
			},
			"16" : {
				type : 'OptionalUint16',
				min : Helper.TYPES_LIMITS.uint_16.min - 1,
				max : Helper.TYPES_LIMITS.uint_16.max + 1
			},
			"17" : {
				type : 'OptionalUint32',
				min : Helper.TYPES_LIMITS.uint_32.min - 1,
				max : Helper.TYPES_LIMITS.uint_32.max + 1
			},
			"18" : {
				type : 'OptionalUint64',
				min : Helper.TYPES_LIMITS.uint_64.min - 1,
				max : Helper.TYPES_LIMITS.uint_64.max + 1
			},
			"19" : {
				type : 'OptionalFloat',
				min : Helper.TYPES_LIMITS.float_32.min - 1,
				max : Helper.TYPES_LIMITS.float_32.max + 1
			},
			"20" : {
				type : 'OptionalDouble',
				min : Helper.TYPES_LIMITS.float_64.min - 1,
				max : Helper.TYPES_LIMITS.float_64.max + 1
			},
			"21" : {
				type : 'OptionalDouble',
				min : Helper.TYPES_LIMITS.float_64.min - 1,
				max : Helper.TYPES_LIMITS.float_64.max + 1
			},
			"22" : {
				type : 'Duration',
				min : Helper.TYPES_LIMITS.int_64.min - 1,
				max : Helper.TYPES_LIMITS.int_64.max + 1
			}
		}
	};

	const table = new Table({ parent : body, id : 30014, metadata : parameter, isMutable : true });
	const tableNode = body.querySelector('.tableWrapper');
	testRunner.Assert(tableNode != null, true, "Table node should not be null");

	let rows = tableNode.querySelectorAll(".row");
	testRunner.Assert(rows.length, 2, "Table should have the correct number of rows: " + 2);

	const newRow = tableNode.querySelector(".row.new");
	testRunner.Assert(newRow != null, true, "New row should not be null");

	const newInputs = newRow.querySelectorAll("input");
	testRunner.Assert(newInputs.length, 23, "New row should have the correct number of inputs");

	//* Verify that all limits in the range
	RowChecking.CheckAttributes(newInputs, [
		{ min : Helper.TYPES_LIMITS.int_8.min, max : Helper.TYPES_LIMITS.int_8.max },
		{ min : Helper.TYPES_LIMITS.int_16.min, max : Helper.TYPES_LIMITS.int_16.max },
		{ min : Helper.TYPES_LIMITS.int_32.min, max : Helper.TYPES_LIMITS.int_32.max },
		{ min : Helper.TYPES_LIMITS.int_64.min, max : Helper.TYPES_LIMITS.int_64.max },
		{ min : Helper.TYPES_LIMITS.uint_8.min, max : Helper.TYPES_LIMITS.uint_8.max },
		{ min : Helper.TYPES_LIMITS.uint_16.min, max : Helper.TYPES_LIMITS.uint_16.max },
		{ min : Helper.TYPES_LIMITS.uint_32.min, max : Helper.TYPES_LIMITS.uint_32.max },
		{ min : Helper.TYPES_LIMITS.uint_64.min, max : Helper.TYPES_LIMITS.uint_64.max },
		{ min : Helper.TYPES_LIMITS.float_32.min, max : Helper.TYPES_LIMITS.float_32.max },
		{ min : Helper.TYPES_LIMITS.float_64.min, max : Helper.TYPES_LIMITS.float_64.max },
		{ min : Helper.TYPES_LIMITS.float_64.min, max : Helper.TYPES_LIMITS.float_64.max },
		{ min : Helper.TYPES_LIMITS.int_8.min, max : Helper.TYPES_LIMITS.int_8.max },
		{ min : Helper.TYPES_LIMITS.int_16.min, max : Helper.TYPES_LIMITS.int_16.max },
		{ min : Helper.TYPES_LIMITS.int_32.min, max : Helper.TYPES_LIMITS.int_32.max },
		{ min : Helper.TYPES_LIMITS.int_64.min, max : Helper.TYPES_LIMITS.int_64.max },
		{ min : Helper.TYPES_LIMITS.uint_8.min, max : Helper.TYPES_LIMITS.uint_8.max },
		{ min : Helper.TYPES_LIMITS.uint_16.min, max : Helper.TYPES_LIMITS.uint_16.max },
		{ min : Helper.TYPES_LIMITS.uint_32.min, max : Helper.TYPES_LIMITS.uint_32.max },
		{ min : Helper.TYPES_LIMITS.uint_64.min, max : Helper.TYPES_LIMITS.uint_64.max },
		{ min : Helper.TYPES_LIMITS.float_32.min, max : Helper.TYPES_LIMITS.float_32.max },
		{ min : Helper.TYPES_LIMITS.float_64.min, max : Helper.TYPES_LIMITS.float_64.max },
		{ min : Helper.TYPES_LIMITS.float_64.min, max : Helper.TYPES_LIMITS.float_64.max },
		{ min : Helper.TYPES_LIMITS.int_64.min, max : Helper.TYPES_LIMITS.int_64.max }
	]);
});

testRunner.Test('Empty limits', () => {
	//* Table's columns can be empty
	const table = new Table({
		parent : body,
		id : 30014,
		metadata : {
			name : 'Commissions',
			type : 'TableData',
			canBeEmpty : false,
			columns : {
				"0" : { type : 'OptionalInt8' },
				"1" : { type : 'OptionalInt16' },
				"2" : { type : 'OptionalInt32' },
				"3" : { type : 'OptionalInt64' },
				"4" : { type : 'OptionalUint8' },
				"5" : { type : 'OptionalUint16' },
				"6" : { type : 'OptionalUint32' },
				"7" : { type : 'OptionalUint64' },
				"8" : { type : 'OptionalFloat' },
				"9" : { type : 'OptionalDouble' },
				"10" : { type : 'OptionalDouble' },
				"11" : { type : 'String' },
				"12" : { type : 'Timer' },
				"13" : { type : 'Duration' }
			}
		},
		isMutable : true
	});
	const tableNode = body.querySelector('.tableWrapper');
	testRunner.Assert(tableNode != null, true, "Table node should not be null");

	let rows = tableNode.querySelectorAll(".row");
	testRunner.Assert(rows.length, 2, "Table should have the correct number of rows: " + 2);

	const newRow = tableNode.querySelector(".row.new");
	testRunner.Assert(newRow != null, true, "New row should not be null");

	const newInputs = newRow.querySelectorAll("input");
	testRunner.Assert(newInputs.length, 14, "New row should have the correct number of inputs");

	//* Try to add row with empty values
	table.AddRow();
	testRunner.Assert(tableNode.classList.contains("changed"), true, "Table should be marked as changed");
	rows = tableNode.querySelectorAll(".row");
	testRunner.Assert(rows.length, 3, "Table should have the correct number of rows: " + 3);
	testRunner.Assert(rows[1].classList.contains("added"), true, "New row should be marked as added");
	let addedInputs = rows[1].querySelectorAll("input");
	testRunner.Assert(addedInputs.length, 14, "New row should have the correct number of inputs");
	RowChecking.CheckValues(addedInputs, [ "", "", "", "", "", "", "", "", "", "", "", "", "", "" ]);

	const table2 = new Table({
		parent : body,
		id : 30014,
		metadata : {
			name : 'Commissions',
			type : 'TableData',
			canBeEmpty : false,
			columns : {
				"0" : { type : 'OptionalInt8', canBeEmpty : false },
				"1" : { type : 'OptionalInt16', canBeEmpty : false },
				"2" : { type : 'OptionalInt32', canBeEmpty : false },
				"3" : { type : 'OptionalInt64', canBeEmpty : false },
				"4" : { type : 'OptionalUint8', canBeEmpty : false },
				"5" : { type : 'OptionalUint16', canBeEmpty : false },
				"6" : { type : 'OptionalUint32', canBeEmpty : false },
				"7" : { type : 'OptionalUint64', canBeEmpty : false },
				"8" : { type : 'OptionalFloat', canBeEmpty : false },
				"9" : { type : 'OptionalDouble', canBeEmpty : false },
				"10" : { type : 'OptionalDouble', canBeEmpty : false },
				"11" : { type : 'String', canBeEmpty : false },
				"12" : { type : 'Timer', canBeEmpty : false },
				"13" : { type : 'Duration', canBeEmpty : false }
			}
		},
		isMutable : true
	});
	const tableNode2 = body.querySelectorAll('.tableWrapper')[1];
	testRunner.Assert(tableNode2 != null, true, "Table node should not be null");

	rows = tableNode2.querySelectorAll(".row");
	testRunner.Assert(rows.length, 2, "Table should have the correct number of rows: " + 2);

	const newRow2 = tableNode2.querySelector(".row.new");
	testRunner.Assert(newRow2 != null, true, "New row should not be null");

	const newInputs2 = newRow2.querySelectorAll("input");
	testRunner.Assert(newInputs2.length, 14, "New row should have the correct number of inputs");

	//* Try to add row with empty values
	table2.AddRow();
	testRunner.Assert(tableNode2.classList.contains("changed"), false, "Table should not be marked as changed");
	rows = tableNode2.querySelectorAll(".row");
	testRunner.Assert(rows.length, 2, "Table should have the correct number of rows: " + 2);
	RowChecking.CheckInvalidStates(
		newInputs2, [ true, true, true, true, true, true, true, true, true, true, true, true, true, true ]);

	//* Set all values to valid
	RowChecking.SetValues(newInputs2, [ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, "12", "2021-01-01", "14n" ]);
	RowChecking.CheckInvalidStates(newInputs2,
		[ false, false, false, false, false, false, false, false, false, false, false, false, false, false ]);

	//* Try to add row with valid values
	table2.AddRow();
	testRunner.Assert(tableNode2.classList.contains("changed"), true, "Table should be marked as changed");
	rows = tableNode2.querySelectorAll(".row");
	testRunner.Assert(rows.length, 3, "Table should have the correct number of rows: " + 3);
	testRunner.Assert(rows[1].classList.contains("added"), true, "New row should be marked as added");
	addedInputs = rows[1].querySelectorAll("input");
	testRunner.Assert(addedInputs.length, 14, "New row should have the correct number of inputs");
	RowChecking.CheckValues(addedInputs,
		[ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, "12", `2021-01-01 00:00:00.000000000 UTC${TestRunner.UTC}`, "14n" ]);
});

testRunner.Test('Add, modify and remove row in immutable table', () => {
	const parameter = {
		name : 'Commissions',
		type : 'TableData',
		canBeEmpty : false,
		columns : {
			"0" : { name : 'Type', type : 'Int16' },
			"1" : { name : '%', type : 'Double' },
		}
	};

	const table = new Table({ parent : body, id : 30014, metadata : parameter, isMutable : false });
	const tableNode = body.querySelector('.tableWrapper');
	testRunner.Assert(tableNode != null, true, "Table node should not be null");

	let rows = tableNode.querySelectorAll(".row");
	testRunner.Assert(rows.length, 2, "Table should have the correct number of rows: " + 2);

	//* Add row
	table.AddRow([ 1, 2.5 ]);
	testRunner.Assert(tableNode.classList.contains("changed"), false, "Table should not be marked as changed");
	rows = tableNode.querySelectorAll(".row");
	testRunner.Assert(rows.length, 3, "Table should have the correct number of rows: " + 3);
	testRunner.Assert(rows[1].classList.contains("added"), false, "New row should not be marked as added");
	testRunner.Assert(rows[1].childNodes.length, 3, "New row should have the correct number of children");

	let addedInputs = rows[1].querySelectorAll("input");
	testRunner.Assert(addedInputs.length, 2, "New row should have the correct number of inputs");
	RowChecking.CheckValues(addedInputs, [ 1, 2.5 ]);
	RowChecking.CheckInvalidStates(addedInputs, [ false, false ]);
	RowChecking.CheckAttributes(addedInputs, [
		{
			type : "number",
			canBeEmpty : false,
			readonly : true,
			min : Helper.TYPES_LIMITS.int_16.min,
			max : Helper.TYPES_LIMITS.int_16.max,
			step : 1
		},
		{
			type : "number",
			canBeEmpty : false,
			readonly : true,
			min : Helper.TYPES_LIMITS.float_64.min,
			max : Helper.TYPES_LIMITS.float_64.max,
			step : 0.01
		}
	]);
	testRunner.Assert(rows[1].classList.contains("added"), false, "New row should not be marked as added");
	testRunner.Assert(rows[1].querySelector(".remove"), null, "New row should not have remove button");

	//* Modify row
	RowChecking.SetValues(addedInputs, [ 2, 3.5 ]);
	RowChecking.CheckInvalidStates(addedInputs, [ false, false ]);
	RowChecking.CheckValues(addedInputs, [ 2, 3.5 ]);
	testRunner.Assert(tableNode.classList.contains("changed"), false, "Table should not be marked as changed");

	//* Remove row
	table.RemoveRow({ index : 1 });
	rows = tableNode.querySelectorAll(".row");
	testRunner.Assert(rows.length, 2, "Table should have the correct number of rows: " + 2);
	testRunner.Assert(tableNode.classList.contains("changed"), false, "Table should not be marked as changed");
});

testRunner.Test('Add row via view and directly to table with two select columns', async () => {
	const parameter = {
		name : 'Commissions',
		type : 'TableData',
		canBeEmpty : false,
		columns : {
			"0" : {
				name : 'Type',
				type : 'Int16',
				stringInterpretation : {
					"0" : "Undefined",
					"1" : "Bond",
					"2" : "Share",
					"3" : "Currency",
					"4" : "ETF",
					"5" : "Futures",
					"6" : "StructuralProduct",
					"7" : "Option",
					"8" : "ClearingCertificate",
					"9" : "Index",
					"10" : "Commodity"
				}
			},
			"1" : { name : '%', type : 'OptionalDouble' },
			"2" : {
				name : 'Type 2',
				type : 'Int16',
				stringInterpretation : {
					"0" : "Undefined",
					"1" : "Bond",
					"2" : "Share",
					"3" : "Currency",
					"4" : "ETF",
					"5" : "Futures",
					"6" : "StructuralProduct",
					"7" : "Option",
					"8" : "ClearingCertificate",
					"9" : "Index",
					"10" : "Commodity"
				}
			},
		},
	};

	MetadataCollector.AddMetadata(888, parameter, true);

	const table = new Table({ parent : body, id : 888, metadata : parameter, isMutable : true });
	const tableNode = body.querySelector('.tableWrapper');
	testRunner.Assert(tableNode != null, true, "Table node should not be null");

	let rows = tableNode.querySelectorAll(".row");
	testRunner.Assert(rows.length, 2, "Table should have the correct number of rows: " + 2);

	TestRunner.Step("Try to add row with empty values");
	let newRow = tableNode.querySelector(".row.new");
	testRunner.Assert(newRow != null, true, "New row should not be null");
	const newInputs = newRow.querySelectorAll("input");
	testRunner.Assert(newInputs.length, 3, "New row should have the correct number of inputs");
	table.AddRow();
	testRunner.Assert(tableNode.classList.contains("changed"), false, "Table should not be marked as changed");

	RowChecking.CheckInvalidStates(newInputs, [ true, false, true ]);
	RowChecking.CheckValues(newInputs, [ "", "", "" ]);

	TestRunner.Step("Add row via view");
	{
		testRunner.Assert(
			Select.GetSelectViewsNumber(), 0, "There should be no select views before click on select input");
		let appsBefore = Application.GetCreatedApplications().size;
		newInputs[0].dispatchEvent(new Event('click'));
		await TestRunner.WaitFor(() => { return Application.GetCreatedApplications().size > appsBefore },
			"Select view should be created after click on select input");
		testRunner.Assert(
			Select.GetSelectViewsNumber(), 1, "There should be one select view after click on select input");

		let selectView = document.querySelector(".selectView");
		testRunner.Assert(selectView != null, true, "Select view should be created");
		let options = selectView.querySelectorAll(".options > div");
		testRunner.Assert(options.length, 11, "Select view should have the correct number of options");
		options[2].dispatchEvent(new Event('click'));
		await TestRunner.WaitFor(() => { return Application.GetCreatedApplications().size === appsBefore },
			"Select view should be closed after click on option");
		testRunner.Assert(Select.GetSelectViewsNumber(), 0, "There should be no select views after click on option");

		newInputs[1].value = "3.5";
		newInputs[1].dispatchEvent(new Event('input'));

		newInputs[2].dispatchEvent(new Event('click'));
		await TestRunner.WaitFor(() => { return Application.GetCreatedApplications().size > appsBefore },
			"Select view should be created after click on select input");
		testRunner.Assert(
			Select.GetSelectViewsNumber(), 1, "There should be one select view after click on select input");
		selectView = document.querySelector(".selectView");
		testRunner.Assert(selectView != null, true, "Select view should be created");
		options = selectView.querySelectorAll(".options > div");
		testRunner.Assert(options.length, 11, "Select view should have the correct number of options");
		options[7].dispatchEvent(new Event('click'));
		await TestRunner.WaitFor(() => { return Application.GetCreatedApplications().size === appsBefore },
			"Select view should be closed after click on option");
		testRunner.Assert(Select.GetSelectViewsNumber(), 0, "There should be no select views after click on option");
	}

	newRow.querySelector(".add").dispatchEvent(new Event('click'));
	testRunner.Assert(tableNode.classList.contains("changed"), true, "Table should be marked as changed");
	rows = tableNode.querySelectorAll(".row");
	testRunner.Assert(rows.length, 3, "Table should have the correct number of rows: " + 3);
	testRunner.Assert(rows[1].classList.contains("added"), true, "New row should be marked as added");
	let addedInputs = rows[1].querySelectorAll("input");
	testRunner.Assert(addedInputs.length, 3, "New row should have the correct number of inputs");
	RowChecking.CheckInvalidStates(addedInputs, [ false, false, false ]);
	RowChecking.CheckValues(addedInputs, [ "Share", 3.5, "Option" ]);

	TestRunner.Step("Add row directly to table");
	{
		testRunner.Assert(
			Select.GetSelectViewsNumber(), 0, "There should be no select views before click on select input");
		let appsBefore = Application.GetCreatedApplications().size;
		table.AddRow([ 1, 4.5, 5 ]);
		await TestRunner.WaitFor(() => { return Application.GetCreatedApplications().size === appsBefore },
			"Select view should be closed after click on option");
		testRunner.Assert(Select.GetSelectViewsNumber(), 0, "There should be no select views after click on option");

		rows = tableNode.querySelectorAll(".row");
		testRunner.Assert(rows.length, 4, "Table should have the correct number of rows: " + 4);
		testRunner.Assert(rows[2].classList.contains("added"), true, "New row should be marked as added");
		addedInputs = rows[2].querySelectorAll("input");
		testRunner.Assert(addedInputs.length, 3, "New row should have the correct number of inputs");
		RowChecking.CheckInvalidStates(addedInputs, [ false, false, false ]);
		RowChecking.CheckValues(addedInputs, [ "Bond", 4.5, "Futures" ]);
	}

	table.Save();
	testRunner.Assert(tableNode.classList.contains("changed"), false, "Table should not be marked as changed");
	rows = tableNode.querySelectorAll(".row");
	testRunner.Assert(rows.length, 4, "Table should have the correct number of rows: " + 4);
	testRunner.Assert(table.GetData(),
		[
			[ 2, 3.5, 7 ],
			[ 1, 4.5, 5 ],
		],
		"Table should have the correct data after save");

	TestRunner.Step("Clean up");
	table.Clear();
	testRunner.Assert(tableNode.classList.contains("changed"), true, "Table should be marked as changed");
	rows = tableNode.querySelectorAll(".row");
	testRunner.Assert(rows.length, 4, "Table should have the correct number of rows: " + 4);
	testRunner.Assert(table.GetData(),
		[
			[ 2, 3.5, 7 ],
			[ 1, 4.5, 5 ],
		],
		"Table should have the correct data after clear");

	table.Save();
	testRunner.Assert(tableNode.classList.contains("changed"), false, "Table should not be marked as changed");
	rows = tableNode.querySelectorAll(".row");
	testRunner.Assert(rows.length, 2, "Table should have the correct number of rows: " + 2);
	testRunner.Assert(table.GetData(), [], "Table should have the correct data after save");
});

testRunner.Run();