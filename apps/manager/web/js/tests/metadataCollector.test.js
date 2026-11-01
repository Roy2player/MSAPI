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
 * @brief Tests presentation metadata validation for parameters and table columns through public registration.
 */

const { testRunner } = require('../test/testRunner');
const MetadataCollector = require('../views/metadataCollector');
const Timer = require('../core/timer');

testRunner.Test('Accept supported descriptions and type-specific display accuracy', () => {
	const cases = [
		{ name : 'Plain', type : 'String' }, { name : 'Empty description', type : 'Int8', description : '' },
		{ name : 'Description', type : 'String', description : 'Details\nwith <markup> as text' },
		...['Float', 'Double', 'OptionalFloat', 'OptionalDouble'].flatMap(
			type => [0.1, 0.25, 2, 1n, 1e-17].map(
				accuracy => ({ name : type, type, accuracy, description : 'Rounded' }))),
		...Object.values(Timer.ACCURACY).map(accuracy => ({ name : 'Timestamp', type : 'Timer', accuracy }))
	];
	for (const [index, metadata] of cases.entries()) {
		const id = 60000 + index;
		MetadataCollector.AddMetadata(id, metadata, true);
		testRunner.Assert(MetadataCollector.HasMetadata(id), true);
		testRunner.Assert(MetadataCollector.GetMetadata(id).metadata, metadata);
	}
});

testRunner.Test('Reject invalid presentation properties for parameters and table columns', () => {
	const invalid = [
		...[null, undefined, 123, true, {}, []].map(description => ({ type : 'String', description })),
		...['Float', 'Double', 'OptionalFloat', 'OptionalDouble'].flatMap(
			type => [0, -1, 0n, -1n, NaN, Infinity, -Infinity, '0.1', null, undefined, true, {}].map(
				accuracy => ({ type, accuracy }))),
		...[0, 5, 30, '23', 23n, null, undefined, NaN].map(accuracy => ({ type : 'Timer', accuracy })),
		...['Int8', 'String', 'Duration', 'Bool', 'system'].map(type => ({ type, accuracy : 0.1 }))
	];
	const originalError = console.error;
	const errors = [];
	console.error = (...args) => { errors.push(args); };
	try {
		for (const [index, properties] of invalid.entries()) {
			const metadata = { name : 'Invalid presentation', ...properties };
			const id = 61000 + index * 2;
			MetadataCollector.AddMetadata(id, metadata, true);
			testRunner.Assert(MetadataCollector.HasMetadata(id), false, `Parameter ${index} must be rejected`);
			MetadataCollector.AddMetadata(
				id + 1, { name : 'Invalid column', type : 'TableData', columns : { '1' : metadata } }, true);
			testRunner.Assert(MetadataCollector.HasMetadata(id + 1), false, `Table column ${index} must be rejected`);
		}
		testRunner.Assert(errors.length, invalid.length * 2);
	}
	finally {
		console.error = originalError;
	}
});

testRunner.Test('Accept display properties on array and object table columns', () => {
	const columns = [
		{ name : 'Price', type : 'Double', accuracy : 0.25, description : 'Quarter increments' },
		{ name : 'Timestamp', type : 'Timer', accuracy : Timer.ACCURACY.Microseconds, description : '' },
		{ name : 'Optional', type : 'OptionalFloat', accuracy : 0.1 },
		{ name : 'Label', type : 'String', description : 'Plain text' }
	];
	for (const [index, tableColumns] of [columns, { '2' : columns[0], '8' : columns[1], '15' : columns[2] }]
			 .entries()) {
		const id = 62000 + index;
		const metadata
			= { name : 'Described table', type : 'TableData', description : 'Table details', columns : tableColumns };
		MetadataCollector.AddMetadata(id, metadata, false);
		testRunner.Assert(MetadataCollector.HasMetadata(id), true);
		testRunner.Assert(MetadataCollector.GetMetadata(id).metadata, metadata);
	}
});

testRunner.Run();