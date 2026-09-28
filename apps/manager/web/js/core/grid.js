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
 * Required Notice: MSAPI, copyright © 2021–2026 Maksim Andreevich Leonov, maks.angels@mail.ru
 *
 * @brief Represents a grid with pool of rendered rows. The grid keeps all currently visible row objects in memory but
 * renders only the pool window, so DOM work is proportional to the viewport capacity rather than the total row count.
 * Columns can be added and swapped. Cell can be updated. Cell can represent any of MSAPI data types, if it is a
 * TableData, it will be clickable and will open a new table view with dynamically updated data.
 *
 * Required parameters:
 * 1) parent - parent node to append grid to.
 * 2) indexColumnId - index column id, which is used to store values and to identify rows. It is required to be
 * unique and will be constant for row even if it will be updated. Without this parameter in row update - it won't be
 * added or updated.
 *
 * Optional parameters:
 * 1) columns - array of columns to be added to the grid.
 * 2) postAddRowFunction - function to be called after row is added. It is passed row object.
 * 3) postUpdateRowFunction - function to be called after row is added or updated. It is passed row object and
 * updated values object.
 *
 * @note Sorting and filtering are coalesced per animation frame for real-time data sources. A row update changes its
 * data immediately, while the active filter and sort are applied by the next scheduled update.
 *
 * Performance considerations:
 * - Sorting currently costs O(N*log(N)), where N is the number of visible rows. Repeated updates in one frame share one
 * sort request, but a continuously changing sort key can still make sorting the dominant cost.
 * - Applying a filter currently scans all rows and evaluates every filter configured for the column. Several active
 * filters can therefore produce O(N*M) work per affected column, where M is the number of filter rules.
 * - Row insertion and removal update the visible-row indexes and may cause pool maintenance. The pool limits DOM
 * operations to rendered rows, but frequent changes near the current window can still cause repeated reordering.
 *
 * Possible future optimizations are maintaining a sorted structure with binary-search insertion, updating only the
 * changed row for filters, caching normalized case-insensitive filter values, and processing very large refreshes in
 * bounded chunks. These approaches trade implementation complexity and update latency for lower per-update cost.
 *
 * @test Yes.
 *
 * @todo Add ability to resize columns width.
 */
class Grid {
	static #template = `<div class="grid"><div class="header row"></div><div class="content"></div></div>`;
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

		if (postAddRowFunction) {
			if (typeof postAddRowFunction !== "function") {
				console.error("Invalid post add row function type, function is expected", postAddRowFunction);
			}
			else {
				this.m_postAddRowFunction = postAddRowFunction;
			}
		}

		if (postUpdateRowFunction) {
			if (typeof postUpdateRowFunction !== "function") {
				console.error("Invalid post update row function type, function is expected", postUpdateRowFunction);
			}
			else {
				this.m_postUpdateRowFunction = postUpdateRowFunction;
			}
		}

		this.m_columnByOrder = new Map();
		this.m_columnById = new Map();
		this.m_rowByIndexValue = new Map();
		this.m_visibleRows = new Array();
		this.m_pendingFilterColumns = new Set();
		this.m_isSortingPending = false;
		this.m_isUpdateScheduled = false;
		this.m_pool = new Pool(this);

		if (!Grid.#templateElement) {
			const template = document.createElement("template");
			template.innerHTML = Grid.#template;
			Grid.#templateElement = template;
		}

