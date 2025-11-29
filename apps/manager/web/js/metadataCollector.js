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
	static #SUPPORTED_TYPES = Object.freeze([
		'Int8', 'Int16', 'Int32', 'Int64',
		'Uint8', 'Uint16', 'Uint32', 'Uint64',
		'Double', 'Float',
		'OptionalInt8', 'OptionalInt16', 'OptionalInt32', 'OptionalInt64',
		'OptionalUint8', 'OptionalUint16', 'OptionalUint32', 'OptionalUint64',
		'OptionalDouble', 'OptionalFloat',
		'String', 'Timer', 'Duration', 'TableData', 'Bool', 'system'
	]);

	static #privateFields = (() => {
		let m_metadataToId = new Map();
		let m_stringInterpretationToId = new Map();
		let m_metadataToAppType = new Map();
		let m_viewPortParameterToAppType = new Map();

		return { m_metadataToId, m_stringInterpretationToId, m_metadataToAppType, m_viewPortParameterToAppType };
	})();

	static AddMetadata(id, metadata, isConst)
	{
		if (typeof metadata !== "object" || typeof (+id) !== "number" || typeof isConst !== "boolean") {
			console.error(`Invalid metadata, id or const attribute: ${metadata}, ${id}, ${isConst}`);
			return false;
		}

		if (MetadataCollector.#privateFields.m_metadataToId.has(+id)) {
			// console.error(`Metadata with id: ${id} already exists`);
			return true;
		}

		if (!MetadataCollector.ValidateMetadata(metadata)) {
			console.error(`Invalid metadata structure for id ${id}`);
			return false;
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

		return true;
	}

	/**************************
	 * @brief Validates metadata structure according to the supported types and required fields.
	 *
	 * @param metadata - Metadata object to validate.
	 * @return True if metadata is valid, false otherwise.
	 */
	static ValidateMetadata(metadata)
	{
		if (typeof metadata !== "object" || metadata === null) {
			console.error("Metadata must be a non-null object");
			return false;
		}

		if (!metadata.hasOwnProperty("name") || typeof metadata.name !== "string") {
			console.error("Metadata must have a 'name' field of type string");
			return false;
		}

		if (!metadata.hasOwnProperty("type") || typeof metadata.type !== "string") {
			console.error("Metadata must have a 'type' field of type string");
			return false;
		}

		if (!MetadataCollector.#SUPPORTED_TYPES.includes(metadata.type)) {
			console.error(`Unsupported metadata type: ${metadata.type}`);
			return false;
		}

		if (MetadataCollector.#IsNumericType(metadata.type)) {
			if (metadata.hasOwnProperty("min") && metadata.min !== "null" && typeof metadata.min !== "number"
				&& typeof metadata.min !== "string") {
				console.error("'min' field must be a number or null");
				return false;
			}
			if (metadata.hasOwnProperty("max") && metadata.max !== "null" && typeof metadata.max !== "number"
				&& typeof metadata.max !== "string") {
				console.error("'max' field must be a number or null");
				return false;
			}
		}

		if (metadata.type.includes("Optional")) {
			if (metadata.hasOwnProperty("canBeEmpty") && typeof metadata.canBeEmpty !== "boolean") {
				console.error("'canBeEmpty' field must be a boolean for optional types");
				return false;
			}
		}

		if (MetadataCollector.#IsIntegerType(metadata.type)) {
			if (metadata.hasOwnProperty("stringInterpretation")) {
				if (!MetadataCollector.#ValidateStringInterpretation(metadata.stringInterpretation)) {
					return false;
				}
			}
		}

		if (metadata.type === "TableData") {
			if (!MetadataCollector.#ValidateTableDataColumns(metadata)) {
				return false;
			}
		}

		return true;
	}

	static #IsNumericType(type)
	{
		return type.includes("Int") || type.includes("Uint") || type.includes("Float") || type.includes("Double");
	}

	static #IsIntegerType(type) { return type.includes("Int") || type.includes("Uint"); }

	static #ValidateStringInterpretation(stringInterpretation)
	{
		if (typeof stringInterpretation !== "object" || stringInterpretation === null) {
			console.error("'stringInterpretation' must be a non-null object");
			return false;
		}

		for (let [key, value] of Object.entries(stringInterpretation)) {
			if (typeof value !== "string") {
				console.error(`String interpretation value for key '${key}' must be a string`);
				return false;
			}
		}

		return true;
	}

	static #ValidateTableDataColumns(metadata)
	{
		if (!metadata.hasOwnProperty("columns")) {
			console.error("TableData metadata must have a 'columns' field");
			return false;
		}

		if (typeof metadata.columns !== "object" || metadata.columns === null) {
			console.error("'columns' field must be a non-null object");
			return false;
		}

		const columns = Array.isArray(metadata.columns) ? metadata.columns : Object.values(metadata.columns);

		for (let column of columns) {
			if (typeof column !== "object" || column === null) {
				console.error("Each column must be a non-null object");
				return false;
			}

			if (!column.hasOwnProperty("name") || typeof column.name !== "string") {
				console.error("Each column must have a 'name' field of type string");
				return false;
			}

			if (!column.hasOwnProperty("type") || typeof column.type !== "string") {
				console.error("Each column must have a 'type' field of type string");
				return false;
			}

			if (column.hasOwnProperty("stringInterpretation")) {
				if (!MetadataCollector.#ValidateStringInterpretation(column.stringInterpretation)) {
					return false;
				}
			}
		}

		return true;
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

		dispatcher.RegisterPanel("Metadata", () => console.error("Not implemented yet"));

		if (!ready) {
			console.error("Metadata collector is not ready");
		}
	}

	/**************************
	 * @brief Gets the string interpretations map for a given parameter ID.
	 *
	 * @param id - Parameter ID to get string interpretations for.
	 * @return String interpretations object or undefined if not found.
	 */
	static GetStringInterpretations(id)
	{
		const stringInterpretation = MetadataCollector.#privateFields.m_stringInterpretationToId.get(+id);
		if (!stringInterpretation) {
			console.error(`String interpretation for parameter id ${id} is not found`);
			return undefined;
		}

		return stringInterpretation;
	}

	/**************************
	 * @deprecated Use GetStringInterpretations instead.
	 */
	static GetStringInterpretation(id) { return MetadataCollector.GetStringInterpretations(id); }

	static IsSelect(id) { return MetadataCollector.#privateFields.m_stringInterpretationToId.has(+id); }

	/**************************
	 * @brief Adds or updates metadata for a specific application type.
	 *
	 * @param appType - Application type identifier.
	 * @param metadata - Metadata object containing mutable and const parameters.
	 */
	static AddAppMetadata(appType, metadata)
	{
		if (typeof appType !== "string" || typeof metadata !== "object" || metadata === null) {
			console.error(`Invalid app type or metadata: ${appType}, ${metadata}`);
			return false;
		}

		MetadataCollector.#privateFields.m_metadataToAppType.set(appType, metadata);
		return true;
	}

	/**************************
	 * @brief Gets metadata for a specific application type.
	 *
	 * @param appType - Application type identifier.
	 * @return Metadata object or undefined if not found.
	 */
	static GetAppMetadata(appType) { return MetadataCollector.#privateFields.m_metadataToAppType.get(appType); }

	/**************************
	 * @brief Gets all application type metadata.
	 *
	 * @return Array of all metadata objects.
	 */
	static GetAllAppMetadata() { return Array.from(MetadataCollector.#privateFields.m_metadataToAppType.values()); }

	/**************************
	 * @brief Sets the view port parameter for a specific application type.
	 *
	 * @param appType - Application type identifier.
	 * @param parameterId - Parameter ID for the view port.
	 */
	static SetViewPortParameterToAppType(appType, parameterId)
	{
		MetadataCollector.#privateFields.m_viewPortParameterToAppType.set(appType, parameterId);
	}

	/**************************
	 * @brief Gets the view port parameter for a specific application type.
	 *
	 * @param appType - Application type identifier.
	 * @return Parameter ID or undefined if not found.
	 */
	static GetViewPortParameterToAppType(appType)
	{
		return MetadataCollector.#privateFields.m_viewPortParameterToAppType.get(appType);
	}
};

if (typeof module !== 'undefined' && typeof module.exports !== 'undefined') {
	module.exports = MetadataCollector;
}