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
 * @brief View to display an MSAPI application with its own UI in the iframe.
 */

if (typeof module !== "undefined" && typeof module.exports !== "undefined") {
	View = require("../view");
}

class AppView extends View {
	constructor(parameters) { super("AppView", parameters); }

	async Constructor(parameters)
	{
		this.m_title += ": " + parameters.appType + " (port: " + this.m_port + ")";
		this.m_parentView.querySelector(".title > span").textContent = this.m_title;

		const parametersToPort = View.GetParameters(this.m_port);

		// Create an iframe to display the app at the given URL and port
		const listeningIp = parametersToPort && parametersToPort[1000008] ? parametersToPort[1000008] : null;
		if (!listeningIp) {
			console.error("Parameter 1000008 (Listening IP) is not available for app on port", this.m_port);
			return false;
		}

		const url = parameters.url || `http://${listeningIp}:${parametersToPort[parameters.viewPortParameter]}/`;
		const iframe = document.createElement("iframe");
		iframe.src = url;
		iframe.style.width = "100%";
		iframe.style.height = "100%";
		iframe.style.border = "none";

		this.m_view.appendChild(iframe);
		this.m_view.classList.add("loading");

		iframe.addEventListener("load", () => {
			try {
				const urlObj = new URL(iframe.src, window.location.origin);
				iframe.contentWindow.postMessage(
					{ type : "init", uid : this.m_uid, origin : window.location.origin }, urlObj.origin);
				this.m_view.classList.remove("loading");
			}
			catch (e) {
				console.error("Failed to postMessage to iframe:", e);
			}
		});

		const refreshIframe = (response, extraParameters) => {
			if (response.status && response.result && "port" in extraParameters
				&& extraParameters.port === this.m_port) {
				this.m_view.classList.add("loading");
				iframe.src = url + (url.includes("?") ? "&" : "?") + "_ts=" + Date.now();
			}
			else {
				console.log("Ignoring refresh request for port", extraParameters.port, "in view for port", this.m_port);
			}
		};

		this.AddCallback("run", refreshIframe);
		this.AddCallback("pause", refreshIframe);

		return true;
	}
}

View.AddViewTemplate("AppView", `<div class="appView"></div>`);

if (typeof module !== "undefined" && typeof module.exports !== "undefined") {
	module.exports = AppView;
}