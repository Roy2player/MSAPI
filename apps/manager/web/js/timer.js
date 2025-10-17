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
 *
 * @test Has unit tests.
 */
class Timer {
	static #privateFields = (() => {
		let m_hasGlobalEventListener = false;
		let m_timers = new Map();

		return { m_hasGlobalEventListener, m_timers };
	})();

	static GetTimersNumber() { return Timer.#privateFields.m_timers.size; }

	static Apply(input)
	{
		input.setAttribute("type", "text");
		input.setAttribute("timestamp", "");
		const UTC = (new Date).getTimezoneOffset() / 60 * -1;
		input.setAttribute("utc", UTC >= 0 ? `+${UTC}` : `-${UTC}`);

		Timer.SetEvent(input);
	}

	static SetEvent(input)
	{
		let isActive = { "isActive" : false };
		let timers = Timer.#privateFields.m_timers;
		timers.set(input, isActive);

		input.addEventListener('input', () => { isActive.isActive = true; });

		if (!Timer.#privateFields.m_hasGlobalEventListener) {
			Timer.#privateFields.m_hasGlobalEventListener = true;
			document.addEventListener("click", (event) => {
				timers.forEach((isActive, input) => {
					if (input.parentNode == null) {
						timers.delete(input);
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

	static ToString(timestamp, utc)
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
		if (isNaN(utc)) {
			return `${date.getFullYear()}-${String(date.getMonth() + 1).padStart(2, '0')}-${
				String(date.getDate()).padStart(2, '0')} ${String(date.getHours()).padStart(2, '0')}:${
				String(date.getMinutes()).padStart(2, '0')}:${String(date.getSeconds()).padStart(2, '0')}.${
				String(Number(timestamp % 1000000000n)).padStart(9, '0')} +Unknown`;
		}

		return `${date.getFullYear()}-${String(date.getMonth() + 1).padStart(2, '0')}-${
			String(date.getDate()).padStart(2, '0')} ${String(date.getHours()).padStart(2, '0')}:${
			String(date.getMinutes()).padStart(2, '0')}:${String(date.getSeconds()).padStart(2, '0')}.${
			String(Number(timestamp % 1000000000n)).padStart(9, '0')} UTC${utc >= 0 ? `+${utc}` : utc}`;
	}

	static Normalize(input)
	{
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

		const nanosecond = parseInt(valueArray.join(""));
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

	static SetValue(input, timestamp)
	{
		if (typeof timestamp !== "bigint") {
			console.error("Nanoseconds timestamp is not a BigInt", timestamp);
		}

		if (timestamp <= 0n) {
			timestamp = 0n;
		}

		//* Empty string is considered as BigInt(0)
		const currentTimestamp = input.getAttribute("timestamp");
		if (currentTimestamp != "" && BigInt(+currentTimestamp) == timestamp) {
			return;
		}

		input.setAttribute("timestamp", timestamp);
		input.value = Timer.ToString(timestamp, input.getAttribute("utc"));
		Timer.Validate(input);
	}
};

if (typeof module !== 'undefined' && typeof module.exports !== 'undefined') {
	module.exports = Timer;
}