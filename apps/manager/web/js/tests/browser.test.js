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
		browser.ResizeObserver = class {
			observe() { }
			disconnect() { }
		};
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
		testRunner.Assert(
			runInContext('MetadataCollector.GetMetadataObject(11).definition.name', context), 'Time accuracy');
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
				metadata: {name: 'Timestamp', type: 'Timer'},
				rowObjectToCellNode: new Map([[{values: {1: timestamp}}, cell]])
			};
			new GridColumnSettingsView({columnObject, viewTitle: 'Accuracy settings'});
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
					metadata: {name: 'Price', type: '${type}', accuracy: 0.01},
					rowObjectToCellNode: new Map([[{values: {1: 1.23456}}, cell]])
				};
				let settings = new GridColumnSettingsView({columnObject, viewTitle: 'Price accuracy'});
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
				`settings.Destructor(); settings = new GridColumnSettingsView({columnObject, viewTitle: 'Reopened'});`,
				context);
			testRunner.Assert(Number(browser.document.querySelector('.group.accuracy input').value), minimum);
			testRunner.Assert(runInContext('columnObject.metadata.accuracy', context), 0.01);
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
			const columnObject = {metadata: {type: 'String'}, rowObjectToCellNode: new Map()};
			let settings = new GridColumnSettingsView({columnObject, viewTitle: 'No accuracy'});
		`,
			context);
		testRunner.Assert(browser.document.querySelector('.group.accuracy input'), null);
		runInContext(`
			settings.Destructor();
			columnObject.metadata.type = 'Double';
			settings = new GridColumnSettingsView({columnObject, viewTitle: 'Default accuracy'});
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

testRunner.Test('Clamp initial view positions after content initialization', () => {
	const dom = CreateBrowser();
	try {
		const browser = dom.window;
		const context = dom.getInternalVMContext();
		// JSDOM has no layout: model the dimensions established by the view constructor.
		Object.defineProperty(
			browser.HTMLElement.prototype, 'offsetWidth', { get() { return parseFloat(this.style.width) || 0; } });
		Object.defineProperty(
			browser.HTMLElement.prototype, 'offsetLeft', { get() { return parseFloat(this.style.left) || 0; } });
		Object.defineProperty(
			browser.HTMLElement.prototype, 'offsetTop', { get() { return parseFloat(this.style.top) || 0; } });
		Object.defineProperty(browser, 'innerWidth', { value : 1000, configurable : true });
		runInContext(`
			View.AddViewTemplate('PositionTest', '<div></div>');
			class PositionTest extends View {
				constructor(parameters) { super('PositionTest', parameters); }
				Constructor(parameters) {
					this.m_rootNode.style.width = parameters.width + 'px';
					this.m_rootNode.style.left = parameters.left + 'px';
					this.m_rootNode.style.top = '75px';
					return true;
				}
			}
		`,
			context);
		for (const [left, width, expected] of [
				 [ 900, 300, 700 ], [ 500, 300, 500 ], [ -20, 300, 0 ], [ 0, 1000, 0 ], [ 900, 1200, 0 ]]) {
			for (const anchored of [false, true]) {
				const view = runInContext(
					`new PositionTest({
					left: ${left}, width: ${width},
					${
						anchored ? `positionUnder: {getBoundingClientRect() { return {left: ${left}, bottom: 250}; }}`
								 : ''}
				})`,
					context);
				testRunner.Assert(view.m_created, true);
				testRunner.Assert(view.m_rootNode.style.left, expected + 'px');
				testRunner.Assert(view.m_rootNode.style.top, anchored ? '250px' : '75px');
				view.Destructor();
			}
		}
	}
	finally {
		dom.window.close();
	}
});

