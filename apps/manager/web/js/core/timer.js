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
 * @brief Timer static class is a namespace for timer related functions. Provides ability to cast input node to
 * timestamp input. Timestamp is a string in format "YYYY-MM-DD HH:MM:SS.NNNNNNNNN UTC+X", where X is web-client's
 * timezone. Format YYYYMMDDHHMMSSNNNNNNNNN is also applicable, the logic of parser is: Read each part of date, if
 * required number of symbols is available, check result with limits, then check and skip next character if it is not an
 * integer and process next part of date - if step is failed, then final date is already parsed parts. Any symbols after
 * nanoseconds part are ignored. Inputed date is taken with local timezone.
 *
 * @brief Supports "canBeEmpty" attribute, which is false by default.
 * @brief Has "type" attribute as "text", "timestamp" attribute with timestamp and "utc" attribute with web-user
 * timezone.
 * @brief Has "input" event listener to normalize input value. If input is invalid, it will be set to previous value.
 * Does not take into account UTC in set value. After click outside input, value will be normalized and validated by
 * additional document event listener.
 * Empty editable inputs show a timestamp mask on focus with the year selected. An untouched mask is removed on blur
 * without committing a timestamp.
 * Inputs with display accuracy show the complete timestamp while editing. Change events commit normalization before
 * subsequent listeners read the timestamp. Short fractional parts use decimal seconds when accuracy is supplied.
 *
 * @test Yes.
 */
class Timer {
	// Length of the displayed date/time prefix for each granularity, before its timezone suffix.
	static ACCURACY = Object.freeze({
		Years : 4,
		Months: 7,
		Days: 10,
		Hours: 13,
		Minutes: 16,
		Seconds: 19,
		Milliseconds: 23,
		Microseconds: 26,
		Nanoseconds: 29
	});

	static #privateFields = (() => {
		let m_hasGlobalEventListener = false;
		let m_inputToEditState = new Map();

