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
 * @brief Displays the existing light and dark CSS color tokens alongside interface examples.
 */

if (typeof module !== "undefined" && typeof module.exports !== "undefined") {
	View = require("../core/view");
	Dispatcher = require("./dispatcher").Dispatcher;
}

class Palette extends View {
	constructor(parameters) { super("Palette", parameters); }

	Constructor()
	{
		if (View.ShowExistedView(this)) {
			return false;
		}

		this.m_parentView.classList.add("paletteWindow");
		this.m_themes = this.m_view.querySelector(".paletteThemes");
		this.m_view.querySelector(".paletteRefresh").addEventListener("click", () => this.RefreshColors());
		this.RefreshColors();
		return true;
	}

	/**************************
	 * @brief Reads root custom properties from the loaded stylesheets, preserving stylesheet order and theme overrides.
	 */
	static ReadThemeColors()
	{
		const themes = { light : new Map(), dark : new Map() };
		const readRules = (rules, modes) => {
			for (const rule of rules) {
				if (rule.media) {
					const condition = rule.media.mediaText;
					const scheme = condition.match(/prefers-color-scheme\s*:\s*(light|dark)/);
					if (scheme) {
						readRules(rule.cssRules, modes.filter(mode => mode === scheme[1]));
					}
					else if (window.matchMedia(condition).matches) {
						readRules(rule.cssRules, modes);
					}
					continue;
				}
				if (rule.selectorText?.split(",").some(selector => selector.trim() === ":root")) {
					for (const property of Array.from(rule.style)) {
						if (property.startsWith("--")) {
							const value = rule.style.getPropertyValue(property).trim();
							for (const mode of modes) {
								themes[mode].set(property, value);
							}
						}
					}
				}
				else if (rule.cssRules) {
					readRules(rule.cssRules, modes);
				}
			}
		};

		for (const sheet of document.styleSheets) {
			try {
				readRules(sheet.cssRules, [ "light", "dark" ]);
			}
			catch (error) {
				// A stylesheet from another origin may not expose its CSS rules.
				console.warn("Cannot read stylesheet colors for Palette", sheet.href, error);
			}
		}
		return themes;
	}

	RefreshColors()
	{
		this.m_themes.replaceChildren();
		const themes = Palette.ReadThemeColors();
		for (const [mode, colors] of Object.entries(themes)) {
			const panel = document.createElement("section");
			panel.className = "paletteTheme";
			panel.dataset.theme = mode;
			panel.style.colorScheme = mode;
			for (const [property, value] of colors) {
				panel.style.setProperty(property, value);
			}

			const template = document.createElement("template");
			template.innerHTML = Palette.previewTemplate;
			panel.appendChild(template.content.cloneNode(true));
			panel.querySelector(".paletteThemeName").textContent = mode === "light" ? "Light" : "Dark";
			panel.querySelector(".paletteCount").textContent = `${colors.size} color tokens`;
			const tokens = panel.querySelector(".paletteTokens");
			for (const [property, value] of colors) {
				const item = document.createElement("li");
				item.className = "paletteToken";
				const swatch = document.createElement("span");
				swatch.className = "paletteSwatch";
				swatch.style.backgroundColor = `var(${property})`;
				swatch.setAttribute("aria-hidden", "true");
				swatch.textContent = "Aa";
				const description = document.createElement("div");
				const name = document.createElement("code");
				name.textContent = property;
				const color = document.createElement("span");
				color.className = "paletteValue";
				color.textContent = value;
				description.append(name, color);
				item.append(swatch, description);
				tokens.appendChild(item);
			}

			const button = panel.querySelector(".paletteAction");
			button.addEventListener("click", () => {
				const enabled = button.getAttribute("aria-pressed") !== "true";
				button.setAttribute("aria-pressed", String(enabled));
				button.textContent = enabled ? "Enabled" : "Enable";
			});
			this.m_themes.appendChild(panel);
		}
	}

	static previewTemplate = `
		<header class="paletteThemeHeader">
			<h2 class="paletteThemeName"></h2>
			<span class="paletteCount"></span>
		</header>
		<div class="paletteExamples">
			<div class="paletteSample">
				<div class="paletteSampleHeader">Application settings</div>
				<div class="paletteForm">
					<label>Instance name<input type="text" value="Market data" spellcheck="false"></label>
					<label>Invalid input<input class="paletteInvalid" type="text" value="Invalid value" aria-invalid="true"></label>
					<label>Disabled input<input type="text" value="Read only" disabled></label>
					<div class="paletteActions">
						<button class="paletteButton paletteAction" type="button" aria-pressed="false">Enable</button>
						<button class="paletteButton" type="button" disabled>Disabled</button>
					</div>
					<p class="paletteError">A value needs your attention.</p>
				</div>
			</div>
			<div class="paletteSample">
				<table class="paletteTable">
					<caption>Grid and status colors</caption>
					<thead><tr><th scope="col">Application</th><th scope="col">State</th></tr></thead>
					<tbody>
						<tr><td>Gateway</td><td><span class="paletteStatus paletteEnabled">Running</span></td></tr>
						<tr><td>Storage</td><td><span class="paletteStatus paletteDisabled">Paused</span></td></tr>
						<tr><td>Strategy</td><td><span class="paletteStatus paletteInvalid">Invalid</span></td></tr>
					</tbody>
				</table>
				<div class="paletteHover">Hover / focus background</div>
				<div class="paletteLoading"><span class="paletteSpinner" aria-hidden="true"></span>Loading preview</div>
			</div>
		</div>
		<h3 class="paletteTokenHeading">Color tokens</h3>
		<ul class="paletteTokens"></ul>`;
}

View.AddViewTemplate("Palette", `<div class="paletteView customView">
	<header class="paletteIntro">
		<div><h1>Interface palette</h1><p>Existing colors, side by side. Hover, focus, and toggle the examples to compare states.</p></div>
		<button class="paletteButton paletteRefresh" type="button">Refresh colors</button>
	</header>
	<div class="paletteThemes"></div>
</div>`);
Dispatcher.RegisterPanel("Palette", () => new Palette());

if (typeof module !== "undefined" && typeof module.exports !== "undefined") {
	module.exports = Palette;
}