testRunner.Test('Keep column settings inside the window after its filter table expands the view', () => {
	const dom = CreateBrowser();
	try {
		const browser = dom.window;
		const context = dom.getInternalVMContext();
		const observers = [];
		browser.ResizeObserver = class {
			constructor(callback)
			{
				this.callback = callback;
				observers.push(this);
			}
			observe(target) { this.target = target; }
			disconnect() { this.target = null; }
		};
		// The filter table is appended by Grid after the settings view constructor finishes.
		Object.defineProperty(browser.HTMLElement.prototype, 'offsetWidth',
			{ get() { return this.querySelector('.filters .table') ? 420 : 220; } });
		Object.defineProperty(
			browser.HTMLElement.prototype, 'offsetLeft', { get() { return parseFloat(this.style.left) || 0; } });
		Object.defineProperty(
			browser.HTMLElement.prototype, 'offsetTop', { get() { return parseFloat(this.style.top) || 0; } });
		Object.defineProperty(browser, 'innerWidth', { value : 1000 });
		Object.defineProperty(browser, 'innerHeight', { value : 768 });
		const settings = runInContext(`
			MetadataCollector.AddMetadata(66001, {name: 'Price', type: 'Int32'}, true);
			const testGrid = new Grid({
				parent: document.querySelector('main > section.views'), indexColumnId: 66001, columns: [66001]
			});
			const column = testGrid.m_idToColumnObject.get(66001);
			column.headerCell.getBoundingClientRect = () => ({left: 900, bottom: 767});
			column.headerCell.querySelector('.settings').click();
			View.GetLastCreatedView();
		`,
			context);
		testRunner.Assert(settings.m_rootNode.querySelector('.filters .table') != null, true);
		for (const observer of observers) {
			if (observer.target === settings.m_rootNode) {
				observer.callback();
			}
		}
		testRunner.Assert(settings.m_rootNode.style.left, '580px');
		testRunner.Assert(settings.m_rootNode.style.top, '718px');
		const observer = observers.find(item => item.target === settings.m_rootNode);
		settings.m_rootNode.classList.add('hidden');
		// Hidden views have zero layout offsets but must retain their saved position.
		Object.defineProperty(settings.m_rootNode, 'offsetLeft', { value : 0 });
		Object.defineProperty(settings.m_rootNode, 'offsetTop', { value : 0 });
		observer.callback();
		testRunner.Assert(settings.m_rootNode.style.left, '580px');
		testRunner.Assert(settings.m_rootNode.style.top, '718px');
		settings.Destructor();
		testRunner.Assert(observer.target, null, 'Closing the view disconnects its position observer');
		observer.callback();
		runInContext('testGrid.Destructor();', context);
	}
	finally {
		dom.window.close();
	}
});

testRunner.Test('Use each enum column options in generated grid filter tables', async () => {
	const dom = CreateBrowser();
	try {
		const browser = dom.window;
		const context = dom.getInternalVMContext();
		runInContext(`
			MetadataCollector.AddMetadata(122, {name: 'Price freshness', type: 'Uint8',
				stringInterpretations: {0: 'Undefined', 1: 'Within day', 2: 'Within week'}}, true);
			MetadataCollector.AddMetadata(123, {name: 'Coupon type', type: 'Uint8',
				stringInterpretations: {0: 'Undefined', 1: 'Fixed', 2: 'Floating'}}, true);
			const testGrid = new Grid({
				parent: document.querySelector('main > section.views'), indexColumnId: 122, columns: [122, 123]
			});
		`,
			context);
		for (const [id, initial, selected] of [[ 122, 'Within day', 'Within week' ], [ 123, 'Fixed', 'Floating' ],
				 [ 122, 'Within day', 'Within week' ]]) {
			const columnObject = runInContext(`testGrid.m_idToColumnObject.get(${id})`, context);
			runInContext(`testGrid.m_idToColumnObject.get(${id}).filters = [[0, 1]];`, context);
			columnObject.headerCell.querySelector('.settings').click();
			const settings = runInContext('View.GetLastCreatedView()', context);
			const table = settings.m_tableIdToTable.get(columnObject.systemTableMetadataId);
			const inputs = table.m_wrapper.querySelector('.row:not(.header):not(.new)').querySelectorAll('input');
			testRunner.Assert(inputs[0].hasAttribute('select'), true, 'The filter operator remains a select');
			testRunner.Assert(inputs[1].hasAttribute('select'), true, 'The enum value must be a select');
			testRunner.Assert(inputs[1].getAttribute('parameter-id'), String(id));
			testRunner.Assert(inputs[1].value, initial);
			inputs[1].click();
			const select = runInContext('View.GetLastCreatedView()', context);
			testRunner.Assert(select.m_viewType, 'SelectView');
			const option = Array.from(select.m_viewNode.querySelectorAll('.options > div'))
							   .find(node => node.firstElementChild.textContent === selected);
			testRunner.Assert(option != null, true, 'The value dropdown uses this column options');
			option.click();
			testRunner.Assert(inputs[1].value, selected);
			// Select attribute mutations mark the table as changed in the next microtask.
			await Promise.resolve();
			table.Save();
			testRunner.Assert(columnObject.filters, [ [ 0n, 2n ] ], 'Saving preserves the numeric enum key');
			settings.Destructor();
		}
		runInContext('testGrid.Destructor();', context);
	}
	finally {
		dom.window.close();
	}
});

