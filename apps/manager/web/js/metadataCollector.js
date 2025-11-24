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
 * @brief Collects metadata from a server and provides access to it by id.
 *
 * @brief Metadata can be of diferent types which are the same as MSAPI Standard types: Int8, Int16, Int32, Int64,
 * Uint8, Uint16, Uint32, Uint64, Double, Float, OptionalInt8, OptionalInt16, OptionalInt32, OptionalInt64,
 * OptionalUint8, OptionalUint16, OptionalUint32, OptionalUint64, OptionalDouble, OptionalFloat, String, Timer,
 * Duration, TableData. And one additional type: system, which is used for show system properties
 * on grids and tables views.
 */
class MetadataCollector {

	static #privateFields = (() => {
		let m_metadataToId = new Map();
		let m_stringInterpretationToId = new Map();

		return { m_metadataToId, m_stringInterpretationToId };
	})();

	static AddMetadata(id, metadata, isConst)
	{
		if (typeof metadata !== "object" || typeof (+id) !== "number" || typeof isConst !== "boolean") {
			console.error(`Invalid metadata, id or const attribute: ${metadata}, ${id}, ${isConst}`);
			return;
		}

		if (MetadataCollector.#privateFields.m_metadataToId.has(+id)) {
			// console.error(`Metadata with id: ${id} already exists`);
			return;
		}

		MetadataCollector.#privateFields.m_metadataToId.set(+id, { metadata, isConst });

		if ("stringInterpretation" in metadata) {
			MetadataCollector.#privateFields.m_stringInterpretationToId.set(+id, metadata.stringInterpretation);
		}
		else if (metadata.type == "TableData") {
			for (let column of Object.entries(metadata.columns)) {
				if (column.length > 1 && "stringInterpretation" in column[1]) {
					const tableColumnHash = Helper.StringHash(id.toString() + "-" + column[0].toString());
					MetadataCollector.#privateFields.m_stringInterpretationToId.set(
						tableColumnHash, column[1].stringInterpretation);

					if (MetadataCollector.#privateFields.m_metadataToId.has(tableColumnHash)) {
						console.error(
							`Table column hash is not among metadata items, it can lead to unexpected results`,
							tableColumnHash);
					}
					MetadataCollector.#privateFields.m_metadataToId.set(
						tableColumnHash, { metadata : column[1], isConst });
				}
			}
		}
	}

	static GetMetadata(id)
	{
		if (!MetadataCollector.#privateFields.m_metadataToId.has(+id)) {
			console.warn(`Metadata with id: ${id} not found`);
			return undefined;
		}

		return MetadataCollector.#privateFields.m_metadataToId.get(+id);
	}

	static Initialization()
	{
		let ready = true;
		View.AddViewTemplate("Metadata", `<div class="customView"></div>`);

		if (!dispatcher) {
			console.error("Dispatcher is not defined");
			ready = false;
		}
		else {
			dispatcher.RegisterPanel("Metadata", () => console.error("Not implemented yet"));
		}

		if (!ready) {
			console.error("Metadata collector is not ready");
		}
	}

	static GetStringInterpretation(id)
	{
		const stringInterpretation = MetadataCollector.#privateFields.m_stringInterpretationToId.get(+id);
		if (!stringInterpretation) {
			console.error(`String interpretation for parameter id ${id} is not found`);
			return undefined;
		}

		return stringInterpretation;
	}

	static IsSelect(id) { return MetadataCollector.#privateFields.m_stringInterpretationToId.has(+id); }
};

if (typeof module !== 'undefined' && typeof module.exports !== 'undefined') {
	module.exports = MetadataCollector;
}