		this.m_parent.appendChild(Grid.#templateElement.content.cloneNode(true));
		this.m_view = this.m_parent.lastElementChild;

		this.m_header = this.m_view.querySelector("div.row.header");
		if (!this.m_header) {
			log.error("Header row is not found");
			return;
		}

		this.m_content = this.m_view.querySelector("div.content");
		if (!this.m_content) {
			log.error("Content div is not found");
			return;
		}

		if (columns instanceof Array === false) {
			console.error("Invalid columns type, array is expected", columns);
		}
		else {
			columns.forEach((column) => { this.AddColumn({ id : column }); });
		}

		if (!Grid.#privateFields.m_hasGlobalEventListener) {
			Grid.#privateFields.m_hasGlobalEventListener = true;
			let settingsViews = Grid.#privateFields.m_settingsViews;
			let tablesViews = Grid.#privateFields.m_tablesViewsForColumnsByRows;

			document.addEventListener("click", (event) => {
				settingsViews.forEach(settingView => {
					if (!settingView.m_parentView.parentNode) {
						settingsViews.delete(settingView);
						return;
					}

					if (!settingView.m_parentView.contains(event.target)) {
						const lastView = View.GetLastCreatedView();
						if (lastView && lastView.m_viewType == "SelectView"
							&& lastView.m_parentView.contains(event.target)) {

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
						if (!tableView.m_parentView.parentNode) {
							container.delete(columnId);
							return;
						}

						if (!tableView.m_parentView.contains(event.target)) {

							const lastView = View.GetLastCreatedView();
							if (lastView && lastView.m_viewType == "TableView"
								&& lastView.m_parentView.contains(event.target)) {

								return;
							}

							if (event.target == tableView.m_eventTarget) {
								View.UpdateZIndex(tableView);
								return;
							}

							tableView.Destructor();
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
				if (!column) {
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
			if (!column) {
				console.error("Shifting is interrupted, column not found", i);
				return;
			}

			column.headerCell.style.order = i - 1;
			column.cells.forEach((cell) => cell.style.order = i - 1);
			this.m_columnByOrder.set(i - 1, column);
		}
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
		if (!metadata) {
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
			index : order,
			headerCell,
			cells : new Map(),
			aligned : Grid.ALIGN_TYPE.center,
			isFilterActive : false,
			filters : [],
			sorting : Grid.SORTING_TYPE.none,
			systemTableMetadataId : 7 // Can be overridden afterwards
		};
		this.m_columnById.set(id, columnObject);
		this.m_columnByOrder.set(order, columnObject);

		this.m_rowByIndexValue.forEach((rowObject, indexValue) => { this.InsertCell({ columnObject, rowObject }); });

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

		settings.addEventListener("click", () => {
			if (settingsView && settingsView.m_parentView.parentNode) {
				return;
			}

			settingsView = new GridSettingsView({
				eventTarget : settings,
				parameterId : id,
				viewTitle : "Manage column settings",
				positionUnder : headerCell,
				canBeHidden : false,
				canBeMaximized : false,
				canBeSticked : false,
				canBeClinged : false,
			});

			settingsViews.add(settingsView);

			let alignLeft = settingsView.m_view.querySelector(".group > .action.alignLeft");
			if (!alignLeft) {
				console.error("Align left is not found");
				return;
			}
			let alignCenter = settingsView.m_view.querySelector(".group > .action.alignCenter");
			if (!alignCenter) {
				console.error("Align center is not found");
				return;
			}
			let alignRight = settingsView.m_view.querySelector(".group > .action.alignRight");
			if (!alignRight) {
				console.error("Align right is not found");
				return;
			}
			let sortingAscending = settingsView.m_view.querySelector(".group > .action.ascending");
			if (!sortingAscending) {
				console.error("Ascending is not found");
				return;
			}
			let sortingNone = settingsView.m_view.querySelector(".group > .action.none");
			if (!sortingNone) {
				console.error("None is not found");
				return;
			}
			let sortingDescending = settingsView.m_view.querySelector(".group > .action.descending");
			if (!sortingDescending) {
				console.error("Descending is not found");
				return;
			}
			let filterGeneral = settingsView.m_view.querySelector(".group > .action.filter");
			if (!filterGeneral) {
				console.error("Filter is not found");
				return;
			}
			let filters = settingsView.m_view.querySelector(".filters");
			if (!filters) {
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
				this.ApplySorting({ columnObject });
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
				this.ApplySorting({ columnObject });
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

				this.ApplyFilters({ columnObject });

				if (columnObject.isFilterActive) {
					filterGeneral.classList.add("active");
				}
				else {
					filterGeneral.classList.remove("active");
				}
			});

			const filterTypeMetadata = MetadataCollector.GetMetadata(columnObject.systemTableMetadataId);
			if (filterTypeMetadata) {
				let filtersTable = new Table({
					parent : filters,
					metadata : {
						"name" : "Filters",
						"type" : "TableData",
						"canBeEmpty" : true,
						"columns" : [
							filterTypeMetadata.metadata.columns[0],
							columnObject.metadata.metadata,
						]
					},
					isMutable : true,
					id : columnObject.systemTableMetadataId,
					postSaveFunction : () => {
						const newData = filtersTable.GetData();
						if (!Helper.DeepEqual(columnObject.filters, newData)) {
							if (newData.length == 0) {
								columnObject.isFilterActive = false;
								filterGeneral.classList.remove("active");
							}
							else {
								columnObject.isFilterActive = true;
								filterGeneral.classList.add("active");
							}

							columnObject.filters = newData;
							this.ApplyFilters({ columnObject });
						}
					}
				});

				settingsView.m_tables.set(columnObject.systemTableMetadataId, filtersTable);

				columnObject.filters.forEach((filter) => { filtersTable.AddRow(filter); });
				filtersTable.Save();
			}
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
	 * @brief Sort visible rows and minimally reorder the rendered pool window.
	 *
	 * Sorting is immediate when called directly, but row and filter updates normally use RequestSorting() so multiple
	 * changes in one animation frame are coalesced. The array sort is O(n log n); pool DOM work is limited to the
	 * rendered window.
	 *
	 * @param columnObject Column object that defines the sort key and direction.
	 * @param clear If true, clear sorting state without changing the current row order.
	 */
	ApplySorting({ columnObject, clear = false })
	{
		if (columnObject.sorting == Grid.SORTING_TYPE.none) {
			this.m_sortingColumnObject = undefined;
			return;
		}

		if (clear) {
			this.m_sortingColumnObject = undefined;
			columnObject.sorting = Grid.SORTING_TYPE.none;
			let sorting = columnObject.headerCell.querySelector(".sorting");
			if (sorting) {
				sorting.classList.remove("active", "ascending", "descending");
				sorting.classList.add("disabled");
			}
			else {
				console.error("Sorting ico element is not found in header cell");
			}

			return;
		}

		if (columnObject.sorting == Grid.SORTING_TYPE.ascending) {
			this.m_sortingColumnObject = columnObject;
			this.m_visibleRows.sort((a, b) => {
				const av = a.values[columnObject.id];
				const bv = b.values[columnObject.id];
				if (av === null || bv === null) {
					if (av === null && bv === null) {
						return 0;
					}

					return av === null ? -1 : 1;
				}

				if (av == bv) {
					return 0;
				}

				return av < bv ? -1 : 1;
			});
			let sorting = columnObject.headerCell.querySelector(".sorting");
			if (sorting) {
				sorting.classList.remove("disabled", "descending");
				sorting.classList.add("active", "ascending");
			}
			else {
				console.error("Sorting ico element is not found in header cell");
			}
		}
		else if (columnObject.sorting == Grid.SORTING_TYPE.descending) {
			this.m_sortingColumnObject = columnObject;
			this.m_visibleRows.sort((a, b) => {
				const av = a.values[columnObject.id];
				const bv = b.values[columnObject.id];
				if (av === null || bv === null) {
					if (av === null && bv === null) {
						return 0;
					}

					return av === null ? 1 : -1;
				}

				if (av == bv) {
					return 0;
				}

				return av > bv ? -1 : 1;
			});

			let sorting = columnObject.headerCell.querySelector(".sorting");
			if (sorting) {
				sorting.classList.remove("disabled", "ascending");
				sorting.classList.add("active", "descending");
			}
		}
		else {
			console.error("Invalid sorting type", columnObject.sorting);
			return;
		}

		for (const [order, other] of this.m_columnByOrder) {
			if (other.sorting == Grid.SORTING_TYPE.none || other.id == columnObject.id) {
				continue;
			}

			other.sorting = Grid.SORTING_TYPE.none;
			let sorting = other.headerCell.querySelector(".sorting");
			if (!sorting) {
				console.error("Sorting ico element is not found in header cell");
				break;
			}

			sorting.classList.remove("active", "ascending", "descending");
			sorting.classList.add("disabled");
			break;
		}

		for (let index = 0; index < this.m_visibleRows.length; ++index) {
			this.m_visibleRows[index].index = index;
		}

		this.m_pool.Reorder();
	}

	/**************************
	 * @brief Request one deferred sort for the current animation frame. Repeated requests are deduplicated. The request
	 * is ignored when no sorting column is active.
	 */
	RequestSorting()
	{
		if (!this.m_sortingColumnObject) {
			return;
		}

		this.m_isSortingPending = true;
		this.ScheduleUpdate();
	}

	/**************************
	 * @brief Request deferred filtering for a column, deduplicated within the current animation frame.
	 *
	 * @param columnObject Column whose active filter rules should be applied.
	 */
	RequestFilters({ columnObject })
	{
		this.m_pendingFilterColumns.add(columnObject);
		this.ScheduleUpdate();
	}

	/**************************
	 * @brief Schedule the shared filter and sort update callback. At most one requestAnimationFrame callback is pending
	 * for this grid.
	 */
	ScheduleUpdate()
	{
		if (this.m_isUpdateScheduled) {
			return;
		}

		this.m_isUpdateScheduled = true;
		requestAnimationFrame(() => this.FlushUpdates());
	}

	/**************************
	 * @brief Apply pending filters first and then the active sort once per frame. Filtering precedes sorting so rows
	 * that become visible are included in the sorted order.
	 */
	FlushUpdates()
	{
		if (!this.m_visibleRows) {
			return;
		}

		const columns = this.m_pendingFilterColumns;
		this.m_pendingFilterColumns = new Set();
		columns.forEach((columnObject) => {
			if (columnObject.isFilterActive && columnObject.filters.length != 0) {
				this.ApplyFilters({ columnObject });
			}
		});

		if (this.m_isSortingPending) {
			this.m_isSortingPending = false;
			if (this.m_sortingColumnObject) {
				this.ApplySorting({ columnObject : this.m_sortingColumnObject });
			}
		}

		// Reset last, so requests made during flush are handled in this flush
		this.m_isUpdateScheduled = false;
	}

	/**************************
	 * @brief Apply a column's filters and maintain the visible-row pool.
	 *
	 * The current implementation scans all rows and filter rules. Its cost is O(n * f) for n rows and f rules in the
	 * column. For high-frequency single-row updates, a row-level predicate and cached normalized values can reduce
	 * unnecessary full-column scans.
	 *
	 * @param columnObject Column object whose filter state should be applied.
	 */
	ApplyFilters({ columnObject })
	{
		if (columnObject.filters.length == 0) {
			let anyUpdate = false;
			this.m_rowByIndexValue.forEach((rowObject, indexValue) => {
				const index = rowObject.filteredBy.indexOf(columnObject);
				if (index == -1) {
					return;
				}
				rowObject.filteredBy.splice(index, 1);

				if (!rowObject.isFiltered) {
					return;
				}

				for (let i = 0; i < rowObject.filteredBy.length; ++i) {
					if (rowObject.filteredBy[i].isFilterActive) {
						return;
					}
				}

				rowObject.row.style.display = "";
				rowObject.isFiltered = false;

				anyUpdate = true;
				this.AddVisibleRow({ rowObject });
			});

			let filter = columnObject.headerCell.querySelector(".filter");
			if (filter) {
				filter.classList.add("disabled");
				filter.classList.remove("active");
			}
			else {
				console.error("Filter ico element is not found in header cell");
			}

			if (anyUpdate) {
				this.RequestSorting();
			}
			return;
		}

		if (columnObject.isFilterActive == false) {
			let anyUpdate = false;
			this.m_rowByIndexValue.forEach((rowObject, indexValue) => {
				const index = rowObject.filteredBy.indexOf(columnObject);
				if (index == -1) {
					return;
				}

				if (!rowObject.isFiltered) {
					return;
				}

				for (let i = 0; i < rowObject.filteredBy.length; ++i) {
					if (rowObject.filteredBy[i].isFilterActive) {
						return;
					}
				}

				rowObject.row.style.display = "";
				rowObject.isFiltered = false;
				anyUpdate = true;
				this.AddVisibleRow({ rowObject });
			});

			let filter = columnObject.headerCell.querySelector(".filter");
			if (filter) {
				filter.classList.remove("disabled", "active");
			}
			else {
				console.error("Filter ico element is not found in header cell");
			}

			if (anyUpdate) {
				this.RequestSorting();
			}
			return;
		}

		let hasFilteredRows = false;
		let applyFilter = (index, rowObject) => {
			if (index == -1) {
				rowObject.filteredBy.push(columnObject);
			}
			if (!rowObject.isFiltered) {
				rowObject.row.style.display = "none";
				rowObject.isFiltered = true;

				this.RemoveVisibleRow({ removedIndex : rowObject.index })
			}
			hasFilteredRows = true;
		};

		let anyNewVisible = false;
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

			if (rowObject.isFiltered) {
				if (index != -1) {
					rowObject.filteredBy.splice(index, 1);
				}

				for (let i = 0; i < rowObject.filteredBy.length; ++i) {
					if (rowObject.filteredBy[i].isFilterActive) {
						return;
					}
				}

				rowObject.row.style.display = "";
				rowObject.isFiltered = false;
				anyNewVisible = true;
				this.AddVisibleRow({ rowObject });
			}
		});

		let filter = columnObject.headerCell.querySelector(".filter");
		if (!filter) {
			console.error("Filter ico element is not found in header cell");

			if (anyNewVisible || hasFilteredRows) {
				this.RequestSorting();
			}

			return;
		}
		filter.classList.remove("disabled");

		let settingsView;
		for (let view of Grid.#privateFields.m_settingsViews) {
			if (view.m_parentView.parentNode && view.m_parameterId == columnObject.id) {
				settingsView = view;
				break;
			}
		}

		if (hasFilteredRows) {
			filter.classList.add("active");
			if (settingsView) {
				settingsView.m_view.querySelector(".group > .action.filter").classList.add("active");
			}

			this.RequestSorting();
			return;
		}

		filter.classList.remove("active");
		columnObject.isFilterActive = false;
		if (settingsView) {
			settingsView.m_view.querySelector(".group > .action.filter").classList.remove("active");
		}

		if (anyNewVisible) {
			this.RequestSorting();
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
		if (!column) {
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
		if (!columnObject) {
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
				document.dispatchEvent(new Event("click"));
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

	InsertCell({ columnObject, rowObject })
	{
		let cell = document.createElement("div");
		cell.classList.add("cell");
		cell.style.order = columnObject.index;
		cell.setAttribute("parameter-id", columnObject.id);

		if (columnObject.metadata.metadata.type == "TableData") {
			cell.classList.add("action", "table");

			let tableViews = Grid.#privateFields.m_tablesViewsForColumnsByRows;

			let tableView;
			cell.addEventListener("click", () => {
				if (tableView && tableView.m_parentView.parentNode) {
					return;
				}

				const indexValue = rowObject.values[this.m_indexColumnId];
				const tableMetadata = MetadataCollector.GetMetadata(this.m_indexColumnId);
				let viewTitle = "";
				if (tableMetadata) {
					viewTitle = tableMetadata.metadata.name + " " + indexValue;
				}
				else {
					viewTitle = "Table " + indexValue;
				}

				tableView = new TableView({
					eventTarget : cell,
					tableId : columnObject.id,
					metadata : columnObject.metadata.metadata,
					viewTitle,
					positionUnder : cell,
					canBeHidden : false,
					canBeMaximized : false,
					canBeSticked : false,
					canBeClinged : false
				});

				let table = tableView.m_tables.get(columnObject.id);
				if (!table) {
					console.error("Table is not found, parameter id:", columnObject.id);
					return;
				}

				const tableValue = rowObject.values[columnObject.id];
				if (tableValue) {
					if ("Rows" in tableValue) {
						for (let row of tableValue.Rows) {
							table.AddRow(row);
						}
					}
					else {
						console.error("Rows is not found in value", tableValue);
					}
				}

				if (!tableViews.has(indexValue)) {
					tableViews.set(indexValue, new Map());
				}
				tableViews.get(indexValue).set(columnObject.id, tableView);
			});

			rowObject.row.appendChild(cell);
			columnObject.cells.set(rowObject, cell);

			return undefined;
		}

		const value = rowObject.values[columnObject.id];

		if (columnObject.metadata.metadata.type == "Duration") {
			let input = document.createElement("input");
			input.readOnly = true;
			Duration.Apply(input, columnObject.metadata.metadata.durationType);
			if (value != undefined) {
				Duration.SetValue(input, BigInt(value));
			}
			cell.appendChild(input);
		}
		else if (columnObject.metadata.metadata.type == "Timer") {
			let input = document.createElement("input");
			input.readOnly = true;
			Timer.Apply(input);
			if (value != undefined) {
				Timer.SetValue(input, BigInt(value));
			}
			cell.appendChild(input);
		}
		else if (MetadataCollector.IsSelect(columnObject.id)) {
			let input = document.createElement("input");
			input.setAttribute("parameter-id", columnObject.id);
			Select.Apply({ input, setEvent : false });
			if (value != undefined) {
				Select.SetValue(input, value);
			}
			cell.appendChild(input);
		}
		else if (value != undefined) {
			Grid.SetValueToCell(cell, value, columnObject.metadata.metadata);
		}

		rowObject.row.appendChild(cell);
		columnObject.cells.set(rowObject, cell);
		return cell;
	}

	AddOrUpdateRow(values)
	{
		if (values instanceof Object === false) {
			console.error("Invalid values", values);
			return undefined;
		}
		if (!values.hasOwnProperty(this.m_indexColumnId)) {
			console.error("Index column", this.m_indexColumnId, "is not found in values:", values);
			return undefined;
		}
		if (this.m_rowByIndexValue.has(values[this.m_indexColumnId])) {
			return this.UpdateRow(values[this.m_indexColumnId], values);
		}

		let row = document.createElement("div");
		row.classList.add("row");
		const rowObject = { row, index : -1, values, filteredBy : [], isFiltered : false };
		this.m_rowByIndexValue.set(values[this.m_indexColumnId], rowObject);

		// Diagnostic
		row.setAttribute("data-index", this.m_rowByIndexValue.size - 1);

		this.m_columnByOrder.forEach((columnObject, order) => {
			let cell = this.InsertCell({ columnObject, rowObject });

			if (columnObject.isFilterActive && columnObject.filters.length != 0) {
				this.RequestFilters({ columnObject });
			}

			if (!cell) {
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

		if (this.m_postAddRowFunction) {
			this.m_postAddRowFunction(rowObject);
		}
		if (this.m_postUpdateRowFunction) {
			this.m_postUpdateRowFunction(rowObject, values);
		}

		this.AddVisibleRow({ rowObject });
		this.RequestSorting();

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

		// TableData handled separately

		cell.innerHTML = value;
	}

	UpdateRow(indexValue, values)
	{
		if (values instanceof Object === false) {
			console.error("Invalid values", values);
			return undefined;
		}

		let rowObject = this.m_rowByIndexValue.get(indexValue);
		if (!rowObject) {
			console.error(`Row with index ${indexValue} as ${indexValue} is not found`);
			return undefined;
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
			if (!metadata) {
				return;
			}

			if (metadata.metadata.type == "TableData") {
				let tableView = Grid.#privateFields.m_tablesViewsForColumnsByRows.get(indexValue)?.get(columnId);
				if (tableView) {
					let table = tableView.m_tables.get(columnId);
					if (!table) {
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
			if (!columnObject) {
				console.error("Column object is not found", columnId);
				return;
			}

			if (columnObject === this.m_sortingColumnObject) {
				this.RequestSorting();
			}
			if (columnObject.isFilterActive && columnObject.filters.length != 0) {
				this.RequestFilters({ columnObject });
			}
		});

		if (this.m_postUpdateRowFunction) {
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
		for (let [indexValue, rowObject] of this.m_rowByIndexValue) {
			if (Grid.MatchRow(cells, rowObject.values)) {
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

		for (let [indexValue, rowObject] of this.m_rowByIndexValue) {
			if (Grid.MatchRow(cells, rowObject.values)) {
				rows.push(rowObject.row);
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
		if (!rowObject) {
			return;
		}

		this.m_rowByIndexValue.delete(indexValue);
		this.RemoveVisibleRow({ removedIndex : rowObject.index });
	}

	AddVisibleRow({ rowObject })
	{
		rowObject.index = this.m_visibleRows.length;
		this.m_visibleRows.push(rowObject);
		this.m_pool.AddRow(rowObject.row);
	}

	RemoveVisibleRow({ removedIndex })
	{
		if (removedIndex < 0 || removedIndex >= this.m_visibleRows.length) {
			return;
		}

		const removedRow = this.m_visibleRows[removedIndex];
		for (let index = removedIndex; index < this.m_visibleRows.length - 1; ++index) {
			let shiftedRow = this.m_visibleRows[index + 1];
			--shiftedRow.index;
			this.m_visibleRows[index] = shiftedRow;
		}

		this.m_visibleRows.pop();
		removedRow.index = -1;
		this.m_pool.RemoveRow(removedIndex);
	}

	ClearRows()
	{
		this.m_columnByOrder.forEach((column) => column.cells.clear());
		this.m_rowByIndexValue.clear();
		this.m_visibleRows = [];
		this.m_pool.Reset();
	}

	Destructor()
	{
		if (this.m_pool) {
			this.m_pool.Destructor();
			this.m_pool = null;
		}
		this.m_pendingFilterColumns?.clear();
		this.m_isSortingPending = false;
		if (this.m_view) {
			this.m_view.remove();
			this.m_view = null;
		}
		if (this.m_columnByOrder) {
			this.m_columnByOrder.clear();
			this.m_columnByOrder = null;
		}
		if (this.m_columnById) {
			this.m_columnById.clear();
			this.m_columnById = null;
		}
		if (this.m_rowByIndexValue) {
			this.m_rowByIndexValue.clear();
			this.m_rowByIndexValue = null;
		}
		if (this.m_visibleRows) {
			this.m_visibleRows = [];
			this.m_visibleRows = null;
		}
	}
}

class Pool {
	constructor(grid)
	{
		this.m_grid = grid;
		this.m_parentNode = this.m_grid.m_parent.parentNode;
		this.m_height = 0;
		this.m_rowHeight = 0;
		this.m_capacity = 0;
		this.m_size = 0;
		this.m_capacityBuffer = 1;
		this.m_shift = 0;
		this.m_isEnd = true;
		this.m_partScrollY = 0;
		this.m_marginRow = null;

		if (typeof this.m_parentNode !== "object") {
			console.error("Invalid parentNode type, object is expected", this.m_parentNode);
			return;
		}

		this.m_parentNode.style.overflowY = "hidden";
		this.m_scrollbarY = document.createElement("div");
		this.m_scrollbarY.classList.add("scrollbarY");
		this.m_barY = document.createElement("div");
		this.m_barY.classList.add("bar");
		this.m_scrollbarY.appendChild(this.m_barY);
		this.m_parentNode.appendChild(this.m_scrollbarY);
		this.m_barYHeight = 0;

		this.m_onWheel = (event) => {
			if (event.ctrlKey || event.metaKey) {
				return;
			}

			event.preventDefault();
			this.Scroll(event.deltaY);
		};
		this.m_wheelOptions = { passive : false };
		this.m_parentNode.addEventListener('wheel', this.m_onWheel, this.m_wheelOptions);

		this.m_stopScrolling = null;
		this.m_onBarMouseDown = (event) => {
			event.preventDefault();
			this.m_barY.classList.add("active");

			const range = this.m_height - this.m_barYHeight;
			const maxShift = Math.max(0, this.m_grid.m_visibleRows.length - this.m_size);
			const startY = event.clientY;
			const startShift = this.m_shift;

			const onMouseMove = (ev) => {
				if (range <= 0 || maxShift == 0) {
					return;
				}

				this.SetShift(Math.round(startShift + (ev.clientY - startY) * maxShift / range));
			};

			const onMouseUp = () => {
				this.m_barY.classList.remove("active");
				document.removeEventListener('mousemove', onMouseMove);
				document.removeEventListener('mouseup', onMouseUp);
				this.m_stopScrolling = null;
			};

			this.m_stopScrolling = onMouseUp;
			document.addEventListener('mousemove', onMouseMove);
			document.addEventListener('mouseup', onMouseUp);
		};
		this.m_barY.addEventListener('mousedown', this.m_onBarMouseDown);

		// Ensure the row container can receive focus
		this.m_parentNode.setAttribute('tabindex', '0');

		this.m_onKeyDown = (e) => {
			switch (e.key) {
			case 'ArrowDown':
				this.m_partScrollY = 0;
				this.Scroll(this.m_rowHeight);
				e.preventDefault();
				return;
			case 'ArrowUp':
				this.m_partScrollY = 0;
				this.Scroll(-this.m_rowHeight);
				e.preventDefault();
				return;
			case 'PageDown':
				this.m_partScrollY = 0;
				this.Scroll(this.m_rowHeight * Math.max(1, this.m_capacity - this.m_capacityBuffer));
				e.preventDefault();
				return;
			case 'PageUp':
				this.m_partScrollY = 0;
				this.Scroll(-this.m_rowHeight * Math.max(1, this.m_capacity - this.m_capacityBuffer));
				e.preventDefault();
				return;
			case 'Home':
				this.SetShift(0);
				e.preventDefault();
				return;
			case 'End':
				this.SetShift(this.m_grid.m_visibleRows.length - this.m_size);
				e.preventDefault();
				return;
			}
		};
		this.m_parentNode.addEventListener('keydown', this.m_onKeyDown);

		this.m_resizeObserver = new ResizeObserver((entries) => {
			for (const entry of entries) {
				this.Resize(entry.target.offsetHeight);
			}
		});

		this.m_resizeObserver.observe(this.m_parentNode);
	}

	/**************************
	 * @brief Disconnect the observer, remove all listeners and the scrollbar, and release the grid reference.
	 */
	Destructor()
	{
		if (this.m_resizeObserver) {
			this.m_resizeObserver.disconnect();
			this.m_resizeObserver = null;
		}
		if (this.m_stopScrolling) {
			this.m_stopScrolling();
		}
		if (this.m_parentNode && typeof this.m_parentNode === "object") {
			if (this.m_onWheel) {
				this.m_parentNode.removeEventListener('wheel', this.m_onWheel, this.m_wheelOptions);
			}
			if (this.m_onKeyDown) {
				this.m_parentNode.removeEventListener('keydown', this.m_onKeyDown);
			}
		}
		if (this.m_barY && this.m_onBarMouseDown) {
			this.m_barY.removeEventListener('mousedown', this.m_onBarMouseDown);
		}
		if (this.m_scrollbarY) {
			this.m_scrollbarY.remove();
		}

		this.m_onWheel = null;
		this.m_onKeyDown = null;
		this.m_onBarMouseDown = null;
		this.m_scrollbarY = null;
		this.m_barY = null;
		this.m_marginRow = null;
		this.m_parentNode = null;
		this.m_grid = null;
	}

	AddRow(row)
	{
		const hasRowHeight = this.m_rowHeight != 0;
		if (!this.SetRowHeight(row)) {
			return;
		}

		if (hasRowHeight) {
			this.Render(this.m_shift);
			return;
		}

		this.Resize(this.m_parentNode.offsetHeight);
	}

	SetRowHeight(row)
	{
		if (this.m_rowHeight != 0) {
			return true;
		}

		const display = row.style.display;
		row.style.display = "";
		this.m_grid.m_content.appendChild(row);
		this.m_rowHeight = Helper.GetFullDimensions(row).height;
		row.remove();
		row.style.display = display;

		if (this.m_rowHeight <= 0) {
			console.error("Row height measurement for pool rendering is failed");
			this.m_rowHeight = 0;
			return false;
		}

		return true;
	}

	Resize(height)
	{
		const headerRowHeight = Helper.GetFullDimensions(this.m_grid.m_header).height;
		if (this.m_rowHeight == 0) {
			return;
		}

		const renderedRow = this.m_grid.m_content.firstElementChild;
		if (renderedRow) {
			renderedRow.style.marginTop = "";
			const rowHeight = Helper.GetFullDimensions(renderedRow).height;
			if (rowHeight > 0) {
				this.m_rowHeight = rowHeight;
			}
			else {
				console.error("Row height measurement for pool rendering is failed");
			}
		}

		const viewHeader = this.m_parentNode.parentNode.querySelector('.viewHeader');
		const headerViewHeight = viewHeader ? Helper.GetFullDimensions(viewHeader).height : 0;
		this.m_scrollbarY.style.top = headerRowHeight + headerViewHeight + "px";

		this.m_height = Math.max(0, height - headerRowHeight + 1 /* Avoid space under scrollbar */);
		this.m_scrollbarY.style.height = this.m_height + "px";
		this.m_capacity = this.m_height == 0 ? 0 : Math.floor(this.m_height / this.m_rowHeight) + this.m_capacityBuffer;
		this.Render(this.m_shift);
	}

	Render(shift)
	{
		const rowsNumber = this.m_grid.m_visibleRows.length;
		const newSize = Math.min(this.m_capacity, rowsNumber);
		const newShift = Math.max(0, Math.min(rowsNumber - newSize, shift));
		const oldEnd = this.m_shift + this.m_size;
		const newEnd = newShift + newSize;
		const hasOverlap = newShift < oldEnd && this.m_shift < newEnd;

		if (hasOverlap) {
			for (let index = this.m_shift; index < newShift; ++index) {
				this.m_grid.m_content.firstElementChild.remove();
			}
			for (let index = oldEnd; index > newEnd; --index) {
				this.m_grid.m_content.lastElementChild.remove();
			}
			for (let index = this.m_shift - 1; index >= newShift; --index) {
				const row = this.GetRow(index);
				if (row) {
					this.m_grid.m_content.prepend(row);
				}
			}
			for (let index = oldEnd; index < newEnd; ++index) {
				const row = this.GetRow(index);
				if (row) {
					this.m_grid.m_content.appendChild(row);
				}
			}
		}
		else {
			this.m_grid.m_content.replaceChildren();
			for (let index = newShift; index < newEnd; ++index) {
				const row = this.GetRow(index);
				if (row) {
					this.m_grid.m_content.appendChild(row);
				}
			}
		}

		this.m_shift = newShift;
		this.m_size = newSize;
		this.m_isEnd = newShift + newSize >= rowsNumber;
		this.UpdateFirstRowMargin();
		this.UpdateScrollbarHeight();
		this.UpdateScrollbarPosition();
	}

	Rerender()
	{
		this.m_grid.m_content.replaceChildren();
		this.m_size = 0;
		this.Render(this.m_shift);
	}

	/**************************
	 * @brief Reorder only the rendered pool rows that changed position.
	 *
	 * This avoids rebuilding the whole pool after sorting. Its work is bounded by the number of rendered rows, not the
	 * total number of rows in the grid.
	 */
	Reorder()
	{
		const content = this.m_grid.m_content;
		const end = this.m_shift + this.m_size;
		let firstChanged = false;

		for (let index = this.m_shift; index < end; ++index) {
			const row = this.GetRow(index);
			const current = content.children[index - this.m_shift] ?? null;
			if (!row || row === current) {
				continue;
			}

			if (index == this.m_shift) {
				firstChanged = true;
			}
			content.insertBefore(row, current);
		}

		while (content.children.length > this.m_size) {
			content.lastElementChild.remove();
		}

		if (firstChanged) {
			this.UpdateFirstRowMargin();
		}
	}

	/**************************
	 * @brief Update pool after visible row at index was removed from grid container.
	 */
	RemoveRow(index)
	{
		const end = this.m_shift + this.m_size;
		if (index < this.m_shift) {
			--this.m_shift;
		}
		else if (index < end) {
			this.Rerender();
			return;
		}

		const rowsNumber = this.m_grid.m_visibleRows.length;
		if (this.m_shift + this.m_size > rowsNumber) {
			this.Rerender();
			return;
		}

		this.m_isEnd = this.m_shift + this.m_size >= rowsNumber;
		this.UpdateFirstRowMargin();
		this.UpdateScrollbarHeight();
		this.UpdateScrollbarPosition();
	}

	Reset()
	{
		if (this.m_marginRow) {
			this.m_marginRow.style.marginTop = "";
			this.m_marginRow = null;
		}
		this.m_grid.m_content.replaceChildren();
		this.m_size = 0;
		this.m_shift = 0;
		this.m_isEnd = true;
		this.m_partScrollY = 0;
		this.UpdateScrollbarHeight();
		this.UpdateScrollbarPosition();
	}

	/**************************
	 * @brief Only one row holds the overflow margin; it is tracked so it is cleared even after being detached.
	 */
	UpdateFirstRowMargin()
	{
		const firstRow = this.m_grid.m_content.firstElementChild;
		if (this.m_marginRow && this.m_marginRow !== firstRow) {
			this.m_marginRow.style.marginTop = "";
		}
		this.m_marginRow = firstRow;

		if (!firstRow) {
			return;
		}

		const overflow = this.m_isEnd ? Math.max(0, this.m_size * this.m_rowHeight - this.m_height) : 0;
		firstRow.style.marginTop = overflow > 0 ? -Math.min(this.m_rowHeight, overflow) + "px" : "";
	}

	GetRow(index)
	{
		const rowObject = this.m_grid.m_visibleRows[index];
		if (!rowObject) {
			console.error("Row is not found while rendering pool", index);
			return null;
		}

		return rowObject.row;
	}

	UpdateScrollbarHeight()
	{
		const MIN_THUMB_SIZE_PX = 20;
		const rowsNumber = this.m_grid.m_visibleRows.length;
		const canScroll = rowsNumber > this.m_size && this.m_height > 0;
		this.m_scrollbarY.style.display = canScroll ? "" : "none";

		if (!canScroll) {
			this.m_barYHeight = this.m_height;
			this.m_barY.style.height = this.m_height + "px";
			return;
		}

		const rawThumbSize = this.m_height * this.m_size / rowsNumber;
		this.m_barYHeight = Math.min(this.m_height, Math.max(MIN_THUMB_SIZE_PX, rawThumbSize));
		this.m_barY.style.height = this.m_barYHeight + "px";
	}

	Scroll(deltaY)
	{
		if (deltaY == 0 || this.m_rowHeight == 0) {
			return;
		}

		if (Math.sign(deltaY) != Math.sign(this.m_partScrollY)) {
			this.m_partScrollY = 0;
		}

		this.m_partScrollY += deltaY;
		const shift = Math.trunc(this.m_partScrollY / this.m_rowHeight);
		if (shift == 0) {
			return;
		}

		this.m_partScrollY -= shift * this.m_rowHeight;
		this.SetShift(this.m_shift + shift);
	}

	SetShift(shift)
	{
		const maxShift = Math.max(0, this.m_grid.m_visibleRows.length - this.m_size);
		const newShift = Math.max(0, Math.min(maxShift, shift));
		if (newShift == this.m_shift) {
			this.m_partScrollY = 0;
			return;
		}

		this.Render(newShift);
	}

	UpdateScrollbarPosition()
	{
		const range = this.m_height - this.m_barYHeight;
		const maxShift = this.m_grid.m_visibleRows.length - this.m_size;
		const position = maxShift > 0 ? this.m_shift * range / maxShift : 0;
		this.m_barY.style.top = position + "px";
	}
}

if (typeof module !== "undefined" && typeof module.exports !== "undefined") {
	module.exports = Grid;
	Helper = require("../help/helper");
	Timer = require("./timer");
	Select = require("./select");
	Duration = require("./duration");
	View = require("./view");
	Table = require("./table");
	TableView = require("../views/tableView");
	GridSettingsView = require("../views/gridSettingsView");
	SelectView = require("../views/selectView");
	MetadataCollector = require("../views/metadataCollector");
}