testRunner.Test('Keep title information independent and use current title text in the dispatcher', () => {
	const dom = CreateBrowser();
	try {
		const browser = dom.window;
		const context = dom.getInternalVMContext();
		const view = runInContext(`
			View.AddViewTemplate('TitleTest', '<div></div>');
			class TitleTest extends View {
				constructor() { super('TitleTest'); }
				Constructor() { return true; }
			}
			const view = new TitleTest();
			view;
		`,
			context);
		testRunner.Assert(view.m_created, true);
		testRunner.Assert(view.m_titleTextNode.textContent, 'TitleTest');
		testRunner.Assert(view.m_titleTextNode.title, 'TitleTest');
		testRunner.Assert(Object.hasOwn(view, 'm_title'), false, 'The title is stored in its DOM nodes');

		view.SetTitleText('<b>Application</b>');
		view.SetTitleInfo('Market data on port 16614');
		view.SetTitleInfo('Orders on port 16615');
		testRunner.Assert(view.m_titleTextNode.textContent, '<b>Application</b>');
		testRunner.Assert(view.m_titleTextNode.childElementCount, 0, 'Title text must not be interpreted as HTML');
		testRunner.Assert(view.m_titleTextNode.title, '<b>Application</b>: Orders on port 16615');
		testRunner.Assert(view.m_titleInfoNode.textContent, 'Orders on port 16615');
		testRunner.Assert(view.m_titleInfoNode.hasAttribute('title'), false, 'Title information has no tooltip');
		testRunner.Assert(view.GetTitle(), '<b>Application</b>: Orders on port 16615');

		view.Hide();
		const entry = browser.document.querySelector('.hiddenViews > .list > div > span');
		testRunner.Assert(entry.textContent, view.GetTitle());
		testRunner.Assert(entry.hasAttribute('title'), false, 'Only the title text node has a title tooltip');
		testRunner.Assert(entry.childElementCount, 0, 'Dispatcher labels must not be interpreted as HTML');
		view.Show();

		view.SetTitleText('Application');
		testRunner.Assert(view.m_titleTextNode.title, 'Application: Orders on port 16615');
		testRunner.Assert(view.m_titleInfoNode.textContent, 'Orders on port 16615');
		view.SetTitleInfo('');
		testRunner.Assert(view.GetTitle(), 'Application');
		testRunner.Assert(view.m_titleTextNode.title, 'Application');
		testRunner.Assert(view.m_titleInfoNode.hasAttribute('title'), false);
		view.Destructor();
	}
	finally {
		dom.window.close();
	}
});

