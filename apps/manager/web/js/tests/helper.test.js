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

const Helper = require('../helper');

let passed = 0;
let failed = 0;

function test(name, actual, expected) {
	if (actual === expected) {
		console.log('✓', name);
		passed++;
	} else {
		console.log('✗', name, '- Expected:', expected, 'Got:', actual);
		failed++;
	}
}

console.log('Testing Helper.Uint32ToIp function:');
console.log('');

test('Localhost (127.0.0.1)', Helper.Uint32ToIp(2130706433), '127.0.0.1');
test('INADDR_ANY (0.0.0.0)', Helper.Uint32ToIp(0), '0.0.0.0');
test('Private IP (192.168.1.1)', Helper.Uint32ToIp(3232235777), '192.168.1.1');
test('BigInt input', Helper.Uint32ToIp(2130706433n), '127.0.0.1');
test('Invalid negative', Helper.Uint32ToIp(-1), '127.0.0.1');
test('Invalid too large', Helper.Uint32ToIp(4294967296), '127.0.0.1');
test('Max valid (255.255.255.255)', Helper.Uint32ToIp(4294967295), '255.255.255.255');
test('Public IP (8.8.8.8)', Helper.Uint32ToIp(134744072), '8.8.8.8');

console.log('');
console.log('Results:', passed, 'passed,', failed, 'failed');

if (failed > 0) {
	process.exit(1);
}
