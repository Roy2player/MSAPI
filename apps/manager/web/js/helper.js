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
 *
 * @brief Helper static class is a namespace for helper functions.
 *
 * @todo Add unit tests.
 */
class Helper {
	static TYPES_LIMITS = Object.freeze({
		int_8 : { min : -128, max: 127 },
		uint_8: { min: 0, max: 255 },
		int_16: { min: -32768, max: 32767 },
		uint_16: { min: 0, max: 65535 },
		int_32: { min: -2147483648, max: 2147483647 },
		uint_32: { min: 0, max: 4294967295 },
		int_64: { min: -9223372036854775808, max: 9223372036854775807 },
		uint_64: { min: 0, max: 18446744073709551615 },
		float_32: { min: -3.4028235e+38, max: 3.4028235e+38 },
		float_64: { min: -1.7976931348623157e+308, max: 1.7976931348623157e+308 }
	});

	static #privateFields = (() => {
		let m_uid = 0;

		return { m_uid };
	})();

	static GenerateUid() { return Helper.#privateFields.m_uid++; }

	static DatetimeLocalToTimestamp(input)
	{
		const value = new Date(input.value).getTime();
		if (typeof value === "number" && value > 0) {
			return value * 1000000;
		}

		return 0;
	}

	static TimestampToDatetimeLocal(timestamp)
	{
		if (typeof timestamp !== "number" || timestamp < 0) {
			return "1970-01-01T00:00";
		}

		const date = new Date(timestamp / 1000000);
		return `${date.getFullYear()}-${String(date.getMonth() + 1).padStart(2, '0')}-${
			String(date.getDate()).padStart(2, '0')}T${String(date.getHours()).padStart(2, '0')}:${
			String(date.getMinutes()).padStart(2, '0')}`;
	}

	static TimerToString(timer)
	{
		if (typeof timer !== "number" || timer < 0) {
			return "1970-01-01:00:00:00.000000000";
		}

		const date = new Date(timer / 1000000);
		return `${date.getFullYear()}-${String(date.getMonth() + 1).padStart(2, '0')}-${
			String(date.getDate()).padStart(2, '0')}:${String(date.getHours()).padStart(2, '0')}:${
			String(date.getMinutes()).padStart(2, '0')}:${String(date.getSeconds()).padStart(2, '0')}.${
			String(timer % 1000000000).padStart(9, '0')}`;
	}

	static ParametersToJson(parameters)
	{
		const seen = new WeakSet();
		function customStringify(parameters)
		{
			if (parameters === null) {
				return 'null';
			}

			if (typeof parameters === 'bigint') {
				return parameters.toString();
			}

			if (typeof parameters === 'number' || typeof parameters === 'boolean') {
				return String(parameters);
			}

			if (typeof parameters === 'string') {
				return "\"" + parameters + "\"";
			}

			if (Array.isArray(parameters)) {
				return '[' + parameters.map(customStringify).join(',') + ']';
			}

			if (typeof parameters === 'object') {
				if (seen.has(parameters)) {
					console.error('Circular reference found, return null');
					return 'null';
				}

				seen.add(parameters);
				const props
					= Object.keys(parameters).map(key => `${JSON.stringify(key)}:${customStringify(parameters[key])}`);
				seen.delete(parameters);
				return '{' + props.join(',') + '}';
			}

			return 'null';
		}

		return customStringify(parameters);
	}

	static JsonStringToObject(jsonString)
	{
		let inString = false;
		let escaped = false;
		let result = [];

		for (let index = 0; index < jsonString.length; index++) {
			let char = jsonString[index];

			if (char === '"' && !escaped) {
				inString = !inString;
			}

			escaped = (char === "\\" && !escaped);

			if (!inString && /[-\d]/.test(char)) {
				let numStart = index;
				while (index + 1 < jsonString.length && /[\d.eE+-]/.test(jsonString[index + 1])) {
					index++;
				}
				let numberStr = jsonString.slice(numStart, index + 1);

				if (!numberStr.includes('.') && !/[eE]/.test(numberStr)) {
					result.push(`"${numberStr}"`);
				}
				else {
					result.push(numberStr);
				}
			}
			else {
				result.push(char);
			}
		}

		let wrappedJson = result.join('');

		return JSON.parse(wrappedJson, (key, value) => {
			if (typeof value === "string" && /^-?\d+$/.test(value)) {
				return BigInt(value);
			}

			return value;
		});
	}

	static FloatToString(num)
	{
		if (num === null) {
			return "";
		}

		num = Number(num);

		if (Math.abs(num) < 1e-6 || Math.abs(num) >= 1e21) {
			return num.toFixed(20).replace(/\.?0+$/, "");
		}

		return num.toString();
	}

	static IsInt(num)
	{
		return (typeof num === "string" && !num.includes('.')) || (Number(num) === num && num % 1 === 0);
	}

	static IsFloat(num)
	{
		return (typeof num == "string" && num.includes('.') && !isNaN(Number(num))) || (Number(num) === num && num % 1 !== 0);
	}

	/**************************
	 * @brief Compare floating point values with default epsilon of 1e-10.
	 *
	 * @return 0 if equal, 1 if first > second, -1 if first < second.
	 */
	static CompareFloats(first, second, epsilon = 1e-10)
	{
		return (Math.abs(first - second) >= epsilon) * ((first > second) - (first < second));
	}

	/**************************
	 * @return True if first < second with default epsilon of 1e-10.
	 */
	static FloatLess(first, second, epsilon = 1e-10) { return second - first > epsilon; }

	/**************************
	 * @return True if first > second with default epsilon of 1e-10.
	 */
	static FloatGreater(first, second, epsilon = 1e-10) { return first - second > epsilon; }

	/**************************
	 * @return True if first == second with default epsilon of 1e-10.
	 */
	static FloatEqual(first, second, epsilon = 1e-10) { return Math.abs(first - second) < epsilon; }

	static StringHash(str)
	{
		if (str.length === 0) {
			return 0;
		}

		var hash = 0, char;
		for (let index = 0; index < str.length; ++index) {
			char = str.charCodeAt(index);
			hash = (hash << 5) - hash + char;
			//* The bitwise OR operation with 0 (hash |= 0) does not change the value of hash. However, it forces the
			//* JavaScript engine to treat hash as a 32-bit signed integer.
			hash |= 0;
		}

		return hash;
	}

	static CheckMinimum(input)
	{
		let min = input.getAttribute("min");
		if (min !== null) {
			if (Helper.IsFloat(input.value) || Helper.IsFloat(min)) {
				if (+input.value < +min) {
					input.classList.add("invalid");
					return false;
				}
			}
			else {
				if (BigInt(input.value) < BigInt(min)) {
					input.classList.add("invalid");
					return false;
				}
			}
		}

		return true;
	}

	static CheckMaximum(input)
	{
		let max = input.getAttribute("max");
		if (max !== null) {
			if (Helper.IsFloat(input.value) || Helper.IsFloat(max)) {
				if (+input.value > +max) {
					input.classList.add("invalid");
					return false;
				}
			}
			else {
				if (BigInt(input.value) > BigInt(max)) {
					input.classList.add("invalid");
					return false;
				}
			}
		}

		return true;
	}

	static CheckEmpty(input)
	{
		let canBeEmpty = input.getAttribute("canBeEmpty");
		if (canBeEmpty === "false" && input.value === "") {
			input.classList.add("invalid");
			return false;
		}

		return true;
	}

	static ValidateLimits(input)
	{
		if (input.hasAttribute("nanoseconds")) {
			return Duration.Validate(input);
		}

		if (input.hasAttribute("timestamp")) {
			return Timer.Validate(input);
		}

		if (input.hasAttribute("select")) {
			return Select.Validate(input);
		}

		if (!Helper.CheckEmpty(input)) {
			return false;
		}

		if (input.type == "number") {
			if (!Helper.CheckMinimum(input)) {
				return false;
			}

			if (!Helper.CheckMaximum(input)) {
				return false;
			}
		}

		input.classList.remove("invalid");
		return true;
	}

	static DeepEqual(obj1, obj2)
	{
		if (obj1 == obj2) {
			return true;
		}

		if (typeof obj1 !== "object" || typeof obj2 !== "object" || obj1 === null || obj2 === null) {
			return false;
		}

		if (Array.isArray(obj1) && Array.isArray(obj2)) {
			if (obj1.length !== obj2.length) {
				return false;
			}
			for (let i = 0; i < obj1.length; i++) {
				if (!Helper.DeepEqual(obj1[i], obj2[i])) {
					return false;
				}
			}
			return true;
		}

		const keys1 = Object.keys(obj1);
		const keys2 = Object.keys(obj2);

		if (keys1.length !== keys2.length) {
			return false;
		}

		for (let key of keys1) {
			if (!keys2.includes(key) || !Helper.DeepEqual(obj1[key], obj2[key])) {
				return false;
			}
		}

		return true;
	}

	static GetFullDimensions(element)
	{
		const rect = element.getBoundingClientRect();
		const styles = window.getComputedStyle(element);

		const marginX = parseFloat(styles.marginLeft) + parseFloat(styles.marginRight);
		const marginY = parseFloat(styles.marginTop) + parseFloat(styles.marginBottom);

		return { width : rect.width + marginX, height : rect.height + marginY };
	}
};

if (typeof module !== 'undefined' && typeof module.exports !== 'undefined') {
	module.exports = Helper;
}