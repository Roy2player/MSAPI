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
 * @brief View to display an MSAPI application with its own UI in the iframe.
 */

if (typeof module !== "undefined" && typeof module.exports !== "undefined") {
	View = require("../core/view");
}

class AppView extends View {
	constructor(parameters) { super("AppView", parameters); }

	Constructor(parameters)
	{
		if (!this.m_port) {
			console.error("Listening port is not provided");
			return false;
		}

		this.m_listeningIp = parameters.ip;
		if (!this.m_listeningIp) {
			console.error("Listening IP is not provided");
			return false;
		}

		const url = parameters.url || `http://${this.m_listeningIp}:${this.m_port}/`;
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

		let isTitleUpdated = false;
		let appState = undefined;
		let stream = new WebSocketStream({
			event : Helper.StringHash32Uint("parameters"),
			handleData : (parameters) => {
				let name = parameters["2000001"];
				if (!isTitleUpdated && name != undefined) {
					if (name) {
						this.m_title += ": " + name + " on port " + this.m_port;
					}
					else {
						this.m_title += ": " + this.m_appType + " on port " + this.m_port;
					}
					isTitleUpdated = true;
					this.m_parentView.querySelector(".title > span").textContent = this.m_title;
				}

				const state = parameters["2000002"];
				if (state != undefined) {
					if (appState == undefined) {
						appState = state;
						return;
					}

					if (state != appState) {
						this.m_view.classList.add("loading");
						iframe.src = url + (url.includes("?") ? "&" : "?") + "reloaded=" + Date.now();
					}
					appState = state;
				}
			},
			handleFailed : (error) => { this.DisplayErrorMessage(error); },
			viewUid : this.m_uid,
			data : { "port" : this.m_port, "filter" : this.m_port }
		});

		return true;
	}
}

View.AddViewTemplate("AppView", `<div class="appView"></div>`);

if (typeof module !== "undefined" && typeof module.exports !== "undefined") {
	module.exports = AppView;
}