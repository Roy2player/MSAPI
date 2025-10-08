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
 * @brief Abstraction for any view that can be created. Can represent interface unit or application.
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
 * Provides functionality:
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
 * @todo New app type should be added fully independently from Application class.
 */
class Application {
	static #privateFields = (() => {
		const m_viewTemplateToViewType = new Map();
		const m_parametersTemplateToAppType = new Map();
		const m_metadataToAppType = new Map();
		const m_viewPortParameterToAppType = new Map();
		const m_parametersToPort = new Map();
		const m_createdApplications = new Map();
		const m_lastCreatedApplication = null;
		return {
			m_viewTemplateToViewType,
			m_parametersTemplateToAppType,
			m_metadataToAppType,
			m_viewPortParameterToAppType,
			m_parametersToPort,
			m_createdApplications,
			m_lastCreatedApplication
		};
	})();

	static m_clingSensitive = 40;

	FindClosestToCling({ top, right, bottom, left })
	{
		top = Math.round(top);
		right = Math.round(right);
		bottom = Math.round(bottom);
		left = Math.round(left);
		let mins = { x : Application.m_clingSensitive + 1, y : Application.m_clingSensitive + 1 };
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

		for (const other of Application.GetCreatedApplications().values()) {
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

		return mins.x <= Application.m_clingSensitive || mins.y <= Application.m_clingSensitive ? clingData : undefined;
	}

	static AddViewTemplate(viewType, template)
	{
		if (Application.#privateFields.m_viewTemplateToViewType.has(viewType)) {
			Application.#privateFields.m_viewTemplateToViewType.set(viewType, { template, templateElement : null });
			return;
		}
		Application.#privateFields.m_viewTemplateToViewType.set(viewType, { template, templateElement : null });
	}

	static GetViewTemplate(viewType) { return Application.#privateFields.m_viewTemplateToViewType.get(viewType); }

	static AddParametersTemplate(appType, templateName, template)
	{
		if (!Application.#privateFields.m_parametersTemplateToAppType.has(appType)) {
			Application.#privateFields.m_parametersTemplateToAppType.set(appType, new Map());
			Application.#privateFields.m_parametersTemplateToAppType.get(appType).set(templateName, template);
			return;
		}
		Application.#privateFields.m_parametersTemplateToAppType.get(appType).set(templateName, template);
	}

	static GetParametersTemplates(appType)
	{
		if (Application.#privateFields.m_parametersTemplateToAppType.has(appType)) {
			return Application.#privateFields.m_parametersTemplateToAppType.get(appType);
		}

		return null;
	}

	static GetParametersTemplate(appType, templateName)
	{
		if (Application.#privateFields.m_parametersTemplateToAppType.has(appType)) {
			if (Application.#privateFields.m_parametersTemplateToAppType.get(appType).has(templateName)) {
				return Application.#privateFields.m_parametersTemplateToAppType.get(appType).get(templateName);
			}
		}

		return null;
	}

	static GetMetadata(appType)
	{
		if (Application.#privateFields.m_metadataToAppType.has(appType)) {
			return Application.#privateFields.m_metadataToAppType.get(appType);
		}

		return null;
	}

	static AddMetadata(appType, metadata) { Application.#privateFields.m_metadataToAppType.set(appType, metadata); }

	static GetAllMetadata() { return Array.from(Application.#privateFields.m_metadataToAppType.values()); }

	static HasParameters(port) { return Application.#privateFields.m_parametersToPort.has(port); }

	static GetParameters(port)
	{
		if (Application.#privateFields.m_parametersToPort.has(port)) {
			return Application.#privateFields.m_parametersToPort.get(port);
		}

		return null;
	}

	static SaveApplication(app)
	{
		if (!app) {
			console.error("Invalid application instance");
			return;
		}

		Application.#privateFields.m_lastCreatedApplication = app;
		Application.#privateFields.m_createdApplications.set(app.m_uid, app);
	}

	static GetLastCreatedApplication() { return Application.#privateFields.m_lastCreatedApplication; }

	static GetCreatedApplications() { return Application.#privateFields.m_createdApplications; }

	static GetApplicationByPort(port)
	{
		for (let app of Application.#privateFields.m_createdApplications.values()) {
			if (app.m_port == port) {
				return app;
			}
		}

		return null;
	}

	static RemoveCreatedApplication(app) { Application.#privateFields.m_createdApplications.delete(app.m_uid); }

	static HandleResponse(type, response, parameters)
	{
		const callbackName = "Handle" + type.charAt(0).toUpperCase() + type.slice(1) + "Response";
		for (let app of Application.#privateFields.m_createdApplications.values()) {
			if (app.hasOwnProperty(callbackName)) {
				app[callbackName](response, parameters);
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

				if (parameters != undefined && parameters.parent != undefined) {
					this.m_parentNode = parameters.parent;
				}
				else {
					this.m_parentNode = document.querySelector("body > main > section.views");
				}

				if (this.m_parentNode === null) {
					console.error("Can't find parent node for application.");
					return false;
				}

				if (!Application.#privateFields.m_viewTemplateToViewType.has(viewType)) {
					console.error("Can't find view template for application.");
					return false;
				}

				if (Application.#generalTemplateElement === null) {
					const template = document.createElement("template");
					template.innerHTML = Application.#generalTemplate;
					Application.#generalTemplateElement = template;
				}

				let specificTemplate = Application.#privateFields.m_viewTemplateToViewType.get(viewType);
				if (specificTemplate.templateElement === null) {
					const template = document.createElement("template");
					template.innerHTML = specificTemplate.template;
					specificTemplate.templateElement = template;
				}

				this.m_parentView = Application.#generalTemplateElement.content.cloneNode(true).firstElementChild;
				this.m_parentView.querySelector(".title > span").textContent = this.m_title;
				this.m_parentView.querySelector(".viewContent")
					.appendChild(specificTemplate.templateElement.content.cloneNode(true));
				this.m_view = this.m_parentView.querySelector(".viewContent").lastElementChild;
				this.m_footer = this.m_parentView.querySelector(".viewFooter");
				this.m_parentNode.appendChild(this.m_parentView);

				this.m_tables = new Map();

				this.MakeDraggable(this.m_parentView.querySelector(".viewHeader .title"), this.m_parentView);
				this.m_parentView.addEventListener("mousedown", () => { Application.UpdateZIndex(this); });

				this.m_uid = Helper.GenerateUid();
				this.m_parentView.setAttribute("uid", this.m_uid);
				Application.SaveApplication(this);
				this.m_parentView.style.zIndex = Application.#privateFields.m_createdApplications.size;

				if (dispatcher !== undefined) {
					this.m_parentView.style.left = dispatcher.m_view.getBoundingClientRect().right + "px";
				}
				this.m_parentView.style.top = "0px";

				if (typeof parameters != "undefined") {
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
					Application.GetCreatedApplications().values().forEach(
						(app) => { app.m_view.classList.add("changing"); });

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
						Application.GetCreatedApplications().values().forEach(
							(app) => { app.m_view.classList.remove("changing"); });
						document.removeEventListener('mousemove', OnMouseMove);
						document.removeEventListener('mouseup', OnMouseUp);
					}

					document.addEventListener('mousemove', OnMouseMove);
					document.addEventListener('mouseup', OnMouseUp);
				}

				let closeButton = this.m_parentView.querySelector(".viewHeader .close");
				if (!closeButton) {
					console.error("Can't find close button for application.");
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
					console.error("Can't find hide button for application.");
					this.Destructor();
				}
				this.m_maximizeButton = this.m_parentView.querySelector(".viewHeader .maximize");
				if (!this.m_maximizeButton) {
					console.error("Can't find maximize button for application.");
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
					console.error("Can't find stick button for application.");
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

				if (!(await this.Constructor(viewType, parameters))) {
					this.Destructor();
					console.error("Can't create application.");
					return;
				}

				if (typeof parameters != "undefined") {
					if (parameters.hasOwnProperty("postCreateFunction")) {
						parameters.postCreateFunction({ view : this })
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
				console.error("Can't create application.", error);
				this.Destructor();
			}
		})();
	}

	async Constructor(viewType, parameters)
	{
		if (viewType == "AppView") {
			this.m_title += ": " + parameters.appType + " (port: " + parameters.port + ")";
			this.m_parentView.querySelector(".title > span").textContent = this.m_title;

			const parametersToPort = Application.#privateFields.m_parametersToPort.get(parameters.port);

			// Create an iframe to display the app at the given URL and port
			const url = parameters.url || `http://127.0.0.1:${parametersToPort[parameters.viewPortParameter]}/`;
			const iframe = document.createElement("iframe");
			iframe.src = url;
			iframe.style.width = "100%";
			iframe.style.height = "100%";
			iframe.style.border = "none";

			this.m_view.appendChild(iframe);

			iframe.addEventListener("load", () => {
				try {
					const urlObj = new URL(iframe.src, window.location.origin);
					iframe.contentWindow.postMessage(
						{ type : "init", uid : this.m_uid, origin : window.location.origin }, urlObj.origin);
				}
				catch (e) {
					console.error("Failed to postMessage to iframe:", e);
				}
			});

			return true;
		}

		if (viewType === "TableView") {
			this.m_title += ": " + parameters.appTitle;
			this.m_parentView.querySelector(".title > span").textContent = this.m_title;

			let table = new Table(
				{ parent : this.m_view, id : parameters.tableId, metadata : parameters.metadata, isMutable : false });
			this.m_tables.set(+parameters.tableId, table);

			return true;
		}

		if (viewType == "GridSettingsView") {
			this.m_parentView.querySelector(".title > span").textContent = parameters.appTitle;
			this.m_eventTarget = parameters.eventTarget;
			this.m_parameterId = parameters.parameterId;
			return true;
		}

		if (viewType === "SelectView") {
			this.m_title = parameters.appTitle;
			this.m_parentView.querySelector(".title > span").textContent = this.m_title;
			this.m_eventTarget = parameters.eventTarget;

			let caseSensitiveElement = this.m_view.querySelector(".search > .caseSensitive");
			let isCaseSensitive = false;
			caseSensitiveElement.classList.add("disabled");

			let search = () => {
				let items = this.m_view.querySelectorAll(".options>*");
				items.forEach((item) => {
					if ((isCaseSensitive && item.innerHTML.includes(searchElement.value))
						|| (!isCaseSensitive
							&& item.innerHTML.toLowerCase().includes(searchElement.value.toLowerCase()))) {

						item.style.display = "";
					}
					else {
						item.style.display = "none";
					}
				});
			};

			caseSensitiveElement.addEventListener("click", () => {
				isCaseSensitive = !isCaseSensitive;
				if (isCaseSensitive) {
					caseSensitiveElement.classList.remove("disabled");
				}
				else {
					caseSensitiveElement.classList.add("disabled");
				}

				search();
			});

			let searchElement = this.m_view.querySelector(".search > input");
			searchElement.addEventListener("input", search);

			return true;
		}

		if (viewType === "InstalledApps") {
			this.m_grid = new Grid({
				parent : this.m_view,
				indexColumnId : 5,
				columns : [ 1, 5 ],
				postAddRowFunction : (rowObject) => {
					let createCell = rowObject.row.querySelector(".cell[parameter-id='1']");
					if (createCell !== null && rowObject.values.hasOwnProperty(5)) {
						createCell.addEventListener(
							"click", () => { new Application("NewApp", { appType : rowObject.values[5] }); });
						createCell.classList.add("action", "create");
					}
				}
			});

			this.AddCallback("getInstalledApps", (response) => {
				if ("apps" in response) {
					response.apps.forEach(app => { this.m_grid.AddOrUpdateRow({ 5 : app.type }); });

					response.apps.forEach(app => {
						if (!Application.#privateFields.m_metadataToAppType.has(app.type)) {
							Application.SendRequest({
								method : "GET",
								mode : "cors",
								headers :
									{ "Accept" : "application/json", "Type" : "getMetadata", "AppType" : app.type }
							});
						}

						if (!Application.#privateFields.m_viewPortParameterToAppType.has(app.type)) {
							Application.#privateFields.m_viewPortParameterToAppType.set(
								app.type, app.viewPortParameter);
						}
					});
				}
			});

			void Application.SendRequest({
				method : "GET",
				mode : "cors",
				headers : { "Accept" : "application/json", "Type" : "getInstalledApps" }
			});

			return true;
		}

		if (viewType === "NewApp") {
			this.m_appType = parameters.appType;
			this.m_title += ": " + this.m_appType;
			this.m_parentView.querySelector(".title > span").textContent = this.m_title;

			this.m_view.querySelector(".button").addEventListener("click", async () => {
				this.HideErrorMessage();
				if (!Application.ValidateInputs(this.m_view)) {
					return;
				}

				this.m_parentView.classList.add("loading");
				let headers = Application.ParseInputs(this.m_view, true);
				headers["Accept"] = "application/json";
				headers["Type"] = "createApp";
				headers["AppType"] = this.m_appType;
				let result = await Application.SendRequest({ method : "GET", mode : "cors", headers });
				if (result.status) {
					this.Destructor();
				}
				else {
					this.m_parentView.classList.remove("loading");
					this.DisplayErrorMessage(result.message);
				}
			});

			return true;
		}

		if (viewType === "ModifyApp") {
			let existedApp = Application.GetApplicationByPort(parameters.port);
			if (existedApp) {
				existedApp.Show();
				return false;
			}

			this.m_appType = parameters.appType;
			this.m_viewPortParameter = parameters.viewPortParameter;

			if (!Application.#privateFields.m_metadataToAppType.has(this.m_appType)) {
				let result = await Application.SendRequest({
					method : "GET",
					mode : "cors",
					headers : { "Accept" : "application/json", "Type" : "getMetadata", "AppType" : this.m_appType }
				});
				if (!("status" in result) || !result.status) {
					console.error("Can't get metadata for application", this.m_appType);
					return false;
				}
			}

			this.m_port = parameters.port;
			let result = await Application.SendRequest({
				method : "GET",
				mode : "cors",
				headers : { "Accept" : "application/json", "Type" : "getParameters", "Port" : this.m_port }
			});
			if (!("status" in result) || !result.status) {
				console.error("Can't get parameters for application", this.m_appType);
				return false;
			}

			if (!Application.#privateFields.m_metadataToAppType.has(this.m_appType)) {
				console.error("Can't find metadata for application", this.m_appType);
				return false;
			}

			const metadata = Application.#privateFields.m_metadataToAppType.get(this.m_appType);
			let inputs = this.m_view.querySelector(".inputs");

			function parseParameter(parameterId, parameter, parent, mutable, saveThis)
			{
				if (parameter.type == "TableData") {
					let tables = parent.querySelector(".tables");
					if (!tables) {
						tables = document.createElement("div");
						tables.classList.add("tables");
						parent.appendChild(tables);
					}

					saveThis.m_tables.set(+parameterId,
						new Table({ parent : tables, id : parameterId, metadata : parameter, isMutable : mutable }));
					return;
				}

				function getGeneral()
				{
					let general = parent.querySelector(".general");
					if (!general) {
						general = document.createElement("div");
						general.classList.add("general", "items", "horizontal");
						let table = parent.querySelector(".tables");
						if (table) {
							parent.insertBefore(general, table);
						}
						else {
							parent.appendChild(general);
						}
					}
					return general;
				}

				let divElement = document.createElement("div");
				divElement.classList.add("item");
				let inputElement = document.createElement("input");
				inputElement.setAttribute("parameter-id", parameterId);

				if (parameter.type == "Bool") {
					inputElement.setAttribute("type", "checkbox");
					let labelElement = document.createElement("label");
					let spanElement = document.createElement("span");
					spanElement.innerHTML = parameter.name + " (" + parameterId + ") " + parameter.type;
					labelElement.appendChild(inputElement);
					divElement.appendChild(spanElement);
					divElement.appendChild(labelElement);
					getGeneral().appendChild(divElement);

					if (!mutable) {
						inputElement.setAttribute("disabled", true);
					}

					return;
				}

				if (parameter.type == "Timer") {
					Timer.Apply(inputElement);
				}
				else if (parameter.type == "String") {
					inputElement.setAttribute("type", "text");
				}
				else if (parameter.type == "Duration") {
					Duration.Apply(inputElement, parameter.durationType);
				}
				else if (MetadataCollector.IsSelect(parameterId)) {
					Select.Apply({ input : inputElement });
				}
				else {
					inputElement.setAttribute("type", "number");
				}

				let spanElement = document.createElement("span");
				spanElement.innerHTML = parameter.name + " (" + parameterId + ") "
					+ parameter.type + (parameter.type == "Duration" ? ", " + parameter.durationType : "");
				divElement.appendChild(spanElement);

				if (mutable) {
					if (parameter.hasOwnProperty("min") && parameter.min != "null") {
						inputElement.setAttribute("min", parameter.min);
					}
					if (parameter.hasOwnProperty("max") && parameter.max != "null") {
						inputElement.setAttribute("max", parameter.max);
					}
					if (parameter.hasOwnProperty("canBeEmpty")) {
						inputElement.setAttribute("canBeEmpty", parameter.canBeEmpty);
					}
					else {
						inputElement.setAttribute("canBeEmpty", false);
					}

					if (parameter.type != "Duration") {
						inputElement.addEventListener("input", () => { Helper.ValidateLimits(inputElement); });
					}
				}
				else {
					inputElement.setAttribute("readonly", true);
					inputElement.setAttribute("disabled", true);
				}

				divElement.appendChild(inputElement);
				getGeneral().appendChild(divElement);
			}

			function getContainerContent(isMutable)
			{
				let container = isMutable ? inputs.querySelector(".mutable") : inputs.querySelector(".const");
				if (!container) {
					container = document.createElement("div");
					container.classList.add(isMutable ? "mutable" : "const", "collection");
					let containerHeader = document.createElement("div");
					containerHeader.classList.add("header");
					let containerHeaderSpan = document.createElement("span");
					containerHeaderSpan.innerHTML = isMutable ? "Mutable" : "Const";
					containerHeader.appendChild(containerHeaderSpan);
					container.appendChild(containerHeader);
					let containerContent = document.createElement("div");
					containerContent.classList.add("content");
					container.appendChild(containerContent);
					inputs.appendChild(container);
				}
				return container.querySelector("div.content");
			}

			if (metadata.hasOwnProperty("mutable")) {
				for (let [parameterId, parameter] of Object.entries(metadata.mutable)) {
					parseParameter(parameterId, parameter, getContainerContent(true), true, this);
				}
			}
			if (metadata.hasOwnProperty("const")) {
				for (let [parameterId, parameter] of Object.entries(metadata.const)) {
					parseParameter(parameterId, parameter, getContainerContent(false), false, this);
				}
			}

			const containerWithInputs = getContainerContent(true).querySelector(".general");

			function displayParameters(saveThis)
			{
				for (let [key, value] of Object.entries(
						 Application.#privateFields.m_parametersToPort.get(parameters.port))) {

					let parameter = false;
					if (metadata.hasOwnProperty("mutable")) {
						parameter = metadata.mutable.hasOwnProperty(key) ? metadata.mutable[key] : false;
					}
					if (!parameter && metadata.hasOwnProperty("const")) {
						parameter = metadata.const.hasOwnProperty(key) ? metadata.const[key] : false;
					}

					if (!parameter) {
						console.error("Parameter is not found: ", key);
						continue;
					}

					let input = inputs.querySelector("[parameter-id=\"" + key + "\"]");
					if (!input) {
						console.error(`Input not found: ${key}`);
						continue;
					}

					if (parameter.type == "TableData") {
						if ("Rows" in value) {
							let table = saveThis.m_tables.get(+key);
							if (!table) {
								console.error("Table id not found in app:", key);
								continue;
							}
							table.Clear();
							for (let row of value.Rows) {
								table.AddRow(row);
							}
							table.Save();
						}
					}
					else if (parameter.type == "Bool") {
						input.checked = value;
					}
					else if (parameter.type == "Duration") {
						Duration.SetValue(input, value);
					}
					else if (parameter.type == "Timer") {
						Timer.SetValue(input, BigInt(value));
					}
					else if (input.hasAttribute("select")) {
						Select.SetValue(input, value);
					}
					else if (Helper.IsFloat(value)) {
						input.value = Helper.FloatToString(value);
					}
					else {
						input.value = value;
					}
				}

				Application.ValidateInputs(containerWithInputs);
			}

			displayParameters(this);

			const parametersToPort = Application.#privateFields.m_parametersToPort.get(this.m_port);
			parametersToPort[5] = this.m_appType;
			parametersToPort[10] = this.m_viewPortParameter;
			if (parametersToPort && parametersToPort.hasOwnProperty(2000001)) {
				this.m_title += ": " + parametersToPort[2000001];
			}
			else {
				this.m_title += ": " + this.m_appType;
			}
			this.m_parentView.querySelector(".title > span").textContent = this.m_title;

			this.AddCallback("getParameters", (response, extraParameters) => {
				if ("parameters" in response && "port" in extraParameters && extraParameters.port == this.m_port) {
					displayParameters(this);
				}
			});

			this.m_view.querySelector(".button").addEventListener("click", async () => {
				this.HideErrorMessage();
				if (!Application.ValidateInputs(containerWithInputs)) {
					return;
				}

				for (let table of this.m_tables.values()) {
					if (table.IsChanged()) {
						return;
					}
				}

				this.m_parentView.classList.add("loading");
				let headers = { "Accept" : "application/json", "Type" : "modify", "Port" : this.m_port };
				let newParameters = Application.ParseInputs(
					containerWithInputs, false, Application.#privateFields.m_metadataToAppType.get(this.m_appType));

				for (const table of this.m_tables.values()) {
					newParameters[table.m_id] = table.GetData();
				}

				headers["Parameters"] = Helper.ParametersToJson(newParameters);

				let result = await Application.SendRequest({ method : "GET", mode : "cors", headers });
				if ("status" in result) {
					this.m_parentView.classList.remove("loading");
					if (!result.status) {
						this.DisplayErrorMessage(result.message);
					}

					Application.SendRequest({
						method : "GET",
						mode : "cors",
						headers : { "Accept" : "application/json", "Type" : "getParameters", "Port" : this.m_port }
					});
				}
				else {
					this.DisplayErrorMessage("Error: No status in response.");
				}
			});

			return true;
		}

		if (viewType === "CreatedApps") {
			this.m_grid = new Grid({
				parent : this.m_view,
				indexColumnId : 1000009,
				columns : [ 10, 2, 3, 4, 2000001, 5, 1000008, 1000009 ],
				indexColumn : "Port",
				postAddRowFunction : (rowObject) => {
					let changeStateCell = rowObject.row.querySelector(".cell[parameter-id='2']");
					if (changeStateCell !== null) {
						changeStateCell.addEventListener("click", () => {
							if (changeStateCell.classList.contains("loading")) {
								return;
							}

							const port = rowObject.values[1000009];
							if (port === undefined) {
								console.error("Can't find server port in row values.");
								return;
							}

							const state = rowObject.values[2000002];
							if (state === undefined) {
								console.error("Can't find application state in row values.");
								return;
							}

							Application.SendRequest({
								method : "GET",
								mode : "cors",
								headers : {
									"Accept" : "application/json",
									"Type" : state == 1 ? "run" : "pause",
									"Port" : rowObject.values[1000009]
								}
							});
							changeStateCell.classList.add("loading");
						});

						changeStateCell.classList.add("action");
					}

					let modifyCell = rowObject.row.querySelector(".cell[parameter-id='3']");
					if (modifyCell !== null) {
						modifyCell.addEventListener("click", () => {
							const port = rowObject.values[1000009];
							if (port === undefined) {
								console.error("Can't find server port in row values.");
								return;
							}

							let appType = rowObject.values[5];
							if (appType === undefined) {
								console.error("Can't find application type in row values.");
								return;
							}

							new Application("ModifyApp", { appType, port });
						});

						modifyCell.classList.add("action", "modify");
					}

					let deleteCell = rowObject.row.querySelector(".cell[parameter-id='4']");
					if (deleteCell !== null) {
						deleteCell.addEventListener("click", () => {
							const port = rowObject.values[1000009];
							if (port === undefined) {
								console.error("Can't find server port in row values.");
								return;
							}

							if (!confirm(`Are you sure you want to delete application ${
									rowObject.values[2000001]} type ${rowObject.values[5]} on port ${port}?`)) {

								return;
							}
							Application.SendRequest({
								method : "GET",
								mode : "cors",
								headers : { "Accept" : "application/json", "Type" : "delete", "Port" : port }
							});
						});

						deleteCell.classList.add("action", "delete");
					}

					let viewCell = rowObject.row.querySelector(".cell[parameter-id='10']");
					if (viewCell !== null) {
						let appType = rowObject.values[5];
						if (appType === undefined) {
							console.error("Can't find application type in row values.");
							return;
						}
						const viewPortParameter = Application.#privateFields.m_viewPortParameterToAppType.get(appType);
						if (viewPortParameter != undefined) {
							viewCell.addEventListener("click", () => {
								const port = rowObject.values[1000009];
								if (port === undefined) {
									console.error("Can't find server port in row values.");
									return;
								}

								new Application("AppView", { appType, port, viewPortParameter })
							});

							viewCell.classList.add("action", "view");
						}
					}
				},
				postUpdateRowFunction : (rowObject, updatedValues) => {
					if (updatedValues.hasOwnProperty(2000002)) {
						let changeStateCell = rowObject.row.querySelector(".cell[parameter-id='2']");
						if (changeStateCell !== null) {
							if (updatedValues[2000002] == 1) {
								changeStateCell.classList.add("run");
								changeStateCell.classList.remove("pause");
							}
							else {
								changeStateCell.classList.add("pause");
								changeStateCell.classList.remove("run");
							}
						}
					}
				}
			});
			this.AddCallback("getCreatedApps", (response) => {
				if ("apps" in response) {
					for (let index of response.apps.map(app => app.port)) {
						if (!this.m_grid.m_rowByIndexValue.has(index)) {
							this.m_grid.RemoveRow({ indexValue : index });
						}
					}

					response.apps.forEach(app => {
						this.m_grid.AddOrUpdateRow({
							1000009 : app.port,
							5 : app.type,
							10 : app.viewPortParameter,
						});
						Application.SendRequest({
							method : "GET",
							mode : "cors",
							headers : { "Accept" : "application/json", "Type" : "getParameters", "Port" : app.port }
						});
					});
				}
			});

			this.AddCallback("run", (response, extraParameters) => {
				if (response.status && "result" in response && response.result) {
					(async () => {
						await Application.SendRequest({
							method : "GET",
							mode : "cors",
							headers : {
								"Accept" : "application/json",
								"Type" : "getParameters",
								"Port" : extraParameters.port
							}
						});
					})();
				}
				let rowObject = this.m_grid.m_rowByIndexValue.get(extraParameters.port);
				if (rowObject !== undefined) {
					let actionCell = rowObject.row.querySelector(".cell[parameter-id='2']");
					if (actionCell !== null) {
						actionCell.classList.remove("loading");
					}
				}
				else {
					console.error("Can't find row for port", extraParameters.port);
				}
			});

			this.AddCallback("pause", (response, extraParameters) => {
				if (response.status && "result" in response && response.result) {
					(async () => {
						await Application.SendRequest({
							method : "GET",
							mode : "cors",
							headers : {
								"Accept" : "application/json",
								"Type" : "getParameters",
								"Port" : extraParameters.port
							}
						});
					})();
				}
				let rowObject = this.m_grid.m_rowByIndexValue.get(extraParameters.port);
				if (rowObject !== undefined) {
					let actionCell = rowObject.row.querySelector(".cell[parameter-id='2']");
					if (actionCell !== null) {
						actionCell.classList.remove("loading");
					}
				}
				else {
					console.error("Can't find row for port", extraParameters.port);
				}
			});

			this.AddCallback("delete", (response, extraParameters) => {
				this.m_grid.RemoveRow({ indexValue : extraParameters.port });
				for (let app of Application.GetCreatedApplications().values()) {
					if (app.m_port == extraParameters.port) {
						app.Destructor();
						return true;
					}
				}

				return false;
			});

			this.AddCallback("getParameters", (response, extraParameters) => {
				if ("parameters" in response && "port" in extraParameters) {
					this.m_grid.AddOrUpdateRow(response.parameters);
				}
			});

			Application.SendRequest({
				method : "GET",
				mode : "cors",
				headers : { "Accept" : "application/json", "Type" : "getCreatedApps" }
			});

			return true;
		}
	}

	Destructor()
	{
		Application.RemoveCreatedApplication(this);
		this.Show();
		if (this.m_tables !== undefined) {
			this.m_tables.forEach(table => { table.Destructor(); });
		}
		this.m_tables.clear();
		if (this.m_grid !== undefined) {
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

		Application.UpdateZIndex(this);

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

		if (dispatcher !== undefined) {
			dispatcher.AddHiddenView(this);
		}
		else {
			this.m_parentView.classList.add("hidden");
		}
	}

	Show()
	{
		if (!this.m_canBeMaximized) {
			return;
		}

		Application.UpdateZIndex(this);
		if (!this.m_parentView.classList.contains("hidden")) {
			return;
		}

		if (dispatcher !== undefined) {
			dispatcher.RemoveHiddenView(this);
		}
		else {
			this.m_parentView.classList.remove("hidden");
		}
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

			Application.GetCreatedApplications().values().forEach((app) => { app.m_view.classList.add("changing"); });

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
			Application.GetCreatedApplications().values().forEach(
				(app) => { app.m_view.classList.remove("changing"); });
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
			if (metadata !== null && metadata !== undefined) {
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
						void new Application("ModifyApp", {
							appType : options.headers["AppType"],
							port : json.port,
						});

						Application.SendRequest({
							method : "GET",
							mode : "cors",
							headers : { "Accept" : "application/json", "Type" : "getCreatedApps" }
						});
					}
					else if (type == "getCreatedApps") {
						Application.HandleResponse(type, json);
					}
					else if (type == "getInstalledApps") {
						Application.HandleResponse(type, json);
					}
					else if (type == "getMetadata") {
						if ("metadata" in json) {
							Application.#privateFields.m_metadataToAppType.set(
								options.headers["AppType"], json["metadata"]);
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
							Application.#privateFields.m_parametersToPort.set(
								options.headers["Port"], json["parameters"]);
							Application.HandleResponse(type, json, { "port" : options.headers["Port"] });
						}
						else {
							return { "status" : false, "message" : "Parameters are not specified in response" };
						}
					}
					else if (type == "run" || type == "pause" || type == "delete") {
						Application.HandleResponse(type, json, { "port" : options.headers["Port"] });
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

	static UpdateZIndex(app)
	{
		if (app.m_parentView.style.zIndex == Application.#privateFields.m_createdApplications.size) {
			return;
		}

		const shiftedPosition = app.m_parentView.style.zIndex;

		Application.#privateFields.m_createdApplications.forEach((item, uid) => {
			if (uid != app.m_uid) {
				if (item.m_parentView.style.zIndex > shiftedPosition) {
					item.m_parentView.style.zIndex = item.m_parentView.style.zIndex - 1;
				}
			}
			else {
				item.m_parentView.style.zIndex = Application.#privateFields.m_createdApplications.size;
			}
		});
	}
}

Application.AddParametersTemplate("NewApp", "default", [
	{ "inputs" : [ { "key" : "ip", "value" : 0 } ] }, { "selects" : [ { "key" : "LogLevel", "value" : 5 } ] },
	{ "checkboxes" : [ { "key" : "lofInFile", "value" : true }, { "key" : "separateDaysLogging", "value" : true } ] }
]);

Application.AddViewTemplate("AppView", `<div class="appView"></div>`);
Application.AddViewTemplate("InstalledApps", `<div></div>`);
Application.AddViewTemplate("NewApp", `<div class="customView">
        <div class="items vertical">
			<div class="item"><input name="name" type="text" placeholder="Name"/></div>
			<div class="item"><input value="127.0.0.1" name="ip" type="text" placeholder="Listening IP" canBeEmpty="false" /></div>
            <div class="item"><input name="port" type="number" placeholder="Listening Port" /></div>
			<div class="item"><input value="../logs/"name="parentPath" type="text" placeholder="Parent directory for logging" /></div>
            <div class="item">
				<label>
					<input name="logInConsole" type="checkbox" />
					<span>Log in console</span>
				</label>
			</div>
            <div class="item">
				<label>
					<input name="logInFile" type="checkbox" />
					<span>Log in file</span>
				</label>
			</div>
			<div class="item">
				<span class="title">Log level:</span>
				<select name="logLevel" canBeEmpty="false">
					<option value="1">ERROR</option>
					<option value="2">WARNING</option>
					<option value="3">INFO</option>
					<option value="4">DEBUG</option>
					<option value="5">PROTOCOL</option>
				</select>
			</div>
            <div class="item">
				<label>
					<input name="separateDaysLogging" type="checkbox" />
					<span>Separate days logging</span>
				</label>
			</div>
        </div>
        <div class="button">Create</div>
    </div>`);
Application.AddViewTemplate("ModifyApp", `<div class="customView">
        <div class="inputs"></div>
        <div class="button">Modify</div>
    </div>`);
Application.AddViewTemplate("CreatedApps", `<div></div>`);
Application.AddViewTemplate("TableView", `<div class="tableView"></div>`);
Application.AddViewTemplate("GridSettingsView", `<div class="gridSettingsView customView">
		<div class="group">
			<span class="text">Align:</span>
			<span class="action alignLeft"></span>
			<span class="action alignCenter"></span>
			<span class="action alignRight"></span>
		</div>
		<div class="group">
			<span class="text">Sorting:</span>
			<span class="action ascending"></span>
			<span class="action none"></span>
			<span class="action descending"></span>
		</div>
		<div class="group">
			<span class="text">Filter:</span>
			<span class="action filter"></span>
		</div>
		<div class="filters"></div>
	</div>`);
Application.AddViewTemplate("SelectView", `<div class="selectView customView">
		<div class="search">
			<div class="searchIco"></div>
			<input type="text" placeholder="Search">
			<div class="caseSensitive"></div>
		</div>
		<div class="options"></div>
	</div>`);

if (typeof module !== 'undefined' && typeof module.exports !== 'undefined') {
	Helper = require("./helper");
	module.exports = Application;
}