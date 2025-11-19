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
 * @brief Represents a grid object. Provides ability to create a grid with columns and rows. Rows can be added, updated,
 * removed and swapped. Columns can be added and swapped. Cell can be updated. Cell can represent any of MSAPI data
 * types, if it is a TableData, it will be clickable and will open a new table view with dynamicly updated data.
 *
 * Has required parameters:
 * @brief parent - parent node to append grid to.
 * @brief indexColumnId - index column id, which is used to store values and to identify rows. It is required to be
 * unique and will be constant for row even if it will be updated. Without this parameter in row update - it won't be
 * added or updated.
 *
 * Has optional parameters:
 * @brief columns - array of columns to be added to the grid.
 * @brief postAddRowFunction - function to be called after row is added. It is passed row object.
 * @brief postUpdateRowFunction - function to be called after row is added or updated. It is passed row object and
 * updated values object.
 *
 * @test Has unit tests.
 *
 * @todo Add ability to resize columns width.
 */
class Grid {
	static #template = `<div class="grid"></div>`;
	static #templateElement = undefined;
	static #privateFields = (() => {
		let m_hasGlobalEventListener = false;
		let m_settingsViews = new Set();
		let m_tablesViewsForColumnsByRows = new Map();

		return { m_hasGlobalEventListener, m_settingsViews, m_tablesViewsForColumnsByRows };
	})();

	static ALIGN_TYPE = Object.freeze({ left : -1, center: 0, right: 1 });

	/**************************
	 * @brief ascending is (from top) Z-A and descending is (from top) A-Z.
	 */
	static SORTING_TYPE = Object.freeze({ ascending : -1, none: 0, descending: 1 });

	static BOOL_FILTER = Object.freeze({ equal : 0 });

	static NUMBER_FILTER
		= Object.freeze({ equal : 0, notEqual: 1, less: 2, greater: 3, lessOrEqual: 4, greaterOrEqual: 5 });

	static STRING_FILTER = Object.freeze({
		equalCaseSensitive : 0,
		equalCaseInsensitive: 1,
		notEqualCaseSensitive: 2,
		notEqualCaseInsensitive: 3,
		containsCaseSensitive: 4,
		containsCaseInsensitive: 5
	});

	static OPTIONAL_NUMBER_FILTER
		= Object.freeze({ equal : 0, notEqual: 1, lessThan: 2, greater: 3, lessOrEqual: 4, greaterOrEqual: 5 });

	/**************************
	 * @return Number of settings views. Lazy clearing.
	 */
	static GetSettingsViewsNumber() { return Grid.#privateFields.m_settingsViews.size; }

	/**************************
	 * @return Number of tables views. Lazy clearing.
	 */
	static GetTablesViewsNumber()
	{
		let size = 0;
		Grid.#privateFields.m_tablesViewsForColumnsByRows.forEach((container, rowId) => { size += container.size; });

		return size;
	}

	constructor(
		{ parent, indexColumnId, columns = [], postAddRowFunction = undefined, postUpdateRowFunction = undefined })
	{
		if (typeof parent !== "object") {
			console.error("Invalid parent type, object is expected", parent);
			return;
		}
		this.m_parent = parent;

		if (typeof indexColumnId !== "number") {
			console.error("Invalid index column id type, number is expected", indexColumnId);
			return;
		}
		this.m_indexColumnId = indexColumnId;

		if (postAddRowFunction != undefined) {
			if (typeof postAddRowFunction !== "function") {
				console.error("Invalid post add row function type, function is expected", postAddRowFunction);
			}
			else {
				this.m_postAddRowFunction = postAddRowFunction;
			}
		}

		if (postUpdateRowFunction != undefined) {
			if (typeof postUpdateRowFunction !== "function") {
				console.error("Invalid post update row function type, function is expected", postUpdateRowFunction);
			}
			else {
				this.m_postUpdateRowFunction = postUpdateRowFunction;
			}
		}

		this.m_columnByOrder = new Map();
		this.m_columnById = new Map();
		this.m_rowByGridRow = new Map();
		this.m_rowByIndexValue = new Map();

		if (Grid.#templateElement === undefined) {
			const template = document.createElement("template");
			template.innerHTML = Grid.#template;
			Grid.#templateElement = template;
		}

		this.m_parent.appendChild(Grid.#templateElement.content.cloneNode(true));
		this.m_view = this.m_parent.lastElementChild;

		this.m_header = document.createElement("div");
		this.m_header.classList.add("header", "row");
		if (columns instanceof Array === false) {
			console.error("Invalid columns type, array is expected", columns);
		}
		else {
			columns.forEach((column) => { this.AddColumn({ id : column }); });
		}

		this.m_view.appendChild(this.m_header);

		if (!Grid.#privateFields.m_hasGlobalEventListener) {
			Grid.#privateFields.m_hasGlobalEventListener = true;
			let settingsViews = Grid.#privateFields.m_settingsViews;
			let tablesViews = Grid.#privateFields.m_tablesViewsForColumnsByRows;

			document.addEventListener("click", (event) => {
				settingsViews.forEach(settingView => {
					if (settingView.m_parentView.parentNode == null) {
						settingsViews.delete(settingView);
						return;
					}

					if (!settingView.m_parentView.contains(event.target)) {
						const lastApp = View.GetLastCreatedApplication();
						if (lastApp != null && lastApp.m_viewType == "SelectView"
							&& lastApp.m_parentView.contains(event.target)) {

							return;
						}

						if (event.target == settingView.m_eventTarget) {
							View.UpdateZIndex(settingView);
							return;
						}

						settingView.Destructor();
					}
				});

				tablesViews.forEach((container, rowId) => {
					container.forEach((tableView, columnId) => {
						if (tableView.m_parentView.parentNode == null) {
							container.delete(columnId);
							return;
						}
					});
				});
			});
		}
	}

	/**************************
	 * @brief Shift column to the forward or backward. If moving forward, a column with order equal to columns count + 1
	 * should not exist. If moving backward, a column with order equal to columns count - 1 should not exist and last
	 * column will be removed.
	 *
	 * @param order - last (if moving forward) or first (if moving backward) column order to be shifted.
	 * @param forward - if true, shift to the right, otherwise shift to the left.
	 * @param bound - first (if moving forward) or last (if moving backward) column order to be shifted.
	 */
	ShiftColumns({ order, forward, bound })
	{
		if (order < 0 || order >= this.m_columnByOrder.size) {
			console.error("Shifting is interrupted, invalid column order", order);
			return;
		}

		if (forward) {
			if (bound == undefined) {
				bound = this.m_columnByOrder.size - 1;
			}
			else if (bound < order || bound >= this.m_columnByOrder.size) {
				console.error("Shifting is interrupted, invalid bound", bound, order, this.m_columnByOrder.size);
				return;
			}

			for (let i = bound; i >= order; --i) {
				let column = this.m_columnByOrder.get(i);
				if (column == undefined) {
					console.error("Shifting is interrupted, column not found", i);
					return;
				}

				column.headerCell.style.order = i + 1;
				column.cells.forEach((cell) => cell.style.order = i + 1);
				this.m_columnByOrder.set(i + 1, column);
			}
			return;
		}

		if (this.m_columnByOrder.has(order - 1)) {
			console.error("Shifting is interrupted, column with order", order - 1, "already exists");
		}

		if (bound == undefined) {
			bound = this.m_columnByOrder.size;
		}
		else if (bound < order || bound > this.m_columnByOrder.size) {
			console.error("Shifting is interrupted, invalid bound", bound);
			return;
		}

		for (let i = order; i <= bound; ++i) {
			let column = this.m_columnByOrder.get(i);
			if (column == undefined) {
				console.error("Shifting is interrupted, column not found", i);
				return;
			}

			column.headerCell.style.order = i - 1;
			column.cells.forEach((cell) => cell.style.order = i - 1);
			this.m_columnByOrder.set(i - 1, column);
		}
	}

	SwapRows({ gridRow1, gridRow2 })
	{
		if (gridRow1 == gridRow2) {
			return;
		}

		let row1 = this.m_rowByGridRow.get(+gridRow1);
		if (row1 == undefined) {
			console.error(`Row with grid row ${gridRow1} not found`);
			return;
		}

		let row2 = this.m_rowByGridRow.get(+gridRow2);
		if (row2 == undefined) {
			console.error(`Row with grid row ${gridRow2} not found`);
			return;
		}

		row1.row.style.gridRow = gridRow2;
		row2.row.style.gridRow = gridRow1;

		this.m_rowByGridRow.set(+gridRow1, row2);
		this.m_rowByGridRow.set(+gridRow2, row1);
	}

	/**************************
	 * @brief Add column to the grid. Column is identified by id, which is stored in the metadata. If column with the
	 * same id already exists, it will be ignored. If order is not specified or less than zero, it will be added to the
	 * end of the grid. If order is specified, it should be in the range of 0 to columns count - 1. If order is out of
	 * range, it will be ignored.
	 *
	 * @param id - column id to be added.
	 * @param order - column order to be added. If not specified, it will be added to the end of the grid.
	 */
	AddColumn({ id, order = -1 })
	{
		const metadata = MetadataCollector.GetMetadata(id);
		if (metadata === null) {
			return;
		}

		for (const column of this.m_columnByOrder.values()) {
			if (column.id == id) {
				console.error("Column already exists", id);
				return;
			}
		}

		if (order > this.m_columnByOrder.size) {
			console.error(`Invalid order: ${order}, columns count: ${this.m_columnByOrder.size}`);
			return;
		}

		if (order < 0) {
			order = this.m_columnByOrder.size;
		}
		else {
			this.ShiftColumns({ order, forward : true });
		}

		let headerCell = document.createElement("div");
		headerCell.classList.add("cell");
		headerCell.style.order = order;
		headerCell.setAttribute("parameter-id", id);

		let text = document.createElement("span");
		text.classList.add("text");
		text.innerHTML = metadata.metadata.name;
		headerCell.appendChild(text);
		this.m_header.appendChild(headerCell);
		let columnObject = {
			metadata,
			id,
			headerCell,
			cells : new Map(),
			aligned : Grid.ALIGN_TYPE.center,
			isFilterActive : false,
			filters : [],
			sorting : Grid.SORTING_TYPE.none,
			systemTableMetadataId : 7 //* Can be overridden afterwards
		};
		this.m_columnById.set(id, columnObject);
		this.m_columnByOrder.set(order, columnObject);

		this.m_rowByGridRow.forEach((values, gridRow) => {
			this.InsertCell(id, this.m_indexColumnId, values[this.m_indexColumnId], metadata.metadata,
				values.values[id], values.row, gridRow, columnObject.cells, order, values.values);
		});

		if (metadata.metadata.type == "TableData" || metadata.metadata.type == "system") {
			return;
		}

		if (columnObject.metadata.metadata.type == "Bool") {
			columnObject.systemTableMetadataId = 6;
		}
		else if (columnObject.metadata.metadata.type == "String") {
			columnObject.systemTableMetadataId = 8;
		}
		else if (columnObject.metadata.metadata.type == "Timer" || columnObject.metadata.metadata.type == "Duration"
			|| columnObject.metadata.metadata.type.includes("Optional")) {

			columnObject.systemTableMetadataId = 9;
		}

		let grouping = document.createElement("div");
		grouping.classList.add("group");
		let settings = document.createElement("span");
		settings.classList.add("settings", "action");
		grouping.appendChild(settings);
		let sorting = document.createElement("span");
		sorting.classList.add("sorting", "action", "disabled");
		grouping.appendChild(sorting);
		let filter = document.createElement("span");
		filter.classList.add("filter", "action", "disabled");
		grouping.appendChild(filter);
		headerCell.insertBefore(grouping, headerCell.firstChild);

		let settingsView;
		let settingsViews = Grid.#privateFields.m_settingsViews;
		let savedThis = this;

		settings.addEventListener("click", () => {
			if (settingsView != undefined && settingsView.m_parentView.parentNode != null) {
				return;
			}

			settingsView = new View("GridSettingsView", {
				eventTarget : settings,
				parameterId : id,
				appTitle : "Manage column settings",
				positionUnder : headerCell,
				canBeHidden : false,
				canBeMaximized : false,
				canBeSticked : false,
				canBeClinged : false,
				postCreateFunction : ({ view }) => {
					let alignLeft = view.m_view.querySelector(".group > .action.alignLeft");
					if (alignLeft == null) {
						console.error("Align left is not found");
						return;
					}
					let alignCenter = view.m_view.querySelector(".group > .action.alignCenter");
					if (alignCenter == null) {
						console.error("Align center is not found");
						return;
					}
					let alignRight = view.m_view.querySelector(".group > .action.alignRight");
					if (alignRight == null) {
						console.error("Align right is not found");
						return;
					}
					let sortingAscending = view.m_view.querySelector(".group > .action.ascending");
					if (sortingAscending == null) {
						console.error("Ascending is not found");
						return;
					}
					let sortingNone = view.m_view.querySelector(".group > .action.none");
					if (sortingNone == null) {
						console.error("None is not found");
						return;
					}
					let sortingDescending = view.m_view.querySelector(".group > .action.descending");
					if (sortingDescending == null) {
						console.error("Descending is not found");
						return;
					}
					let filterGeneral = view.m_view.querySelector(".group > .action.filter");
					if (filterGeneral == null) {
						console.error("Filter is not found");
						return;
					}
					let filters = view.m_view.querySelector(".filters");
					if (filters == null) {
						console.error("Container for filters is not found");
						return;
					}

					if (columnObject.aligned == Grid.ALIGN_TYPE.left) {
						alignLeft.classList.add("active");
					}
					else if (columnObject.aligned == Grid.ALIGN_TYPE.center) {
						alignCenter.classList.add("active");
					}
					else if (columnObject.aligned == Grid.ALIGN_TYPE.right) {
						alignRight.classList.add("active");
					}
					else {
						console.error("Invalid alignment type", columnObject.aligned);
					}

					alignLeft.addEventListener("click", () => {
						if (alignLeft.classList.contains("active")) {
							return;
						}

						alignLeft.classList.add("active");
						alignCenter.classList.remove("active");
						alignRight.classList.remove("active");
						columnObject.aligned = Grid.ALIGN_TYPE.left;
						Grid.ApplyAlignment({ columnObject });
					});

					alignCenter.addEventListener("click", () => {
						if (alignCenter.classList.contains("active")) {
							return;
						}

						alignLeft.classList.remove("active");
						alignCenter.classList.add("active");
						alignRight.classList.remove("active");
						columnObject.aligned = Grid.ALIGN_TYPE.center;
						Grid.ApplyAlignment({ columnObject });
					});

					alignRight.addEventListener("click", () => {
						if (alignRight.classList.contains("active")) {
							return;
						}

						alignLeft.classList.remove("active");
						alignCenter.classList.remove("active");
						alignRight.classList.add("active");
						columnObject.aligned = Grid.ALIGN_TYPE.right;
						Grid.ApplyAlignment({ columnObject });
					});

					if (columnObject.sorting == Grid.SORTING_TYPE.ascending) {
						sortingAscending.classList.add("active");
						sortingNone.classList.add("disabled");
					}
					else if (columnObject.sorting == Grid.SORTING_TYPE.none) {
						sortingNone.classList.add("active");
					}
					else if (columnObject.sorting == Grid.SORTING_TYPE.descending) {
						sortingDescending.classList.add("active");
						sortingNone.classList.add("disabled");
					}
					else {
						console.error("Invalid sorting type", columnObject.sorting);
					}

					sortingAscending.addEventListener("click", () => {
						if (sortingAscending.classList.contains("active")) {
							return;
						}

						sortingAscending.classList.add("active");
						sortingNone.classList.remove("active");
						sortingDescending.classList.remove("active");
						sortingNone.classList.add("disabled");
						columnObject.sorting = Grid.SORTING_TYPE.ascending;
						savedThis.ApplySorting({ columnObject });
					});

					sortingNone.addEventListener("click", () => {
						if (sortingNone.classList.contains("active")) {
							return;
						}

						sortingAscending.classList.remove("active");
						sortingNone.classList.add("active");
						sortingDescending.classList.remove("active");
						columnObject.sorting = Grid.SORTING_TYPE.none;
					});

					sortingDescending.addEventListener("click", () => {
						if (sortingDescending.classList.contains("active")) {
							return;
						}

						sortingAscending.classList.remove("active");
						sortingNone.classList.remove("active");
						sortingDescending.classList.add("active");
						sortingNone.classList.add("disabled");
						columnObject.sorting = Grid.SORTING_TYPE.descending;
						savedThis.ApplySorting({ columnObject });
					});

					if (columnObject.isFilterActive) {
						filterGeneral.classList.add("active");
					}

					filterGeneral.addEventListener("click", () => {
						if (columnObject.filters.length == 0) {
							return;
						}
						if (columnObject.isFilterActive) {
							columnObject.isFilterActive = false;
						}
						else {
							columnObject.isFilterActive = true;
						}

						savedThis.ApplyFilters({ columnObject });
					});

					let filtersTable = new Table({
						parent : filters,
						metadata : {
							"name" : "Filters",
							"type" : "TableData",
							"canBeEmpty" : true,
							"columns" : [
								MetadataCollector.GetMetadata(columnObject.systemTableMetadataId).metadata.columns[0],
								columnObject.metadata.metadata,
							]
						},
						isMutable : true,
						id : columnObject.systemTableMetadataId,
						postSaveFunction : () => {
							const newData = filtersTable.GetData();
							if (columnObject.filters != newData) {
								if (newData.length == 0) {
									columnObject.isFilterActive = false;
									filterGeneral.classList.remove("active");
								}
								else {
									columnObject.isFilterActive = true;
									filterGeneral.classList.add("active");
								}

								columnObject.filters = newData;
								savedThis.ApplyFilters({ columnObject });
							}
						}
					});

					view.m_tables.set(columnObject.systemTableMetadataId, filtersTable);

					columnObject.filters.forEach((filter) => { filtersTable.AddRow(filter); });
					filtersTable.Save();
				}
			});

			settingsViews.add(settingsView);
		});

		filter.addEventListener("click", () => {
			if (columnObject.filters.length == 0) {
				return;
			}

			columnObject.isFilterActive = !columnObject.isFilterActive;
			this.ApplyFilters({ columnObject });
		});

		sorting.addEventListener("click", () => {
			if (columnObject.sorting == Grid.SORTING_TYPE.ascending) {
				columnObject.sorting = Grid.SORTING_TYPE.descending;
				this.ApplySorting({ columnObject });
			}
			else if (columnObject.sorting == Grid.SORTING_TYPE.descending) {
				columnObject.sorting = Grid.SORTING_TYPE.ascending;
				this.ApplySorting({ columnObject });
			}
		});
	}

	static ApplyAlignment({ columnObject })
	{
		if (columnObject.aligned == Grid.ALIGN_TYPE.left) {
			columnObject.cells.forEach((cell) => { cell.style.textAlign = "left"; });
			return;
		}
		if (columnObject.aligned == Grid.ALIGN_TYPE.center) {
			columnObject.cells.forEach((cell) => { cell.style.textAlign = "center"; });
			return;
		}
		if (columnObject.aligned == Grid.ALIGN_TYPE.right) {
			columnObject.cells.forEach((cell) => { cell.style.textAlign = "right"; });
			return;
		}

		console.error("Invalid alignment type", columnObject.aligned);
	}

	/**************************
	 * @brief Based on sorting type, sort rows by column
	 *
	 * @attention columnObject should be always provided.
	 *
	 * @param columnObject - column object to be sorted.
	 * @param clear - if true, sorting will be cleared and sorting type will be set to none.
	 */
	ApplySorting({ columnObject, clear = false })
	{
		if (columnObject.sorting == Grid.SORTING_TYPE.none) {
			return;
		}

		let sorted = [];

		if (clear) {
			this.m_rowByIndexValue.forEach((rowObject, indexValue) => {
				sorted.push({ row : rowObject.row, value : rowObject.values[this.m_indexColumnId] });
			});
			columnObject.sorting = Grid.SORTING_TYPE.none;
			sorted.sort((a, b) => {
				if (a.value === null || b.value === null) {
					if (a.value === null && b.value === null)
						return 0;
					if (a.value === null)
						return -1;
					if (b.value === null)
						return 1;
				}

				return a.value < b.value ? -1 : a.value > b.value ? 1 : 0;
			});
			let sorting = columnObject.headerCell.querySelector(".sorting");
			if (sorting != null) {
				sorting.classList.remove("disabled", "descending");
				sorting.classList.remove("active", "ascending");
			}
			else {
				console.error("Sorting ico element is not found in header cell");
			}
		}
		else {
			this.m_rowByIndexValue.forEach((rowObject, indexValue) => {
				sorted.push({ row : rowObject.row, value : rowObject.values[columnObject.id] });
			});

			if (columnObject.sorting == Grid.SORTING_TYPE.ascending) {
				sorted.sort((a, b) => {
					if (a.value === null || b.value === null) {
						if (a.value === null && b.value === null)
							return 0;
						if (a.value === null)
							return -1;
						if (b.value === null)
							return 1;
					}

					return a.value < b.value ? -1 : a.value > b.value ? 1 : 0;
				});
				let sorting = columnObject.headerCell.querySelector(".sorting");
				if (sorting != null) {
					sorting.classList.remove("disabled", "descending");
					sorting.classList.add("active", "ascending");
				}
				else {
					console.error("Sorting ico element is not found in header cell");
				}
			}
			else if (columnObject.sorting == Grid.SORTING_TYPE.descending) {
				sorted.sort((a, b) => {
					if (a.value === null || b.value === null) {
						if (a.value === null && b.value === null)
							return 0;
						if (a.value === null)
							return 1;
						if (b.value === null)
							return -1;
					}

					return a.value > b.value ? -1 : a.value < b.value ? 1 : 0;
				});

				let sorting = columnObject.headerCell.querySelector(".sorting");
				if (sorting != null) {
					sorting.classList.remove("disabled", "ascending");
					sorting.classList.add("active", "descending");
				}
			}
			else {
				console.error("Invalid sorting type", columnObject.sorting);
				return;
			}

			this.m_columnByOrder.forEach((object, order) => {
				if (object.id != columnObject.id) {
					let sorting = object.headerCell.querySelector(".sorting");
					if (object.sorting == Grid.SORTING_TYPE.ascending) {
						if (sorting != null) {
							sorting.classList.remove("active", "ascending");
							sorting.classList.add("disabled");
						}
						else {
							console.error("Sorting ico element is not found in header cell");
						}
					}
					else if (object.sorting == Grid.SORTING_TYPE.descending) {
						if (sorting != null) {
							sorting.classList.remove("active", "descending");
							sorting.classList.add("disabled");
						}
						else {
							console.error("Sorting ico element is not found in header cell");
						}
					}
					else {
						return;
					}

					object.sorting = Grid.SORTING_TYPE.none;
				}
			});
		}

		for (let i = 0; i < sorted.length; ++i) {
			this.SwapRows({ gridRow1 : sorted[i].row.style.gridRow, gridRow2 : i + 2 });
		}
	}

	/**************************
	 * @brief Based on filter activeness and filters array, filter rows by column values.
	 */
	ApplyFilters({ columnObject })
	{
		if (columnObject.filters.length == 0) {
			this.m_rowByIndexValue.forEach((rowObject, indexValue) => {
				const index = rowObject.filteredBy.indexOf(columnObject);
				if (index == -1) {
					return;
				}
				rowObject.filteredBy.splice(index, 1);

				if (!rowObject.isFiltired) {
					return;
				}

				for (let i = 0; i < rowObject.filteredBy.length; ++i) {
					if (rowObject.filteredBy[i].isFilterActive) {
						return;
					}
				}

				rowObject.row.style.display = "";
				rowObject.isFiltired = false;
			});

			let filter = columnObject.headerCell.querySelector(".filter");
			if (filter != null) {
				filter.classList.add("disabled");
				filter.classList.remove("active");
			}
			else {
				console.error("Filter ico element is not found in header cell");
			}

			return;
		}

		if (columnObject.isFilterActive == false) {
			this.m_rowByIndexValue.forEach((rowObject, indexValue) => {
				const index = rowObject.filteredBy.indexOf(columnObject);
				if (index == -1) {
					return;
				}

				if (!rowObject.isFiltired) {
					return;
				}

				for (let i = 0; i < rowObject.filteredBy.length; ++i) {
					if (rowObject.filteredBy[i].isFilterActive) {
						return;
					}
				}

				rowObject.row.style.display = "";
				rowObject.isFiltired = false;
			});

			let filter = columnObject.headerCell.querySelector(".filter");
			if (filter != null) {
				filter.classList.remove("disabled", "active");
			}
			else {
				console.error("Filter ico element is not found in header cell");
			}
			return;
		}

		let hasFilteredRows = false;
		let applyFilter = (index, rowObject) => {
			if (index == -1) {
				rowObject.filteredBy.push(columnObject);
			}
			if (!rowObject.isFiltired) {
				rowObject.row.style.display = "none";
				rowObject.isFiltired = true;
			}
			hasFilteredRows = true;
		};

		this.m_rowByIndexValue.forEach((rowObject, indexValue) => {
			const index = rowObject.filteredBy.indexOf(columnObject);

			if (columnObject.systemTableMetadataId == 7 || columnObject.systemTableMetadataId == 9) {
				if (columnObject.metadata.metadata.type.includes("Float")
					|| columnObject.metadata.metadata.type.includes("Double")) {

					for (let i = 0; i < columnObject.filters.length; ++i) {
						if (columnObject.filters[i][0] == Grid.NUMBER_FILTER.equal) {
							if (!Helper.FloatEqual(rowObject.values[columnObject.id], columnObject.filters[i][1])) {
								applyFilter(index, rowObject);
								return;
							}
							continue;
						}

						if (columnObject.filters[i][0] == Grid.NUMBER_FILTER.notEqual) {
							if (Helper.FloatEqual(rowObject.values[columnObject.id], columnObject.filters[i][1])) {
								applyFilter(index, rowObject);
								return;
							}
							continue;
						}

						if (columnObject.filters[i][0] == Grid.NUMBER_FILTER.less) {
							if (!Helper.FloatLess(rowObject.values[columnObject.id], columnObject.filters[i][1])) {
								applyFilter(index, rowObject);
								return;
							}
							continue;
						}

						if (columnObject.filters[i][0] == Grid.NUMBER_FILTER.greater) {
							if (!Helper.FloatGreater(rowObject.values[columnObject.id], columnObject.filters[i][1])) {
								applyFilter(index, rowObject);
								return;
							}
							continue;
						}

						if (columnObject.filters[i][0] == Grid.NUMBER_FILTER.lessOrEqual) {
							if (Helper.FloatGreater(rowObject.values[columnObject.id], columnObject.filters[i][1])) {
								applyFilter(index, rowObject);
								return;
							}
							continue;
						}

						if (columnObject.filters[i][0] == Grid.NUMBER_FILTER.greaterOrEqual) {
							if (Helper.FloatLess(rowObject.values[columnObject.id], columnObject.filters[i][1])) {
								applyFilter(index, rowObject);
								return;
							}
							continue;
						}
					}
				}
				else {
					for (let i = 0; i < columnObject.filters.length; ++i) {
						if (columnObject.filters[i][0] == Grid.NUMBER_FILTER.equal) {
							if (rowObject.values[columnObject.id] != columnObject.filters[i][1]) {
								applyFilter(index, rowObject);
								return;
							}
							continue;
						}

						if (columnObject.filters[i][0] == Grid.NUMBER_FILTER.notEqual) {
							if (rowObject.values[columnObject.id] == columnObject.filters[i][1]) {
								applyFilter(index, rowObject);
								return;
							}
							continue;
						}

						if (columnObject.filters[i][0] == Grid.NUMBER_FILTER.less) {
							if (rowObject.values[columnObject.id] >= columnObject.filters[i][1]) {
								applyFilter(index, rowObject);
								return;
							}
							continue;
						}

						if (columnObject.filters[i][0] == Grid.NUMBER_FILTER.greater) {
							if (rowObject.values[columnObject.id] <= columnObject.filters[i][1]) {
								applyFilter(index, rowObject);
								return;
							}
							continue;
						}

						if (columnObject.filters[i][0] == Grid.NUMBER_FILTER.lessOrEqual) {
							if (rowObject.values[columnObject.id] > columnObject.filters[i][1]) {
								applyFilter(index, rowObject);
								return;
							}
							continue;
						}

						if (columnObject.filters[i][0] == Grid.NUMBER_FILTER.greaterOrEqual) {
							if (rowObject.values[columnObject.id] < columnObject.filters[i][1]) {
								applyFilter(index, rowObject);
								return;
							}
							continue;
						}
					}
				}
			}
			else if (columnObject.systemTableMetadataId == 6) {
				for (let i = 0; i < columnObject.filters.length; ++i) {
					if (rowObject.values[columnObject.id] != columnObject.filters[i][1]) {
						applyFilter(index, rowObject);
						return;
					}
				}
			}
			else if (columnObject.systemTableMetadataId == 8) {
				for (let i = 0; i < columnObject.filters.length; ++i) {
					if (columnObject.filters[i][0] == Grid.STRING_FILTER.equalCaseSensitive) {
						if (rowObject.values[columnObject.id] != columnObject.filters[i][1]) {
							applyFilter(index, rowObject);
							return;
						}
						continue;
					}

					if (columnObject.filters[i][0] == Grid.STRING_FILTER.equalCaseInsensitive) {
						if (rowObject.values[columnObject.id].toUpperCase()
							!= columnObject.filters[i][1].toUpperCase()) {
							applyFilter(index, rowObject);
							return;
						}
						continue;
					}

					if (columnObject.filters[i][0] == Grid.STRING_FILTER.notEqualCaseSensitive) {
						if (rowObject.values[columnObject.id] == columnObject.filters[i][1]) {
							applyFilter(index, rowObject);
							return;
						}
						continue;
					}

					if (columnObject.filters[i][0] == Grid.STRING_FILTER.notEqualCaseInsensitive) {
						if (rowObject.values[columnObject.id].toUpperCase()
							== columnObject.filters[i][1].toUpperCase()) {
							applyFilter(index, rowObject);
							return;
						}
						continue;
					}

					if (columnObject.filters[i][0] == Grid.STRING_FILTER.containsCaseSensitive) {
						if (rowObject.values[columnObject.id].indexOf(columnObject.filters[i][1]) == -1) {
							applyFilter(index, rowObject);
							return;
						}
						continue;
					}

					if (columnObject.filters[i][0] == Grid.STRING_FILTER.containsCaseInsensitive) {
						if (rowObject.values[columnObject.id].toUpperCase().indexOf(
								columnObject.filters[i][1].toUpperCase())
							== -1) {

							applyFilter(index, rowObject);
							return;
						}
						continue;
					}
				}
			}

			if (rowObject.isFiltired) {
				if (index != -1) {
					rowObject.filteredBy.splice(index, 1);
				}

				for (let i = 0; i < rowObject.filteredBy.length; ++i) {
					if (rowObject.filteredBy[i].isFilterActive) {
						return;
					}
				}

				rowObject.row.style.display = "";
				rowObject.isFiltired = false;
			}
		});

		let filter = columnObject.headerCell.querySelector(".filter");
		if (filter == null) {
			console.error("Filter ico element is not found in header cell");
			return;
		}
		filter.classList.remove("disabled");

		let settingsView;
		for (let view of Grid.#privateFields.m_settingsViews) {
			if (view.m_parentView.parentNode != null && view.m_parameterId == columnObject.id) {
				settingsView = view;
				break;
			}
		}

		if (hasFilteredRows) {
			filter.classList.add("active");
			if (settingsView != undefined) {
				settingsView.m_view.querySelector(".group > .action.filter").classList.add("active");
			}
		}
		else {
			filter.classList.remove("active");
			columnObject.isFilterActive = false;
			if (settingsView != undefined) {
				settingsView.m_view.querySelector(".group > .action.filter").classList.remove("active");
			}
		}
	}

	/**************************
	 * @brief Move column to the new order and shift all columns after it (before move) if moved forward and before it
	 * (before move) if moved back.
	 *
	 * @param order - column order to be moved.
	 * @param newOrder - New column order, should be on the grid.
	 */
	MoveColumn({ order, newOrder })
	{
		if (order == newOrder) {
			return;
		}

		let column = this.m_columnByOrder.get(order);
		if (column == undefined) {
			console.error("Move columns is interrupted, column to move not found, order:", order);
			return;
		}

		if (!this.m_columnByOrder.has(newOrder)) {
			console.error("Column with new order not found", newOrder);
			return;
		}

		if (newOrder < order) {
			this.m_columnByOrder.delete(order);
			this.ShiftColumns({ order : newOrder, forward : true, bound : order - 1 });
			this.m_columnByOrder.set(newOrder, column);
			return;
		}

		this.m_columnByOrder.delete(order);
		this.ShiftColumns({ order : order + 1, forward : false, bound : newOrder });
		this.m_columnByOrder.set(newOrder, column);
	}

	RemoveColumn({ order })
	{
		if (order < 0 || order >= this.m_columnByOrder.size) {
			console.error("Invalid column order", order);
			return;
		}

		let columnObject = this.m_columnByOrder.get(order);
		if (columnObject === undefined) {
			console.error("Column not found", order);
			return;
		}

		if (columnObject.isFilterActive) {
			columnObject.isFilterActive = false;
			this.ApplyFilters({ columnObject });
		}

		if (columnObject.sorting != Grid.SORTING_TYPE.none) {
			this.ApplySorting({ columnObject, clear : true });
		}

		for (let settingsView of Grid.#privateFields.m_settingsViews) {
			if (settingsView.m_parameterId == columnObject.id) {
				document.dispatchEvent(new Event('click'));
			}
		}

		columnObject.headerCell.remove();
		columnObject.cells.forEach((cell) => cell.remove());
		this.m_columnById.delete(columnObject.id);
		this.m_columnByOrder.delete(order);

		if (this.m_columnByOrder.size == 0 || order >= this.m_columnByOrder.size) {
			return;
		}

		this.ShiftColumns({ order : order + 1, forward : false });
		this.m_columnByOrder.delete(this.m_columnByOrder.size - 1);
	}

	InsertCell(id, indexColumn, indexValue, metadata, value, row, gridRow, cells, order, values)
	{
		let cell = document.createElement("div");
		cell.classList.add("cell");
		cell.style.order = order;
		cell.setAttribute("parameter-id", id);

		if (metadata.type == "TableData") {
			cell.classList.add("action", "table");
			if (typeof View === "undefined") {
				console.error("Application is not defined");
				return undefined;
			}

			let tableViews = Grid.#privateFields.m_tablesViewsForColumnsByRows;

			let tableView;
			cell.addEventListener("click", () => {
				if (tableView != undefined && tableView.m_parentView.parentNode != null) {
					return;
				}

				tableView = new View("TableView", {
					tableId : id,
					metadata : metadata,
					appTitle : MetadataCollector.GetMetadata(indexColumn).metadata.name + " " + indexValue,
					positionUnder : cell,
					canBeHidden : false,
					canBeMaximized : false,
					canBeSticked : false,
					canBeClinged : false,
					postCreateFunction : () => {
						let table = tableView.m_tables.get(id);
						if (table === undefined) {
							console.error("Table is not found", id);
							return;
						}

						const tableValue = values[id];
						if (tableValue != undefined) {
							if ("Rows" in tableValue) {
								for (let row of tableValue.Rows) {
									table.AddRow(row);
								}
							}
							else {
								console.error("Rows is not found in value", tableValue);
							}
						}
					}
				});

				if (!tableViews.has(values[this.m_indexColumnId])) {
					tableViews.set(values[this.m_indexColumnId], new Map());
				}
				tableViews.get(values[this.m_indexColumnId]).set(id, tableView);
			});

			row.appendChild(cell);
			cells.set(gridRow, cell);

			return undefined;
		}

		if (metadata.type == "Duration") {
			let input = document.createElement("input");
			input.readOnly = true;
			Duration.Apply(input, metadata.durationType);
			if (value != undefined) {
				Duration.SetValue(input, BigInt(value));
			}
			cell.appendChild(input);
		}
		else if (metadata.type == "Timer") {
			let input = document.createElement("input");
			input.readOnly = true;
			Timer.Apply(input);
			if (value != undefined) {
				Timer.SetValue(input, BigInt(value));
			}
			cell.appendChild(input);
		}
		else if (MetadataCollector.IsSelect(id)) {
			let input = document.createElement("input");
			input.setAttribute("parameter-id", id);
			Select.Apply({ input, setEvent : false });
			if (value != undefined) {
				Select.SetValue(input, value);
			}
			cell.appendChild(input);
		}
		else if (value != undefined) {
			Grid.SetValueToCell(cell, value, metadata);
		}

		row.appendChild(cell);
		cells.set(gridRow, cell);
		return cell;
	}

	AddOrUpdateRow(values)
	{
		if (values instanceof Object === false) {
			console.error("Invalid values", values);
			return null;
		}
		if (!values.hasOwnProperty(this.m_indexColumnId)) {
			console.error("Index column", this.m_indexColumnId, "is not found in values:", values);
			return null;
		}
		if (this.m_rowByIndexValue.has(values[this.m_indexColumnId])) {
			return this.UpdateRow(values[this.m_indexColumnId], values);
		}

		let row = document.createElement("div");
		row.classList.add("row");
		row.style.gridRow = this.m_view.children.length + 1;
		const rowObject = { row, values, filteredBy : [], isFiltired : false };
		this.m_rowByGridRow.set(+row.style.gridRow, rowObject);
		this.m_rowByIndexValue.set(values[this.m_indexColumnId], rowObject);

		this.m_columnByOrder.forEach((columnObject, order) => {
			let cell = this.InsertCell(columnObject.id, this.m_indexColumnId, values[this.m_indexColumnId],
				columnObject.metadata.metadata, values[columnObject.id], row, +row.style.gridRow, columnObject.cells,
				order, values);

			this.ApplySorting({ columnObject });
			if (columnObject.isFilterActive && columnObject.filters.length != 0 && !rowObject.isFiltired) {
				this.ApplyFilters({ columnObject });
			}

			if (cell == undefined) {
				return;
			}

			if (columnObject.aligned == Grid.ALIGN_TYPE.left) {
				cell.style.textAlign = "left";
			}
			else if (columnObject.aligned == Grid.ALIGN_TYPE.center) {
				cell.style.textAlign = "center";
			}
			else if (columnObject.aligned == Grid.ALIGN_TYPE.right) {
				cell.style.textAlign = "right";
			}
			else {
				console.error("Invalid alignment type", columnObject.aligned);
			}
		});

		this.m_view.appendChild(row);

		if (this.m_postAddRowFunction !== undefined) {
			this.m_postAddRowFunction(rowObject);
		}
		if (this.m_postUpdateRowFunction !== undefined) {
			this.m_postUpdateRowFunction(rowObject, values);
		}

		return row;
	}

	static SetValueToCell(cell, value, metadata)
	{
		if (metadata.type == "bool") {
			cell.innerHTML = value;
			if (value == false) {
				cell.classList.add("false");
			}
			else {
				cell.classList.add("true");
			}
			return;
		}

		if (metadata.type == "Timer") {
			Timer.SetValue(cell.querySelector("input"), BigInt(value));
			return;
		}

		if (metadata.type == "Duration") {
			Duration.SetValue(cell.querySelector("input"), BigInt(value));
			return;
		}

		if (MetadataCollector.IsSelect(cell.getAttribute("parameter-id"))) {
			Select.SetValue(cell.querySelector("input"), value);
			return;
		}

		if (Helper.IsFloat(value)) {
			cell.innerHTML = Helper.FloatToString(value);
			return;
		}

		//* TableData handled separately

		cell.innerHTML = value;
	}

	UpdateRow(indexValue, values)
	{
		if (values instanceof Object === false) {
			console.error("Invalid values", values);
			return null;
		}

		let rowObject = this.m_rowByIndexValue.get(indexValue);
		if (rowObject === undefined) {
			console.error(`Row with index ${indexValue} as ${indexValue} is not found`);
			return null;
		}

		let changedColumns = [];
		for (let [key, value] of Object.entries(values)) {
			if (rowObject.values[key] != value) {
				changedColumns.push(+key);
			}
			rowObject.values[key] = value;
		}
		rowObject.row.querySelectorAll(".cell").forEach((cell) => {
			let columnId = +cell.getAttribute("parameter-id");
			if (changedColumns.indexOf(columnId) == -1) {
				return;
			}

			const metadata = MetadataCollector.GetMetadata(columnId);
			if (metadata === null) {
				return;
			}

			if (metadata.metadata.type == "TableData") {
				let tableView = Grid.#privateFields.m_tablesViewsForColumnsByRows.get(indexValue)?.get(columnId);
				if (tableView != undefined) {
					let table = tableView.m_tables.get(columnId);
					if (table == undefined) {
						console.error("Table is not found", columnId);
						return;
					}
					table.Clear();
					for (let row of values[columnId].Rows) {
						table.AddRow(row);
					}
					table.Save();
				}
			}
			else {
				Grid.SetValueToCell(cell, values[columnId], metadata.metadata);
			}
			let columnObject = this.m_columnById.get(columnId);
			if (columnObject === undefined) {
				console.error("Column object is not found", columnId);
				return;
			}

			this.ApplySorting({ columnObject });
			if (columnObject.isFilterActive && columnObject.filters.length != 0) {
				this.ApplyFilters({ columnObject });
			}
		});

		if (this.m_postUpdateRowFunction !== undefined) {
			this.m_postUpdateRowFunction(rowObject, values);
		}

		return rowObject.row;
	}

	static MatchRow(cells, values)
	{
		if (cells instanceof Object === false) {
			console.error("Invalid cells", cells);
			return false;
		}

		for (let [key, value] of Object.entries(cells)) {
			if (!values.hasOwnProperty(key)) {
				return false;
			}

			if (Helper.IsFloat(value)) {
				if (Helper.FloatToString(values[key]) !== Helper.FloatToString(value)) {
					return false;
				}
			}
			else if (!Helper.DeepEqual(values[key], value)) {
				return false;
			}
		}

		return true;
	}

	HasRow(cells)
	{
		for (let [gridRow, values] of this.m_rowByGridRow) {
			if (Grid.MatchRow(cells, values.values)) {
				return true;
			}
		}

		return false;
	}

	GetRows(cells)
	{
		if (cells instanceof Object === false) {
			console.error("Invalid cells type, object is expected", cells);
			return [];
		}

		let rows = [];

		for (let [gridRow, values] of this.m_rowByGridRow) {
			if (Grid.MatchRow(cells, values.values)) {
				rows.push(values.row);
			}
		}

		return rows;
	}

	/**************************
	 * @brief Remove row from the grid. Row is identified by index value, which is stored in the index column.
	 *
	 * @param indexValue - index value of the row to be removed.
	 */
	RemoveRow({ indexValue })
	{
		let rowObject = this.m_rowByIndexValue.get(indexValue);
		if (rowObject === undefined) {
			return;
		}

		for (let index = +rowObject.row.style.gridRow; index < this.m_rowByGridRow.size + 1; index++) {
			this.SwapRows({ gridRow1 : index, gridRow2 : index + 1 });
		}

		this.m_rowByIndexValue.delete(indexValue);
		if (!this.m_rowByGridRow.delete(+rowObject.row.style.gridRow)) {
			console.error("Row not found in m_rowByGridRow", rowObject.row.style.gridRow);
			return;
		}
		rowObject.row.remove();
	}

	ClearRows()
	{
		while (this.m_view.children.length > 1) {
			this.m_view.removeChild(this.m_view.lastElementChild);
		}

		this.m_columnByOrder.forEach((column) => column.cells.clear());
		this.m_rowByGridRow.clear();
		this.m_rowByIndexValue.clear();
	}

	Destructor()
	{
		if (this.m_view !== undefined) {
			this.m_view.remove();
			this.m_view = null;
		}
		if (this.m_columnByOrder !== undefined) {
			this.m_columnByOrder.clear();
			this.m_columnByOrder = null;
		}
		if (this.m_columnById !== undefined) {
			this.m_columnById.clear();
			this.m_columnById = null;
		}
		if (this.m_rowByGridRow !== undefined) {
			this.m_rowByGridRow.clear();
			this.m_rowByGridRow = null;
		}
		if (this.m_rowByIndexValue !== undefined) {
			this.m_rowByIndexValue.clear();
			this.m_rowByIndexValue = null;
		}
	}
}

if (typeof module !== 'undefined' && typeof module.exports !== 'undefined') {
	module.exports = Grid;
	Helper = require('./helper');
	Timer = require('./timer');
	Select = require('./select');
	Duration = require('./duration');
	View = require('./application');
	Table = require('./table');
	MetadataCollector = require('./metadataCollector');
}