		return { m_hasGlobalEventListener, m_inputToEditState };
	})();

	static GetTimersNumber() { return Timer.#privateFields.m_inputToEditState.size; }

	static Apply(input)
	{
		input.setAttribute("type", "text");
		input.setAttribute("timestamp", "");
		const UTC = (new Date).getTimezoneOffset() / 60 * -1;
		input.setAttribute("utc", UTC >= 0 ? `+${UTC}` : `-${UTC}`);

		Timer.SetEvent(input);
	}

	/**************************
	 * @brief Registers timestamp editing handlers and commits changes before subsequent change listeners run.
	 * Expanding a nonempty input to full precision on focus preserves its caret and selection direction.
	 *
	 * @param input - Input initialized by Apply or cloned with its timestamp and utc attributes.
	 *
	 * @test Yes.
	 */
	static SetEvent(input)
	{
		let isActive = { "isActive" : false };
		let inputToEditState = Timer.#privateFields.m_inputToEditState;
		inputToEditState.set(input, isActive);

		let hasFocusMask = false;
		input.addEventListener("focus", () => {
			if (input.readOnly || input.disabled) {
				return;
			}

			if (input.value != "") {
				if (input.hasAttribute("accuracy")) {
					const { selectionStart, selectionEnd, selectionDirection } = input;
					input.value = Timer.ToString(BigInt(input.getAttribute("timestamp")), input.getAttribute("utc"));
					input.setSelectionRange(selectionStart, selectionEnd, selectionDirection);
				}
				return;
			}

			input.value = Timer.ToString(BigInt(input.getAttribute("timestamp")), input.getAttribute("utc"));
			input.setSelectionRange(0, 4);
			hasFocusMask = true;
		});

		input.addEventListener("blur", () => {
			if (hasFocusMask && input.getAttribute("timestamp") == "") {
				input.value = "";
			}
			else if (!isActive.isActive && input.hasAttribute("accuracy") && input.getAttribute("timestamp") != "") {
				Timer.SetValue(input, BigInt(input.getAttribute("timestamp")));
			}
			hasFocusMask = false;
		});

		input.addEventListener("input", () => {
			hasFocusMask = false;
			isActive.isActive = true;
		});

		input.addEventListener("change", () => {
			// Commit before table listeners compare the timestamp with its saved value.
			Timer.Normalize(input);
			Timer.Validate(input);
			isActive.isActive = false;
		});

		if (!Timer.#privateFields.m_hasGlobalEventListener) {
			Timer.#privateFields.m_hasGlobalEventListener = true;
			document.addEventListener("click", (event) => {
				inputToEditState.forEach((isActive, input) => {
					if (!input.parentNode) {
						inputToEditState.delete(input);
						return;
					}

					if (isActive.isActive && event.target != input) {
						Timer.Normalize(input);
						Timer.Validate(input);
						isActive.isActive = false;
					}
				});
			});
		}
	}

	/**************************
	 * @brief Formats a timestamp at the requested display granularity without rounding the stored timestamp.
	 *
	 * @param timestamp - Complete timestamp in nanoseconds.
	 * @param utc - Existing timezone label.
	 * @param accuracy - Optional Timer.ACCURACY value; omission displays nanoseconds.
	 *
	 * @return Formatted local date/time with the timezone suffix.
	 *
	 * @test Yes.
	 */
	static ToString(timestamp, utc, accuracy)
	{
		if (typeof timestamp !== "bigint") {
			console.error("Nanoseconds timestamp is not a BigInt", timestamp);
			return "1970-01-01 00:00:00.000000000 UTC+0";
		}

		if (timestamp < 0n) {
			console.error("Nanoseconds timestamp is negative", timestamp);
			return "1970-01-01 00:00:00.000000000 UTC+0";
		}

		const date = new Date(Number(timestamp / 1000000n));
		utc = parseInt(utc);
		const text = `${date.getFullYear()}-${String(date.getMonth() + 1).padStart(2, '0')}-${
			String(date.getDate()).padStart(2, '0')} ${String(date.getHours()).padStart(2, '0')}:${
			String(date.getMinutes()).padStart(2, '0')}:${String(date.getSeconds()).padStart(2, '0')}.${
			String(Number(timestamp % 1000000000n)).padStart(9, '0')}`;

		const length = accuracy ?? Timer.ACCURACY.Nanoseconds;
		if (length === undefined) {
			console.error("Unknown Timer accuracy", accuracy);
		}

		const suffix = isNaN(utc) ? " +Unknown" : ` UTC${utc >= 0 ? `+${utc}` : utc}`;

		return text.slice(0, length ?? text.length) + suffix;
	}

	/**************************
	 * @brief Parses edited timestamp text while preserving the raw timestamp of an unchanged display.
	 * Inputs with accuracy interpret short fractional parts as decimal seconds; other inputs retain nanosecond parsing.
	 *
	 * @param input - Timestamp input whose text is committed and formatted at its display accuracy.
	 *
	 * @test Yes.
	 */
	static Normalize(input)
	{
		const timestamp = input.getAttribute("timestamp");
		if (timestamp != ""
			&& input.value
				== Timer.ToString(BigInt(timestamp), input.getAttribute("utc"), input.getAttribute("accuracy"))) {

			return;
		}

		let valueArray = Array.from(input.value);
		if (valueArray.length < 4) {
			Timer.SetValue(input, BigInt(input.getAttribute("timestamp")));
			return;
		}

		const year = parseInt(valueArray.splice(0, 4).join(""));
		if (isNaN(year) || year < 1970) {
			Timer.SetValue(input, BigInt(input.getAttribute("timestamp")));
			return;
		}

		if (valueArray.length < 2) {
			Timer.SetValue(input, BigInt(new Date(year, 0, 1, 0, 0, 0, 0).getTime()) * 1000000n);
			return;
		}

		if (!Number.isInteger(valueArray[0])) {
			valueArray.splice(0, 1);
		}

		const month = parseInt(valueArray.splice(0, 2).join(""));
		if (isNaN(month) || month < 1 || month > 12) {
			Timer.SetValue(input, BigInt(new Date(year, 0, 1, 0, 0, 0, 0).getTime()) * 1000000n);
			return;
		}

		if (valueArray.length < 2) {
			Timer.SetValue(input, BigInt(new Date(year, month - 1, 1, 0, 0, 0, 0).getTime()) * 1000000n);
			return;
		}

		if (!Number.isInteger(valueArray[0])) {
			valueArray.splice(0, 1);
		}

		const day = parseInt(valueArray.splice(0, 2).join(""));
		if (isNaN(day) || day < 1 || day > new Date(year, month, 0).getDate()) {
			Timer.SetValue(input, BigInt(new Date(year, month - 1, 1, 0, 0, 0, 0).getTime()) * 1000000n);
			return;
		}

		if (valueArray.length < 2) {
			Timer.SetValue(input, BigInt(new Date(year, month - 1, day, 0, 0, 0, 0).getTime()) * 1000000n);
			return;
		}

		if (!Number.isInteger(valueArray[0])) {
			valueArray.splice(0, 1);
		}

		const hour = parseInt(valueArray.splice(0, 2).join(""));
		if (isNaN(hour) || hour < 0 || hour > 23) {
			Timer.SetValue(input, BigInt(new Date(year, month - 1, day, 0, 0, 0, 0).getTime()) * 1000000n);
			return;
		}

		if (valueArray.length < 2) {
			Timer.SetValue(input, BigInt(new Date(year, month - 1, day, hour, 0, 0, 0).getTime()) * 1000000n);
			return;
		}

		if (!Number.isInteger(valueArray[0])) {
			valueArray.splice(0, 1);
		}

		const minute = parseInt(valueArray.splice(0, 2).join(""));
		if (isNaN(minute) || minute < 0 || minute > 59) {
			Timer.SetValue(input, BigInt(new Date(year, month - 1, day, hour, 0, 0, 0).getTime()) * 1000000n);
			return;
		}

		if (valueArray.length < 2) {
			Timer.SetValue(input, BigInt(new Date(year, month - 1, day, hour, minute, 0, 0).getTime()) * 1000000n);
			return;
		}

		if (!Number.isInteger(valueArray[0])) {
			valueArray.splice(0, 1);
		}

		const second = parseInt(valueArray.splice(0, 2).join(""));
		if (isNaN(second) || second < 0 || second > 59) {
			Timer.SetValue(input, BigInt(new Date(year, month - 1, day, hour, minute, 0, 0).getTime()) * 1000000n);
			return;
		}

		if (valueArray.length < 2) {
			Timer.SetValue(input, BigInt(new Date(year, month - 1, day, hour, minute, second, 0).getTime()) * 1000000n);
			return;
		}

		if (!Number.isInteger(valueArray[0])) {
			valueArray.splice(0, 1);
		}

		let nanosecond = parseInt(valueArray.join(""));
		if (input.hasAttribute("accuracy")) {
			const fraction = valueArray.join("").match(/^\d+/)?.[0];
			if (fraction && fraction.length < 9) {
				nanosecond *= 10 ** (9 - fraction.length);
			}
		}

		if (isNaN(nanosecond) || nanosecond < 0) {
			Timer.SetValue(input, BigInt(new Date(year, month - 1, day, hour, minute, second, 0).getTime()) * 1000000n);
			return;
		}

		if (nanosecond > 999999999) {
			Timer.SetValue(input,
				BigInt(new Date(year, month - 1, day, hour, minute, second, 999999).getTime()) * 1000000n + 999999n);
			return;
		}

		Timer.SetValue(input,
			BigInt(new Date(year, month - 1, day, hour, minute, second, Math.floor(nanosecond / 1000000)).getTime())
					* 1000000n
				+ BigInt(nanosecond % 1000000));
	}

	static Validate(input)
	{
		if (input.getAttribute("canBeEmpty") != "true" && BigInt(input.getAttribute("timestamp")) <= 0n) {
			input.classList.add("invalid");
			return false;
		}

		input.classList.remove("invalid");
		return true;
	}

	/**************************
	 * @brief Sets and validates a timestamp input, clearing both its display and timestamp attribute for empty values.
	 * Normalizes nonpositive timestamps to zero.
	 *
	 * @param input - Timestamp input receiving the value.
	 * @param timestamp - Nanoseconds convertible to BigInt, or null, undefined or an empty string to clear the input.
	 *
	 * @test Yes.
	 */
	static SetValue(input, timestamp)
	{
		if (timestamp == null || timestamp === "") {
			input.value = "";
			input.setAttribute("timestamp", "");
			Timer.Validate(input);
			return;
		}

		timestamp = BigInt(timestamp);
		if (timestamp <= 0n) {
			timestamp = 0n;
		}

		// Empty string is considered as BigInt(0)
		const currentTimestamp = input.getAttribute("timestamp");
		const formatted = Timer.ToString(timestamp, input.getAttribute("utc"), input.getAttribute("accuracy"));
		if (currentTimestamp != "" && BigInt(currentTimestamp) == timestamp && input.value == formatted) {
			return;
		}

		input.setAttribute("timestamp", timestamp);
		input.value = formatted;
		Timer.Validate(input);
	}
};

if (typeof module !== "undefined" && typeof module.exports !== "undefined") {
	module.exports = Timer;
}