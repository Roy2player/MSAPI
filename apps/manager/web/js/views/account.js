/**************************
 * @file        account.js
 * @version     6.0
 * @date        2026-04-11
 * @author      maks.angels@mail.ru
 * @copyright   © 2021–2026 Maksim Andreevich Leonov
 *
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
 * @brief View to display logging and sign up forms, manage account and its information. Only one instance can be
 * created.
 */

if (typeof module !== "undefined" && typeof module.exports !== "undefined") {
	View = require("../core/view");
	MetadataCollector = require("./metadataCollector");
	Dispatcher = require("./dispatcher").Dispatcher;
	Dynamic = require("../help/dynamic");
}

class Account extends View {
	static m_isAuthorized = false;
	static m_login = "";

	static authorizedTemplate = `
	<div>
		<div class="info">
			<div class="items vertical">
				<div class="item">
					<p class="text">Login:</p>
				</div>
				<div class="item">
					<input data-form-item="input" name="login" type="login" readonly/>
				</div>
				<div class="item">
					<input data-form-item="input" name="newLogin" type="newLogin" placeholder="New login"/>
				</div>
				<div class="item">
					<input data-form-item="input" name="newPassword" type="password" placeholder="New password"/>
				</div>
				<div class="item">
					<p class="text">
						🔑 Length between 8 and 28 characters, and presence of at least one lowercase letter, one uppercase letter, one digit, and one special character.
					</p>
				</div>
			</div>
			<div class="buttons vertical">
				<div class="button multi modify" data-form-item="button">Modify</div>
				<div class="button multi logout">Logout</div>
			</div>
		</div>
	</div>
	`;

	static unauthorizedTemplate = `
	<div>
		<div class="switch">
			<div class="logic active" data-trigger-id="LoginButton" data-trigger-remove="SignupButton,SignupContent" data-trigger-add="LoginButton,LoginContent">Login</div>
			<div class="signup" data-trigger-id="SignupButton" data-trigger-remove="LoginButton,LoginContent" data-trigger-add="SignupButton,SignupContent">Signup</div>
		</div>
		<div class="contentSwitch">
			<div class="login active" data-trigger-id="LoginContent">
				<div class="items vertical">
					<div class="item">
						<input data-form-item="input" name="login" type="login" placeholder="Login" />
					</div>
					<div class="item">
						<input data-form-item="input" name="password" type="password" placeholder="Password" />
					</div>
				</div>
				<div data-form-item="button" class="button multi">Go in</div>
			</div>
			<div class="signup" data-trigger-id="SignupContent">
				<div class="items vertical">
					<div class="item">
						<input data-form-item="input" name="login" type="login" placeholder="Login" />
					</div>
					<div class="item">
						<input data-form-item="input" name="password" type="password" placeholder="Password" />
					</div>
					<div class="item">
						<p class="text">
							🔑 Length between 8 and 28 characters, and presence of at least one lowercase letter, one uppercase letter, one digit, and one special character.
						</p>
					</div>
				</div>
				<div data-form-item="button" class="button multi">Join</div>
			</div>
		</div>
	</div>
	`;

	constructor(parameters) { super("Account", parameters); }

	Constructor(parameters)
	{
		if (View.ShowExistedView(this)) {
			return false;
		}
		return this.SwitchTemplate();
	}

