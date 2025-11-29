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
const Select = require('../select');
const MetadataCollector = require('../metadataCollector');
const View = require('../view');
const Dispatcher = require('../dispatcher');

let testRunner = new TestRunner();

global.dispatcher = new Dispatcher();

const parameter_const = 7683294087;
const parameter_mutable = 908766347869289;

MetadataCollector.AddMetadata(parameter_const, {
	"name" : "Price type",
	"type" : "Int16",
	"min" : 1,
	"max" : 3,
	"stringInterpretation" : { "0" : "Undefined", "1" : "Fair", "2" : "Soft" }
},
	true);

MetadataCollector.AddMetadata(parameter_mutable, {
	"name" : "Application state",
	"type" : "Int16",
	"stringInterpretation" : { "0" : "Undefined", "1" : "Paused", "2" : "Running" }
},
	false);

testRunner.Test('Cast input to select and check', async () => {
	TestRunner.Step('Cast input to select');
	let input = document.createElement('input');
	Select.Apply({ input });
	input.setAttribute('parameter-id', parameter_const);
	testRunner.Assert(Object.keys(getEventListeners(document)).length, 0)
	testRunner.Assert(getEventListeners(input).click.length, 1)
	testRunner.Assert(Select.GetSelectViewsNumber(), 0);
	testRunner.Assert(input.type, 'text');
	testRunner.Assert(input.getAttribute('select'), '');
	testRunner.Assert(input.value, '');
	testRunner.Assert(input.classList.contains('invalid'), false);
	testRunner.Assert(input.classList.contains('select'), true);
	testRunner.Assert(input.readOnly, true);
	testRunner.Assert(
		input.outerHTML, `<input class="select" select="" type="text" readonly="" parameter-id="${parameter_const}">`);

	TestRunner.Step(`Set value by method`);
	Select.SetValue(input, 0);
	testRunner.Assert(input.getAttribute('select'), '0');
	testRunner.Assert(input.value, 'Undefined');
	testRunner.Assert(input.classList.contains('invalid'), false);
	testRunner.Assert(
		input.outerHTML, `<input class="select" select="0" type="text" readonly="" parameter-id="${parameter_const}">`);

	Select.SetValue(input, 1);
	testRunner.Assert(input.getAttribute('select'), '1');
	testRunner.Assert(input.value, 'Fair');
	testRunner.Assert(input.classList.contains('invalid'), false);
	testRunner.Assert(
		input.outerHTML, `<input class="select" select="1" type="text" readonly="" parameter-id="${parameter_const}">`);

	TestRunner.Step(`Set min limit`);
	input.setAttribute('min', '1');
	Select.SetValue(input, 0);
	testRunner.Assert(input.getAttribute('select'), '0');
	testRunner.Assert(input.value, 'Undefined');
	testRunner.Assert(input.classList.contains('invalid'), true);
	testRunner.Assert(input.outerHTML,
		`<input class="select invalid" select="0" type="text" readonly="" parameter-id="${parameter_const}" min="1">`);

	Select.SetValue(input, 1);
	testRunner.Assert(input.getAttribute('select'), '1');
	testRunner.Assert(input.value, 'Fair');
	testRunner.Assert(input.classList.contains('invalid'), false);
	testRunner.Assert(input.outerHTML,
		`<input class="select" select="1" type="text" readonly="" parameter-id="${parameter_const}" min="1">`);

	TestRunner.Step(`Set max limit`);
	input.setAttribute('max', '2');
	Select.SetValue(input, 3);
	testRunner.Assert(input.getAttribute('select'), '3');
	testRunner.Assert(input.value, '3 - not found');
	testRunner.Assert(input.classList.contains('invalid'), true);

	Select.SetValue(input, 2);
	testRunner.Assert(input.getAttribute('select'), '2');
	testRunner.Assert(input.value, 'Soft');
	testRunner.Assert(input.classList.contains('invalid'), false);
	testRunner.Assert(input.outerHTML,
		`<input class="select" select="2" type="text" readonly="" parameter-id="${parameter_const}" min="1" max="2">`);

	TestRunner.Step('Cast one more input to select without event listener');
	let input2 = document.createElement('input');
	body.appendChild(input2);
	Select.Apply({ input : input2, setEvent : false });
	testRunner.Assert(Select.GetSelectViewsNumber(), 0);
	input2.setAttribute('parameter-id', parameter_mutable);
	testRunner.Assert(Object.keys(getEventListeners(document)).length, 0)
	testRunner.Assert(Object.keys(getEventListeners(input2)).length, 0)
	testRunner.Assert(input2.type, 'text');
	testRunner.Assert(input2.getAttribute('select'), '');
	testRunner.Assert(input2.value, '');
	testRunner.Assert(input2.classList.contains('invalid'), false);
	testRunner.Assert(input2.classList.contains('select'), true);
	testRunner.Assert(input2.readOnly, true);
	testRunner.Assert(input2.outerHTML,
		`<input class="select" select="" type="text" readonly="" parameter-id="${parameter_mutable}">`);

	Select.SetValue(input2, 2);
	testRunner.Assert(input2.getAttribute('select'), '2');
	testRunner.Assert(input2.value, 'Running');
	testRunner.Assert(input2.classList.contains('invalid'), false);
	testRunner.Assert(input2.outerHTML,
		`<input class="select" select="2" type="text" readonly="" parameter-id="${parameter_mutable}">`);

	TestRunner.Step('Click to the input with event listener');
	input.dispatchEvent(new Event('click'));
	await TestRunner.WaitFor(
		() => { return View.GetCreatedViews().has(0) && View.GetCreatedViews().get(0).m_created; });

	testRunner.Assert(getEventListeners(document).click.length, 1);
	testRunner.Assert(Select.GetSelectViewsNumber(), 1);
	let views = document.querySelector('.views').querySelectorAll('.view');
	testRunner.Assert(views.length, 1);
	testRunner.Assert(views[0].querySelector('.viewHeader .title > span').innerHTML, 'Select: Price type 7683294087');
	testRunner.Assert(views[0].querySelector('.viewHeader .stick').style.display, 'none');
	testRunner.Assert(views[0].querySelector('.viewHeader .hide').style.display, 'none');
	testRunner.Assert(views[0].querySelector('.viewHeader .maximize').style.display, 'none');
	testRunner.Assert(views[0].querySelector('.viewHeader .close') != null, true);

	TestRunner.Step('Search and select item');
	let searchInput = views[0].querySelector('.selectView > .search > input');
	testRunner.Assert(searchInput != null, true);
	testRunner.Assert(searchInput.value, '');
	testRunner.Assert(searchInput.type, 'text');
	testRunner.Assert(searchInput.placeholder, 'Search');
	testRunner.Assert(searchInput.outerHTML, '<input type="text" placeholder="Search">');
	let caseSensitiveButton = views[0].querySelector('.selectView > .search > .caseSensitive');
	testRunner.Assert(caseSensitiveButton != null, true);
	testRunner.Assert(caseSensitiveButton.classList.contains('disabled'), true);

	let options = views[0].querySelector('.selectView > .options');
	testRunner.Assert(options != null, true);
	testRunner.Assert(options.children.length, 3);
	testRunner.Assert(options.children[0].querySelectorAll('span')[0].innerHTML, 'Undefined');
	testRunner.Assert(options.children[0].querySelectorAll('span')[1].innerHTML, '0');
	testRunner.Assert(options.children[0].style.display, '');
	testRunner.Assert(options.children[1].querySelectorAll('span')[0].innerHTML, 'Fair');
	testRunner.Assert(options.children[1].querySelectorAll('span')[1].innerHTML, '1');
	testRunner.Assert(options.children[1].style.display, '');
	testRunner.Assert(options.children[2].querySelectorAll('span')[0].innerHTML, 'Soft');
	testRunner.Assert(options.children[2].querySelectorAll('span')[1].innerHTML, '2');
	testRunner.Assert(options.children[2].style.display, '');

	caseSensitiveButton.dispatchEvent(new Event('click'));
	testRunner.Assert(caseSensitiveButton.classList.contains('disabled'), false);
	options = views[0].querySelector('.selectView > .options');
	testRunner.Assert(options.children.length, 3);

	searchInput.value = 'f';
	searchInput.dispatchEvent(new Event('input'));
	options = views[0].querySelector('.selectView > .options');
	testRunner.Assert(options != null, true);
	testRunner.Assert(options.children.length, 3);
	testRunner.Assert(options.children[0].style.display, '');
	testRunner.Assert(options.children[1].style.display, 'none');
	testRunner.Assert(options.children[2].style.display, '');

	searchInput.value = 'F';
	searchInput.dispatchEvent(new Event('input'));
	options = views[0].querySelector('.selectView > .options');
	testRunner.Assert(options != null, true);
	testRunner.Assert(options.children.length, 3);
	testRunner.Assert(options.children[0].style.display, 'none');
	testRunner.Assert(options.children[1].style.display, '');
	testRunner.Assert(options.children[2].style.display, 'none');

	searchInput.value = 'fair';
	searchInput.dispatchEvent(new Event('input'));
	options = views[0].querySelector('.selectView > .options');
	testRunner.Assert(options.children[0].style.display, 'none');
	testRunner.Assert(options.children[1].style.display, 'none');
	testRunner.Assert(options.children[2].style.display, 'none');

	caseSensitiveButton.dispatchEvent(new Event('click'));
	options = views[0].querySelector('.selectView > .options');
	testRunner.Assert(options.children[0].style.display, 'none');
	testRunner.Assert(options.children[1].style.display, '');
	testRunner.Assert(options.children[2].style.display, 'none');

	options.children[1].dispatchEvent(new Event('click'));
	await TestRunner.WaitFor(() => { return !View.GetCreatedViews().has(0) });
	testRunner.Assert(Select.GetSelectViewsNumber(), 0);

	testRunner.Assert(getEventListeners(document).click.length, 1);
	testRunner.Assert(input.getAttribute('select'), '1');
	testRunner.Assert(input.value, 'Fair');
	testRunner.Assert(input.classList.contains('invalid'), false);
	testRunner.Assert(input.outerHTML,
		`<input class="select" select="1" type="text" readonly="" parameter-id="${parameter_const}" min="1" max="2">`);

	TestRunner.Step('Click to the input with event listener and close view');
	input.dispatchEvent(new Event('click'));
	await TestRunner.WaitFor(
		() => { return View.GetCreatedViews().has(1) && View.GetCreatedViews().get(1).m_created; });

	testRunner.Assert(getEventListeners(document).click.length, 1);
	testRunner.Assert(Select.GetSelectViewsNumber(), 1);
	views = document.querySelector('.views').querySelectorAll('.view');
	testRunner.Assert(views.length, 1);
	views[0].querySelector('.viewHeader .close').dispatchEvent(new Event('click'));
	await TestRunner.WaitFor(() => { return !View.GetCreatedViews().has(1) });
	testRunner.Assert(getEventListeners(document).click.length, 1);
	testRunner.Assert(getEventListeners(input).click.length, 1);
	testRunner.Assert(Select.GetSelectViewsNumber(), 1);
	testRunner.Assert(input.getAttribute('select'), '1');
	testRunner.Assert(input.value, 'Fair');
	testRunner.Assert(input.classList.contains('invalid'), false);
	testRunner.Assert(input.outerHTML,
		`<input class="select" select="1" type="text" readonly="" parameter-id="${parameter_const}" min="1" max="2">`);

	TestRunner.Step('Click to the input with event listener and click again');
	input.dispatchEvent(new Event('click'));
	testRunner.Assert(getEventListeners(document).click.length, 1);
	await TestRunner.WaitFor(
		() => { return View.GetCreatedViews().has(2) && View.GetCreatedViews().get(2).m_created; });
	testRunner.Assert(getEventListeners(input).click.length, 1);
	testRunner.Assert(Select.GetSelectViewsNumber(), 2);
	testRunner.Assert(input.getAttribute('select'), '1');
	testRunner.Assert(input.value, 'Fair');
	testRunner.Assert(input.classList.contains('invalid'), false);
	testRunner.Assert(input.outerHTML,
		`<input class="select" select="1" type="text" readonly="" parameter-id="${parameter_const}" min="1" max="2">`);
	input.dispatchEvent(new Event('click'));

	TestRunner.Step('Click outside select view');
	document.dispatchEvent(new Event('click'));
	await TestRunner.WaitFor(() => { return !View.GetCreatedViews().has(2) });
	testRunner.Assert(getEventListeners(document).click.length, 1);
	testRunner.Assert(Select.GetSelectViewsNumber(), 1);
	testRunner.Assert(getEventListeners(input).click.length, 1);
	testRunner.Assert(input.getAttribute('select'), '1');
	testRunner.Assert(input.value, 'Fair');
	testRunner.Assert(input.classList.contains('invalid'), false);
	testRunner.Assert(input.outerHTML,
		`<input class="select" select="1" type="text" readonly="" parameter-id="${parameter_const}" min="1" max="2">`);

	TestRunner.Step('Add one more input with event listener and click to it');
	let input3 = document.createElement('input');
	body.appendChild(input3);
	Select.Apply({ input : input3 });
	input3.setAttribute('parameter-id', parameter_const);
	input3.dispatchEvent(new Event('click'));
	await TestRunner.WaitFor(
		() => { return View.GetCreatedViews().has(3) && View.GetCreatedViews().get(3).m_created; });
	testRunner.Assert(getEventListeners(document).click.length, 1);
	testRunner.Assert(Select.GetSelectViewsNumber(), 2);
	testRunner.Assert(getEventListeners(input3).click.length, 1);
	testRunner.Assert(input3.getAttribute('select'), '');
	testRunner.Assert(input3.value, '');
	testRunner.Assert(input3.classList.contains('invalid'), false);
	testRunner.Assert(input3.classList.contains('select'), true);
	testRunner.Assert(input3.readOnly, true);
	testRunner.Assert(
		input3.outerHTML, `<input class="select" select="" type="text" readonly="" parameter-id="${parameter_const}">`);

	TestRunner.Step('Click to the input with event listener and click to another option');
	input.dispatchEvent(new Event('click'));
	await TestRunner.WaitFor(
		() => { return View.GetCreatedViews().has(4) && View.GetCreatedViews().get(4).m_created; });
	testRunner.Assert(getEventListeners(document).click.length, 1);
	testRunner.Assert(Select.GetSelectViewsNumber(), 3);
	testRunner.Assert(getEventListeners(input).click.length, 1);
	testRunner.Assert(input.getAttribute('select'), '1');
	testRunner.Assert(input.value, 'Fair');
	testRunner.Assert(input.classList.contains('invalid'), false);
	testRunner.Assert(input.outerHTML,
		`<input class="select" select="1" type="text" readonly="" parameter-id="${parameter_const}" min="1" max="2">`);
	document.querySelector('.views')
		.querySelectorAll('.view')[1]
		.querySelector('.selectView > .options')
		.children[2]
		.dispatchEvent(new Event('click'));
	await TestRunner.WaitFor(() => { return !View.GetCreatedViews().has(4) });
	testRunner.Assert(getEventListeners(document).click.length, 1);
	testRunner.Assert(Select.GetSelectViewsNumber(), 1);
	testRunner.Assert(input.getAttribute('select'), '2');
	testRunner.Assert(input.value, 'Soft');
	testRunner.Assert(input.classList.contains('invalid'), false);
	testRunner.Assert(input.outerHTML,
		`<input class="select" select="2" type="text" readonly="" parameter-id="${parameter_const}" min="1" max="2">`);

	TestRunner.Step('Set value by method for first mutable input');
	Select.SetValue(input, 0);
	testRunner.Assert(input.getAttribute('select'), '0');
	testRunner.Assert(input.value, 'Undefined');
	testRunner.Assert(input.classList.contains('invalid'), true);
	testRunner.Assert(input.outerHTML,
		`<input class="select invalid" select="0" type="text" readonly="" parameter-id="${
			parameter_const}" min="1" max="2">`);

	Select.SetValue(input, 1);
	testRunner.Assert(input.getAttribute('select'), '1');
	testRunner.Assert(input.value, 'Fair');
	testRunner.Assert(input.classList.contains('invalid'), false);
	testRunner.Assert(input.outerHTML,
		`<input class="select" select="1" type="text" readonly="" parameter-id="${parameter_const}" min="1" max="2">`);

	TestRunner.Step('Set value by method for second const input');
	Select.SetValue(input2, 1);
	testRunner.Assert(input2.getAttribute('select'), '1');
	testRunner.Assert(input2.value, 'Paused');
	testRunner.Assert(input2.classList.contains('invalid'), false);
	testRunner.Assert(input2.outerHTML,
		`<input class="select" select="1" type="text" readonly="" parameter-id="${parameter_mutable}">`);

	Select.SetValue(input2, 5);
	testRunner.Assert(input2.getAttribute('select'), '5');
	testRunner.Assert(input2.value, '5 - not found');
	testRunner.Assert(input2.classList.contains('invalid'), false);
	testRunner.Assert(input2.outerHTML,
		`<input class="select" select="5" type="text" readonly="" parameter-id="${parameter_mutable}">`);

	TestRunner.Step('Set empty value manually and validate');
	input.value = '';
	Select.Validate(input);
	testRunner.Assert(input.classList.contains('invalid'), true);
	testRunner.Assert(input.outerHTML,
		`<input class="select invalid" select="1" type="text" readonly="" parameter-id="${
			parameter_const}" min="1" max="2">`);

	input2.value = '';
	Select.Validate(input2);
	testRunner.Assert(input2.classList.contains('invalid'), true);
	testRunner.Assert(input2.outerHTML,
		`<input class="select invalid" select="5" type="text" readonly="" parameter-id="${parameter_mutable}">`);

	input.remove();
	input2.remove();
	input3.remove();
});

testRunner.Run();