testRunner.Test('Keep view placement and pointer operations aligned below a page header', () => {
	for (const [left, top, borderLeft, borderTop, scrollLeft, scrollTop, width, height] of [
			 [ 0, 40, 0, 0, 0, 0, 1000, 728 ], [ 30, 96, 2, 3, 11, 17, 900, 600 ],
			 [ -30, -15, 2, 3, 11, 17, 1200, 900 ]]) {
		const dom = CreateBrowser();
		try {
			const browser = dom.window;
			const context = dom.getInternalVMContext();
			const main = browser.document.querySelector('main');
			const visibleLeft = Math.max(0, left + borderLeft);
			const visibleTop = Math.max(0, top + borderTop);
			const visibleRight = Math.min(1000, left + borderLeft + width);
			const visibleBottom = Math.min(768, top + borderTop + height);
			Object.defineProperties(browser, { innerWidth : { value : 1000 }, innerHeight : { value : 768 } });
			main.getBoundingClientRect = () => ({ left, top });
			Object.defineProperties(main, {
				clientLeft : { value : borderLeft },
				clientTop : { value : borderTop },
				clientWidth : { value : width },
				clientHeight : { value : height }
			});
			main.scrollLeft = scrollLeft;
			main.scrollTop = scrollTop;
			// JSDOM has no layout: model the containing block and the resulting viewport rectangles.
			Object.defineProperties(browser.HTMLElement.prototype, {
				offsetParent : { get() { return this.classList.contains('view') ? main : null; } },
				offsetLeft : { get() { return parseFloat(this.style.left) || 0; } },
				offsetTop : { get() { return parseFloat(this.style.top) || 0; } },
				offsetWidth : {
					get() {
						return (parseFloat(this.style.width) || 0) * (this.style.width.endsWith('%') ? width / 100 : 1);
					}
				},
				offsetHeight : {
					get() {
						if (this.classList.contains('viewHeader')) {
							return 20;
						}
						return (parseFloat(this.style.height) || 0)
							* (this.style.height.endsWith('%') ? height / 100 : 1);
					}
				}
			});
			const originalRect = browser.HTMLElement.prototype.getBoundingClientRect;
			browser.HTMLElement.prototype.getBoundingClientRect = function() {
				if (!this.classList.contains('view')) {
					return originalRect.call(this);
				}
				const x = left + borderLeft - main.scrollLeft + this.offsetLeft;
				const y = top + borderTop - main.scrollTop + this.offsetTop;
				return { left : x, top : y, right : x + this.offsetWidth, bottom : y + this.offsetHeight };
			};
			runInContext(`
				View.AddViewTemplate('OffsetTest', '<div></div>');
				class OffsetTest extends View {
					constructor(parameters) { super('OffsetTest', parameters); }
					Constructor() {
						this.m_rootNode.style.width = '200px';
						this.m_rootNode.style.height = '100px';
						return true;
					}
				}
			`,
				context);
			const view = runInContext(`new OffsetTest({
				positionUnder: {getBoundingClientRect() { return {left: 250, bottom: 200}; }},
				canBeClinged: false
			})`,
				context);
			const root = view.m_rootNode;
			const handle = view.m_headerNode.querySelector('.title');
			const mouse = (target, type, clientX, clientY) => {
				target.dispatchEvent(new browser.MouseEvent(type, { bubbles : true, button : 0, clientX, clientY }));
			};
			testRunner.Assert(view.m_created, true);
			testRunner.Assert(root.getBoundingClientRect().left, 250, 'Popup aligns with its viewport anchor');
			testRunner.Assert(root.getBoundingClientRect().top, 200, 'Popup starts directly below its anchor');

			mouse(handle, 'mousedown', 260, 210);
			mouse(browser.document, 'mousemove', 260, 210);
			testRunner.Assert(root.getBoundingClientRect().top, 200, 'Starting a drag must not add the header height');
			mouse(browser.document, 'mousemove', 293, 238);
			testRunner.Assert(root.getBoundingClientRect().left, 283);
			testRunner.Assert(root.getBoundingClientRect().top, 228);
			main.scrollTop += 13;
			mouse(browser.document, 'mousemove', 293, 238);
			testRunner.Assert(root.getBoundingClientRect().top, 228, 'Scrolling preserves the pointer grab offset');
			mouse(browser.document, 'mousemove', -100, -100);
			testRunner.Assert(root.getBoundingClientRect().left, visibleLeft);
			testRunner.Assert(root.getBoundingClientRect().top, visibleTop, 'Dragging stays below the page header');
			mouse(browser.document, 'mouseup', -100, -100);

			root.style.left = '5000px';
			root.style.top = '5000px';
			view.ClampPosition();
			testRunner.Assert(root.getBoundingClientRect().right, visibleRight);
			testRunner.Assert(root.getBoundingClientRect().top, visibleBottom - 50,
				'Position clamping keeps the minimum visible height inside the containing block');

			const other = runInContext('new OffsetTest({canBeClinged: true})', context);
			other.m_rootNode.style.left = '400px';
			other.m_rootNode.style.top = '300px';
			root.style.left = '100px';
			root.style.top = '100px';
			view.m_canBeClinged = true;
			let rect = root.getBoundingClientRect();
			const otherRect = other.m_rootNode.getBoundingClientRect();
			mouse(handle, 'mousedown', rect.left + 10, rect.top + 10);
			mouse(browser.document, 'mousemove', otherRect.right + 18, otherRect.top + 18);
			testRunner.Assert(root.getBoundingClientRect().left, otherRect.right, 'Snapping uses the same coordinates');
			testRunner.Assert(root.getBoundingClientRect().top, otherRect.top);
			mouse(browser.document, 'mouseup', otherRect.right + 18, otherRect.top + 18);
			view.m_canBeClinged = false;

			root.style.left = '100px';
			root.style.top = '100px';
			rect = root.getBoundingClientRect();
			mouse(root.querySelector('.handleResize.right.oneDirection'), 'mousedown', rect.right, rect.top + 30);
			mouse(browser.document, 'mousemove', 5000, rect.top + 30);
			testRunner.Assert(
				root.getBoundingClientRect().right, visibleRight, 'Resizing stops at the containing block edge');
			mouse(browser.document, 'mouseup', 5000, rect.top + 30);

			root.style.top = '300px';
			rect = root.getBoundingClientRect();
			mouse(root.querySelector('.handleResize.bottom.oneDirection'), 'mousedown', rect.left + 30, rect.bottom);
			mouse(browser.document, 'mousemove', rect.left + 30, rect.bottom + 120);
			mouse(browser.document, 'mouseup', rect.left + 30, rect.bottom + 120);
			testRunner.Assert(root.style.height, '220px', 'Manual resizing sets a fixed pixel height');
			browser.innerHeight = 250;
			browser.dispatchEvent(new browser.Event('resize'));
			testRunner.Assert(root.getBoundingClientRect().top, 200,
				'A smaller viewport keeps a resized view accessible without a view-size observer notification');
			testRunner.Assert(root.style.height, '220px', 'Clamping preserves the manually resized height');

			view.ToggleStick();
			const stickedTop = root.style.top;
			browser.innerHeight = 200;
			browser.dispatchEvent(new browser.Event('resize'));
			testRunner.Assert(root.style.top, stickedTop, 'Viewport resize preserves sticked view placement');
			view.ToggleStick();
			testRunner.Assert(root.getBoundingClientRect().top, 150, 'Unsticking reapplies the bottom boundary');
			browser.innerHeight = 768;
			browser.dispatchEvent(new browser.Event('resize'));
			root.style.top = '100px';
			root.style.width = '200px';
			root.style.height = '100px';
			rect = root.getBoundingClientRect();
			mouse(root.querySelector('.handleResize.top.oneDirection'), 'mousedown', rect.left + 30, rect.top);
			mouse(browser.document, 'mousemove', rect.left + 30, -100);
			testRunner.Assert(root.getBoundingClientRect().top, visibleTop);
			testRunner.Assert(
				root.getBoundingClientRect().bottom, rect.bottom, 'Top resizing preserves the bottom edge');
			mouse(browser.document, 'mouseup', rect.left + 30, -100);

			root.style.height = '100px';
			view.Maximize();
			rect = root.getBoundingClientRect();
			mouse(handle, 'mousedown', rect.left + 60, rect.top + 10);
			mouse(browser.document, 'mousemove', 300, 200);
			testRunner.Assert(view.m_maximized, false);
			testRunner.Assert(root.classList.contains('maximized'), false);
			testRunner.Assert(root.getBoundingClientRect().left, 270);
			testRunner.Assert(root.getBoundingClientRect().top, 190, 'Restoring during drag stays under the pointer');
			mouse(browser.document, 'mouseup', 300, 200);
			view.Destructor();
			other.Destructor();
		}
		finally {
			dom.window.close();
		}
	}
});

