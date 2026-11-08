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
 * @brief Abstraction for any view that can be created, represent interface unit or application.
 * It provides functionality for initializing views, handling metadata, managing parameters, and interacting with the
 * backend through HTTP requests or WebSocket streams. To use it:
 * - Extend the Class: Create a new class that inherits from View, provide a unique view type name as a first
 * argument to super.
 * - Override the Constructor: Implement the Constructor method to define custom initialization logic for your view.
 * - Register a Template: Use View.AddViewTemplate to define the HTML structure for the view.
 * - Add Callbacks: Use AddCallback to handle specific HTTP responses.
 * - Send Requests: Use SendRequest to communicate with the backend via HTTP.
 * - Open streams: Use OpenStream to communicate with the backend via WebSocket.
 *
 * Has three parts:
 * 1) Header which contains view tile and on view options:
 *	- Stick/unstick to disables/enables moving and resizing of view and another actions except closing;
 * @field m_canBeSticked - flag of view can be sticked/unsticked.
 * @field m_sticked - flag of view is sticked;
 *	- Hide to hide view, all hidden views are stored in dispatcher and can be showed back from there;
 * @field m_canBeHidden - flag of view can be hidden;
 *	- Maximize/normalize to enlarge view to full window size and return view to previous size and position;
 * @field m_canBeMaximized - flag of view can be maximized/normalized.
 * @field m_maximized - flag of view is maximized;
 * @brief m_savedViewParameters - saved view parameters before maximize.
 *	- Close to close view and remove it from DOM tree.
 * @field m_canBeClosed - flag of view can be closed.
 * 2) Content which contains the main part of a view, exact in that part the specific view template is placed;
 * @brief m_viewNode - root node of the specific view template.
 * @brief m_contentNode - container node for the specific view template.
 * 3) Footer which contains information about errors.
 *
 * Features:
 * 1) Creating view from template, template should be added by static method before creating view;
 * @field m_viewType - type of view, should match template added by static method;
 * 2) Moving view by dragging header. View cannot be moved outside window borders except bottom border;
 * 3) Resizing view by dragging any side or corner of view, borders of that view are highlights. View cannot be resized
 *outside window borders except bottom border. View cannot be resized smaller than some minimum in each dimension;
 *
 * When view is moved or resized, all other views are marked as "changing" and their content become inaccessible.
 *
 * 4) Resizing view to its ergonomic size by double clicking any side or corner of view;
 * 5) Clinging to other views and window borders when moving or resizing. Does not cling to window bottom border.
 * @static m_clingSensitive - distance to other views and window borders to cling.
 * @field m_canBeClinged - flag of view is clingy to other views.
 *
 * Communication between MSAPI Manager happens through HTTP requests, each request has its type and response can be
 * parsed by any view if it has corresponding callback.
 *
 * Common fields are:
 * @brief m_parameters - parameters of view.
 * @brief m_uid - unique identifier of view.
 * @brief m_port - port associated with view.
 * @brief m_rootNode - root node of the complete view wrapper.
 * @brief m_mountNode - existing node that receives the view.
 * @brief m_headerNode - header node of view.
 * @brief m_titleTextNode - title text span.
 * @brief m_titleInfoNode - span for optional information independent of the title.
 * @brief m_tableIdToTable - map of tables in view.
 * @brief m_grid - grid in view.
 * @brief m_created - flag of view creation.
 * @brief m_maximizeButton - maximize button element.
 * @brief m_footerNode - footer node of view.
 * @brief m_errorMessage - error message element in footer.
 * @brief m_silentExit - flag to suppress error logging on failed Constructor call.
 *
 * Parameters:
 * - isInterfaceUnit - if true, view will be created without default view wrapper.
 *
 * @todo Think if web sockets can be used for communication.
 */

