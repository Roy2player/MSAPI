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
 * @brief Abstraction for any view that can be created, represent interface unit or application.
 * It provides functionality for initializing views, handling metadata, managing parameters, and interacting with the
 * backend through HTTP requests. To use it:
 * - Extend the Class: Create a new class that inherits from View, provide a unique view type name as a first
 * argument to super.
 * - Override the Constructor: Implement the Constructor method to define custom initialization logic for your view.
 * - Register a Template: Use View.AddViewTemplate to define the HTML structure for the view.
 * - Add Callbacks: Use AddCallback to handle specific HTTP responses.
 * - Send Requests: Use View.SendRequest to communicate with the backend.
 *
 * Has three parts:
 * 1) Header which contains view tile and on view options:
 * @field m_title - title of view.
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
 * @brief m_view - root of template.
 * @brief m_parentView - parent of root of template.
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
 * @brief m_parentNode - parent node of view.
 * @brief m_tables - map of tables in view.
 * @brief m_grid - grid in view.
 * @brief m_created - flag of view creation.
 * @brief m_maximizeButton - maximize button element.
 *
 * @todo Think if web sockets can be used for communication.
 */
class View {
	static #privateFields = (() => {
		const m_viewTemplateToViewType = new Map();
		const m_parametersTemplateToViewType = new Map();
		const m_parametersToPort = new Map();
		const m_createdViews = new Map();
		const m_lastCreatedView = null;
		return {
			m_viewTemplateToViewType,
			m_parametersTemplateToViewType,
			m_parametersToPort,
			m_createdViews,
			m_lastCreatedView
		};
	})();

	static m_clingSensitive = 40;

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

		CheckX(0, left);
		CheckX(window.innerWidth, right);
		CheckY(0, top);

		for (const other of View.GetCreatedViews().values()) {
			if (other == this || !other.m_canBeClinged) {
				continue;
			}
			const oRect = other.m_parentView.getBoundingClientRect();
			const oTop = Math.round(oRect.top);
			const oRight = Math.round(oRect.right);
			const oBottom = Math.round(oRect.bottom);
			const oLeft = Math.round(oRect.left);

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
		if (View.#privateFields.m_viewTemplateToViewType.has(viewType)) {
			console.warn("View template for view type", viewType, "is already registered, overwriting");
		}
		View.#privateFields.m_viewTemplateToViewType.set(viewType, { template, templateElement : null });
	}

	static GetViewTemplate(viewType) { return View.#privateFields.m_viewTemplateToViewType.get(viewType); }

	/**************************
	 * @deprecated Use MetadataCollector.SetViewPortParameterToAppType instead.
	 */
	static SetViewPortParameterToAppType(appType, parameterId)
	{
		MetadataCollector.SetViewPortParameterToAppType(appType, parameterId);
	}

	/**************************
	 * @deprecated Use MetadataCollector.GetViewPortParameterToAppType instead.
	 */
	static GetViewPortParameterToAppType(appType) { return MetadataCollector.GetViewPortParameterToAppType(appType); }

	static AddParametersTemplate(viewType, templateName, template)
	{
		if (!View.#privateFields.m_parametersTemplateToViewType.has(viewType)) {
			View.#privateFields.m_parametersTemplateToViewType.set(viewType, new Map());
			View.#privateFields.m_parametersTemplateToViewType.get(viewType).set(templateName, template);
			return;
		}
		View.#privateFields.m_parametersTemplateToViewType.get(viewType).set(templateName, template);
	}

	static GetParametersTemplates(viewType) { return View.#privateFields.m_parametersTemplateToViewType.get(viewType); }

	static GetParametersTemplate(viewType, templateName)
	{
		if (View.#privateFields.m_parametersTemplateToViewType.has(viewType)) {
			if (View.#privateFields.m_parametersTemplateToViewType.get(viewType).has(templateName)) {
				return View.#privateFields.m_parametersTemplateToViewType.get(viewType).get(templateName);
			}
		}

		return undefined;
	}

	/**************************
	 * @deprecated Use MetadataCollector.GetAppMetadata instead.
	 */
	static GetMetadata(appType) { return MetadataCollector.GetAppMetadata(appType); }

	/**************************
	 * @deprecated Use MetadataCollector.AddAppMetadata instead.
	 */
	static AddMetadata(appType, metadata) { MetadataCollector.AddAppMetadata(appType, metadata); }

	/**************************
	 * @deprecated Use MetadataCollector.GetAllAppMetadata instead.
	 */
	static GetAllMetadata() { return MetadataCollector.GetAllAppMetadata(); }

	static HasParameters(port) { return View.#privateFields.m_parametersToPort.has(port); }

	static GetParameters(port) { return View.#privateFields.m_parametersToPort.get(port); }

	static SaveView(view)
	{
		if (!view) {
			console.error("Invalid view instance");
			return;
		}

		View.#privateFields.m_lastCreatedView = view;
		View.#privateFields.m_createdViews.set(view.m_uid, view);
	}

	static GetLastCreatedView() { return View.#privateFields.m_lastCreatedView; }

	static GetCreatedViews() { return View.#privateFields.m_createdViews; }

	static GetViewByPort(port)
	{
		for (let view of View.#privateFields.m_createdViews.values()) {
			if (view.m_port == port) {
				return view;
			}
		}

		return undefined;
	}

	static RemoveCreatedView(view) { View.#privateFields.m_createdViews.delete(view.m_uid); }

	static HandleResponse(type, response, parameters)
	{
		const callbackName = "Handle" + type.charAt(0).toUpperCase() + type.slice(1) + "Response";
		for (let view of View.#privateFields.m_createdViews.values()) {
			if (view.hasOwnProperty(callbackName)) {
				view[callbackName](response, parameters);
			}
		}
	}

	static #generalTemplate = `
	<div class="view">
		<div class="viewHeader">
			<div class="title"><span></span></div>
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

	constructor(viewType, parameters)
	{
		(async () => {
			try {
				this.m_viewType = viewType;
				this.m_title = viewType;
				this.m_parameters = parameters;

				if (parameters && parameters.parent) {
					this.m_parentNode = parameters.parent;
				}
				else {
					this.m_parentNode = document.querySelector("body > main > section.views");
				}

				if (!this.m_parentNode) {
					console.error("Can't find parent node for view.");
					return false;
				}

				if (!View.#privateFields.m_viewTemplateToViewType.has(viewType)) {
					console.error("Can't find view template for view.");
					return false;
				}

				if (!View.#generalTemplateElement) {
					const template = document.createElement("template");
					template.innerHTML = View.#generalTemplate;
					View.#generalTemplateElement = template;
				}

				let specificTemplate = View.#privateFields.m_viewTemplateToViewType.get(viewType);
				if (!specificTemplate.templateElement) {
					const template = document.createElement("template");
					template.innerHTML = specificTemplate.template;
					specificTemplate.templateElement = template;
				}

				this.m_parentView = View.#generalTemplateElement.content.cloneNode(true).firstElementChild;
				this.m_parentView.querySelector(".title > span").textContent = this.m_title;
				this.m_parentView.querySelector(".viewContent")
					.appendChild(specificTemplate.templateElement.content.cloneNode(true));
				this.m_view = this.m_parentView.querySelector(".viewContent").lastElementChild;
				this.m_footer = this.m_parentView.querySelector(".viewFooter");
				this.m_parentNode.appendChild(this.m_parentView);

				this.m_tables = new Map();

				this.MakeDraggable(this.m_parentView.querySelector(".viewHeader .title"), this.m_parentView);
				this.m_parentView.addEventListener("mousedown", () => { View.UpdateZIndex(this); });

				this.m_uid = Helper.GenerateUid();
				this.m_parentView.setAttribute("uid", this.m_uid);
				View.SaveView(this);
				this.m_parentView.style.zIndex = View.#privateFields.m_createdViews.size;

				this.m_parentView.style.left = dispatcher.m_view.getBoundingClientRect().right + "px";
				this.m_parentView.style.top = "0px";

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

				this.m_parentView.querySelectorAll('.handleResize').forEach(handle => {
					const direction = Array.from(handle.classList);
					handle.addEventListener('mousedown', e => { StartResizing(e, direction, this); });

					handle.addEventListener('dblclick', e => {
						if (this.m_sticked) {
							return;
						}

						if (direction.includes('right') || direction.includes('left')) {
							let w = Helper.GetFullDimensions(this.m_view).width;
							const style = window.getComputedStyle(this.m_parentView);
							w += parseFloat(style.marginLeft) || 0;
							w += parseFloat(style.marginRight) || 0;
							w += parseFloat(style.borderLeftWidth) || 0;
							w += parseFloat(style.borderRightWidth) || 0;
							w += parseFloat(style.paddingLeft) || 0;
							w += parseFloat(style.paddingRight) || 0;
							this.m_parentView.style.width = w + 'px';
						}

						if (direction.includes('bottom') || direction.includes('top')) {
							let h = Helper.GetFullDimensions(this.m_view).height;
							h += Helper.GetFullDimensions(this.m_parentView.querySelector('.viewHeader')).height
								+ Helper.GetFullDimensions(this.m_parentView.querySelector('.viewFooter')).height;
							const style = window.getComputedStyle(this.m_parentView);
							h += parseFloat(style.marginTop) || 0;
							h += parseFloat(style.marginBottom) || 0;
							h += parseFloat(style.borderTopWidth) || 0;
							h += parseFloat(style.borderBottomWidth) || 0;
							h += parseFloat(style.paddingTop) || 0;
							h += parseFloat(style.paddingBottom) || 0;
							this.m_parentView.style.height = h + 'px';
						}
					});
				});

				function StartResizing(element, direction, savedThis)
				{
					if (savedThis.m_sticked) {
						return;
					}

					savedThis.m_parentView.classList.add("resizing");
					View.GetCreatedViews().values().forEach((view) => { view.m_view.classList.add("changing"); });

					element.preventDefault();
					const startX = element.clientX;
					const startY = element.clientY;
					const startWidth = savedThis.m_parentView.offsetWidth;
					const startHeight = savedThis.m_parentView.offsetHeight;
					const startTop = savedThis.m_parentView.offsetTop;
					const startLeft = savedThis.m_parentView.offsetLeft;

					function OnMouseMove(ev)
					{
						let newWidth = startWidth;
						let newHeight = startHeight;
						let newTop = startTop;
						let newLeft = startLeft;
						let includesTop = direction.includes('top');
						let includesRight = direction.includes('right');
						let includesBottom = direction.includes('bottom');
						let includesLeft = direction.includes('left');

						if (includesRight) {
							newWidth += ev.clientX - startX;
							if (newWidth + newLeft > window.innerWidth) {
								newWidth = window.innerWidth - newLeft;
							}

							if (newWidth < 50) {
								newWidth = 50;
							}
						}
						if (includesLeft) {
							const dx = ev.clientX - startX;
							if (newLeft + dx < 0) {
								newWidth += newLeft;
								newLeft = 0;
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
							newHeight += ev.clientY - startY;

							if (newHeight < 50) {
								newHeight = 50;
							}
						}
						if (includesTop) {
							const dy = ev.clientY - startY;
							if (newTop + dy < 0) {
								newHeight += newTop;
								newTop = 0;
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
							savedThis.m_parentView.style.left = newLeft + "px";
							savedThis.m_parentView.style.top = newTop + "px";
							savedThis.m_parentView.style.width = newWidth + "px";
							savedThis.m_parentView.style.height = newHeight + "px";
						}
						else {
							const clingData = savedThis.FindClosestToCling({
								top : newTop,
								right : newLeft + newWidth,
								bottom : newTop + newHeight,
								left : newLeft
							});

							if (clingData) {
								if (includesRight) {
									savedThis.m_parentView.style.left = newLeft + "px";
									savedThis.m_parentView.style.width = newWidth + (clingData.left - newLeft) + "px";
								}
								else {
									savedThis.m_parentView.style.left = clingData.left + "px";
									savedThis.m_parentView.style.width = newWidth - (clingData.left - newLeft) + "px";
								}

								if (includesBottom) {
									savedThis.m_parentView.style.top = newTop + "px";
									savedThis.m_parentView.style.height = newHeight + (clingData.top - newTop) + "px";
								}
								else {
									savedThis.m_parentView.style.top = clingData.top + "px";
									savedThis.m_parentView.style.height = newHeight - (clingData.top - newTop) + "px";
								}
							}
							else {
								savedThis.m_parentView.style.left = newLeft + "px";
								savedThis.m_parentView.style.top = newTop + "px";
								savedThis.m_parentView.style.width = newWidth + "px";
								savedThis.m_parentView.style.height = newHeight + "px";
							}
						}
					}

					function OnMouseUp()
					{
						savedThis.m_parentView.classList.remove("resizing");
						View.GetCreatedViews().values().forEach(
							(view) => { view.m_view.classList.remove("changing"); });
						document.removeEventListener('mousemove', OnMouseMove);
						document.removeEventListener('mouseup', OnMouseUp);
					}

					document.addEventListener('mousemove', OnMouseMove);
					document.addEventListener('mouseup', OnMouseUp);
				}

				let closeButton = this.m_parentView.querySelector(".viewHeader .close");
				if (!closeButton) {
					console.error("Can't find close button for view.");
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

				let hideButton = this.m_parentView.querySelector(".viewHeader .hide");
				if (!hideButton) {
					console.error("Can't find hide button for view.");
					this.Destructor();
				}
				this.m_maximizeButton = this.m_parentView.querySelector(".viewHeader .maximize");
				if (!this.m_maximizeButton) {
					console.error("Can't find maximize button for view.");
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

				let stickButton = this.m_parentView.querySelector(".viewHeader .stick");
				if (!stickButton) {
					console.error("Can't find stick button for view.");
					this.Destructor();
				}
				if (this.m_canBeSticked) {
					this.m_sticked = false;
					stickButton.addEventListener("click", () => {
						this.m_sticked = !this.m_sticked;
						stickButton.classList.toggle("on");
						this.m_parentView.classList.toggle("sticked");
					});
				}
				else {
					stickButton.style.display = "none";
				}

				if (!(await this.Constructor(parameters))) {
					this.Destructor();
					console.error("Can't create view.");
					return;
				}

				if (parameters) {
					if (parameters.hasOwnProperty("postCreateFunction")) {
						if (typeof parameters.postCreateFunction != "function") {
							console.error("Invalid post create function type, function is expected",
								parameters.postCreateFunction);
							this.Destructor();
							return;
						}
						parameters.postCreateFunction({ view : this });
					}

					if (parameters.hasOwnProperty("positionUnder")) {
						this.m_parentView.style.left = parameters.positionUnder.getBoundingClientRect().left + "px";
						this.m_parentView.style.top = parameters.positionUnder.getBoundingClientRect().bottom + "px";
						this.m_parentView.style.position = "absolute";
					}
				}

				this.m_created = true;
			}
			catch (error) {
				console.error("Can't create view.", error);
				this.Destructor();
			}
		})();
	}

	async Constructor(parameters)
	{
		console.error("Constructor is not implemented for view of type", this.m_viewType);
		return false;
	}

	Destructor()
	{
		View.RemoveCreatedView(this);
		this.Show();
		if (this.m_tables) {
			this.m_tables.forEach(table => { table.Destructor(); });
		}
		this.m_tables.clear();
		if (this.m_grid) {
			this.m_grid.Destructor();
		}
		delete this.m_grid;
		this.m_parentView.remove();
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
				left : this.m_parentView.style.left,
				top : this.m_parentView.style.top,
				width : this.m_parentView.style.width,
				height : this.m_parentView.style.height
			};

			this.m_parentView.style.position = "absolute";
			this.m_parentView.style.top = "0";
			this.m_parentView.style.left = "0";
			this.m_parentView.style.width = "100%";
			this.m_parentView.style.height = "100%";

			this.m_maximized = true;
			this.m_maximizeButton.classList.add("normalize");
			this.m_parentView.classList.add("maximized");
		}
	}

	Normalize()
	{
		if (this.m_sticked || !this.m_maximized || !this.m_canBeMaximized) {
			return;
		}

		this.m_parentView.style.top = this.m_savedViewParameters.top;
		this.m_parentView.style.left = this.m_savedViewParameters.left;
		this.m_parentView.style.width = this.m_savedViewParameters.width;
		this.m_parentView.style.height = this.m_savedViewParameters.height;

		this.m_maximized = false;
		this.m_maximizeButton.classList.remove("normalize");
		this.m_parentView.classList.remove("maximized");
	}

	Hide()
	{
		if (this.m_sticked || !this.m_canBeMaximized || this.m_parentView.classList.contains("hidden")) {
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
		if (!this.m_parentView.classList.contains("hidden")) {
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

	MakeDraggable(node, element)
	{
		let isDragging = false;
		let startX, startY, offsetX, offsetY;

		node.addEventListener("mousedown", (e) => {
			if (this.m_sticked) {
				return;
			}

			View.GetCreatedViews().values().forEach((view) => { view.m_view.classList.add("changing"); });

			startX = e.clientX;
			startY = e.clientY;
			document.addEventListener("mousemove", onMouseMove);
			document.addEventListener("mouseup", onMouseUp);
		});

		const onMouseMove = (e) => {
			if (!isDragging) {
				isDragging = true;

				if (this.m_maximized) {
					this.m_parentView.style.left = e.clientX - 30 + "px";
					this.m_parentView.style.top = e.clientY - 10 + "px";
					this.m_parentView.style.width = this.m_savedViewParameters.width;
					this.m_parentView.style.height = this.m_savedViewParameters.height;

					this.m_maximized = false;
					this.m_maximizeButton.classList.remove("normalize");
				}

				offsetX = e.clientX - element.getBoundingClientRect().left;
				offsetY = e.clientY - element.getBoundingClientRect().top;
				element.style.position = "absolute";
				element.style.left
					= `${Math.max(0, Math.min(e.clientX - offsetX, window.innerWidth - element.offsetWidth))}px`;
				element.style.top = `${Math.max(0, e.clientY - offsetY)}px`;
				return;
			}

			if (!this.m_canBeClinged) {
				element.style.left
					= `${Math.max(0, Math.min(e.clientX - offsetX, window.innerWidth - element.offsetWidth))}px`;
				element.style.top = `${Math.max(0, e.clientY - offsetY)}px`;
				return;
			}

			let newLeft = Math.max(0, Math.min(e.clientX - offsetX, window.innerWidth - element.offsetWidth));
			let newTop = Math.max(0, e.clientY - offsetY);

			const clingData = this.FindClosestToCling({
				top : newTop,
				right : newLeft + element.offsetWidth,
				bottom : newTop + element.offsetHeight,
				left : newLeft
			});

			if (clingData) {
				element.style.left = Math.max(0, clingData.left) + "px";
				element.style.top = Math.max(0, clingData.top) + "px";
			}
			else {
				element.style.left = `${newLeft}px`;
				element.style.top = `${newTop}px`;
			}
		};

		const onMouseUp = () => {
			isDragging = false;
			document.removeEventListener("mousemove", onMouseMove);
			document.removeEventListener("mouseup", onMouseUp);
			View.GetCreatedViews().values().forEach((view) => { view.m_view.classList.remove("changing"); });
		};
	}

	DisplayErrorMessage(message)
	{
		if (this.m_errorMessage === undefined) {
			this.m_errorMessage = document.createElement("div");
			this.m_errorMessage.classList.add("errorMessage");
			this.m_footer.appendChild(this.m_errorMessage);
		}

		this.m_footer.classList.add("visible");
		this.m_errorMessage.innerHTML = message || "Error is not specified";
	}

	HideErrorMessage()
	{
		if (this.m_errorMessage !== undefined && this.m_errorMessage.classList.contains("visible")) {
			this.m_footer.classList.remove("visible");
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
					if (type === "createApp") {
						void new ModifyApp({
							appType : options.headers["AppType"],
							port : json.port,
						});

						View.SendRequest({
							method : "GET",
							mode : "cors",
							headers : { "Accept" : "application/json", "Type" : "getCreatedApps" }
						});
					}
					else if (type == "getCreatedApps") {
						View.HandleResponse(type, json);
					}
					else if (type == "getInstalledApps") {
						View.HandleResponse(type, json);
					}
					else if (type == "getMetadata") {
						if ("metadata" in json) {
							MetadataCollector.AddAppMetadata(options.headers["AppType"], json["metadata"]);
							if (json["metadata"].hasOwnProperty("mutable")) {
								for (let [parameterId, parameter] of Object.entries(json["metadata"].mutable)) {
									MetadataCollector.AddMetadata(parameterId, parameter, false);
								}
							}
							if (json["metadata"].hasOwnProperty("const")) {
								for (let [parameterId, parameter] of Object.entries(json["metadata"].const)) {
									MetadataCollector.AddMetadata(parameterId, parameter, true);
								}
							}
						}
						else {
							return { "status" : false, "message" : "Metadata are not specified in response" };
						}
					}
					else if (type == "getParameters") {
						if ("parameters" in json) {
							View.#privateFields.m_parametersToPort.set(options.headers["Port"], json["parameters"]);
							View.HandleResponse(type, json, { "port" : options.headers["Port"] });
						}
						else {
							return { "status" : false, "message" : "Parameters are not specified in response" };
						}
					}
					else if (type == "run" || type == "pause" || type == "delete") {
						View.HandleResponse(type, json, { "port" : options.headers["Port"] });
					}
					else if (type == "modify") {
					}
					else {
						return { "status" : false, "message" : "Unknown request type" };
					}
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
		if (view.m_parentView.style.zIndex == View.#privateFields.m_createdViews.size) {
			return;
		}

		const shiftedPosition = view.m_parentView.style.zIndex;

		View.#privateFields.m_createdViews.forEach((item, uid) => {
			if (uid != view.m_uid) {
				if (item.m_parentView.style.zIndex > shiftedPosition) {
					item.m_parentView.style.zIndex = item.m_parentView.style.zIndex - 1;
				}
			}
			else {
				item.m_parentView.style.zIndex = View.#privateFields.m_createdViews.size;
			}
		});
	}
}

if (typeof module !== 'undefined' && typeof module.exports !== 'undefined') {
	Helper = require("./helper");
	module.exports = View;
}