testRunner.Test('Preserve sticked view placement and clamp unsticked views during zoom and content growth', () => {
	const dom = CreateBrowser();
	try {
		const browser = dom.window;
		const context = dom.getInternalVMContext();
		const observers = [];
		browser.ResizeObserver = class {
			constructor(callback)
			{
				this.callback = callback;
				observers.push(this);
			}
			observe(target) { this.target = target; }
			disconnect() { this.target = null; }
		};
		Object.defineProperties(browser, {
			innerWidth : { value : 1000, writable : true },
			innerHeight : { value : 768, writable : true },
			devicePixelRatio : { value : 1, writable : true },
			visualViewport : { value : { scale : 1 } }
		});
		Object.defineProperties(browser.HTMLElement.prototype, {
			offsetWidth : { get() { return parseFloat(this.style.width) || 0; } },
			offsetLeft : { get() { return parseFloat(this.style.left) || 0; } },
			offsetTop : { get() { return parseFloat(this.style.top) || 0; } }
		});
		const view = runInContext(`
			View.AddViewTemplate('ZoomTest', '<div></div>');
			class ZoomTest extends View {
				constructor() { super('ZoomTest'); }
				Constructor() {
					this.m_rootNode.style.width = '300px';
					this.m_rootNode.style.left = '700px';
					this.m_rootNode.style.top = '600px';
					return true;
				}
			}
			new ZoomTest();
		`,
			context);
		const observer = observers.find(item => item.target === view.m_rootNode);
		testRunner.Assert(view.m_created, true);
		observer.callback();
		view.ToggleStick();

		for (const [ratio, width, height] of [[ 1.25, 800, 614 ], [ 1.5, 667, 512 ], [ 1, 1000, 768 ]]) {
			browser.devicePixelRatio = ratio;
			browser.innerWidth = width;
			browser.innerHeight = height;
			observer.callback();
			observer.callback();
			view.ClampPosition();
			testRunner.Assert(view.m_rootNode.style.left, '700px', 'Sticked views preserve horizontal placement');
			testRunner.Assert(view.m_rootNode.style.top, '600px', 'Sticked views preserve vertical placement');
		}
		view.m_rootNode.style.left = '-30px';
		browser.visualViewport.scale = 2;
		observer.callback();
		testRunner.Assert(view.m_rootNode.style.left, '-30px', 'Pinch zoom preserves sticked view placement');
		browser.visualViewport.scale = 1;
		observer.callback();
		testRunner.Assert(view.m_rootNode.style.left, '-30px', 'Zooming back preserves placement');

		view.m_rootNode.style.left = '700px';
		view.m_rootNode.style.width = '420px';
		observer.callback();
		testRunner.Assert(view.m_rootNode.style.left, '700px', 'Content growth does not move sticked views');
		view.ToggleStick();
		testRunner.Assert(view.m_rootNode.style.left, '580px', 'Unsticking immediately reapplies position bounds');

		browser.devicePixelRatio = 1.25;
		browser.innerWidth = 800;
		browser.innerHeight = 614;
		observer.callback();
		testRunner.Assert(view.m_rootNode.style.left, '380px', 'Page zoom clamps unsticked views');
		testRunner.Assert(view.m_rootNode.style.top, '564px');
		browser.visualViewport.scale = 2;
		view.m_rootNode.style.left = '-30px';
		observer.callback();
		testRunner.Assert(view.m_rootNode.style.left, '0px', 'Pinch zoom retains boundary checks for unsticked views');
		view.m_rootNode.style.left = '380px';
		view.m_rootNode.style.width = '500px';
		observer.callback();
		testRunner.Assert(view.m_rootNode.style.left, '300px', 'Content growth still clamps unsticked views');
		view.Destructor();
		testRunner.Assert(observer.target, null);
	}
	finally {
		dom.window.close();
	}
});

testRunner.Run();