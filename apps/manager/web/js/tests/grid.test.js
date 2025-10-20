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

const { TestRunner, TableChecker } = require('./testRunner');
const Grid = require('../grid');
const MetadataCollector = require('../metadataCollector');
const Application = require('../application');
global.dispatcher = undefined;

const testRunner = new TestRunner();

MetadataCollector.AddMetadata(1, { name : "Create", type : "system" }, true);
MetadataCollector.AddMetadata(2, { name : "Change state", type : "system" }, true);
MetadataCollector.AddMetadata(3, { name : "Modify", type : "system" }, true);
MetadataCollector.AddMetadata(4, { name : "Delete", type : "system" }, true);
MetadataCollector.AddMetadata(5, { name : "Type", type : "String" }, true);
MetadataCollector.AddMetadata(6, {
	name : "Table template with boolean operator",
	type : "TableData",
	id : 6,
	columns : [ { name : "Boolean operator", type : "Int8", stringInterpretation : { 0 : "Equal", 1 : "Not equal" } } ]
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

const duration_seconds_parameter_1 = 30002;
MetadataCollector.AddMetadata(duration_seconds_parameter_1,
	{ "name" : "Start trading time delay", "type" : "Duration", "canBeEmpty" : false, "durationType" : "Seconds" },
	true);

const duration_seconds_parameter_2 = 30003;
MetadataCollector.AddMetadata(duration_seconds_parameter_2,
	{ "name" : "Stop trading time delay", "type" : "Duration", "canBeEmpty" : false, "durationType" : "Nanoseconds" },
	true);

const bool_parameter = 30005;
MetadataCollector.AddMetadata(bool_parameter, { "name" : "Active minimum trend strike mode", "type" : "Bool" }, true);

const scalar_parameter = 30006;
MetadataCollector.AddMetadata(scalar_parameter, { "name" : "Object identifier", "type" : "Int64" }, true);

const float_point_parameter = 30008;
MetadataCollector.AddMetadata(
	float_point_parameter, { "name" : "Minimum trend strike price limit", "type" : "Double" }, true);

const optional_float_parameter = 30012;
MetadataCollector.AddMetadata(
	optional_float_parameter, { "name" : "Fair price", "type" : "OptionalDouble", "canBeEmpty" : true }, true);

const optional_scalar_parameter = 30013;
MetadataCollector.AddMetadata(
	optional_scalar_parameter, { "name" : "Gateway id", "type" : "OptionalInt32", "canBeEmpty" : true }, true);

const table_parameter_1 = 30014;
MetadataCollector.AddMetadata(table_parameter_1, {
	"name" : "Commissions",
	"type" : "TableData",
	"canBeEmpty" : false,
	"columns" : {
		"0" : {
			"type" : "Int16",
			"name" : "Instrument type",
			"stringInterpretation" : {
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
		"1" : { "type" : "Double", "name" : "%" }
	}
},
	true);

const table_parameter_2 = 30015;
MetadataCollector.AddMetadata(table_parameter_2, {
	"name" : "Figis to trade",
	"type" : "TableData",
	"canBeEmpty" : false,
	"columns" :
		{ "0" : { "type" : "Uint64", "name" : "figi" }, "1" : { "type" : "Uint64", "name" : "Default lots volume" } }
},
	true);

const select_parameter_1 = 1000006;
MetadataCollector.AddMetadata(select_parameter_1, {
	"name" : "Server state",
	"type" : "Int16",
	"stringInterpretation" : { "0" : "Undefined", "1" : "Initialization", "2" : "Running", "3" : "Stopped" }
},
	true);

const select_parameter_2 = 1000007;
MetadataCollector.AddMetadata(select_parameter_2, {
	"name" : "Application state",
	"type" : "Int16",
	"stringInterpretation" : { "0" : "Undefined", "1" : "Paused", "2" : "Running" }
},
	true);

const string_parameter = 2000001;
MetadataCollector.AddMetadata(string_parameter, { "name" : "Name", "type" : "String" }, true);

const timer_parameter = 2000002;
MetadataCollector.AddMetadata(timer_parameter, { "name" : "Timer", "type" : "Timer" }, true);

const system_create_parameter = 1;
MetadataCollector.AddMetadata(system_create_parameter, { name : "Create", type : "system" }, true);

class GridChecking {
	static CheckColumnsInOrder(grid, columns)
	{
		testRunner.Assert(grid.m_columnByOrder.size, columns.length);
		testRunner.Assert(grid.m_view.querySelectorAll('.header .cell').length, columns.length);

		for (let index = 0; index < columns.length; index++) {
			const columnObject = grid.m_columnByOrder.get(columns[index].order);
			testRunner.Assert(
				columnObject != undefined, true, `Column with order ${columns[index].order} is not found`);
			testRunner.Assert(columnObject.headerCell.querySelector(".text").innerHTML, columns[index].name);
		}
	}

	static CheckRowsInOrder(grid, orders)
	{
		const rows = grid.m_view.querySelectorAll('.row');
		testRunner.Assert(rows.length - 1, orders.length);

		for (let index = 0; index < orders.length; index++) {
			testRunner.Assert(+rows[index + 1].style.gridRow, orders[index]);
		}
	}

	static async CheckRows(grid, values)
	{
		const rows = grid.m_view.querySelectorAll('.row');
		testRunner.Assert(rows.length - 1, values.length);

		for (let index = 0; index < values.length; index++) {
			let cellCounter = 0;
			for (const [key, value] of Object.entries(values[index])) {
				++cellCounter;
				const cell = rows[index + 1].querySelector(`[parameter-id="${key}"]`);
				const input = cell.querySelector('input');
				if (input !== null) {
					testRunner.Assert(
						input.value, value, `Cell value is unexpected for row ${index + 1} and column ${key}`);
					if (input.hasAttribute('nanoseconds') || input.hasAttribute('timestamp')) {
						testRunner.Assert(getEventListeners(input).click, undefined);
						testRunner.Assert(getEventListeners(input).input.length, 1);
					}
					else {
						//* Select
						testRunner.Assert(getEventListeners(input).click, undefined);
						testRunner.Assert(getEventListeners(input).input, undefined);
					}
				}
				else {
					if (cell.classList.contains('table')) {
						testRunner.Assert(
							cell.innerHTML, "", `Cell value is unexpected for row ${index + 1} and column ${key}`);

						testRunner.Assert(getEventListeners(cell).click.length, 1);
						testRunner.Assert(getEventListeners(cell).input, undefined);

						TestRunner.Step('Click on the table cell, verify table view content');
						const createdApplications = Application.GetCreatedApplications();
						const sizeBefore = createdApplications.size;
						cell.dispatchEvent(new Event('click'));
						await TestRunner.WaitFor(() => createdApplications.size > sizeBefore, 'Table view is created');

						const views = document.querySelectorAll('.views > .view');
						const tableView = views[views.length - 1];
						const viewUid = +tableView.getAttribute('uid');
						const tableViewInstance = createdApplications.get(viewUid);

						await TestRunner.WaitFor(() => tableViewInstance.m_created, 'Table view is created');
						testRunner.Assert(tableView.querySelector('.tableWrapper').classList.contains('const'), true);
						testRunner.Assert(
							tableView.querySelector('.viewHeader .title span').innerHTML, values[index][key].title);
						testRunner.Assert(tableView.querySelector('.tableWrapper > div > .header').innerHTML,
							values[index][key].header);
						let columnHeaders
							= tableView.querySelectorAll('.tableWrapper > div > .table .row.header > div > span');
						testRunner.Assert(columnHeaders.length, values[index][key].columnHeaders.length);
						for (let i = 0; i < columnHeaders.length; i++) {
							testRunner.Assert(columnHeaders[i].innerHTML, values[index][key].columnHeaders[i]);
						}

						let tableRows
							= tableView.querySelectorAll('.tableWrapper > div > .table .row:not(.header,.new)');

						testRunner.Assert(tableRows.length, values[index][key].values.length);
						for (let i = 0; i < tableRows.length; i++) {
							let cells = tableRows[i].querySelectorAll('input');
							testRunner.Assert(cells.length, values[index][key].values[i].length);
							for (let j = 0; j < cells.length; j++) {
								testRunner.Assert(cells[j].value, values[index][key].values[i][j]);
							}
						}

						if (tableView.querySelectorAll('.tableWrapper > div > .table .row:not(.header,.new) .invalid')
								.length
							> 0) {

							testRunner.Assert(
								tableView.querySelectorAll('.tableWrapper > div > .table .row.new .invalid').length, 0,
								'New row is invalid');
						}

						TestRunner.Step('Click on the cell again and verify that one more table view is not created');
						cell.dispatchEvent(new Event('click'));
						await TestRunner.Wait(100);
						testRunner.Assert(createdApplications.size, sizeBefore + 1);

						TestRunner.Step('Click on the table view and outside view, verify table view is not destroyed');
						tableView.dispatchEvent(new Event('click'), { bubbles : true });
						await TestRunner.Wait(100);
						testRunner.Assert(createdApplications.size, sizeBefore + 1);

						document.dispatchEvent(new Event('click'), { bubbles : true });
						await TestRunner.Wait(100);
						testRunner.Assert(createdApplications.size, sizeBefore + 1);

						TestRunner.Step('Click on the table view close button, verify table view is destroyed');
						tableView.querySelector('.viewHeader .close').dispatchEvent(new Event('click'));
						await TestRunner.WaitFor(
							() => createdApplications.size == sizeBefore, 'Table view is destroyed');
					}
					else {
						testRunner.Assert(
							cell.innerHTML, value, `Cell value is unexpected for row ${index + 1} and column ${key}`);
						testRunner.Assert(getEventListeners(cell).click, undefined);
						testRunner.Assert(getEventListeners(cell).input, undefined);
					}
				}
			}
			testRunner.Assert(cellCounter, rows[index + 1].querySelectorAll('.cell').length);
		}
	}

	static AddOrUpdateRow(grid, values)
	{
		grid.AddOrUpdateRow(values);
		testRunner.Assert(grid.HasRow(values), true);

		for (const [key, value] of Object.entries(values)) {
			let object = {};
			object[key] = value;
			for (const [key, value] of Object.entries(values)) {
				if (key != key) {
					object[key] = value;
				}
				testRunner.Assert(grid.HasRow(object), true);
				testRunner.Assert(grid.GetRows(object).length > 0, true);
			}
		}
	}

	static SizeOfFilteredRows(grid)
	{
		let counter = 0;
		grid.m_view.querySelectorAll(".row:not(.header)").forEach((row) => {
			if (row.style.display == "none") {
				++counter;
			}
		});

		return counter;
	}

	static async CheckColumnHeaders(grid, values)
	{
		const columnHeaders = grid.m_view.querySelectorAll('.header .cell');
		testRunner.Assert(columnHeaders.length, values.length);
		for (let index = 0; index < values.length; index++) {
			if (values[index].sorting == null || values[index].align == null || values[index].isFilterActive == null
				|| values[index].filters == null) {

				testRunner.Assert(values[index].sorting, null);
				testRunner.Assert(values[index].align, null);
				testRunner.Assert(values[index].isFilterActive, null);
				testRunner.Assert(values[index].filters, null);
				testRunner.Assert(columnHeaders[index].querySelector('.group'), null);
				continue;
			}

			const settings = columnHeaders[index].querySelector('.settings');
			testRunner.Assert(settings != null, true);
			testRunner.Assert(settings.classList.contains('action'), true);

			const sorting = columnHeaders[index].querySelector('.sorting');
			testRunner.Assert(sorting != null, true);
			testRunner.Assert(sorting.classList.contains('action'), true);
			testRunner.Assert(
				sorting.classList.contains('ascending'), values[index].sorting == Grid.SORTING_TYPE.ascending);
			testRunner.Assert(
				sorting.classList.contains('descending'), values[index].sorting == Grid.SORTING_TYPE.descending);
			testRunner.Assert(sorting.classList.contains('active'), values[index].sorting != Grid.SORTING_TYPE.none);
			testRunner.Assert(sorting.classList.contains('disabled'), values[index].sorting == Grid.SORTING_TYPE.none);

			const columnOrder = +columnHeaders[index].style.order;
			let columnObject = grid.m_columnByOrder.get(columnOrder);
			testRunner.Assert(columnObject != undefined, true);
			testRunner.Assert(columnObject.cells.size, grid.m_rowByGridRow.size);

			const filter = columnHeaders[index].querySelector('.filter');
			testRunner.Assert(filter != null, true);
			testRunner.Assert(filter.classList.contains('action'), true);
			testRunner.Assert(
				filter.classList.contains('active'), values[index].isFilterActive, `Column id is ${columnObject.id}`);
			testRunner.Assert(filter.classList.contains('disabled'), values[index].filters.length == 0,
				`Column id is ${columnObject.id}`);

			let settingsView;
			if (values[index].settingsView != null) {
				settingsView = columnHeaders[index].settingsView;
			}
			else {
				const sizeBefore = Application.GetCreatedApplications().size;
				settings.dispatchEvent(new Event('click'), { bubbles : true });
				await TestRunner.WaitFor(
					() => Application.GetCreatedApplications().size > sizeBefore, 'Settings view is created');

				const views = document.querySelectorAll('.views > .view');
				settingsView = Application.GetCreatedApplications().get(+views[views.length - 1].getAttribute('uid'));
			}

			testRunner.Assert(
				settingsView.m_parentView.querySelector('.title > span').innerHTML, "Manage column settings");
			const sortingAscending = settingsView.m_view.querySelector('.ascending');
			testRunner.Assert(sortingAscending != null, true);
			testRunner.Assert(
				sortingAscending.classList.contains('active'), values[index].sorting == Grid.SORTING_TYPE.ascending);
			testRunner.Assert(sortingAscending.classList.contains('action'), true);
			const sortingNone = settingsView.m_view.querySelector('.none');
			testRunner.Assert(sortingNone != null, true);
			testRunner.Assert(
				sortingNone.classList.contains('active'), values[index].sorting == Grid.SORTING_TYPE.none);
			testRunner.Assert(sortingNone.classList.contains('action'), true);
			const sortingDescending = settingsView.m_view.querySelector('.descending');
			testRunner.Assert(sortingDescending != null, true);
			testRunner.Assert(
				sortingDescending.classList.contains('active'), values[index].sorting == Grid.SORTING_TYPE.descending);
			testRunner.Assert(sortingDescending.classList.contains('action'), true);

			const alignLeft = settingsView.m_view.querySelector('.alignLeft');
			testRunner.Assert(alignLeft != null, true);
			testRunner.Assert(alignLeft.classList.contains('active'), values[index].align == Grid.ALIGN_TYPE.left);
			testRunner.Assert(alignLeft.classList.contains('action'), true);
			const alignCenter = settingsView.m_view.querySelector('.alignCenter');
			testRunner.Assert(alignCenter != null, true);
			testRunner.Assert(alignCenter.classList.contains('active'), values[index].align == Grid.ALIGN_TYPE.center);
			testRunner.Assert(alignCenter.classList.contains('action'), true);
			const alignRight = settingsView.m_view.querySelector('.alignRight');
			testRunner.Assert(alignRight != null, true);
			testRunner.Assert(alignRight.classList.contains('active'), values[index].align == Grid.ALIGN_TYPE.right);
			testRunner.Assert(alignRight.classList.contains('action'), true);

			const columnId = +columnHeaders[index].getAttribute('parameter-id');

			GridChecking.CheckColumnAlignment({ grid, columnId, align : values[index].align });

			const filterInView = settingsView.m_view.querySelector('.filter');
			testRunner.Assert(filterInView != null, true);
			testRunner.Assert(filterInView.classList.contains('active'), values[index].isFilterActive,
				`Column id is ${columnObject.id}`);
			testRunner.Assert(filterInView.classList.contains('action'), true);

			const table = [...settingsView.m_tables.entries() ][0][1];
			testRunner.Assert(Helper.DeepEqual(table.GetData(), values[index].filters), true);

			// TestRunner.Step(
			// 'Click on the selection input, check that select view is created and choose one of the values');
			const select = table.m_wrapper.querySelector('.select');
			testRunner.Assert(select != null, true);
			let sizeBefore = Application.GetCreatedApplications().size;
			select.dispatchEvent(new Event('click'));
			await TestRunner.WaitFor(
				() => Application.GetCreatedApplications().size > sizeBefore, 'Select view is created');
			const views = document.querySelectorAll('.views > .view');
			const selectView = Application.GetCreatedApplications().get(+views[views.length - 1].getAttribute('uid'));
			selectView.m_view.querySelector('.options > div').dispatchEvent(new Event('click'));
			await TestRunner.WaitFor(
				() => Application.GetCreatedApplications().size == sizeBefore, 'Select view is destroyed');
			testRunner.Assert(selectView.m_parentView.parentView == null, true);
			testRunner.Assert(settingsView.m_parentView != null, true);

			if (values[index].settingsView == null) {
				document.dispatchEvent(new Event('click'));
				await TestRunner.WaitFor(
					() => Application.GetCreatedApplications().size == sizeBefore - 1, 'Settings view is destroyed');
			}
		}
	}

	static async ChangeSettingsByClick(
		{ grid, inView /* on column overview */, columnId, align, sorting, filter, filters, headers })
	{
		let fillPropertyInData = (property, value) => {
			if (headers == null) {
				return;
			}

			headers.get(columnId)[property] = value;
		};

		const headerCell = grid.m_view.querySelector(`.header .cell[parameter-id="${columnId}"]`);
		testRunner.Assert(headerCell != null, true);
		if (inView) {
			const sizeBefore = Application.GetCreatedApplications().size;
			headerCell.querySelector(`.settings`).dispatchEvent(new Event('click'));
			await TestRunner.WaitFor(() => Application.GetCreatedApplications().size > sizeBefore,
				'Settings view is created', 'Settings view is created');
			const views = document.querySelectorAll('.views > .view');
			const settingsView = views[views.length - 1];
			testRunner.Assert(settingsView != null, true);

			if (align != null) {
				if (align == Grid.ALIGN_TYPE.left) {
					settingsView.querySelector('.alignLeft').dispatchEvent(new Event('click'));
				}
				else if (align == Grid.ALIGN_TYPE.center) {
					settingsView.querySelector('.alignCenter').dispatchEvent(new Event('click'));
				}
				else if (align == Grid.ALIGN_TYPE.right) {
					settingsView.querySelector('.alignRight').dispatchEvent(new Event('click'));
				}

				fillPropertyInData('align', align);
			}

			if (sorting != null) {
				if (sorting == Grid.SORTING_TYPE.ascending) {
					settingsView.querySelector('.ascending').dispatchEvent(new Event('click'));
				}
				else if (sorting == Grid.SORTING_TYPE.descending) {
					settingsView.querySelector('.descending').dispatchEvent(new Event('click'));
				}
				else if (sorting == Grid.SORTING_TYPE.none) {
					sortingButton = settingsView.querySelector('.none');
					if (sortingButton.classList.contains('disabled')) {
						testRunner.Assert(true, false, 'Sorting is not available');
					}
					else {
						settingsView.querySelector('.none').dispatchEvent(new Event('click'));
					}
				}

				fillPropertyInData('sorting', sorting);
			}

			if (filters != null) {
				const filteredBefore = GridChecking.SizeOfFilteredRows(grid);
				await GridChecking.SetFilters(settingsView.getAttribute('uid'), filters);
				const filteredAfter = GridChecking.SizeOfFilteredRows(grid);
				fillPropertyInData('filters', filters);

				if (filters.length == 0 || filteredAfter == 0) {
					fillPropertyInData('isFilterActive', false);
				}
				else if (filteredBefore != filteredAfter) {
					fillPropertyInData('isFilterActive', true);
				}
			}

			if (filter != null) {
				settingsView.querySelector('.filter').dispatchEvent(new Event('click'));
				fillPropertyInData(
					'isFilterActive', !settingsView.querySelector('.filter').classList.contains('active'));
			}

			document.dispatchEvent(new Event('click'));
			await TestRunner.WaitFor(
				() => Application.GetCreatedApplications().size == sizeBefore, 'Settings view is destroyed');

			if (headers != null) {
				await GridChecking.CheckColumnHeaders(grid, Array.from(headers.values()));
			}
			return;
		}

		if (align != null) {
			testRunner.Assert(true, false, 'Align is not available');
		}

		if (sorting != null) {
			const sortingButton = headerCell.querySelector('.sorting');
			if (sortingButton.classList.contains('disabled')) {
				testRunner.Assert(true, false, 'Sorting is not available');
			}
			else {
				if (sorting == Grid.SORTING_TYPE.ascending || sorting == Grid.SORTING_TYPE.descending) {
					sortingButton.dispatchEvent(new Event('click'));
					fillPropertyInData('sorting', sorting);
				}
				else if (sorting == Grid.SORTING_TYPE.none) {
					testRunner.Assert(true, false, 'Sorting with none type is not available');
				}
			}
		}

		if (filter != null) {
			const filterButton = headerCell.querySelector('.filter');
			if (filterButton.classList.contains('disabled')) {
				testRunner.Assert(true, false, `Filter is not available, column id is ${columnId}`);
			}
			else {
				fillPropertyInData('isFilterActive', !filterButton.classList.contains('active'));
				filterButton.dispatchEvent(new Event('click'));
			}
		}

		if (headers != null) {
			await GridChecking.CheckColumnHeaders(grid, Array.from(headers.values()));
		}
	}

	static async SetFilters(settingsViewId, filters)
	{
		const settings = Application.GetCreatedApplications().get(+settingsViewId);
		testRunner.Assert(settings != undefined, true);
		testRunner.Assert(settings.m_tables.size, 1);
		const tableIt = settings.m_tables[Symbol.iterator]();
		const [, table] = tableIt.next().value;
		table.Clear();
		table.Save();

		for (let i = 0; i < filters.length; i++) {
			TableChecker.AddRow(testRunner, table, filters[i]);
		}

		testRunner.Assert(table.GetData(), filters);
	}

	static CheckRowsVisibility(grid, indexes)
	{
		const rows = grid.m_view.querySelectorAll('.row:not(.header)');
		for (let index = 0; index < grid.m_rowByGridRow.size; index++) {
			testRunner.Assert(rows[index].style.display, indexes.includes(index + 2) ? '' : 'none',
				indexes.includes(index + 2) ? `Row ${index + 2} is not visible` : `Row ${index + 2} is visible`);
		}
	}

	static CheckColumnAlignment({ grid, columnId, align })
	{
		const cells = grid.m_view.querySelectorAll(`.row:not(.header) .cell[parameter-id="${columnId}"]`);
		testRunner.Assert(cells.length > 0, true);
		cells.forEach((cell) => {
			testRunner.Assert(
				cell.style.textAlign == "left", align == Grid.ALIGN_TYPE.left, `Column id is ${columnId}`);
			testRunner.Assert(cell.style.textAlign == "center" || cell.style.textAlign == "",
				align == Grid.ALIGN_TYPE.center, `Column id is ${columnId}`);
			testRunner.Assert(
				cell.style.textAlign == "right", align == Grid.ALIGN_TYPE.right, `Column id is ${columnId}`);
		});
	}
}

testRunner.Test('Create and modify grid', async () => {
	TestRunner.Step('Create grid with all parameter types');
	const grid = new Grid({
		parent : body,
		indexColumnId : scalar_parameter,
		columns : [
			duration_seconds_parameter_1, duration_seconds_parameter_1, bool_parameter, scalar_parameter,
			float_point_parameter, optional_scalar_parameter, table_parameter_1, table_parameter_2, select_parameter_1,
			select_parameter_2, string_parameter, timer_parameter, duration_seconds_parameter_2
		]
	});

	testRunner.Assert(grid.m_parent, body);
	testRunner.Assert(grid.m_indexColumnId, scalar_parameter);
	testRunner.Assert(grid.m_postAddRowFunction, undefined);
	testRunner.Assert(grid.m_postUpdateRowFunction, undefined);
	testRunner.Assert(grid.m_view, body.querySelector('.grid'));
	testRunner.Assert(grid.m_header, body.querySelector('.grid').querySelector('.header'));
	GridChecking.CheckColumnsInOrder(grid, [
		{ name : 'Start trading time delay', order : 0 }, { name : 'Active minimum trend strike mode', order : 1 },
		{ name : 'Object identifier', order : 2 }, { name : 'Minimum trend strike price limit', order : 3 },
		{ name : 'Gateway id', order : 4 }, { name : 'Commissions', order : 5 }, { name : 'Figis to trade', order : 6 },
		{ name : 'Server state', order : 7 }, { name : 'Application state', order : 8 }, { name : 'Name', order : 9 },
		{ name : 'Timer', order : 10 }, { name : 'Stop trading time delay', order : 11 }
	]);

	testRunner.Assert(grid.HasRow({ [scalar_parameter] : -9223372036854775808n }), false);
	testRunner.Assert(grid.HasRow({}), false);

	TestRunner.Step('Add row to the grid');
	GridChecking.AddOrUpdateRow(grid, {
		[duration_seconds_parameter_1] : 9223286400000000000n,
		[bool_parameter] : true,
		[scalar_parameter] : -9223372036854775808n,
		[float_point_parameter] : -1.7976931348623157,
		[optional_scalar_parameter] : null,
		[table_parameter_1] : {
			"Rows" : [
				[ -1, -1.7976931348623157 ], [ 0, 50.77693134862315 ], [ 1, -321.4693134862315 ],
				[ 2, 100.797690023157 ], [ 3, -1927634.134862315 ], [ 4, 1.7976931348623157 ], [ 5, -765876 ],
				[ 6, 0.00000010005 ], [ 7, 3 ], [ 8, 1.7976931348623157 ], [ 9, -1.7976931348623157 ],
				[ 10, 1.7976931348623157 ], [ 11, -1.7976931348623157 ]
			]
		},
		[table_parameter_2] : {
			"Rows" :
				[ [ 18446744073709551615n, 18446744073709551615n ], [ -9223372036854775808n, -9223372036854775808n ] ]
		},
		[select_parameter_1] : 1,
		[select_parameter_2] : 1,
		[string_parameter] : "Here is the string parameter, absolutely random thing to display",
		[timer_parameter] : 1742414646123456789n,
		[duration_seconds_parameter_2] : 9223286400000000000n
	});

	await GridChecking.CheckRows(grid, [ {
		[duration_seconds_parameter_1] : "106751d",
		[bool_parameter] : "true",
		[scalar_parameter] : "-9223372036854775808",
		[float_point_parameter] : "-1.7976931348623157",
		[optional_scalar_parameter] : "",
		[table_parameter_1] : {
			"title" : "TableView: Object identifier -9223372036854775808",
			"header" : "Commissions (30014)",
			"columnHeaders" : [ "Instrument type (0) Int16", "% (1) Double" ],
			"values" : [
				[ "-1 - not found", "-1.7976931348623157" ], [ "Undefined", "50.77693134862315" ],
				[ "Bond", "-321.4693134862315" ], [ "Share", "100.797690023157" ], [ "Currency", "-1927634.134862315" ],
				[ "ETF", "1.7976931348623157" ], [ "Futures", "-765876" ], [ "StructuralProduct", "0.00000010005" ],
				[ "Option", "3" ], [ "ClearingCertificate", "1.7976931348623157" ], [ "Index", "-1.7976931348623157" ],
				[ "Commodity", "1.7976931348623157" ], [ "11 - not found", "-1.7976931348623157" ]
			]
		},
		[table_parameter_2] : {
			"title" : "TableView: Object identifier -9223372036854775808",
			"header" : "Figis to trade (30015)",
			"columnHeaders" : [ "figi (0) Uint64", "Default lots volume (1) Uint64" ],
			"values" : [
				[ "18446744073709551615", "18446744073709551615" ], [ "-9223372036854775808", "-9223372036854775808" ]
			]
		},
		[select_parameter_1] : "Initialization",
		[select_parameter_2] : "Paused",
		[string_parameter] : "Here is the string parameter, absolutely random thing to display",
		[timer_parameter] :
			`2025-03-19 ${TestRunner.GetHourWithUtcShift(2025, 3, 20)}:04:06.123456789 UTC${TestRunner.UTC}`,
		[duration_seconds_parameter_2] : "106751d"
	} ]);

	testRunner.Assert(grid.HasRow({ [scalar_parameter] : -9223372036854775808n }), true);
	testRunner.Assert(grid.HasRow({ [optional_scalar_parameter] : null }), true);
	testRunner.Assert(grid.HasRow({ [select_parameter_1] : 1, [select_parameter_2] : 1 }), true);
	testRunner.Assert(grid.HasRow({ [select_parameter_1] : 1, [select_parameter_2] : 2 }), false);
	testRunner.Assert(grid.HasRow({
		[string_parameter] : "Here is the string parameter, absolutely random thing to display",
		[timer_parameter] : 1742414646123456789n,
		[duration_seconds_parameter_2] : 9223286400000000000n
	}),
		true);

	testRunner.Assert(grid.HasRow({
		[table_parameter_1] : {
			"Rows" : [
				[ -1, -1.7976931348623157 ], [ 0, 50.77693134862315 ], [ 1, -321.4693134862315 ],
				[ 2, 100.797690023157 ], [ 3, -1927634.134862315 ], [ 4, 1.7976931348623157 ], [ 5, -765876 ],
				[ 6, 0.00000010005 ], [ 7, 3 ], [ 8, 1.7976931348623157 ], [ 9, -1.7976931348623157 ],
				[ 10, 1.7976931348623157 ], [ 11, -1.7976931348623157 ]
			]
		}
	}),
		true);
	testRunner.Assert(grid.HasRow({
		[table_parameter_1] : {
			"Rows" : [
				[ -1, -1.7976931348623157 ], [ 0, 50.77693134862315 ], [ 1, -321.4693134862315 ],
				[ 2, 100.797690023157 ], [ 3, -1927634.134862315 ], [ 4, 1.7976931348623157 ], [ 5, -765876 ],
				[ 6, 0.00000010005 ], [ 7, 3 ], [ 8, 1.7976931348623157 ], [ 9, -1.7976931348623157 ],
				[ 10, 1.7976931348623155 ], [ 11, -1.7976931348623157 ]
			]
		}
	}),
		false);

	TestRunner.Step('Add one more row to the grid');
	GridChecking.AddOrUpdateRow(grid, {
		[duration_seconds_parameter_1] : 9223286399999999999n,
		[bool_parameter] : false,
		[scalar_parameter] : -9223372036854775807n,
		[float_point_parameter] : -1.7976931348623157,
		[optional_scalar_parameter] : 0,
		[table_parameter_1] : {
			"Rows" : [
				[ -1, -1.7976931348623157 ], [ 0, 50.77693134862315 ], [ 1, -321.4693134862315 ],
				[ 2, 100.797690023157 ], [ 3, -1927634.134862315 ], [ 4, 1.7976931348623157 ], [ 5, -765876 ],
				[ 6, 0.00000010005 ], [ 7, 3 ], [ 8, 1.7976931348623157 ], [ 9, -1.7976931348623157 ],
				[ 10, 1.7976931348623157 ], [ 11, -1.7976931348623157 ]
			]
		},
		[table_parameter_2] : {
			"Rows" :
				[ [ 18446744073709551615n, 18446744073709551615n ], [ -9223372036854775808n, -9223372036854775808n ] ]
		},
		[select_parameter_1] : 2,
		[select_parameter_2] : 2,
		[string_parameter] : "Here is the string parameter, absolutely random thing to display",
		[timer_parameter] : 9190005763000000001n,
		[duration_seconds_parameter_2] : 9223286399999999999n
	});

	await GridChecking.CheckRows(grid, [
		{
			[duration_seconds_parameter_1] : "106751d",
			[bool_parameter] : "true",
			[scalar_parameter] : "-9223372036854775808",
			[float_point_parameter] : "-1.7976931348623157",
			[optional_scalar_parameter] : "",
			[table_parameter_1] : {
				"title" : "TableView: Object identifier -9223372036854775808",
				"header" : "Commissions (30014)",
				"columnHeaders" : [ "Instrument type (0) Int16", "% (1) Double" ],
				"values" : [
					[ "-1 - not found", "-1.7976931348623157" ], [ "Undefined", "50.77693134862315" ],
					[ "Bond", "-321.4693134862315" ], [ "Share", "100.797690023157" ],
					[ "Currency", "-1927634.134862315" ], [ "ETF", "1.7976931348623157" ], [ "Futures", "-765876" ],
					[ "StructuralProduct", "0.00000010005" ], [ "Option", "3" ],
					[ "ClearingCertificate", "1.7976931348623157" ], [ "Index", "-1.7976931348623157" ],
					[ "Commodity", "1.7976931348623157" ], [ "11 - not found", "-1.7976931348623157" ]
				]
			},
			[table_parameter_2] : {
				"title" : "TableView: Object identifier -9223372036854775808",
				"header" : "Figis to trade (30015)",
				"columnHeaders" : [ "figi (0) Uint64", "Default lots volume (1) Uint64" ],
				"values" : [
					[ "18446744073709551615", "18446744073709551615" ],
					[ "-9223372036854775808", "-9223372036854775808" ]
				]
			},
			[select_parameter_1] : "Initialization",
			[select_parameter_2] : "Paused",
			[string_parameter] : "Here is the string parameter, absolutely random thing to display",
			[timer_parameter] :
				`2025-03-19 ${TestRunner.GetHourWithUtcShift(2025, 3, 20)}:04:06.123456789 UTC${TestRunner.UTC}`,
			[duration_seconds_parameter_2] : "106751d"
		},
		{
			[duration_seconds_parameter_1] : "106750d 23h 59m 59s",
			[bool_parameter] : "false",
			[scalar_parameter] : "-9223372036854775807",
			[float_point_parameter] : "-1.7976931348623157",
			[optional_scalar_parameter] : "0",
			[table_parameter_1] : {
				"title" : "TableView: Object identifier -9223372036854775807",
				"header" : "Commissions (30014)",
				"columnHeaders" : [ "Instrument type (0) Int16", "% (1) Double" ],
				"values" : [
					[ "-1 - not found", "-1.7976931348623157" ], [ "Undefined", "50.77693134862315" ],
					[ "Bond", "-321.4693134862315" ], [ "Share", "100.797690023157" ],
					[ "Currency", "-1927634.134862315" ], [ "ETF", "1.7976931348623157" ], [ "Futures", "-765876" ],
					[ "StructuralProduct", "0.00000010005" ], [ "Option", "3" ],
					[ "ClearingCertificate", "1.7976931348623157" ], [ "Index", "-1.7976931348623157" ],
					[ "Commodity", "1.7976931348623157" ], [ "11 - not found", "-1.7976931348623157" ]
				]
			},
			[table_parameter_2] : {
				"title" : "TableView: Object identifier -9223372036854775807",
				"header" : "Figis to trade (30015)",
				"columnHeaders" : [ "figi (0) Uint64", "Default lots volume (1) Uint64" ],
				"values" : [
					[ "18446744073709551615", "18446744073709551615" ],
					[ "-9223372036854775808", "-9223372036854775808" ]
				]
			},
			[select_parameter_1] : "Running",
			[select_parameter_2] : "Running",
			[string_parameter] : "Here is the string parameter, absolutely random thing to display",
			[timer_parameter] :
				`2261-03-21 ${TestRunner.GetHourWithUtcShift(2261, 3, 19)}:22:43.000000001 UTC${TestRunner.UTC}`,
			[duration_seconds_parameter_2] : "106750d 23h 59m 59s 999l 999u 999n"
		}
	]);

	testRunner.Assert(grid.HasRow({ [scalar_parameter] : -9223372036854775807n }), true);
	testRunner.Assert(grid.HasRow({ [scalar_parameter] : -9223372036854775806n }), false);

	TestRunner.Step('Update row in the grid');
	GridChecking.AddOrUpdateRow(grid, {
		[duration_seconds_parameter_1] : 9223286399001345009n,
		[bool_parameter] : true,
		[scalar_parameter] : -9223372036854775807n,
		[float_point_parameter] : 1.7976931348623157,
		[optional_scalar_parameter] : -36384,
		[table_parameter_1] : {
			"Rows" : [
				[ -1, -2.7976931348623157 ], [ 0, 51.77693134862315 ], [ 1, -331.4693134862315 ],
				[ 2, 102.797690023157 ], [ 3, -2927634.134862315 ], [ 4, 1.897693134862315 ], [ 5, -76566876 ],
				[ 6, -0.00400010005 ], [ 7, 3 ], [ 8, 1.798693134862315 ], [ 9, -1.797693134862315 ],
				[ 10, 2.7976931348623157 ], [ 11, -1.7976931368623157 ]
			]
		},
		[table_parameter_2] : {
			"Rows" : [
				[ 18446744073709551614n, 18446744073709551614n ], [ -9223372036854775807n, -9223372036854775807n ],
				[ 18446744073709551615n, 18446744073709551615n ], [ 9223372036854775808n, 9223372036854775808n ]
			]
		},
		[select_parameter_1] : 3,
		[select_parameter_2] : 3,
		[string_parameter] : "",
		[timer_parameter] : 9190005763000000002n,
		[duration_seconds_parameter_2] : 9223286399010000999n
	});

	await GridChecking.CheckRows(grid, [
		{
			[duration_seconds_parameter_1] : "106751d",
			[bool_parameter] : "true",
			[scalar_parameter] : "-9223372036854775808",
			[float_point_parameter] : "-1.7976931348623157",
			[optional_scalar_parameter] : "",
			[table_parameter_1] : {
				"title" : "TableView: Object identifier -9223372036854775808",
				"header" : "Commissions (30014)",
				"columnHeaders" : [ "Instrument type (0) Int16", "% (1) Double" ],
				"values" : [
					[ "-1 - not found", "-1.7976931348623157" ], [ "Undefined", "50.77693134862315" ],
					[ "Bond", "-321.4693134862315" ], [ "Share", "100.797690023157" ],
					[ "Currency", "-1927634.134862315" ], [ "ETF", "1.7976931348623157" ], [ "Futures", "-765876" ],
					[ "StructuralProduct", "0.00000010005" ], [ "Option", "3" ],
					[ "ClearingCertificate", "1.7976931348623157" ], [ "Index", "-1.7976931348623157" ],
					[ "Commodity", "1.7976931348623157" ], [ "11 - not found", "-1.7976931348623157" ]
				]
			},
			[table_parameter_2] : {
				"title" : "TableView: Object identifier -9223372036854775808",
				"header" : "Figis to trade (30015)",
				"columnHeaders" : [ "figi (0) Uint64", "Default lots volume (1) Uint64" ],
				"values" : [
					[ "18446744073709551615", "18446744073709551615" ],
					[ "-9223372036854775808", "-9223372036854775808" ]
				]
			},
			[select_parameter_1] : "Initialization",
			[select_parameter_2] : "Paused",
			[string_parameter] : "Here is the string parameter, absolutely random thing to display",
			[timer_parameter] :
				`2025-03-19 ${TestRunner.GetHourWithUtcShift(2025, 3, 20)}:04:06.123456789 UTC${TestRunner.UTC}`,
			[duration_seconds_parameter_2] : "106751d"
		},
		{
			[duration_seconds_parameter_1] : "106750d 23h 59m 59s",
			[bool_parameter] : "true",
			[scalar_parameter] : "-9223372036854775807",
			[float_point_parameter] : "1.7976931348623157",
			[optional_scalar_parameter] : "-36384",
			[table_parameter_1] : {
				"title" : "TableView: Object identifier -9223372036854775807",
				"header" : "Commissions (30014)",
				"columnHeaders" : [ "Instrument type (0) Int16", "% (1) Double" ],
				"values" : [
					[ "-1 - not found", "-2.7976931348623157" ], [ "Undefined", "51.77693134862315" ],
					[ "Bond", "-331.4693134862315" ], [ "Share", "102.797690023157" ],
					[ "Currency", "-2927634.134862315" ], [ "ETF", "1.897693134862315" ], [ "Futures", "-76566876" ],
					[ "StructuralProduct", "-0.00400010005" ], [ "Option", "3" ],
					[ "ClearingCertificate", "1.798693134862315" ], [ "Index", "-1.797693134862315" ],
					[ "Commodity", "2.7976931348623157" ], [ "11 - not found", "-1.7976931368623157" ]
				]
			},
			[table_parameter_2] : {
				"title" : "TableView: Object identifier -9223372036854775807",
				"header" : "Figis to trade (30015)",
				"columnHeaders" : [ "figi (0) Uint64", "Default lots volume (1) Uint64" ],
				"values" : [
					[ "18446744073709551614", "18446744073709551614" ],
					[ "-9223372036854775807", "-9223372036854775807" ],
					[ "18446744073709551615", "18446744073709551615" ], [ "9223372036854775808", "9223372036854775808" ]
				]
			},
			[select_parameter_1] : "Stopped",
			[select_parameter_2] : "3 - not found",
			[string_parameter] : "",
			[timer_parameter] :
				`2261-03-21 ${TestRunner.GetHourWithUtcShift(2261, 3, 19)}:22:43.000000002 UTC${TestRunner.UTC}`,
			[duration_seconds_parameter_2] : "106750d 23h 59m 59s 10l 999n"
		}
	]);

	TestRunner.Step('Remove row from the grid');
	grid.RemoveRow({ indexValue : -9223372036854775807n });
	await GridChecking.CheckRows(grid, [ {
		[duration_seconds_parameter_1] : "106751d",
		[bool_parameter] : "true",
		[scalar_parameter] : "-9223372036854775808",
		[float_point_parameter] : "-1.7976931348623157",
		[optional_scalar_parameter] : "",
		[table_parameter_1] : {
			"title" : "TableView: Object identifier -9223372036854775808",
			"header" : "Commissions (30014)",
			"columnHeaders" : [ "Instrument type (0) Int16", "% (1) Double" ],
			"values" : [
				[ "-1 - not found", "-1.7976931348623157" ], [ "Undefined", "50.77693134862315" ],
				[ "Bond", "-321.4693134862315" ], [ "Share", "100.797690023157" ], [ "Currency", "-1927634.134862315" ],
				[ "ETF", "1.7976931348623157" ], [ "Futures", "-765876" ], [ "StructuralProduct", "0.00000010005" ],
				[ "Option", "3" ], [ "ClearingCertificate", "1.7976931348623157" ], [ "Index", "-1.7976931348623157" ],
				[ "Commodity", "1.7976931348623157" ], [ "11 - not found", "-1.7976931348623157" ]
			]
		},
		[table_parameter_2] : {
			"title" : "TableView: Object identifier -9223372036854775808",
			"header" : "Figis to trade (30015)",
			"columnHeaders" : [ "figi (0) Uint64", "Default lots volume (1) Uint64" ],
			"values" : [
				[ "18446744073709551615", "18446744073709551615" ], [ "-9223372036854775808", "-9223372036854775808" ]
			]
		},
		[select_parameter_1] : "Initialization",
		[select_parameter_2] : "Paused",
		[string_parameter] : "Here is the string parameter, absolutely random thing to display",
		[timer_parameter] :
			`2025-03-19 ${TestRunner.GetHourWithUtcShift(2025, 3, 20)}:04:06.123456789 UTC${TestRunner.UTC}`,
		[duration_seconds_parameter_2] : "106751d"
	} ]);
	testRunner.Assert(grid.HasRow({ [scalar_parameter] : -9223372036854775807n }), false);
	testRunner.Assert(grid.HasRow({ [scalar_parameter] : -9223372036854775808n }), true);
	testRunner.Assert(grid.HasRow({ [optional_scalar_parameter] : -36384 }), false);
	testRunner.Assert(grid.HasRow({ [optional_scalar_parameter] : null }), true);
	testRunner.Assert(grid.HasRow({ [select_parameter_1] : 3, [select_parameter_2] : 1 }), false);
	testRunner.Assert(grid.HasRow({ [select_parameter_1] : 1, [select_parameter_2] : 1 }), true);
	testRunner.Assert(
		grid.HasRow({ [string_parameter] : "Here is the string parameter, absolutely random thing to display" }), true);
	testRunner.Assert(grid.HasRow({ [string_parameter] : "" }), false);
	testRunner.Assert(grid.HasRow({ [timer_parameter] : 1742414646123456789n }), true);
	testRunner.Assert(grid.HasRow({ [timer_parameter] : 9190005763000000001n }), false);
	testRunner.Assert(grid.HasRow({ [duration_seconds_parameter_2] : 9223286399999999998n }), false);

	{
		TestRunner.Step('Clear rows from the grid');
		const columnsLengthBefore = grid.m_columnByOrder.size;
		grid.ClearRows();
		testRunner.Assert(grid.m_columnByOrder.size, columnsLengthBefore);
		testRunner.Assert(grid.m_rowByGridRow.size, 0);
		testRunner.Assert(grid.m_rowByIndexValue.size, 0);
		await GridChecking.CheckRows(grid, []);
	}

	TestRunner.Step('Destroy grid');
	grid.Destructor();
	testRunner.Assert(grid.m_view, null);
	testRunner.Assert(grid.m_columnByOrder, null);
	testRunner.Assert(grid.m_rowByGridRow, null);
	testRunner.Assert(grid.m_rowByIndexValue, null);
	testRunner.Assert(body.querySelector('.grid'), null);
});

testRunner.Test('Test post add row and column functions, manage rows and columns', async () => {
	TestRunner.Step('Create grid with all post-functions');

	//* External event listener button due to the fact GridChecking.CheckRows checks availability of neccessary event
	//* listeners accordingly to the parameter type
	let incrementButton = document.createElement('button');
	body.appendChild(incrementButton);

	const grid = new Grid({
		parent : body,
		indexColumnId : timer_parameter,
		columns : [ timer_parameter, bool_parameter, scalar_parameter, string_parameter ],
		postAddRowFunction : (rowObject) => {
			let scalarParamRow = rowObject.row.querySelector(`.cell[parameter-id="${scalar_parameter}"]`);
			if (scalarParamRow) {
				incrementButton.addEventListener(
					'click', () => { scalarParamRow.innerHTML = +scalarParamRow.innerHTML + 1; });
			}
		},
		postUpdateRowFunction : (rowObject, updatedValues) => {
			if (updatedValues.hasOwnProperty(bool_parameter)) {
				stringCell = rowObject.row.querySelector(`.cell[parameter-id="${string_parameter}"]`);
				if (stringCell) {
					stringCell.innerHTML = updatedValues[bool_parameter] ? 'True' : 'False';
				}
			}
		}
	});

	testRunner.Assert(grid.m_parent, body);
	testRunner.Assert(grid.m_indexColumnId, timer_parameter);
	testRunner.Assert(grid.m_postAddRowFunction !== undefined, true);
	testRunner.Assert(grid.m_postUpdateRowFunction !== undefined, true);
	testRunner.Assert(grid.m_view, body.querySelector('.grid'));
	testRunner.Assert(grid.m_header, body.querySelector('.grid').querySelector('.header'));
	GridChecking.CheckColumnsInOrder(grid, [
		{ name : 'Timer', order : 0 }, { name : 'Active minimum trend strike mode', order : 1 },
		{ name : 'Object identifier', order : 2 }, { name : 'Name', order : 3 }
	]);

	let data = [];
	for (let i = 0; i < 20; i++) {
		GridChecking.AddOrUpdateRow(grid, {
			[bool_parameter] : i % 2 == 1,
			[scalar_parameter] : -1n + BigInt(i),
			[string_parameter] : "",
			[timer_parameter] : 1745193600000000000n + BigInt(i)
		});

		data.push({
			[bool_parameter] : i % 2 == 1 ? "true" : "false",
			[scalar_parameter] : `${- 1n + BigInt(i)}`,
			[string_parameter] : i % 2 == 1 ? "True" : "False",
			[timer_parameter] : `2025-04-21 ${TestRunner.GetHourWithUtcShift(2025, 4, 0)}:00:00.${
				String(i).padStart(9, '0')} UTC${TestRunner.UTC}`
		});
	}
	GridChecking.CheckRows(grid, data);

	TestRunner.Step('Click on the button and check that scalar parameter is incremented');
	incrementButton.dispatchEvent(new Event('click'), { bubbles : true });
	incrementButton.dispatchEvent(new Event('click'), { bubbles : true });
	await TestRunner.WaitFor(
		() => grid.m_view.querySelectorAll(`.cell[parameter-id="${scalar_parameter}"]`)[1].innerHTML == 1,
		'Scalar parameter is incremented');
	for (let i = 0; i < data.length; i++) {
		data[i][scalar_parameter] = `${- 1n + BigInt(i) + 2n}`;
	}
	GridChecking.CheckRows(grid, data);

	TestRunner.Step('Change some rows');
	for (let i = 0; i < data.length; i++) {
		GridChecking.AddOrUpdateRow(grid, {
			[bool_parameter] : data[i][bool_parameter] == "true" ? false : true,
			[timer_parameter] : 1745193600000000000n + BigInt(i),
			[string_parameter] : `String ${i}`
		});

		data[i][bool_parameter] = data[i][bool_parameter] == "true" ? "false" : "true";
		data[i][string_parameter] = data[i][bool_parameter] == "true" ? "True" : "False";
	}
	await GridChecking.CheckRows(grid, data);

	TestRunner.Step('Update only even rows');
	for (let i = 0; i < data.length; i += 2) {
		GridChecking.AddOrUpdateRow(grid, {
			[bool_parameter] : data[i][bool_parameter] == "true" ? false : true,
			[timer_parameter] : 1745193600000000000n + BigInt(i),
			[string_parameter] : `String ${i}`
		});

		data[i][bool_parameter] = data[i][bool_parameter] == "true" ? "false" : "true";
		data[i][string_parameter] = data[i][bool_parameter] == "true" ? "True" : "False";
	}
	await GridChecking.CheckRows(grid, data);

	TestRunner.Step('Add column in the end of the grid');
	grid.AddColumn({ id : optional_scalar_parameter });
	GridChecking.CheckColumnsInOrder(grid, [
		{ name : 'Timer', order : 0 }, { name : 'Active minimum trend strike mode', order : 1 },
		{ name : 'Object identifier', order : 2 }, { name : 'Name', order : 3 }, { name : 'Gateway id', order : 4 }
	]);
	for (let i = 0; i < data.length; i++) {
		data[i][optional_scalar_parameter] = "";
	}
	GridChecking.CheckRows(grid, data);

	TestRunner.Step('Add column in the beginning of the grid');
	grid.AddColumn({ id : float_point_parameter, order : 0 });
	GridChecking.CheckColumnsInOrder(grid, [
		{ name : 'Minimum trend strike price limit', order : 0 }, { name : 'Timer', order : 1 },
		{ name : 'Active minimum trend strike mode', order : 2 }, { name : 'Object identifier', order : 3 },
		{ name : 'Name', order : 4 }, { name : 'Gateway id', order : 5 }
	]);
	for (let i = 0; i < data.length; i++) {
		data[i][float_point_parameter] = "";
	}
	GridChecking.CheckRows(grid, data);

	TestRunner.Step('Update rows with new columns');
	for (let i = 0; i < data.length; i++) {
		GridChecking.AddOrUpdateRow(grid, {
			[bool_parameter] : data[i][bool_parameter] == "true" ? false : true,
			[timer_parameter] : 1745193600000000000n + BigInt(i),
			[string_parameter] : `String ${i}`,
			[float_point_parameter] : -10.001 + i,
			[optional_scalar_parameter] : -10 + i
		});

		data[i][bool_parameter] = data[i][bool_parameter] == "true" ? "false" : "true";
		data[i][string_parameter] = data[i][bool_parameter] == "true" ? "True" : "False";
		data[i][float_point_parameter] = `${- 10.001 + i}`;
		data[i][optional_scalar_parameter] = `${- 10 + i}`;
	}
	await GridChecking.CheckRows(grid, data);

	TestRunner.Step('Click on the button and check that scalar parameter is incremented');
	incrementButton.dispatchEvent(new Event('click'), { bubbles : true });
	incrementButton.dispatchEvent(new Event('click'), { bubbles : true });
	incrementButton.dispatchEvent(new Event('click'), { bubbles : true });
	incrementButton.dispatchEvent(new Event('click'), { bubbles : true });
	await TestRunner.WaitFor(
		() => grid.m_view.querySelectorAll(`.cell[parameter-id="${scalar_parameter}"]`)[1].innerHTML == 5,
		'Scalar parameter is incremented');
	for (let i = 0; i < data.length; i++) {
		data[i][scalar_parameter] = `${- 1n + BigInt(i) + 6n}`;
	}
	GridChecking.CheckRows(grid, data);

	TestRunner.Step('Remove column from the beggining of the grid');
	grid.RemoveColumn({ order : 0 });
	GridChecking.CheckColumnsInOrder(grid, [
		{ name : 'Timer', order : 0 }, { name : 'Active minimum trend strike mode', order : 1 },
		{ name : 'Object identifier', order : 2 }, { name : 'Name', order : 3 }, { name : 'Gateway id', order : 4 }
	]);
	for (let i = 0; i < data.length; i++) {
		delete data[i][float_point_parameter];
	}
	GridChecking.CheckRows(grid, data);

	TestRunner.Step('Remove column from the end of the grid');
	grid.RemoveColumn({ order : 4 });
	GridChecking.CheckColumnsInOrder(grid, [
		{ name : 'Timer', order : 0 }, { name : 'Active minimum trend strike mode', order : 1 },
		{ name : 'Object identifier', order : 2 }, { name : 'Name', order : 3 }
	]);
	for (let i = 0; i < data.length; i++) {
		delete data[i][optional_scalar_parameter];
	}
	GridChecking.CheckRows(grid, data);

	TestRunner.Step('Update rows');
	for (let i = 0; i < data.length; i++) {
		GridChecking.AddOrUpdateRow(grid, {
			[bool_parameter] : data[i][bool_parameter] == "true" ? false : true,
			[timer_parameter] : 1745193600000000000n + BigInt(i),
			[string_parameter] : `String ${i}`
		});

		data[i][bool_parameter] = data[i][bool_parameter] == "true" ? "false" : "true";
		data[i][string_parameter] = data[i][bool_parameter] == "true" ? "True" : "False";
	}
	await GridChecking.CheckRows(grid, data);

	TestRunner.Step('Click on the button and check that scalar parameter is incremented');
	incrementButton.dispatchEvent(new Event('click'), { bubbles : true });
	incrementButton.dispatchEvent(new Event('click'), { bubbles : true });
	await TestRunner.WaitFor(
		() => grid.m_view.querySelectorAll(`.cell[parameter-id="${scalar_parameter}"]`)[1].innerHTML == 7,
		'Scalar parameter is incremented');
	for (let i = 0; i < data.length; i++) {
		data[i][scalar_parameter] = `${- 1n + BigInt(i) + 8n}`;
	}
	GridChecking.CheckRows(grid, data);

	TestRunner.Step("Add column in the middle of the grid");
	grid.AddColumn({ id : optional_scalar_parameter, order : 2 });
	GridChecking.CheckColumnsInOrder(grid, [
		{ name : 'Timer', order : 0 }, { name : 'Active minimum trend strike mode', order : 1 },
		{ name : 'Gateway id', order : 2 }, { name : 'Object identifier', order : 3 }, { name : 'Name', order : 4 }
	]);
	for (let i = 0; i < data.length; i++) {
		data[i][optional_scalar_parameter] = `${- 10 + i}`;
	}
	GridChecking.CheckRows(grid, data);

	TestRunner.Step('Move columns');
	grid.MoveColumn({ order : 0, newOrder : 1 });
	GridChecking.CheckColumnsInOrder(grid, [
		{ name : 'Active minimum trend strike mode', order : 0 }, { name : 'Timer', order : 1 },
		{ name : 'Gateway id', order : 2 }, { name : 'Object identifier', order : 3 }, { name : 'Name', order : 4 }
	]);
	GridChecking.CheckRows(grid, data);
	grid.MoveColumn({ order : 0, newOrder : 4 });
	GridChecking.CheckColumnsInOrder(grid, [
		{ name : 'Timer', order : 0 }, { name : 'Gateway id', order : 1 }, { name : 'Object identifier', order : 2 },
		{ name : 'Name', order : 3 }, { name : 'Active minimum trend strike mode', order : 4 }
	]);
	GridChecking.CheckRows(grid, data);
	grid.MoveColumn({ order : 4, newOrder : 0 });
	GridChecking.CheckColumnsInOrder(grid, [
		{ name : 'Active minimum trend strike mode', order : 0 }, { name : 'Timer', order : 1 },
		{ name : 'Gateway id', order : 2 }, { name : 'Object identifier', order : 3 }, { name : 'Name', order : 4 }
	]);
	GridChecking.CheckRows(grid, data);
	grid.MoveColumn({ order : 2, newOrder : 1 });
	GridChecking.CheckColumnsInOrder(grid, [
		{ name : 'Active minimum trend strike mode', order : 0 }, { name : 'Gateway id', order : 1 },
		{ name : 'Timer', order : 2 }, { name : 'Object identifier', order : 3 }, { name : 'Name', order : 4 }
	]);
	GridChecking.CheckRows(grid, data);

	TestRunner.Step('Update rows with moved columns');
	for (let i = 0; i < data.length; i++) {
		GridChecking.AddOrUpdateRow(grid, {
			[bool_parameter] : data[i][bool_parameter] == "true" ? false : true,
			[timer_parameter] : 1745193600000000000n + BigInt(i),
			[string_parameter] : `String ${i}`,
			[optional_scalar_parameter] : -10 * i
		});

		data[i][bool_parameter] = data[i][bool_parameter] == "true" ? "false" : "true";
		data[i][string_parameter] = data[i][bool_parameter] == "true" ? "True" : "False";
		data[i][optional_scalar_parameter] = `${- 10 * i}`;
	}
	await GridChecking.CheckRows(grid, data);

	TestRunner.Step('Remove some rows from the grid');
	GridChecking.CheckRowsInOrder(grid, [ 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21 ]);
	for (let i = data.length; i >= 0; i -= 3) {
		grid.RemoveRow({ indexValue : 1745193600000000000n + BigInt(i) });
		data.splice(i, 1);
	}
	GridChecking.CheckRowsInOrder(grid, [ 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 ]);
	await GridChecking.CheckRows(grid, data);

	TestRunner.Step('Swap rows');
	grid.SwapRows({ gridRow1 : 2, gridRow2 : 5 });
	GridChecking.CheckRowsInOrder(grid, [ 5, 3, 4, 2, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 ]);
	await GridChecking.CheckRows(grid, data);

	grid.SwapRows({ gridRow1 : 0, gridRow2 : 2 });
	GridChecking.CheckRowsInOrder(grid, [ 5, 3, 4, 2, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 ]);
	await GridChecking.CheckRows(grid, data);

	grid.SwapRows({ gridRow1 : 2, gridRow2 : 15 });
	GridChecking.CheckRowsInOrder(grid, [ 5, 3, 4, 15, 6, 7, 8, 9, 10, 11, 12, 13, 14, 2 ]);
	await GridChecking.CheckRows(grid, data);

	grid.SwapRows({ gridRow1 : 15, gridRow2 : 3 });
	GridChecking.CheckRowsInOrder(grid, [ 5, 15, 4, 3, 6, 7, 8, 9, 10, 11, 12, 13, 14, 2 ]);
	await GridChecking.CheckRows(grid, data);

	grid.SwapRows({ gridRow1 : 10, gridRow2 : 9 });
	GridChecking.CheckRowsInOrder(grid, [ 5, 15, 4, 3, 6, 7, 8, 10, 9, 11, 12, 13, 14, 2 ]);
	await GridChecking.CheckRows(grid, data);

	TestRunner.Step('Add rows to the grid');
	for (let i = 20; i < 26; i++) {
		GridChecking.AddOrUpdateRow(grid, {
			[bool_parameter] : i % 2 == 1,
			[scalar_parameter] : -1n + BigInt(i),
			[string_parameter] : "",
			[timer_parameter] : 1745193600000000000n + BigInt(i),
			[optional_scalar_parameter] : -10 * i
		});

		data.push({
			[bool_parameter] : i % 2 == 1 ? "true" : "false",
			[scalar_parameter] : `${- 1n + BigInt(i)}`,
			[string_parameter] : i % 2 == 1 ? "True" : "False",
			[timer_parameter] : `2025-04-21 ${TestRunner.GetHourWithUtcShift(2025, 4, 0)}:00:00.${
				String(i).padStart(9, '0')} UTC${TestRunner.UTC}`,
			[optional_scalar_parameter] : `${- 10 * i}`
		});
	}
	GridChecking.CheckRowsInOrder(grid, [ 5, 15, 4, 3, 6, 7, 8, 10, 9, 11, 12, 13, 14, 2, 16, 17, 18, 19, 20, 21 ]);
	GridChecking.CheckRows(grid, data);

	{
		TestRunner.Step('Clear rows from the grid');
		const columnsLengthBefore = grid.m_columnByOrder.size;
		grid.ClearRows();
		testRunner.Assert(grid.m_columnByOrder.size, columnsLengthBefore);
		testRunner.Assert(grid.m_rowByGridRow.size, 0);
		testRunner.Assert(grid.m_rowByIndexValue.size, 0);
		await GridChecking.CheckRows(grid, []);
	}

	TestRunner.Step('Destroy grid');
	grid.Destructor();
	testRunner.Assert(grid.m_view, null);
	testRunner.Assert(grid.m_columnByOrder, null);
	testRunner.Assert(grid.m_rowByGridRow, null);
	testRunner.Assert(grid.m_rowByIndexValue, null);
	testRunner.Assert(body.querySelector('.grid'), null);
});

testRunner.Test('Test sorting, align and filtering functionality. Include settings window', async () => {
	let columns = [
		timer_parameter, bool_parameter, scalar_parameter, string_parameter, float_point_parameter,
		optional_scalar_parameter
	];
	const grid = new Grid({ parent : body, indexColumnId : timer_parameter, columns });

	testRunner.Assert(grid.m_parent, body);
	testRunner.Assert(grid.m_indexColumnId, timer_parameter);
	testRunner.Assert(grid.m_postAddRowFunction == undefined, true);
	testRunner.Assert(grid.m_postUpdateRowFunction == undefined, true);
	testRunner.Assert(grid.m_view, body.querySelector('.grid'));
	testRunner.Assert(grid.m_header, body.querySelector('.grid').querySelector('.header'));
	GridChecking.CheckColumnsInOrder(grid, [
		{ name : 'Timer', order : 0 }, { name : 'Active minimum trend strike mode', order : 1 },
		{ name : 'Object identifier', order : 2 }, { name : 'Name', order : 3 },
		{ name : 'Minimum trend strike price limit', order : 4 }, { name : 'Gateway id', order : 5 }
	]);

	let dictionary = new Map();
	dictionary.set(0, 'Hello World');
	dictionary.set(1, 'JavaScript Code');
	dictionary.set(2, 'Grid Test');
	dictionary.set(3, '');
	dictionary.set(4, 'Collector Application');
	dictionary.set(5, 'Table Checker');
	dictionary.set(6, 'Settings View');
	dictionary.set(7, '	');
	dictionary.set(8, 'Filter Rows');
	dictionary.set(9, 'Columns Values');
	dictionary.set(10, 'Dynamic Grid');
	dictionary.set(11, 'Test Runner');
	dictionary.set(12, ' ');
	dictionary.set(13, 'Application State');
	dictionary.set(14, 'Table Data');
	dictionary.set(15, 'Column Headers');
	dictionary.set(16, 'Row Visibility');
	dictionary.set(17, '\n');
	dictionary.set(18, '\t');
	dictionary.set(19, 'Cell Values');

	let boolParameterAscending = [ 2, 12, 3, 13, 4, 14, 5, 15, 6, 16, 7, 17, 8, 18, 9, 19, 10, 20, 11, 21 ];
	let boolParameterDescending = [ 12, 2, 13, 3, 14, 4, 15, 5, 16, 6, 17, 7, 18, 8, 19, 9, 20, 10, 21, 11 ];
	let timerParameterAscending = [ 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21 ];
	let timerParameterDescending = [ 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2 ];
	let scalarParameterAscending = timerParameterAscending;
	let scalarParameterDescending = timerParameterDescending;
	let stringParameterAscending = [ 15, 16, 14, 2, 9, 19, 18, 3, 13, 11, 12, 21, 6, 7, 20, 10, 17, 5, 4, 8 ];
	let stringParameterDescending = [ 8, 7, 9, 21, 14, 4, 5, 19, 10, 12, 11, 2, 17, 16, 3, 13, 6, 18, 20, 15 ];
	let floatPointParameterAscending = timerParameterAscending;
	let floatPointParameterDescending = timerParameterDescending;
	let optionalScalarParameterAscending = [ 2, 9, 10, 3, 11, 12, 4, 13, 14, 5, 15, 16, 6, 17, 18, 7, 19, 20, 8, 21 ];
	let optionalScalarParameterDescending = [ 15, 14, 13, 16, 12, 11, 17, 10, 9, 18, 8, 7, 19, 6, 5, 20, 4, 3, 21, 2 ];

	let rowsOrders = new Map();
	rowsOrders.set(bool_parameter, { ascending : boolParameterAscending, descending : boolParameterDescending });
	rowsOrders.set(timer_parameter, { ascending : timerParameterAscending, descending : timerParameterDescending });
	rowsOrders.set(scalar_parameter, { ascending : scalarParameterAscending, descending : scalarParameterDescending });
	rowsOrders.set(string_parameter, { ascending : stringParameterAscending, descending : stringParameterDescending });
	rowsOrders.set(float_point_parameter,
		{ ascending : floatPointParameterAscending, descending : floatPointParameterDescending });
	rowsOrders.set(optional_scalar_parameter,
		{ ascending : optionalScalarParameterAscending, descending : optionalScalarParameterDescending });

	let data = [];
	let rawData = [];
	for (let i = 0; i < 20; i++) {
		GridChecking.AddOrUpdateRow(grid, {
			[bool_parameter] : i % 2 == 1,
			[scalar_parameter] : -5n + BigInt(i),
			[string_parameter] : dictionary.get(i),
			[timer_parameter] : 1745193600000000000n + BigInt(i),
			[float_point_parameter] : -10.001 + i,
			[optional_scalar_parameter] : i % 3 == 0 ? null : -10 + i
		});

		rawData.push({
			[bool_parameter] : i % 2 == 1,
			[scalar_parameter] : -5n + BigInt(i),
			[string_parameter] : dictionary.get(i),
			[timer_parameter] : 1745193600000000000n + BigInt(i),
			[float_point_parameter] : -10.001 + i,
			[optional_scalar_parameter] : i % 3 == 0 ? null : -10 + i
		});

		data.push({
			[bool_parameter] : i % 2 == 1 ? "true" : "false",
			[scalar_parameter] : `${- 5n + BigInt(i)}`,
			[string_parameter] : dictionary.get(i),
			[timer_parameter] : `2025-04-21 ${TestRunner.GetHourWithUtcShift(2025, 4, 0)}:00:00.${
				String(i).padStart(9, '0')} UTC${TestRunner.UTC}`,
			[float_point_parameter] : `${- 10.001 + i}`,
			[optional_scalar_parameter] : i % 3 == 0 ? "" : `${- 10 + i}`
		});
	}
	await GridChecking.CheckRows(grid, data);

	let headers = new Map();
	for (const id of columns) {
		headers.set(id, {
			settingsView : null,
			sorting : Grid.SORTING_TYPE.none,
			align : Grid.ALIGN_TYPE.center,
			isFilterActive : false,
			filters : []
		});
	}

	await GridChecking.CheckColumnHeaders(grid, Array.from(headers.values()));

	GridChecking.CheckRowsInOrder(grid, [ 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21 ]);
	GridChecking.CheckRowsVisibility(grid, [ 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21 ]);

	for (const [order, columnObject] of grid.m_columnByOrder.entries()) {
		TestRunner.Step("Change align to left for " + columnObject.id);
		await GridChecking.ChangeSettingsByClick(
			{ grid, inView : true, columnId : columnObject.id, align : Grid.ALIGN_TYPE.left, headers });
	}

	for (const [order, columnObject] of grid.m_columnByOrder.entries()) {
		TestRunner.Step("Change align to right for " + columnObject.id);
		await GridChecking.ChangeSettingsByClick(
			{ grid, inView : true, columnId : columnObject.id, align : Grid.ALIGN_TYPE.right, headers });
	}

	await GridChecking.CheckRows(grid, data);

	for (let index = 0; index < columns.length; ++index) {
		if (index != 0) {
			headers.get(columns[index - 1]).sorting = Grid.SORTING_TYPE.none;
		}

		TestRunner.Step("Change sorting to ascending by view for " + columns[index] + " column id");
		await GridChecking.ChangeSettingsByClick(
			{ grid, inView : true, columnId : columns[index], sorting : Grid.SORTING_TYPE.ascending, headers });
		GridChecking.CheckRowsInOrder(grid, rowsOrders.get(columns[index]).ascending);

		TestRunner.Step("Change sorting to descending by view for " + columns[index] + " column id");
		await GridChecking.ChangeSettingsByClick(
			{ grid, inView : true, columnId : columns[index], sorting : Grid.SORTING_TYPE.descending, headers });
		GridChecking.CheckRowsInOrder(grid, rowsOrders.get(columns[index]).descending);

		TestRunner.Step("Change sorting ascending by column for " + columns[index] + " column id");
		await GridChecking.ChangeSettingsByClick(
			{ grid, inView : false, columnId : columns[index], sorting : Grid.SORTING_TYPE.ascending, headers });
		GridChecking.CheckRowsInOrder(grid, rowsOrders.get(columns[index]).ascending);

		TestRunner.Step("Change sorting descending by column for " + columns[index] + " column id");
		await GridChecking.ChangeSettingsByClick(
			{ grid, inView : false, columnId : columns[index], sorting : Grid.SORTING_TYPE.descending, headers });
		GridChecking.CheckRowsInOrder(grid, rowsOrders.get(columns[index]).descending);

		await GridChecking.CheckRows(grid, data);
	}

	let timerFilters = [
		{ "filters" : [ [ Grid.NUMBER_FILTER.equal, 1745193600000000000n ] ], "shownRows" : [ 2 ] }, {
			"filters" : [ [ Grid.NUMBER_FILTER.notEqual, 1745193600000000001n ] ],
			"shownRows" : [ 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21 ]
		},
		{ "filters" : [ [ Grid.NUMBER_FILTER.less, 1745193600000000007n ] ], "shownRows" : [ 2, 3, 4, 5, 6, 7, 8 ] }, {
			"filters" : [ [ Grid.NUMBER_FILTER.greater, 1745193600000000005n ] ],
			"shownRows" : [ 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21 ]
		},
		{
			"filters" : [ [ Grid.NUMBER_FILTER.lessOrEqual, 1745193600000000007n ] ],
			"shownRows" : [ 2, 3, 4, 5, 6, 7, 8, 9 ]
		},
		{
			"filters" : [ [ Grid.NUMBER_FILTER.greaterOrEqual, 1745193600000000005n ] ],
			"shownRows" : [ 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21 ]
		}
	];
	let boolFilters = [
		{ "filters" : [ [ Grid.BOOL_FILTER.equal, true ] ], "shownRows" : [ 3, 5, 7, 9, 11, 13, 15, 17, 19, 21 ] },
		{ "filters" : [ [ Grid.BOOL_FILTER.equal, false ] ], "shownRows" : [ 2, 4, 6, 8, 10, 12, 14, 16, 18, 20 ] }
	];
	let scalarFilters = [
		{ "filters" : [ [ Grid.NUMBER_FILTER.equal, 0n ] ], "shownRows" : [ 7 ] }, {
			"filters" : [ [ Grid.NUMBER_FILTER.notEqual, 1n ] ],
			"shownRows" : [ 2, 3, 4, 5, 6, 7, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21 ]
		},
		{ "filters" : [ [ Grid.NUMBER_FILTER.less, 4n ] ], "shownRows" : [ 2, 3, 4, 5, 6, 7, 8, 9, 10 ] },
		{ "filters" : [ [ Grid.NUMBER_FILTER.greater, 5n ] ], "shownRows" : [ 13, 14, 15, 16, 17, 18, 19, 20, 21 ] },
		{ "filters" : [ [ Grid.NUMBER_FILTER.lessOrEqual, 4n ] ], "shownRows" : [ 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 ] }, {
			"filters" : [ [ Grid.NUMBER_FILTER.greaterOrEqual, 5n ] ],
			"shownRows" : [ 12, 13, 14, 15, 16, 17, 18, 19, 20, 21 ]
		}
	];
	let stringFilters = [
		{ "filters" : [ [ Grid.STRING_FILTER.equalCaseSensitive, "Grid Test" ] ], "shownRows" : [ 4 ] },
		{ "filters" : [ [ Grid.STRING_FILTER.equalCaseInsensitive, "TABLE DATA" ] ], "shownRows" : [ 16 ] }, {
			"filters" : [ [ Grid.STRING_FILTER.notEqualCaseSensitive, "Dynamic grid" ] ],
			"shownRows" : [ 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21 ]
		},
		{
			"filters" : [ [ Grid.STRING_FILTER.notEqualCaseInsensitive, "Dynamic grid" ] ],
			"shownRows" : [ 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 14, 15, 16, 17, 18, 19, 20, 21 ]
		},
		{ "filters" : [ [ Grid.STRING_FILTER.containsCaseSensitive, "table" ] ], "shownRows" : [] },
		{ "filters" : [ [ Grid.STRING_FILTER.containsCaseInsensitive, "GRID" ] ], "shownRows" : [ 4, 12 ] },
		{ "filters" : [ [ Grid.STRING_FILTER.equalCaseSensitive, "" ] ], "shownRows" : [ 5 ] }
	];
	let floatPointFilters = [
		{ "filters" : [ [ Grid.NUMBER_FILTER.equal, -0.001 ] ], "shownRows" : [ 12 ] }, {
			"filters" : [ [ Grid.NUMBER_FILTER.notEqual, 0.999 ] ],
			"shownRows" : [ 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14, 15, 16, 17, 18, 19, 20, 21 ]
		},
		{ "filters" : [ [ Grid.NUMBER_FILTER.less, -2.001 ] ], "shownRows" : [ 2, 3, 4, 5, 6, 7, 8, 9 ] },
		{ "filters" : [ [ Grid.NUMBER_FILTER.greater, 2.999 ] ], "shownRows" : [ 16, 17, 18, 19, 20, 21 ] },
		{ "filters" : [ [ Grid.NUMBER_FILTER.lessOrEqual, -2.001 ] ], "shownRows" : [ 2, 3, 4, 5, 6, 7, 8, 9, 10 ] },
		{ "filters" : [ [ Grid.NUMBER_FILTER.greaterOrEqual, 2.999 ] ], "shownRows" : [ 15, 16, 17, 18, 19, 20, 21 ] }
	];
	let optionalScalarFilters = [
		{ "filters" : [ [ Grid.NUMBER_FILTER.equal, null ] ], "shownRows" : [ 2, 5, 8, 11, 14, 17, 20 ] }, {
			"filters" : [ [ Grid.NUMBER_FILTER.notEqual, -9n ] ],
			"shownRows" : [ 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21 ]
		},
		{ "filters" : [ [ Grid.NUMBER_FILTER.less, -5n ] ], "shownRows" : [ 3, 4, 6 ] }, {
			"filters" : [ [ Grid.NUMBER_FILTER.greater, -3n ] ],
			"shownRows" : [ 2, 5, 8, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21 ]
		},
		{ "filters" : [ [ Grid.NUMBER_FILTER.lessOrEqual, -5n ] ], "shownRows" : [ 3, 4, 6, 7 ] }, {
			"filters" : [ [ Grid.NUMBER_FILTER.greaterOrEqual, -3n ] ],
			"shownRows" : [ 2, 5, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21 ]
		}
	];

	let rowsFilters = new Map();
	rowsFilters.set(timer_parameter, timerFilters);
	rowsFilters.set(bool_parameter, boolFilters);
	rowsFilters.set(scalar_parameter, scalarFilters);
	rowsFilters.set(string_parameter, stringFilters);
	rowsFilters.set(float_point_parameter, floatPointFilters);
	rowsFilters.set(optional_scalar_parameter, optionalScalarFilters);

	{
		const arr = Array.from(rowsFilters.entries());
		for (let index = 0; index < rowsFilters.size; index++) {
			if (index != 0) {
				await GridChecking.ChangeSettingsByClick(
					{ grid, inView : true, columnId : arr[index - 1][0], filters : [], headers });
				GridChecking.CheckRowsVisibility(
					grid, [ 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21 ]);
			}

			const [parameter, filters] = arr[index];

			for (let i = 0; i < filters.length; i++) {
				TestRunner.Step(`Set filter №${i + 1} for column ${parameter}`);
				await GridChecking.ChangeSettingsByClick(
					{ grid, inView : true, columnId : parameter, filters : filters[i].filters, headers });
				GridChecking.CheckRowsVisibility(grid, filters[i].shownRows);
			}

			if (filters.length > 1) {
				const combinations = getAllCombinationsWithIndexes(filters);

				for (let p = 0; p < combinations.length; p++) {
					const combinedFilters = combinations[p].value.map(f => f.filters[0]);
					const combinedShownRows = combinations[p].value.reduce(
						(acc, f) => acc.filter(row => f.shownRows.includes(row)), combinations[p].value[0].shownRows);

					TestRunner.Step(`Set combined filters (sequence ${p + 1}) for column ${
						parameter}: ${"Filter(s) " + combinations[p].indexes.map((_, index) => `${_}`).join(", ")}`);
					await GridChecking.ChangeSettingsByClick(
						{ grid, inView : true, columnId : parameter, filters : combinedFilters, headers });
					GridChecking.CheckRowsVisibility(grid, combinedShownRows);
				}
			}

			await GridChecking.CheckRows(grid, data);
		}

		function getAllCombinationsWithIndexes(array)
		{
			const result = [];

			const generate = (start, comboValues, comboIndexes) => {
				if (comboValues.length > 0) {
					result.push({ value : [...comboValues ], indexes : [...comboIndexes ] });
				}

				for (let i = start; i < array.length; i++) {
					comboValues.push(array[i]);
					comboIndexes.push(i);
					generate(i + 1, comboValues, comboIndexes);
					comboValues.pop();
					comboIndexes.pop();
				}
			};

			generate(0, [], []);
			return result;
		}
	}

	{
		let combinations = [ {
			filters : [
				{
					column : timer_parameter,
					filters : [ [ Grid.OPTIONAL_NUMBER_FILTER.lessOrEqual, 1745193600000000118n ] ]
				},
				{ column : bool_parameter, filters : [ [ Grid.BOOL_FILTER.equal, true ] ] }, {
					column : scalar_parameter,
					filters : [ [ Grid.NUMBER_FILTER.greater, 0n ], [ Grid.NUMBER_FILTER.lessOrEqual, 12n ] ]
				},
				{
					column : string_parameter,
					filters : [ [ Grid.STRING_FILTER.notEqualCaseInsensitive, "Columns Values" ] ]
				},
				{
					column : float_point_parameter,
					filters : [ [ Grid.NUMBER_FILTER.greater, 0 ], [ Grid.NUMBER_FILTER.less, 7 ] ]
				},
				{ column : optional_scalar_parameter, filters : [ [ Grid.NUMBER_FILTER.notEqual, null ] ] }
			],
			shownRows : [ 13, 15, 19 ]
		} ];

		for (let i = 0; i < columns.length; i++) {
			await GridChecking.ChangeSettingsByClick(
				{ grid, inView : true, columnId : columns[i], filters : [], headers });
		}
		GridChecking.CheckRowsVisibility(
			grid, [ 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21 ]);

		for (let i = 0; i < combinations.length; i++) {
			if (i != 0) {
				for (let j = 0; j < combinations[i - 1].filters.length; j++) {
					const filter = combinations[i - 1].filters[j];
					await GridChecking.ChangeSettingsByClick(
						{ grid, inView : true, columnId : filter.column, filters : [], headers });
				}
				GridChecking.CheckRowsVisibility(
					grid, [ 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21 ]);
			}

			const combination = combinations[i];
			TestRunner.Step(
				`Set combined filters №${i + 1} for columns ${combination.filters.map(f => f.column).join(", ")}`);
			for (let j = 0; j < combination.filters.length; j++) {
				const filter = combination.filters[j];
				await GridChecking.ChangeSettingsByClick(
					{ grid, inView : true, columnId : filter.column, filters : filter.filters, headers });
			}
			GridChecking.CheckRowsVisibility(grid, combination.shownRows);
			await GridChecking.CheckRows(grid, data);
		}

		TestRunner.Step(`Add new rows for grid with active filters`);
		for (let i = 0; i < columns.length; i++) {
			await GridChecking.ChangeSettingsByClick(
				{ grid, inView : true, columnId : columns[i], filters : [], headers });
		}
		GridChecking.CheckRowsVisibility(
			grid, [ 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21 ]);

		for (let j = 0; j < combinations[0].filters.length; j++) {
			const filter = combinations[0].filters[j];
			await GridChecking.ChangeSettingsByClick(
				{ grid, inView : true, columnId : filter.column, filters : filter.filters, headers });
		}
		GridChecking.CheckRowsVisibility(grid, combinations[0].shownRows);
		await GridChecking.CheckColumnHeaders(grid, Array.from(headers.values()));

		for (let i = 0; i < 20; i++) {
			GridChecking.AddOrUpdateRow(grid, {
				[bool_parameter] : i % 2 == 1,
				[scalar_parameter] : -5n + BigInt(i),
				[string_parameter] : dictionary.get(i),
				[timer_parameter] : 1745193600000000020n + BigInt(i),
				[float_point_parameter] : -10.001 + i,
				[optional_scalar_parameter] : i % 3 == 0 ? null : -10 + i
			});

			rawData.push({
				[bool_parameter] : i % 2 == 1,
				[scalar_parameter] : -5n + BigInt(i),
				[string_parameter] : dictionary.get(i),
				[timer_parameter] : 1745193600000000020n + BigInt(i),
				[float_point_parameter] : -10.001 + i,
				[optional_scalar_parameter] : i % 3 == 0 ? null : -10 + i
			});

			data.push({
				[bool_parameter] : i % 2 == 1 ? "true" : "false",
				[scalar_parameter] : `${- 5n + BigInt(i)}`,
				[string_parameter] : dictionary.get(i),
				[timer_parameter] : `2025-04-21 ${TestRunner.GetHourWithUtcShift(2025, 4, 0)}:00:00.${
					String(i + 20).padStart(9, '0')} UTC${TestRunner.UTC}`,
				[float_point_parameter] : `${- 10.001 + i}`,
				[optional_scalar_parameter] : i % 3 == 0 ? "" : `${- 10 + i}`
			});
		}

		await GridChecking.CheckColumnHeaders(grid, Array.from(headers.values()));

		rowsOrders.set(timer_parameter, {
			ascending : [
				2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
				30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41
			],
			descending : [
				41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16,
				15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2
			]
		});
		rowsOrders.set(bool_parameter, {
			ascending : [
				2, 22, 3, 23, 4, 24, 5, 25, 6, 26, 7, 27, 8, 28, 9, 29, 10, 30, 11, 31, 12, 32, 13, 33, 14, 34, 15, 35,
				16, 36, 17, 37, 18, 38, 19, 39
			],
			descending : [
				22, 2, 23, 3, 24, 4, 25, 5, 26, 6, 27, 7, 28, 8, 29, 9, 30, 10, 31, 11, 32, 12, 33, 13, 34, 14, 35, 15,
				36, 16, 37, 17, 38, 18, 39, 19
			]
		});
		rowsOrders.set(scalar_parameter, {
			ascending : [
				2, 4, 8, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 3, 5, 7, 9, 11, 13, 15,
				17, 19, 21, 23, 25, 27, 29, 31, 33, 35, 37, 39, 41
			],
			descending : [
				40, 38, 36, 34, 32, 30, 28, 26, 24, 22, 20, 18, 16, 14, 12, 10, 8, 6, 4, 2, 41, 39, 37, 35, 33, 31, 29,
				27, 25, 23, 21, 19, 17, 15, 13, 11, 9, 7, 5, 3
			]
		});
		rowsOrders.set(string_parameter, {
			ascending : [
				28, 30, 26, 2, 16, 36, 34, 4, 24, 20, 22, 40, 10, 12, 38, 18, 32, 8, 5, 14, 29, 31, 27, 3, 17, 37, 35,
				6, 25, 21, 41, 23, 13, 11, 39, 19, 9, 33, 7, 15
			],
			descending : [
				14, 12, 16, 40, 26, 6, 8, 36, 18, 22, 20, 2, 32, 30, 4, 24, 10, 34, 37, 28, 15, 13, 17, 41, 27, 7, 9,
				38, 19, 23, 3, 21, 31, 33, 5, 25, 35, 11, 39, 29
			]
		});
		rowsOrders.set(float_point_parameter, {
			ascending : [
				2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 3, 5, 7, 9, 11, 13, 15, 17,
				19, 21, 23, 25, 27, 29, 31, 33, 35, 37, 39, 41
			],
			descending : [
				40, 38, 36, 34, 32, 30, 28, 26, 24, 22, 20, 18, 16, 14, 12, 10, 8, 6, 4, 2, 41, 39, 37, 35, 33, 31, 29,
				27, 25, 23, 21, 19, 17, 15, 13, 11, 9, 7, 5, 3
			]
		});
		rowsOrders.set(optional_scalar_parameter, {
			ascending : [
				2, 16, 18, 3, 20, 22, 4, 24, 26, 5, 28, 30, 6, 32, 34, 7, 36, 38, 8, 40, 9, 17, 19, 10, 21, 23, 11, 25,
				27, 12, 39, 31, 13, 33, 35, 14, 37, 39, 15, 41
			],
			descending : [
				28, 26, 24, 29, 22, 20, 30, 18, 16, 31, 14, 12, 32, 10, 8, 33, 6, 4, 34, 2, 35, 27, 25, 36, 23, 21, 37,
				19, 17, 38, 15, 13, 39, 11, 9, 40, 7, 5, 41, 3
			]
		});

		GridChecking.CheckRowsInOrder(grid, rowsOrders.get(optional_scalar_parameter).descending);

		GridChecking.CheckRowsVisibility(grid, [ 13, 15, 19, 33, 35, 39 ]);
		await GridChecking.CheckRows(grid, data);

		TestRunner.Step(`Update some rows which are visible`);
		let swappedData = structuredClone(data);
		for (let swapIndex of [13, 15, 19, 33, 35, 39]) {
			swapIndex = swapIndex - 2;
			swappedData[swapIndex] = structuredClone(data[swapIndex - 1]);
			swappedData[swapIndex][timer_parameter] = data[swapIndex][timer_parameter];
			GridChecking.AddOrUpdateRow(grid, {
				[bool_parameter] : rawData[swapIndex - 1][bool_parameter],
				[scalar_parameter] : rawData[swapIndex - 1][scalar_parameter],
				[string_parameter] : rawData[swapIndex - 1][string_parameter],
				[timer_parameter] : rawData[swapIndex][timer_parameter],
				[float_point_parameter] : rawData[swapIndex - 1][float_point_parameter],
				[optional_scalar_parameter] : rawData[swapIndex - 1][optional_scalar_parameter]
			});

			swappedData[swapIndex - 1] = structuredClone(data[swapIndex]);
			swappedData[swapIndex - 1][timer_parameter] = data[swapIndex - 1][timer_parameter];
			GridChecking.AddOrUpdateRow(grid, {
				[bool_parameter] : rawData[swapIndex][bool_parameter],
				[scalar_parameter] : rawData[swapIndex][scalar_parameter],
				[string_parameter] : rawData[swapIndex][string_parameter],
				[timer_parameter] : rawData[swapIndex - 1][timer_parameter],
				[float_point_parameter] : rawData[swapIndex][float_point_parameter],
				[optional_scalar_parameter] : rawData[swapIndex][optional_scalar_parameter]
			});

			await GridChecking.CheckRows(grid, swappedData);

			for (let array of rowsOrders.values()) {
				let tmp = array.ascending[swapIndex - 1];
				array.ascending[swapIndex - 1] = array.ascending[swapIndex];
				array.ascending[swapIndex] = tmp;

				tmp = array.descending[swapIndex - 1];
				array.descending[swapIndex - 1] = array.descending[swapIndex];
				array.descending[swapIndex] = tmp;
			}
		}

		GridChecking.CheckRowsInOrder(grid, rowsOrders.get(optional_scalar_parameter).descending);
		GridChecking.CheckRowsVisibility(grid, [ 12, 14, 18, 32, 34, 38 ]);

		TestRunner.Step(`Remove column with active filters`);
		{
			const headerCell = grid.m_view.querySelector(`.header .cell[parameter-id="${optional_scalar_parameter}"]`);
			testRunner.Assert(headerCell != null, true);
			const sizeBefore = Application.GetCreatedApplications().size;
			headerCell.querySelector(`.settings`).dispatchEvent(new Event('click'));
			await TestRunner.WaitFor(() => Application.GetCreatedApplications().size > sizeBefore);

			grid.RemoveColumn({ order : 5 });
			await TestRunner.WaitFor(() => Application.GetCreatedApplications().size == sizeBefore);

			let header = headers.get(optional_scalar_parameter);
			header.sorting = Grid.SORTING_TYPE.none;
			header.filters = [];
			header.align = Grid.ALIGN_TYPE.center;
			header.isFilterActive = false;
		}
		GridChecking.CheckRowsVisibility(grid, [ 12, 14, 17, 18, 32, 34, 37, 38 ]);
		GridChecking.CheckColumnsInOrder(grid, [
			{ name : 'Timer', order : 0 }, { name : 'Active minimum trend strike mode', order : 1 },
			{ name : 'Object identifier', order : 2 }, { name : 'Name', order : 3 },
			{ name : 'Minimum trend strike price limit', order : 4 }
		]);
		GridChecking.CheckRowsInOrder(grid, [
			2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
			31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41
		]);

		grid.AddColumn({ id : optional_scalar_parameter });
		GridChecking.CheckRowsVisibility(grid, [ 12, 14, 17, 18, 32, 34, 37, 38 ]);
		GridChecking.CheckColumnsInOrder(grid, [
			{ name : 'Timer', order : 0 }, { name : 'Active minimum trend strike mode', order : 1 },
			{ name : 'Object identifier', order : 2 }, { name : 'Name', order : 3 },
			{ name : 'Minimum trend strike price limit', order : 4 }, { name : 'Gateway id', order : 5 }
		]);
		GridChecking.CheckRowsInOrder(grid, [
			2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
			31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41
		]);

		await GridChecking.CheckColumnHeaders(grid, Array.from(headers.values()));

		for (let columnId of columns.slice(0, 5)) {
			GridChecking.ChangeSettingsByClick({ grid, inView : false, columnId, filter : false });
		}
		GridChecking.CheckRowsVisibility(grid, [
			2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
			31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41
		]);

		for (let columnId of columns.slice(0, 5)) {
			GridChecking.ChangeSettingsByClick({ grid, inView : false, columnId, filter : true });
		}
		GridChecking.CheckRowsVisibility(grid, [ 12, 14, 17, 18, 32, 34, 37, 38 ]);
	}

	TestRunner.Step('Check number of settings views');
	testRunner.Assert(Grid.GetSettingsViewsNumber(), 1);
	document.dispatchEvent(new Event('click'));
	testRunner.Assert(Grid.GetSettingsViewsNumber(), 0);

	{
		TestRunner.Step('Clear rows from the grid');
		const columnsLengthBefore = grid.m_columnByOrder.size;
		grid.ClearRows();
		testRunner.Assert(grid.m_columnByOrder.size, columnsLengthBefore);
		testRunner.Assert(grid.m_rowByGridRow.size, 0);
		testRunner.Assert(grid.m_rowByIndexValue.size, 0);
		await GridChecking.CheckRows(grid, []);
	}

	TestRunner.Step('Destroy grid');
	grid.Destructor();
	testRunner.Assert(grid.m_view, null);
	testRunner.Assert(grid.m_columnByOrder, null);
	testRunner.Assert(grid.m_rowByGridRow, null);
	testRunner.Assert(grid.m_rowByIndexValue, null);
	testRunner.Assert(body.querySelector('.grid'), null);
});

testRunner.Test('Test table type, its view logic', async () => {
	TestRunner.Step('Create grid with table and system parameters');
	const grid = new Grid({
		parent : body,
		indexColumnId : scalar_parameter,
		columns : [ table_parameter_1, system_create_parameter, scalar_parameter ]
	});

	testRunner.Assert(grid.m_parent, body);
	testRunner.Assert(grid.m_indexColumnId, scalar_parameter);
	testRunner.Assert(grid.m_postAddRowFunction, undefined);
	testRunner.Assert(grid.m_postUpdateRowFunction, undefined);
	testRunner.Assert(grid.m_view, body.querySelector('.grid'));
	testRunner.Assert(grid.m_header, body.querySelector('.grid').querySelector('.header'));
	GridChecking.CheckColumnsInOrder(grid, [
		{ name : 'Commissions', order : 0 }, { name : 'Create', order : 1 }, { name : 'Object identifier', order : 2 }
	]);

	TestRunner.Step('Add two rows with empty table');
	GridChecking.AddOrUpdateRow(
		grid, { [table_parameter_1] : { "Rows" : [] }, [system_create_parameter] : null, [scalar_parameter] : 0n });
	GridChecking.AddOrUpdateRow(
		grid, { [table_parameter_1] : { "Rows" : [] }, [system_create_parameter] : null, [scalar_parameter] : 1n });

	await GridChecking.CheckRows(grid, [
		{
			[table_parameter_1] : {
				"title" : "TableView: Object identifier 0",
				"header" : "Commissions (30014)",
				"columnHeaders" : [ "Instrument type (0) Int16", "% (1) Double" ],
				"values" : []
			},
			[system_create_parameter] : '',
			[scalar_parameter] : '0'
		},
		{
			[table_parameter_1] : {
				"title" : "TableView: Object identifier 1",
				"header" : "Commissions (30014)",
				"columnHeaders" : [ "Instrument type (0) Int16", "% (1) Double" ],
				"values" : []
			},
			[system_create_parameter] : '',
			[scalar_parameter] : '1'
		}
	]);

	TestRunner.Step('Check, that there is not settings button for table and system parameters');
	for (let parameter of [table_parameter_1, system_create_parameter]) {
		const headerCell = grid.m_view.querySelector(`.header .cell[parameter-id="${parameter}"]`);
		testRunner.Assert(headerCell != null, true);
		testRunner.Assert(headerCell.querySelector(`.settings`), null);
	}

	{
		TestRunner.Step('Open table view for first and second rows');
		const tableCells = grid.m_view.querySelectorAll(`.row:not(.header) .cell[parameter-id="${table_parameter_1}"]`);
		testRunner.Assert(tableCells.length, 2);

		const applicationsBefore = Application.GetCreatedApplications().size;
		tableCells[0].dispatchEvent(new Event('click'));
		tableCells[1].dispatchEvent(new Event('click'));
		await TestRunner.WaitFor(() => Application.GetCreatedApplications().size == applicationsBefore + 2);

		let views = Array.from(Application.GetCreatedApplications().values());
		let tableView1 = views[applicationsBefore];
		let tableView2 = views[applicationsBefore + 1];

		TableChecker.CheckValues(testRunner, tableView1.m_view.querySelectorAll('.row:not(.new) input'), []);
		TableChecker.CheckValues(testRunner, tableView2.m_view.querySelectorAll('.row:not(.new) input'), []);

		TestRunner.Step('Update table parameter for first row');
		GridChecking.AddOrUpdateRow(
			grid, { [table_parameter_1] : { "Rows" : [ [ 1n, 0.1 ] ] }, [scalar_parameter] : 0n });
		TableChecker.CheckValues(
			testRunner, tableView1.m_view.querySelectorAll('.row:not(.new) input'), [ "Bond", "0.1" ]);
		TableChecker.CheckValues(testRunner, tableView2.m_view.querySelectorAll('.row:not(.new) input'), []);

		TestRunner.Step('Update table parameter for second row');
		GridChecking.AddOrUpdateRow(
			grid, { [table_parameter_1] : { "Rows" : [ [ 2n, 0.2 ], [ 3n, 0.44 ] ] }, [scalar_parameter] : 1n });
		TableChecker.CheckValues(
			testRunner, tableView1.m_view.querySelectorAll('.row:not(.new) input'), [ "Bond", "0.1" ]);
		TableChecker.CheckValues(testRunner, tableView2.m_view.querySelectorAll('.row:not(.new) input'),
			[ "Share", "0.2", "Currency", "0.44" ]);

		TestRunner.Step('Close table views');
		tableView1.m_parentView.querySelector('.close').dispatchEvent(new Event('click'));
		tableView2.m_parentView.querySelector('.close').dispatchEvent(new Event('click'));
		await TestRunner.WaitFor(() => Application.GetCreatedApplications().size == applicationsBefore);
		testRunner.Assert(Grid.GetTablesViewsNumber(), 2);

		document.dispatchEvent(new Event('click'));
		await TestRunner.WaitFor(() => Grid.GetSettingsViewsNumber() == 0);

		TestRunner.Step('Open table views again and check, that they are the same');
		tableCells[0].dispatchEvent(new Event('click'));
		tableCells[1].dispatchEvent(new Event('click'));
		await TestRunner.WaitFor(() => Application.GetCreatedApplications().size == applicationsBefore + 2);

		views = Array.from(Application.GetCreatedApplications().values());
		tableView1 = views[applicationsBefore];
		tableView2 = views[applicationsBefore + 1];
		TableChecker.CheckValues(
			testRunner, tableView1.m_view.querySelectorAll('.row:not(.new) input'), [ "Bond", "0.1" ]);
		TableChecker.CheckValues(testRunner, tableView2.m_view.querySelectorAll('.row:not(.new) input'),
			[ "Share", "0.2", "Currency", "0.44" ]);
		testRunner.Assert(Grid.GetTablesViewsNumber(), 2);
	}
});

testRunner.Run();