	SwitchTemplate()
	{
		let currentTemplate = this.m_view.querySelector(":first-child");
		if (currentTemplate) {
			currentTemplate.remove();
		}

		if (Account.m_isAuthorized) {
			const template = document.createElement("template");
			template.innerHTML = Account.authorizedTemplate;
			this.m_view.appendChild(template.content.cloneNode(true));

			let infoForm = this.m_view.querySelector(".info");
			if (!infoForm) {
				console.warn("Info form is not found");
				return false;
			}

			let login = infoForm.querySelector(".item input[name='login']");
			if (!login) {
				console.warn("Info login input is not found");
				return false;
			}

			let modifyButton = infoForm.querySelector(".button.modify");
			if (!modifyButton) {
				console.warn("Modify button is not found");
				return false;
			}

			let logoutButton = infoForm.querySelector(".button.logout");
			if (!logoutButton) {
				console.warn("Logout button is not found");
				return false;
			}

			let newLogin = "";
			modifyButton.addEventListener("click", () => {
				this.m_parentView.classList.add("loading");
				this.HideErrorMessage();

				let data = View.ParseInputs(infoForm, true);
				if (data["newPassword"] == "") {
					delete data.newPassword;
				}
				newLogin = data["newLogin"];
				if (data["newLogin"] == "") {
					delete data.newLogin;
				}

				const handleResponse = (response) => {
					this.m_parentView.classList.remove("loading");
					this.HideErrorMessage();

					let noErrors = true;
					if ("login" in response) {
						if (response["login"] == true) {
							login.value = newLogin;
							Account.m_login = newLogin;
						}
						else {
							noErrors = false;
							this.DisplayErrorMessage(response["error"]);
						}
					}

					if ("password" in response) {
						if (response["password"] != true) {
							noErrors = false;
							this.DisplayErrorMessage(response["error"]);
						}
					}

					if (noErrors) {
						this.SwitchTemplate();
					}
				};

				new WebSocketSingle({
					event : Helper.StringHash32Uint("modifyAccount"),
					handleResponse : handleResponse,
					handleFailed : (error) => {
						this.m_parentView.classList.remove("loading");
						this.DisplayErrorMessage(error);
					},
					data : data,
					viewUid : this.m_uid
				});
			});

			logoutButton.addEventListener("click", () => {
				this.m_parentView.classList.add("loading");
				this.HideErrorMessage();

				const handleResponse = (response) => {
					this.m_parentView.classList.remove("loading");
					this.HideErrorMessage();
					Account.m_isAuthorized = false;
					Account.m_login = "";
					this.SwitchTemplate();
				};

				new WebSocketSingle({
					event : Helper.StringHash32Uint("logout"),
					handleResponse : handleResponse,
					handleFailed : (error) => {
						this.m_parentView.classList.remove("loading");
						this.DisplayErrorMessage(error);
					},
					viewUid : this.m_uid
				});
			});

			login.value = Account.m_login;

			Dynamic.InitTriggers(this.m_view);

			return true;
		}

		const template = document.createElement("template");
		template.innerHTML = Account.unauthorizedTemplate;
		this.m_view.appendChild(template.content.cloneNode(true));

		let loginForm = this.m_view.querySelector(".contentSwitch > .login");
		if (!loginForm) {
			console.warn("Login form is not found");
			return false;
		}

		let loginButton = loginForm.querySelector(".button");
		if (!loginButton) {
			console.warn("Login button is not found");
			return false;
		}

		let loginSwitch = this.m_view.querySelector(".switch > .logic");
		if (!loginSwitch) {
			console.warn("Login switch button is not found");
			return false;
		}

		let loginNameInput = loginForm.querySelector("[name='login']");
		if (!loginNameInput) {
			console.warn("Login name input is not found");
			return false;
		}

		let signupForm = this.m_view.querySelector(".contentSwitch > .signup");
		if (!signupForm) {
			console.warn("Signup form is not found");
			return false;
		}

		let signupButton = signupForm.querySelector(".button");
		if (!signupButton) {
			console.warn("Signup button is not found");
			return false;
		}

		let signupLoginInput = signupForm.querySelector("[name='login']");
		if (!signupLoginInput) {
			console.warn("Signup login input is not found");
			return false;
		}

		Dynamic.InitForm(loginForm);
		Dynamic.InitForm(signupForm);

		loginButton.addEventListener("click", () => {
			this.m_parentView.classList.add("loading");
			this.HideErrorMessage();

			const handleResponse = (response) => {
				this.m_parentView.classList.remove("loading");
				this.HideErrorMessage();
				Account.m_isAuthorized = true;
				Account.m_login = loginNameInput.value;
				this.SwitchTemplate();
			};

			new WebSocketSingle({
				event : Helper.StringHash32Uint("login"),
				handleResponse : handleResponse,
				handleFailed : (error) => {
					this.m_parentView.classList.remove("loading");
					this.DisplayErrorMessage(error);
				},
				data : View.ParseInputs(loginForm, true),
				viewUid : this.m_uid
			});
		});

		signupButton.addEventListener("click", () => {
			this.m_parentView.classList.add("loading");
			this.HideErrorMessage();

			const handleResponse = (response) => {
				this.m_parentView.classList.remove("loading");
				this.HideErrorMessage();
				loginNameInput.value = signupLoginInput.value;
				Dynamic.ClearForm(signupForm);
				loginSwitch.click();
			};

			new WebSocketSingle({
				event : Helper.StringHash32Uint("register"),
				handleResponse : handleResponse,
				handleFailed : (error) => {
					this.m_parentView.classList.remove("loading");
					this.DisplayErrorMessage(error);
				},
				data : View.ParseInputs(signupForm, true),
				viewUid : this.m_uid
			});
		});

		Dynamic.InitTriggers(this.m_view);

		return true;
	}
}

View.AddViewTemplate("Account", `<div class="customView accountView"></div>`);
Dispatcher.RegisterPanel("Account", () => new Account());

if (typeof module !== "undefined" && typeof module.exports !== "undefined") {
	module.exports = Account;
}