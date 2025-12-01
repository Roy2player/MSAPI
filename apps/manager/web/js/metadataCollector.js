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

if (typeof module !== 'undefined' && typeof module.exports !== 'undefined') {
	View = require('./view');
	Dispatcher = require('./dispatcher').Dispatcher;
}

class MetadataCollector extends View {
	static #SUPPORTED_TYPES = Object.freeze([
		'Int8', 'Int16', 'Int32', 'Int64', 'Uint8', 'Uint16', 'Uint32', 'Uint64', 'Double', 'Float', 'OptionalInt8',
		'OptionalInt16', 'OptionalInt32', 'OptionalInt64', 'OptionalUint8', 'OptionalUint16', 'OptionalUint32',
		'OptionalUint64', 'OptionalDouble', 'OptionalFloat', 'String', 'Timer', 'Duration', 'TableData', 'Bool',
		'system'
	]);

	static #privateFields = (() => {
		let m_metadataToId = new Map();
		let m_stringInterpretationToId = new Map();
		let m_metadataToAppType = new Map();
		let m_viewPortParameterToAppType = new Map();

		return { m_metadataToId, m_stringInterpretationToId, m_metadataToAppType, m_viewPortParameterToAppType };
	})();

	constructor(parameters) { super("MetadataCollector", parameters); }

	async Constructor(parameters) { return true; }

	static AddMetadata(id, metadata, isConst)
	{
		if (typeof metadata !== "object" || typeof (+id) !== "number" || typeof isConst !== "boolean") {
			console.error(`Invalid metadata, id or const attribute: ${metadata}, ${id}, ${isConst}`);
			return;
		}

		if (MetadataCollector.#privateFields.m_metadataToId.has(+id)) {
			return;
		}

		if (!MetadataCollector.#ValidateMetadata(metadata)) {
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

	/**************************
	 * @brief Validates metadata structure according to the supported types and required fields.
	 *
	 * @param metadata - Metadata object to validate.
	 *
	 * @return True if metadata is valid, false otherwise.
	 */
	static #ValidateMetadata(metadata)
	{
		if (typeof metadata !== "object" || metadata === null) {
			console.error("Metadata must be a non-null object", metadata);
			return false;
		}

		if (!metadata.hasOwnProperty("name") || typeof metadata.name !== "string") {
			console.error("Metadata must have a 'name' field of type string", metadata);
			return false;
		}

		if (!metadata.hasOwnProperty("type") || typeof metadata.type !== "string") {
			console.error("Metadata must have a 'type' field of type string", metadata);
			return false;
		}

		if (!MetadataCollector.#SUPPORTED_TYPES.includes(metadata.type)) {
			console.error("Unsupported metadata type", metadata);
			return false;
		}

		if (metadata.type.includes("Int") || metadata.type.includes("Uint") || metadata.type.includes("Duration")) {
			if (metadata.hasOwnProperty("min") && typeof metadata.min !== "bigint") {
				console.error("'min' field must be a bigint", metadata);
				return false;
			}
			if (metadata.hasOwnProperty("max") && typeof metadata.max !== "bigint") {
				console.error("'max' field must be a bigint", metadata);
				return false;
			}
		}

		if (metadata.type.includes("Float") || metadata.type.includes("Double")) {
			if (metadata.hasOwnProperty("min") && typeof metadata.min !== "number") {
				console.error("'min' field must be a number", metadata);
				return false;
			}
			if (metadata.hasOwnProperty("max") && typeof metadata.max !== "number") {
				console.error("'max' field must be a number", metadata);
				return false;
			}
		}

		if (metadata.type.includes("Optional") || metadata.type === "String" || metadata.type === "Timer"
			|| metadata.type === "Duration" || metadata.type === "TableData") {
			if (metadata.hasOwnProperty("canBeEmpty") && typeof metadata.canBeEmpty !== "boolean") {
				console.error("'canBeEmpty' field must be a boolean for optional types", metadata);
				return false;
			}
		}

		if (!metadata.type.includes("Optional") && (metadata.type.includes("Int") || metadata.type.includes("Uint"))) {
			if (metadata.hasOwnProperty("stringInterpretation")) {
				if (!MetadataCollector.#ValidateStringInterpretation(metadata)) {
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

	static #ValidateStringInterpretation(metadata)
	{
		const stringInterpretation = metadata.stringInterpretation;
		if (typeof stringInterpretation !== "object" || stringInterpretation === null) {
			console.error("'stringInterpretation' must be a non-null object", metadata);
			return false;
		}

		for (let [key, value] of Object.entries(stringInterpretation)) {
			if (typeof value !== "string") {
				console.error(`String interpretation value for key '${key}' must be a string`, metadata);
				return false;
			}
		}

		return true;
	}

	static #ValidateTableDataColumns(metadata)
	{
		if (!metadata.hasOwnProperty("columns")) {
			console.error("TableData metadata must have a 'columns' field", metadata);
			return false;
		}

		if (typeof metadata.columns !== "object" || metadata.columns === null) {
			console.error("'columns' field must be a non-null object", metadata);
			return false;
		}

		for (let column of Object.values(metadata.columns)) {
			if (typeof column !== "object" || column === null) {
				console.error("Each column must be a non-null object", metadata);
				return false;
			}

			if (!column.hasOwnProperty("type") || typeof column.type !== "string") {
				console.error("Each column must have a 'type' field of type string", metadata);
				return false;
			}

			if (column.hasOwnProperty("name") && typeof column.name !== "string") {
				console.error("Column 'name' field must be of type string", metadata);
				return false;
			}

			if (column.hasOwnProperty("stringInterpretation")) {
				if (!MetadataCollector.#ValidateStringInterpretation(column)) {
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

	/**************************
	 * @param id - Parameter ID to get string interpretations for.
	 *
	 * @return String interpretations object or undefined if not found.
	 */
	static GetStringInterpretations(id)
	{
		const stringInterpretation = MetadataCollector.#privateFields.m_stringInterpretationToId.get(+id);
		if (!stringInterpretation) {
			console.warn(`String interpretation for parameter id ${id} are not found`);
			return undefined;
		}

		return stringInterpretation;
	}

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

		//* Validate structure: should have at least 'mutable' or 'const' sections
		if (!metadata.hasOwnProperty("mutable") && !metadata.hasOwnProperty("const")) {
			console.warn(`App metadata for '${appType}' should have 'mutable' or 'const' sections`);
		}

		MetadataCollector.#privateFields.m_metadataToAppType.set(appType, metadata);

		for (let section of ["mutable", "const"]) {
			if (metadata.hasOwnProperty(section)) {
				if (typeof metadata[section] !== "object" || metadata[section] === null) {
					console.error(`'${section}' section must be a non-null object for app type '${appType}'`);
					continue;
				}

				for (let [id, paramMetadata] of Object.entries(metadata[section])) {
					MetadataCollector.AddMetadata(+id, paramMetadata, section === "const");
				}
			}
		}

		return true;
	}

	/**************************
	 * @param appType - Application type identifier.
	 *
	 * @return Metadata object or undefined if not found.
	 */
	static GetAppMetadata(appType) { return MetadataCollector.#privateFields.m_metadataToAppType.get(appType); }

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
	 * @param appType - Application type identifier.
	 *
	 * @return Parameter ID or undefined if not found.
	 */
	static GetViewPortParameterToAppType(appType)
	{
		return MetadataCollector.#privateFields.m_viewPortParameterToAppType.get(appType);
	}
};

View.AddViewTemplate("MetadataCollector", `<div class="customView"></div>`);
Dispatcher.RegisterPanel("MetadataCollector", () => new MetadataCollector());

//* Metadata items which are used by MSAPI Frontend views
MetadataCollector.AddMetadata(1, { name : "Create", type : "system" }, true);
MetadataCollector.AddMetadata(2, { name : "Change state", type : "system" }, true);
MetadataCollector.AddMetadata(3, { name : "Modify", type : "system" }, true);
MetadataCollector.AddMetadata(4, { name : "Delete", type : "system" }, true);
MetadataCollector.AddMetadata(5, { name : "Type", type : "String" }, true);
MetadataCollector.AddMetadata(6, {
	name : "Table template with boolean operator",
	type : "TableData",
	columns : [ { name : "Boolean operator", type : "Int8", stringInterpretation : { 0 : "Equal" } } ]
},
	true);
MetadataCollector.AddMetadata(7, {
	name : "Table template with number operator",
	type : "TableData",
	columns : [ {
		name : "Number operator",
		type : "Int8",
		stringInterpretation : {
			0 : "Equal",
			1 : "Not equal",
			2 : "Less than",
			3 : "Greater than",
			4 : "Less than or equal to",
			5 : "Greater than or equal to"
		}
	} ]
},
	true);
MetadataCollector.AddMetadata(8, {
	name : "Table template with string operator",
	type : "TableData",
	columns : [ {
		name : "String operator",
		type : "Int8",
		stringInterpretation : {
			0 : "Equal case sensitive",
			1 : "Equal case insensitive",
			2 : "Not equal case sensitive",
			3 : "Not equal case insensitive",
			4 : "Contains case sensitive",
			5 : "Contains case insensitive"
		}
	} ]
},
	true);
MetadataCollector.AddMetadata(9, {
	name : "Table template with optional number operator",
	type : "TableData",
	columns : [ {
		name : "Optional number operator",
		type : "Int8",
		stringInterpretation : {
			0 : "Equal",
			1 : "Not equal",
			2 : "Less than",
			3 : "Greater than",
			4 : "Less than or equal to",
			5 : "Greater than or equal to"
		}
	} ]
},
	true);
MetadataCollector.AddMetadata(10, { name : "Open view", type : "system" }, true);

if (typeof module !== 'undefined' && typeof module.exports !== 'undefined') {
	module.exports = MetadataCollector;
}