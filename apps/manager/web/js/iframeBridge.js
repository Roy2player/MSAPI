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
 * @brief Bridge script for iframe communication with MSAPI Manager.
 *
 * This script should be included in app views that are displayed in iframes within the MSAPI Manager.
 *
 * @usage Include this script in your app's HTML:
 * <script src="/iframeBridge.js"></script>
 */

// Listen for initialization message from parent window
window.addEventListener("message", (event) => {
	if (event.data && event.data.type === "init") {
		window.parentOrigin = event.data.origin;
		window.uid = event.data.uid;
	}
});

// Forward click events to parent window
window.addEventListener("click", (event) => {
	if (window.parent && window.uid && window.parentOrigin) {
		window.parent.postMessage({ type : "iframeClick", uid : window.uid }, window.parentOrigin);
	}
});