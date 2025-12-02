/**************************
 * @file        duration.js
 * @version     6.0
 * @date        2024-10-08
 * @author      maks.angels@mail.ru
 * @copyright   © 2021–2025 Maksim Andreevich Leonov
 *
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
 * @brief Duration static class is a namespace for duration related functions. Provides ability to cast input node to
 * duration input. Time duration units are: days (d), hours (h), minutes (m), seconds (s), milliseconds (l),
 * microseconds (u) and nanoseconds (n). Duration cannot contain more precise units than its type and number of days is
 * limited.
 *
 * @brief Supports "min" and "max" attributes for validation, and "canBeEmpty" attribute, which is false by default.
 * @brief Has "type" attribute as "text", "durationType" attribute to store duration type and "nanoseconds" attribute.
 * @brief Has "input" event listener to normalize input value. If input is invalid, it will be set to previous value.
 * Can be cleared by empty input or 0. After click outside input, value will be normalized and validated by additional
 * document event listener.
 *
 * @test Has unit tests.
 */
class Duration {
	static DAYS_LIMIT = 106751n;

	static #privateFields = (() => {
		let m_hasGlobalEventListener = false;
		let m_durations = new Map();

		return { m_hasGlobalEventListener, m_durations };
	})();

	static GetDurationsNumber() { return Duration.#privateFields.m_durations.size; }

	static mask = {
		"Days" : "d",
		"Hours": "dh",
		"Minutes": "dhm",
		"Seconds": "dhms",
		"Milliseconds": "dhmsl",
		"Microseconds": "dhmslu",
		"Nanoseconds": "dhmslun"
	}

	static Apply(input, type)
	{
		input.setAttribute("type", "text");
		input.setAttribute("durationType", type);
		input.setAttribute("nanoseconds", "");

		Duration.SetEvent(input);
	}

	static SetEvent(input)
	{
		let isActive = { "isActive" : false };
		let durations = Duration.#privateFields.m_durations;
		durations.set(input, isActive);

		input.addEventListener("input", () => { isActive.isActive = true; });

		if (!Duration.#privateFields.m_hasGlobalEventListener) {
			Duration.#privateFields.m_hasGlobalEventListener = true;
			document.addEventListener("click", (event) => {
				durations.forEach((isActive, input) => {
					if (!input.parentNode) {
						durations.delete(input);
						return;
					}

					if (isActive.isActive && event.target != input) {
						Duration.Normalize(input);
						Duration.Validate(input);
						isActive.isActive = false;
					}
				});
			});
		}
	}

	static ToString(mask, nanoseconds)
	{
		if (typeof nanoseconds !== "bigint") {
			console.error("Nanoseconds timestamp is not a BigInt", nanoseconds);
			return "0n";
		}

		let result = "";
		if (nanoseconds < 0n) {
			result = "-";
			nanoseconds = -nanoseconds;
		}
		for (let i = 0; i < mask.length; i++) {
			let componentType = mask[i];
			let value = 0;
			if (componentType == "d") {
				value = Math.floor(Number(nanoseconds / 86400000000000n));
				if (value > 0) {
					result += value + "d";
				}
				nanoseconds = nanoseconds % 86400000000000n;
			}
			else if (componentType == "h") {
				value = Math.floor(Number(nanoseconds / 3600000000000n));
				if (value > 0) {
					if (result != "") {
						result += " " + value + "h";
					}
					else {
						result += value + "h";
					}
				}
				nanoseconds = nanoseconds % 3600000000000n;
			}
			else if (componentType == "m") {
				value = Math.floor(Number(nanoseconds / 60000000000n));
				if (value > 0) {
					if (result != "") {
						result += " " + value + "m";
					}
					else {
						result += value + "m";
					}
				}
				nanoseconds = nanoseconds % 60000000000n;
			}
			else if (componentType == "s") {
				value = Math.floor(Number(nanoseconds / 1000000000n));
				if (value > 0) {
					if (result != "") {
						result += " " + value + "s";
					}
					else {
						result += value + "s";
					}
				}
				nanoseconds = nanoseconds % 1000000000n;
			}
			else if (componentType == "l") {
				value = Math.floor(Number(nanoseconds / 1000000n));
				if (value > 0) {
					if (result != "") {
						result += " " + value + "l";
					}
					else {
						result += value + "l";
					}
				}
				nanoseconds = nanoseconds % 1000000n;
			}
			else if (componentType == "u") {
				value = Math.floor(Number(nanoseconds / 1000n));
				if (value > 0) {
					if (result != "") {
						result += " " + value + "u";
					}
					else {
						result += value + "u";
					}
				}
				nanoseconds = nanoseconds % 1000n;
			}
			else if (componentType == "n") {
				if (nanoseconds > 0n) {
					if (result != "") {
						result += " " + nanoseconds + "n";
					}
					else {
						result += nanoseconds + "n";
					}
				}
			}
			else {
				console.error("Invalid mask", mask);
				return "Invalid mask";
			}

			if (nanoseconds == 0n) {
				return result;
			}
		}

		return result;
	}

	static Normalize(input)
	{
		if (input.value == "" || input.value == "0") {
			input.value = "";
			input.setAttribute("nanoseconds", 0);
			return;
		}

		const checkComponent = (valueArray, componentType, componentValue, startIndex, endIndex, nanoseconds) => {
			if (componentValue <= 0n) {
				return nanoseconds;
			}

			if (componentType == "d") {
				if (componentValue >= Duration.DAYS_LIMIT) {
					valueArray.splice(startIndex, endIndex - startIndex, ...Duration.DAYS_LIMIT.toString());
					nanoseconds += Duration.DAYS_LIMIT * 86400000000000n;
					return nanoseconds;
				}

				nanoseconds += componentValue * 86400000000000n;
				return nanoseconds;
			}

			if (componentType == "h") {
				if (componentValue > 23n) {
					valueArray.splice(startIndex, endIndex - startIndex, ...'23');
					nanoseconds += 23n * 3600000000000n;
					return nanoseconds;
				}
				nanoseconds += componentValue * 3600000000000n;
				return nanoseconds;
			}

			if (componentType == "m") {
				if (componentValue > 59n) {
					valueArray.splice(startIndex, endIndex - startIndex, ...'59');
					nanoseconds += 59n * 60000000000n;
					return nanoseconds;
				}
				nanoseconds += componentValue * 60000000000n;
				return nanoseconds;
			}

			if (componentType == "s") {
				if (componentValue > 59n) {
					valueArray.splice(startIndex, endIndex - startIndex, ...'59');
					nanoseconds += 59n * 1000000000n;
					return nanoseconds;
				}
				nanoseconds += componentValue * 1000000000n;
				return nanoseconds;
			}

			if (componentType == "l") {
				if (componentValue > 999n) {
					valueArray.splice(startIndex, endIndex - startIndex, ...'999');
					nanoseconds += 999n * 1000000n;
					return nanoseconds;
				}
				nanoseconds += componentValue * 1000000n;
				return nanoseconds;
			}

			if (componentType == "u") {
				if (componentValue > 999n) {
					valueArray.splice(startIndex, endIndex - startIndex, ...'999');
					nanoseconds += 999n * 1000n;
					return nanoseconds;
				}
				nanoseconds += componentValue * 1000n;
				return nanoseconds;
			}

			if (componentType == "n") {
				if (componentValue > 999n) {
					valueArray.splice(startIndex, endIndex - startIndex, ...'999');
					nanoseconds += 999n;
					return nanoseconds;
				}
				nanoseconds += componentValue;
				return nanoseconds;
			}

			return nanoseconds;
		};

		let valueArray = Array.from(input.value);

		{
			let spaces = 0;
			while (spaces < valueArray.length && valueArray[spaces] == ' ') {
				++spaces
			};
			valueArray.splice(0, spaces);
		}

		let isMask = false;
		let componentType = "";
		let componentValue = 0n;
		let startIndex = -1;
		let endIndex = 0;
		let nanoseconds = 0n;
		let maskIndex = -1;
		const mask = Duration.mask[input.getAttribute("durationType")];
		let isNegative = Array.from(input.value)[0] == "-";

		for (let index = 0; index < valueArray.length;) {
			while (!isMask) {
				if (valueArray[index] >= "0" && valueArray[index] <= "9") {
					if (valueArray[index] == "0" && startIndex == -1) {
						valueArray.splice(index, 1);
						--index;
						continue;
					}
					if (startIndex == -1) {
						startIndex = index;
					}
				}
				else if (startIndex != -1) {
					endIndex = index;
					componentValue = BigInt(valueArray.slice(startIndex, endIndex).join(''));
					isMask = true;
					break;
				}
				else if (index < 2 || valueArray[index - 1] == " " || valueArray[index] != " ") {
					valueArray.splice(index, 1);
					--index;
				}

				++index;
				if (index >= valueArray.length || valueArray.length == 0) {
					valueArray.splice(index - 1, 1);
					index = valueArray.length;
					break;
				}
			}

			while (isMask) {
				const position = mask.indexOf(valueArray[index]);
				if (position != -1) {
					if (position > maskIndex) {
						maskIndex = position;
					}
					else {
						console.error(`Invalid mask position ${position}, mask: ${mask}, input value: ${input.value}`);
						input.value = Duration.ToString(mask, BigInt(input.getAttribute("nanoseconds")));
						return;
					}

					isMask = false;
					componentType = mask[maskIndex];
					if (index + 1 < valueArray.length && valueArray[index + 1] != " ") {
						valueArray.splice(index + 1, 0, " ");
					}
					const sizeBefore = valueArray.length;
					endIndex = index;
					nanoseconds
						= checkComponent(valueArray, componentType, componentValue, startIndex, endIndex, nanoseconds);
					if (nanoseconds >= Duration.DAYS_LIMIT * 86400000000000n) {
						index = valueArray.length;
						break;
					}
					const sizeAfter = valueArray.length;
					if (sizeBefore != sizeAfter) {
						index += sizeAfter - sizeBefore;
						endIndex = index;
					}
					startIndex = -1;
				}
				else {
					valueArray.splice(index, 1);
					--index;
				}

				++index;
				if (isMask && index >= valueArray.length) {
					console.error("Cannot normalize value, index:", index, "mask:", mask, "input value:", input.value);
					input.value = Duration.ToString(mask, BigInt(input.getAttribute("nanoseconds")));
					return;
				}
			}
		}

		if (maskIndex == -1) {
			console.error("Cannot normalize value, mask:", mask, "input value:", input.value);
			input.value = Duration.ToString(mask, BigInt(input.getAttribute("nanoseconds")));
			return;
		}

		if (isMask || startIndex != -1 || endIndex + 1 != valueArray.length) {
			valueArray = valueArray.slice(0, endIndex + 1);
		}

		input.value = isNegative ? "-" + valueArray.join('') : valueArray.join('');
		input.setAttribute("nanoseconds", isNegative ? -nanoseconds : nanoseconds);
	}

	static Validate(input)
	{
		const nanoseconds = BigInt(input.getAttribute("nanoseconds"));

		if (input.getAttribute("canBeEmpty") != "true" && nanoseconds == 0n) {
			input.classList.add("invalid");
			return false;
		}

		if (input.hasAttribute("min") && nanoseconds < BigInt(input.getAttribute("min"))) {
			input.classList.add("invalid");
			return false;
		}

		if (input.hasAttribute("max") && nanoseconds > BigInt(input.getAttribute("max"))) {
			input.classList.add("invalid");
			return false;
		}

		input.classList.remove("invalid");
		return true;
	}

	static SetValue(input, nanoseconds)
	{
		if (typeof nanoseconds !== "bigint") {
			console.error("Nanoseconds timestamp is not a BigInt", nanoseconds);
			return;
		}

		//* Empty string is considered as BigInt(0)
		const currentNanoseconds = input.getAttribute("nanoseconds");
		if (currentNanoseconds != "" && BigInt(+currentNanoseconds) == nanoseconds) {
			return;
		}

		input.setAttribute("nanoseconds", nanoseconds);
		input.value = Duration.ToString(Duration.mask[input.getAttribute("durationType")], nanoseconds);
		if (input.value == "") {
			input.setAttribute("nanoseconds", 0);
		}

		Duration.Normalize(input);
		Duration.Validate(input);
	}
};

if (typeof module !== "undefined" && typeof module.exports !== "undefined") {
	module.exports = Duration;
}