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
 * It handles initialization messages from the parent window and forwards click events to update
 * the z-index of the application window.
 *
 * @usage Include this script in your app's HTML:
 * <script src="/iframe-bridge.js"></script>
 *
 * Or load it from the manager:
 * <script src="http://[manager-ip]:[manager-port]/iframe-bridge.js"></script>
 */
(function() {
	'use strict';

	// Listen for initialization message from parent window
	window.addEventListener("message", (event) => {
		if (event.data && event.data.type === "init") {
			window.parentOrigin = event.data.origin;
			window.uid = event.data.uid;
		}
	});

	// Forward click events to parent window to update z-index
	window.addEventListener('click', function(event) {
		if (window.parent && window.uid && window.parentOrigin) {
			window.parent.postMessage(
				{ type: 'iframeClick', uid: window.uid },
				window.parentOrigin
			);
		}
	});
})();