class View {
	static #privateFields = (() => {
		const m_viewTypeToTemplateObject = new Map();
		// Each view type maps template names to parameter templates.
		const m_viewTypeToParametersTemplates = new Map();
		const m_portToParameters = new Map();
		const m_uidToView = new Map();
		const m_lastCreatedView = null;
		return {
			m_viewTypeToTemplateObject,
			m_viewTypeToParametersTemplates,
			m_portToParameters,
			m_uidToView,
			m_lastCreatedView
		};
	})();

	// Minimum visible height in pixels when placing a new view near the window's bottom edge.
	static MIN_VISIBLE_HEIGHT = 50;

	static m_clingSensitive = 20;

	m_mountNode = undefined;
	m_rootNode = undefined;
	m_headerNode = undefined;
	m_titleTextNode = undefined;
	m_titleInfoNode = undefined;
	m_contentNode = undefined;
	m_viewNode = undefined;
	m_footerNode = undefined;

	/**************************
	 * @brief Resolves the positioned parent's viewport origin and visible area, accounting for its border and scroll.
	 * Uses document coordinates when the view belongs to the initial containing block.
	 *
	 * @return Viewport origin and visible bounds expressed in the view's containing-block coordinates.
	 *
	 * @test Yes.
	 */
	GetPositionContext()
	{
		const parent = this.m_rootNode.offsetParent;
		if (!parent || (parent === document.body && window.getComputedStyle(parent).position === "static")) {
			return {
				originLeft : -window.scrollX,
				originTop : -window.scrollY,
				left : window.scrollX,
				top : window.scrollY,
				right : window.scrollX + window.innerWidth,
				bottom : window.scrollY + window.innerHeight
			};
		}

		const rect = parent.getBoundingClientRect();
		const innerLeft = rect.left + parent.clientLeft;
		const innerTop = rect.top + parent.clientTop;
		const originLeft = innerLeft - parent.scrollLeft;
		const originTop = innerTop - parent.scrollTop;
		const left = Math.max(0, innerLeft) - originLeft;
		const top = Math.max(0, innerTop) - originTop;
		return {
			originLeft,
			originTop,
			left,
			top,
			right : Math.max(left, Math.min(window.innerWidth, innerLeft + parent.clientWidth) - originLeft),
			bottom : Math.max(top, Math.min(window.innerHeight, innerTop + parent.clientHeight) - originTop)
		};
	}

	/**************************
	 * @brief Finds nearby view edges and containing-block boundaries in the current view's positioning coordinates.
	 *
	 * @param bounds Proposed top, right, bottom, and left edges relative to the view's containing block.
	 *
	 * @return Adjusted left and top coordinates when an edge is close enough to snap, or undefined otherwise.
	 *
	 * @test Yes.
	 */
	FindClosestToCling({ top, right, bottom, left })
	{
		top = Math.round(top);
		right = Math.round(right);
		bottom = Math.round(bottom);
		left = Math.round(left);
		let mins = { x : View.m_clingSensitive + 1, y : View.m_clingSensitive + 1 };
		let clingData = { left : left, top : top, clinged : false };

		function CheckX(aPos, bPos)
		{
			let diff = aPos - bPos;
			let absDiff = Math.abs(diff);
			if (absDiff == 0) {
				return;
			}
			if (absDiff < mins.x) {
				mins.x = absDiff;
				clingData.left = left + diff;
				clingData.clinged = true;
			}
		}

		function CheckY(aPos, bPos)
		{
			let diff = aPos - bPos;
			let absDiff = Math.abs(diff);
			if (absDiff == 0) {
				return;
			}
			if (absDiff < mins.y) {
				mins.y = absDiff;
				clingData.top = top + diff;
				clingData.clinged = true;
			}
		}

		const context = this.GetPositionContext();
		CheckX(context.left, left);
		CheckX(context.right, right);
		CheckY(context.top, top);

		for (const other of View.GetCreatedViews().values()) {
			if (other == this || !other.m_canBeClinged) {
				continue;
			}
			const oRect = other.m_rootNode.getBoundingClientRect();
			const oTop = Math.round(oRect.top - context.originTop);
			const oRight = Math.round(oRect.right - context.originLeft);
			const oBottom = Math.round(oRect.bottom - context.originTop);
			const oLeft = Math.round(oRect.left - context.originLeft);

			CheckX(oLeft, right);
			CheckX(oRight, left);
			CheckX(oLeft, left);
			CheckX(oRight, right);

			CheckY(oTop, bottom);
			CheckY(oBottom, top);
			CheckY(oTop, top);
			CheckY(oBottom, bottom);
		}

		return mins.x <= View.m_clingSensitive || mins.y <= View.m_clingSensitive ? clingData : undefined;
	}

	static AddViewTemplate(viewType, template)
	{
		if (View.#privateFields.m_viewTypeToTemplateObject.has(viewType)) {
			console.warn("View template for view type", viewType, "is already registered, overwriting");
		}
		View.#privateFields.m_viewTypeToTemplateObject.set(viewType, { template, templateElement : null });
	}

	static GetViewTemplate(viewType) { return View.#privateFields.m_viewTypeToTemplateObject.get(viewType); }

	static AddParametersTemplate(viewType, templateName, template)
	{
		if (!View.#privateFields.m_viewTypeToParametersTemplates.has(viewType)) {
			View.#privateFields.m_viewTypeToParametersTemplates.set(viewType, new Map());
			View.#privateFields.m_viewTypeToParametersTemplates.get(viewType).set(templateName, template);
			return;
		}
		View.#privateFields.m_viewTypeToParametersTemplates.get(viewType).set(templateName, template);
	}

	static GetParametersTemplates(viewType)
	{
		return View.#privateFields.m_viewTypeToParametersTemplates.get(viewType);
	}

	static GetParametersTemplate(viewType, templateName)
	{
		if (View.#privateFields.m_viewTypeToParametersTemplates.has(viewType)) {
			if (View.#privateFields.m_viewTypeToParametersTemplates.get(viewType).has(templateName)) {
				return View.#privateFields.m_viewTypeToParametersTemplates.get(viewType).get(templateName);
			}
		}

		return undefined;
	}

	static SaveView(view)
	{
		if (!view) {
			console.error("Invalid view instance");
			return;
		}

		View.#privateFields.m_lastCreatedView = view;
		View.#privateFields.m_uidToView.set(view.m_uid, view);
	}

	static GetLastCreatedView() { return View.#privateFields.m_lastCreatedView; }

	static GetCreatedViews() { return View.#privateFields.m_uidToView; }

	static ShowExistedView(newView)
	{
		for (let view of View.#privateFields.m_uidToView.values()) {
			if (view.m_viewType == newView.m_viewType && view.m_port == newView.m_port) {
				view.Show();
				newView.m_silentExit = true;
				return true;
			}
		}

		return false;
	}

	static RemoveCreatedView(view) { View.#privateFields.m_uidToView.delete(view.m_uid); }

	static HandleResponse(type, response, parameters)
	{
		const callbackName = "Handle" + type.charAt(0).toUpperCase() + type.slice(1) + "Response";
		for (let view of View.#privateFields.m_uidToView.values()) {
			if (view.hasOwnProperty(callbackName)) {
				view[callbackName](response, parameters);
			}
		}
	}

	static #generalTemplate = `
	<div class="view">
		<div class="viewHeader">
			<div class="title"><span class="text"></span><span class="info"></span></div>
			<div class="options">
				<div class="stick"><span></span></div>
				<div class="hide"><span></span></div>
				<div class="maximize"><span></span></div>
				<div class="close"><span></span></div>
			</div>
		</div>
		<div class="viewContent"></div>
		<div class="viewFooter"></div>
		<div class="handleResize top left twoDirections"></div>
		<div class="handleResize top right twoDirections"></div>
		<div class="handleResize bottom left twoDirections"></div>
		<div class="handleResize bottom right twoDirections"></div>
		<div class="handleResize top oneDirection"></div>
		<div class="handleResize right oneDirection"></div>
		<div class="handleResize bottom oneDirection"></div>
		<div class="handleResize left oneDirection"></div>
	</div>`;
	static #generalTemplateElement = null;

	/**************************
	 * @brief Creates and initializes a view from its registered template. Converts viewport anchors into
	 * containing-block coordinates and clamps views to the same horizontal bounds as dragging. Keeps the header and a
	 * minimum visible strip above the bottom edge. Observes subsequent size changes to preserve these bounds when
	 * callers append more content after construction.
	 *
	 * @param viewType Registered view type.
	 * @param parameters View configuration and optional positionUnder anchor.
	 *
	 * @test Yes.
	 */
	constructor(viewType, parameters)
	{
		try {
			this.m_viewType = viewType;
			this.m_parameters = parameters;

			if (parameters && parameters.parent) {
				this.m_mountNode = parameters.parent;
			}
			else {
				this.m_mountNode = document.querySelector("body > main > section.views");
			}

			if (!this.m_mountNode) {
				console.error("Can't find mount node for view");
				return false;
			}

			if (!View.#privateFields.m_viewTypeToTemplateObject.has(viewType)) {
				console.error("Can't find view template for view");
				return false;
			}

			let specificTemplate = View.#privateFields.m_viewTypeToTemplateObject.get(viewType);
			if (!specificTemplate.templateElement) {
				const template = document.createElement("template");
				template.innerHTML = specificTemplate.template;
				specificTemplate.templateElement = template;
			}

			if (parameters && parameters.isInterfaceUnit) {
				this.m_viewNode = specificTemplate.templateElement.content.cloneNode(true).firstElementChild;
				this.m_mountNode.appendChild(this.m_viewNode);

				if (!this.Constructor(parameters)) {
					this.Destructor();
					console.error("Can't create view");
					return;
				}

				return;
			}

			if (!View.#generalTemplateElement) {
				const template = document.createElement("template");
				template.innerHTML = View.#generalTemplate;
				View.#generalTemplateElement = template;
			}

			this.m_rootNode = View.#generalTemplateElement.content.cloneNode(true).firstElementChild;
			this.m_headerNode = this.m_rootNode.querySelector(".viewHeader");
			this.m_titleTextNode = this.m_headerNode.querySelector(".title > .text");
			this.m_titleInfoNode = this.m_headerNode.querySelector(".title > .info");
			this.m_contentNode = this.m_rootNode.querySelector(".viewContent");
			this.m_footerNode = this.m_rootNode.querySelector(".viewFooter");

			this.SetTitleText(viewType);
			this.m_contentNode.appendChild(specificTemplate.templateElement.content.cloneNode(true));
			this.m_viewNode = this.m_contentNode.lastElementChild;
			this.m_mountNode.appendChild(this.m_rootNode);

			this.m_tableIdToTable = new Map();

			this.MakeDraggable(this.m_headerNode.querySelector(".title"), this.m_rootNode);
			this.m_rootNode.addEventListener("mousedown", () => { View.UpdateZIndex(this); });

			this.m_rootNode.style.left = "0px";
			this.m_rootNode.style.top = "0px";

			if (parameters) {
				this.m_canBeHidden = parameters.canBeHidden != false;
				this.m_canBeMaximized = parameters.canBeMaximized != false;
				this.m_canBeSticked = parameters.canBeSticked != false;
				this.m_canBeClosed = parameters.canBeClosed != false;
				this.m_canBeClinged = parameters.canBeClinged != false;
			}
			else {
				this.m_canBeHidden = true;
				this.m_canBeMaximized = true;
				this.m_canBeSticked = true;
				this.m_canBeClosed = true;
				this.m_canBeClinged = true;
			}

			this.m_rootNode.querySelectorAll('.handleResize').forEach(handle => {
				const direction = Array.from(handle.classList);
				handle.addEventListener('mousedown', e => { StartResizing(e, direction, this); });

				handle.addEventListener('dblclick', e => {
					if (this.m_sticked) {
						return;
					}

					if (direction.includes('right') || direction.includes('left')) {
						let w = Helper.GetFullDimensions(this.m_viewNode).width;
						const style = window.getComputedStyle(this.m_rootNode);
						w += parseFloat(style.marginLeft) || 0;
						w += parseFloat(style.marginRight) || 0;
						w += parseFloat(style.borderLeftWidth) || 0;
						w += parseFloat(style.borderRightWidth) || 0;
						w += parseFloat(style.paddingLeft) || 0;
						w += parseFloat(style.paddingRight) || 0;
						this.m_rootNode.style.width = w + 'px';
					}

					if (direction.includes('bottom') || direction.includes('top')) {
						let h = Helper.GetFullDimensions(this.m_viewNode).height;
						h += Helper.GetFullDimensions(this.m_headerNode).height
							+ Helper.GetFullDimensions(this.m_footerNode).height;
						const style = window.getComputedStyle(this.m_rootNode);
						h += parseFloat(style.marginTop) || 0;
						h += parseFloat(style.marginBottom) || 0;
						h += parseFloat(style.borderTopWidth) || 0;
						h += parseFloat(style.borderBottomWidth) || 0;
						h += parseFloat(style.paddingTop) || 0;
						h += parseFloat(style.paddingBottom) || 0;
						this.m_rootNode.style.height = h + 'px';
					}
				});
			});

			function StartResizing(element, direction, savedThis)
			{
				if (savedThis.m_sticked) {
					return;
				}

				savedThis.m_rootNode.classList.add("resizing");
				View.GetCreatedViews().values().forEach((view) => { view.m_contentNode.classList.add("changing"); });

				element.preventDefault();
				const startContext = savedThis.GetPositionContext();
				const startX = element.clientX - startContext.originLeft;
				const startY = element.clientY - startContext.originTop;
				const startWidth = savedThis.m_rootNode.offsetWidth;
				const startHeight = savedThis.m_rootNode.offsetHeight;
				const startTop = savedThis.m_rootNode.offsetTop;
				const startLeft = savedThis.m_rootNode.offsetLeft;

				function OnMouseMove(ev)
				{
					const context = savedThis.GetPositionContext();
					const dx = ev.clientX - context.originLeft - startX;
					const dy = ev.clientY - context.originTop - startY;
					let newWidth = startWidth;
					let newHeight = startHeight;
					let newTop = startTop;
					let newLeft = startLeft;
					let includesTop = direction.includes('top');
					let includesRight = direction.includes('right');
					let includesBottom = direction.includes('bottom');
					let includesLeft = direction.includes('left');

					if (includesRight) {
						newWidth += dx;
						if (newWidth + newLeft > context.right) {
							newWidth = context.right - newLeft;
						}

						if (newWidth < 50) {
							newWidth = 50;
						}
					}
					if (includesLeft) {
						if (newLeft + dx < context.left) {
							newWidth += newLeft - context.left;
							newLeft = context.left;
						}
						else {
							newWidth -= dx;
							newLeft += dx;
						}

						if (newWidth < 50) {
							newWidth = 50;
							newLeft = startLeft + (startWidth - newWidth);
						}
					}
					if (includesBottom) {
						newHeight += dy;

						if (newHeight < 50) {
							newHeight = 50;
						}
					}
					if (includesTop) {
						if (newTop + dy < context.top) {
							newHeight += newTop - context.top;
							newTop = context.top;
						}
						else {
							newHeight -= dy;
							newTop += dy;
						}

						if (newHeight < 50) {
							newHeight = 50;
							newTop = startTop + (startHeight - newHeight);
						}
					}

					if (!savedThis.m_canBeClinged) {
						savedThis.m_rootNode.style.left = newLeft + "px";
						savedThis.m_rootNode.style.top = newTop + "px";
						savedThis.m_rootNode.style.width = newWidth + "px";
						savedThis.m_rootNode.style.height = newHeight + "px";
					}
					else {
						const clingData = savedThis.FindClosestToCling(
							{ top : newTop, right : newLeft + newWidth, bottom : newTop + newHeight, left : newLeft });

						if (clingData) {
							if (includesRight) {
								savedThis.m_rootNode.style.left = newLeft + "px";
								savedThis.m_rootNode.style.width = newWidth + (clingData.left - newLeft) + "px";
							}
							else {
								savedThis.m_rootNode.style.left = clingData.left + "px";
								savedThis.m_rootNode.style.width = newWidth - (clingData.left - newLeft) + "px";
							}

							if (includesBottom) {
								savedThis.m_rootNode.style.top = newTop + "px";
								savedThis.m_rootNode.style.height = newHeight + (clingData.top - newTop) + "px";
							}
							else {
								savedThis.m_rootNode.style.top = clingData.top + "px";
								savedThis.m_rootNode.style.height = newHeight - (clingData.top - newTop) + "px";
							}
						}
						else {
							savedThis.m_rootNode.style.left = newLeft + "px";
							savedThis.m_rootNode.style.top = newTop + "px";
							savedThis.m_rootNode.style.width = newWidth + "px";
							savedThis.m_rootNode.style.height = newHeight + "px";
						}
					}
				}

				function OnMouseUp()
				{
					savedThis.m_rootNode.classList.remove("resizing");
					View.GetCreatedViews().values().forEach(
						(view) => { view.m_contentNode.classList.remove("changing"); });
					document.removeEventListener('mousemove', OnMouseMove);
					document.removeEventListener('mouseup', OnMouseUp);
				}

				document.addEventListener('mousemove', OnMouseMove);
				document.addEventListener('mouseup', OnMouseUp);
			}

			let closeButton = this.m_headerNode.querySelector(".close");
			if (!closeButton) {
				console.error("Can't find close button for view");
				return;
			}
			if (this.m_canBeClosed) {
				closeButton.addEventListener("click", () => {
					if (this.m_sticked) {
						return;
					}
					this.Destructor();
				});
			}
			else {
				closeButton.style.display = "none";
			}

			let hideButton = this.m_headerNode.querySelector(".hide");
			if (!hideButton) {
				console.error("Can't find hide button for view");
				this.Destructor();
			}
			this.m_maximizeButton = this.m_headerNode.querySelector(".maximize");
			if (!this.m_maximizeButton) {
				console.error("Can't find maximize button for view");
				this.Destructor();
			}

			if (this.m_canBeMaximized) {
				hideButton.addEventListener("click", () => { this.Hide(); });
				this.m_maximizeButton.addEventListener("click", () => { this.Maximize(); });
			}
			else {
				hideButton.style.display = "none";
				this.m_maximizeButton.style.display = "none";
			}

			this.m_stickButton = this.m_headerNode.querySelector(".stick");
			if (!this.m_stickButton) {
				console.error("Can't find stick button for view");
				this.Destructor();
			}

			if (this.m_canBeSticked) {
				this.m_stickButton.addEventListener("click", () => { this.ToggleStick(); });
			}
			else {
				this.m_stickButton.style.display = "none";
			}

			if (parameters && parameters.hasOwnProperty("port")) {
				this.m_port = parameters.port;
			}

			this.m_uid = Helper.GenerateUid();
			if (!this.Constructor(parameters)) {
				this.Destructor();
				if (!this.m_silentExit) {
					console.error("Can't create view");
				}
				return;
			}

			View.SaveView(this);
			this.m_rootNode.setAttribute("uid", this.m_uid);
			this.m_rootNode.style.zIndex = View.#privateFields.m_uidToView.size;

			if (parameters) {
				if (parameters.hasOwnProperty("positionUnder")) {
					const anchor = parameters.positionUnder.getBoundingClientRect();
					const context = this.GetPositionContext();
					this.m_rootNode.style.position = "absolute";
					this.m_rootNode.style.left = anchor.left - context.originLeft + "px";
					this.m_rootNode.style.top = anchor.bottom - context.originTop + "px";
				}
			}

			this.ClampPosition();
			// Callers can add content after construction, as Grid does with its column-filter table.
			this.m_positionResizeObserver = new ResizeObserver(() => this.ClampPosition());
			this.m_positionResizeObserver.observe(this.m_rootNode, { box : "border-box" });

			this.m_created = true;
		}
		catch (error) {
			console.error("Can't create view:", error);
			this.Destructor();
		}
	}

	/**************************
	 * @brief Clamps an attached view to its containing block's visible horizontal area and keeps its header and
	 * minimum visible height above the bottom edge. Handles initial placement, content size changes, and viewport
	 * resizing without moving sticked, hidden, or detached views, including during browser zoom changes.
	 *
	 * @test Yes.
	 */
	ClampPosition()
	{
		if (this.m_sticked || !this.m_rootNode?.isConnected || this.m_rootNode.classList.contains("hidden")) {
			return;
		}

		const context = this.GetPositionContext();
		this.m_rootNode.style.left
			= Math.max(context.left, Math.min(this.m_rootNode.offsetLeft, context.right - this.m_rootNode.offsetWidth))
			+ "px";

		const visibleHeight
			= Math.max(View.MIN_VISIBLE_HEIGHT, this.m_headerNode.offsetTop + this.m_headerNode.offsetHeight);
		this.m_rootNode.style.top
			= Math.max(context.top, Math.min(this.m_rootNode.offsetTop, context.bottom - visibleHeight)) + "px";
	}

	/**************************
	 * @brief Sets the main title text and refreshes its combined title tooltip without changing the additional
	 * information.
	 *
	 * @param text Plain title text.
	 *
	 * @pre The view has a header wrapper.
	 *
	 * @test Yes.
	 */
	SetTitleText(text)
	{
		this.m_titleTextNode.textContent = text;
		this.m_titleTextNode.title = this.GetTitle();
	}

	/**************************
	 * @brief Sets the additional title information and refreshes the combined tooltip on the title text node
	 * without changing the main title text.
	 *
	 * @param info Plain additional information, or an empty string to clear it.
	 *
	 * @pre The view has a header wrapper.
	 *
	 * @test Yes.
	 */
	SetTitleInfo(info)
	{
		this.m_titleInfoNode.textContent = info;
		this.m_titleTextNode.title = this.GetTitle();
	}

	/**************************
	 * @pre The view has a header wrapper.
	 *
	 * @return Current title text and nonempty additional information joined by a colon and a space.
	 *
	 * @test Yes.
	 */
	GetTitle()
	{
		return [ this.m_titleTextNode.textContent, this.m_titleInfoNode.textContent ].filter(Boolean).join(": ");
	}

	Constructor(parameters)
	{
		console.error("Constructor is not implemented for view of type", this.m_viewType);
		return false;
	}

	Destructor()
	{
		this.m_positionResizeObserver?.disconnect();
		View.RemoveCreatedView(this);
		if (this.m_tableIdToTable) {
			this.m_tableIdToTable.forEach(table => { table.Destructor(); });
			this.m_tableIdToTable.clear();
		}
		if (this.m_grid) {
			this.m_grid.Destructor();
			delete this.m_grid;
		}
		if (this.m_rootNode) {
			if (this.m_rootNode.style.zIndex != "") {
				this.Show();
			}
			this.m_rootNode.remove();
		}
		WebSocketHandler.ClearViewRelatedEvents(this.m_uid);
	}

	/**************************
	 * @brief Sets or clears the loading animation on the view's content node.
	 *
	 * @param isLoading True to show the loading animation, false to hide it.
	 */
	SetLoading(isLoading) { this.m_contentNode.classList.toggle("loading", isLoading); }

	/**************************
	 * @brief Toggles the view's sticked state. Fixes an intrinsically sized view to its rendered width before hiding
	 * header controls, preserving its width when unsticked. Retains explicit pixel and percentage widths and reapplies
	 * position bounds when unsticked.
	 *
	 * @test Yes.
	 */
	ToggleStick()
	{
		if (!this.m_canBeSticked) {
			return;
		}

		const width = this.m_rootNode.style.width;
		if (!this.m_sticked && (!width || [ "auto", "min-content", "max-content", "fit-content" ].includes(width))) {
			this.m_rootNode.style.width = this.m_rootNode.getBoundingClientRect().width + "px";
		}

		this.m_sticked = !this.m_sticked;
		this.m_stickButton.classList.toggle("on");
		this.m_rootNode.classList.toggle("sticked");

		if (!this.m_sticked) {
			this.ClampPosition();
		}
	}

	Maximize()
	{
		if (this.m_sticked || !this.m_canBeMaximized) {
			return;
		}

		View.UpdateZIndex(this);

		if (this.m_maximized) {
			this.Normalize();
		}
		else {
			this.Show();
			this.m_savedViewParameters = {
				left : this.m_rootNode.style.left,
				top : this.m_rootNode.style.top,
				width : this.m_rootNode.style.width,
				height : this.m_rootNode.style.height
			};

			this.m_rootNode.style.position = "absolute";
			this.m_rootNode.style.top = "0";
			this.m_rootNode.style.left = "0";
			this.m_rootNode.style.width = "100%";
			this.m_rootNode.style.height = "100%";

			this.m_maximized = true;
			this.m_maximizeButton.classList.add("normalize");
			this.m_rootNode.classList.add("maximized");
		}
	}

	Normalize()
	{
		if (this.m_sticked || !this.m_maximized || !this.m_canBeMaximized) {
			return;
		}

		this.m_rootNode.style.top = this.m_savedViewParameters.top;
		this.m_rootNode.style.left = this.m_savedViewParameters.left;
		this.m_rootNode.style.width = this.m_savedViewParameters.width;
		this.m_rootNode.style.height = this.m_savedViewParameters.height;

		this.m_maximized = false;
		this.m_maximizeButton.classList.remove("normalize");
		this.m_rootNode.classList.remove("maximized");
	}

	Hide()
	{
		if (this.m_sticked || !this.m_canBeMaximized || this.m_rootNode.classList.contains("hidden")) {
			return;
		}

		dispatcher.AddHiddenView(this);
	}

	Show()
	{
		if (!this.m_canBeMaximized) {
			return;
		}

		View.UpdateZIndex(this);
		if (!this.m_rootNode.classList.contains("hidden")) {
			return;
		}

		dispatcher.RemoveHiddenView(this);
	}

	AddCallback(type, callback)
	{
		const callbackName = "Handle" + type.charAt(0).toUpperCase() + type.slice(1) + "Response";
		if (this.hasOwnProperty(callbackName)) {
			console.error(`Callback ${callbackName} already exists`);
			return;
		}
		this[callbackName] = callback;
	}

	/**************************
	 * @brief Enables dragging with the primary mouse button while the view is not sticked. Preserves the
	 * pointer offset and converts viewport movement into containing-block coordinates.
	 *
	 * @param node Handle node that starts dragging.
	 * @param element View node to move.
	 *
	 * @test Yes.
	 */
	MakeDraggable(node, element)
	{
		let isDragging = false;
		let offsetX, offsetY;

		node.addEventListener("mousedown", (e) => {
			if (e.button !== 0 /* primary mouse button */ || this.m_sticked) {
				return;
			}

			View.GetCreatedViews().values().forEach((view) => { view.m_contentNode.classList.add("changing"); });

			const rect = element.getBoundingClientRect();
			offsetX = e.clientX - rect.left;
			offsetY = e.clientY - rect.top;
			document.addEventListener("mousemove", onMouseMove);
			document.addEventListener("mouseup", onMouseUp);
		});

		const onMouseMove = (e) => {
			if (!isDragging) {
				isDragging = true;

				if (this.m_maximized) {
					this.m_rootNode.style.width = this.m_savedViewParameters.width;
					this.m_rootNode.style.height = this.m_savedViewParameters.height;
					// Keep a restored header under the pointer, independent of its containing block's position.
					offsetX = 30;
					offsetY = 10;
					this.m_maximized = false;
					this.m_maximizeButton.classList.remove("normalize");
					this.m_rootNode.classList.remove("maximized");
				}

				element.style.position = "absolute";
			}

			const context = this.GetPositionContext();
			let newLeft = Math.max(
				context.left, Math.min(e.clientX - offsetX - context.originLeft, context.right - element.offsetWidth));
			let newTop = Math.max(context.top, e.clientY - offsetY - context.originTop);

			if (this.m_canBeClinged) {
				const clingData = this.FindClosestToCling({
					top : newTop,
					right : newLeft + element.offsetWidth,
					bottom : newTop + element.offsetHeight,
					left : newLeft
				});

				if (clingData) {
					newLeft = Math.max(context.left, Math.min(clingData.left, context.right - element.offsetWidth));
					newTop = Math.max(context.top, clingData.top);
				}
			}

			element.style.left = `${newLeft}px`;
			element.style.top = `${newTop}px`;
		};

		const onMouseUp = () => {
			isDragging = false;
			document.removeEventListener("mousemove", onMouseMove);
			document.removeEventListener("mouseup", onMouseUp);
			View.GetCreatedViews().values().forEach((view) => { view.m_contentNode.classList.remove("changing"); });
		};
	}

	DisplayErrorMessage(message)
	{
		if (this.m_errorMessage === undefined) {
			this.m_errorMessage = document.createElement("div");
			this.m_errorMessage.classList.add("errorMessage");
			this.m_footerNode.appendChild(this.m_errorMessage);
		}

		this.m_footerNode.classList.add("visible");
		this.m_errorMessage.innerHTML = message || "Error is not specified";
	}

	HideErrorMessage()
	{
		if (this.m_errorMessage !== undefined && this.m_footerNode.classList.contains("visible")) {
			this.m_footerNode.classList.remove("visible");
		}
	}

	static ValidateInputs(tree)
	{
		let valid = true;
		tree.querySelectorAll("input:not([type='checkbox'])")
			.forEach(element => { valid &= Helper.ValidateLimits(element); });

		return valid;
	}

	static ParseInputs(tree, areNamed, metadata)
	{
		let inputs = tree.querySelectorAll("input:not([type='checkbox'])");
		let selects = tree.querySelectorAll("select");
		let checkboxes = tree.querySelectorAll("input[type='checkbox']");
		let data = {};

		if (areNamed) {
			[selects, inputs].forEach(
				nodeList => { nodeList.forEach(element => { data[element.name] = element.value; }); });

			checkboxes.forEach(element => { data[element.name] = element.checked ? "true" : "false"; });
		}
		else {
			if (metadata) {
				if (metadata.hasOwnProperty("mutable")) {
					[selects, inputs].forEach(nodeList => {
						nodeList.forEach(element => {
							const id = element.getAttribute("parameter-id");
							if (!metadata.mutable.hasOwnProperty(id)) {
								console.error("Can't find parameter with id", id);
								return;
							}

							const type = metadata.mutable[id].type;
							if (element.hasAttribute("select")) {
								data[id] = BigInt(element.getAttribute("select"));
							}
							else if (type.includes("Int") || type.includes("Uint")) {
								if (type.includes("Optional")) {
									if (element.value === "") {
										data[id] = null;
									}
									else {
										data[id] = BigInt(element.value);
									}
								}
								else {
									data[id] = BigInt(element.value);
								}
							}
							else if (type == "Float" || type == "Double") {
								if (type.includes("Optional")) {
									if (element.value === "") {
										data[id] = null;
									}
									else {
										data[id] = +element.value;
									}
								}
								else {
									data[id] = +element.value;
								}
							}
							else if (type == "Bool") {
								console.error("boolean element should be a checkbox");
							}
							else if (type == "Duration") {
								data[id] = BigInt(element.getAttribute("nanoseconds"));
							}
							else if (type == "Timer") {
								data[id] = BigInt(element.getAttribute("timestamp"));
							}
							else if (type == "String") {
								data[id] = element.value;
							}
							else {
								console.error("Unknown type of parameter in metadata", type);
							}
						});
					});
				}
				else {
					console.error("Mutable metadata is not specified");
				}
			}
			else {
				console.error("Metadata is not specified");
			}

			checkboxes.forEach(
				element => { data[element.getAttribute("parameter-id")] = element.checked ? true : false; });
		}

		return data;
	}

	static async SendRequest(options)
	{
		async function sendRequest()
		{
			let response = await fetch("api", options);
			if (response.ok) {
				let json = await response.text();
				json = Helper.JsonStringToObject(json);
				if ("status" in json && json["status"]) {
					let type = options.headers["Type"];
					return { "status" : false, "message" : "Unknown request type" };
				}
				else {
					return { "status" : false, "message" : "message" in json ? json["message"] : "Request failed" };
				}

				return json;
			}

			return { "status" : false, "message" : "Request failed" };
		}

		return await sendRequest();
	}

	static UpdateZIndex(view)
	{
		if (view.m_rootNode.style.zIndex == View.#privateFields.m_uidToView.size) {
			return;
		}

		const shiftedPosition = view.m_rootNode.style.zIndex;

		View.#privateFields.m_uidToView.forEach((item, uid) => {
			if (uid != view.m_uid) {
				if (item.m_rootNode.style.zIndex > shiftedPosition) {
					item.m_rootNode.style.zIndex = item.m_rootNode.style.zIndex - 1;
				}
			}
			else {
				item.m_rootNode.style.zIndex = View.#privateFields.m_uidToView.size;
			}
		});
	}
}

// Fixed-size views may cross viewport edges without triggering their ResizeObserver.
window.addEventListener("resize", () => {
	for (const view of View.GetCreatedViews().values()) {
		view.ClampPosition();
	}
});

// Handle clicks inside iframes to prevent losing focus.
window.addEventListener("message", function(event) {
	if (event.data && event.data.type === "iframeClick") {
		const view = View.GetCreatedViews().get(event.data.uid);
		if (!view) {
			console.error("View not found for UID:", event.data.uid);
			return;
		}

		view.m_rootNode.dispatchEvent(new Event("mousedown", { bubbles : true }));
	}
});

if (typeof module !== "undefined" && typeof module.exports !== "undefined") {
	Helper = require("../help/helper");
	WebSocketHandler = require("./webSocketHandler").WebSocketHandler;
	module.exports = View;
}