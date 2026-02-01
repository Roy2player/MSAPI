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
 */

let globalActions = {}

globalActions.Clear = function () {
}

globalActions.GetInstalledApps = function () {
    let rowsContainer = document.querySelector('.MSAPIManagerInstalledApps > .grid > .rows');
    if (!rowsContainer) {
        console.log("Installed apps container not found");
        return;
    }

    rowsContainer.querySelectorAll('div').forEach(element => {
        element.remove();
    });

    let options = {
        method: "GET",
        mode: "cors",
        headers: { "Accept": "application/json", "Type": "getInstalledApps" }
    }

    async function sendRequest() {
        let response = await fetch("api", options);
        if (response.ok) {
            let json = await response.json();
            if (json['status'] == "true") {
                json['apps'].forEach(app => {
                    let row = document.createElement('div');
                    row.classList.add("row");

                    let action = document.createElement('span');
                    action.classList.add("action");
                    action.setAttribute("action", "create");
                    action.addEventListener("click", () => {
                        globalActions.CreateApp(app['id'])
                    });
                    row.append(action);

                    let name = document.createElement('span');
                    name.classList.add("text");
                    name.innerHTML = app['name'];
                    row.append(name);

                    rowsContainer.append(row);
                });
            }
            else {
                console.log("Request failed");
            }
        }
        else {
            console.log("Request failed");
        }
    }

    sendRequest();
}

globalActions.CreateApp = function (id) {
    let options = {
        method: "GET",
        mode: "cors",
        headers: { "Accept": "application/json", "Type": "createApp", "Id": id }
    }

    async function sendRequest() {
        let response = await fetch("api", options);
        if (response.ok) {
            let json = await response.json();
            if (json['status'] == "true") {
                globalActions.GetCreatedApps();
            }
            else {
                console.log("Request failed");
            }
        }
        else {
            console.log("Request failed");
        }
    }

    sendRequest();
}

globalActions.GetCreatedApps = function () {
    let rowsContainer = document.querySelector('.MSAPIManagerCreatedApps > .grid > .rows');
    if (!rowsContainer) {
        console.log("Installed apps container not found");
        return;
    }

    rowsContainer.querySelectorAll('div').forEach(element => {
        element.remove();
    });

    let options = {
        method: "GET",
        mode: "cors",
        headers: { "Accept": "application/json", "Type": "getCreatedApps" }
    }

    async function sendRequest() {
        let response = await fetch("api", options);
        if (response.ok) {
            let json = await response.json();
            if (json['status'] == "true") {
                json['apps'].forEach(app => {
                    let row = document.createElement('div');
                    row.classList.add("row");

                    let action = document.createElement('span');
                    action.classList.add("action");
                    action.setAttribute("action", "create");
                    row.append(action);
                    // option.addEventListener("click", globalActions.STCGraphGetTrades);

                    let state = document.createElement('span');
                    state.classList.add("text");
                    state.innerHTML = app['state'];
                    row.append(state);

                    let name = document.createElement('span');
                    name.classList.add("text");
                    name.innerHTML = app['name'];
                    row.append(name);

                    let pid = document.createElement('span');
                    pid.classList.add("text");
                    pid.innerHTML = app['pid'];
                    row.append(pid);

                    let createdTime = document.createElement('span');
                    createdTime.classList.add("text");
                    createdTime.innerHTML = app['created time'];
                    row.append(createdTime);

                    rowsContainer.append(row);
                });
            }
            else {
                console.log("Request failed");
            }
        }
        else {
            console.log("Request failed");
        }
    }

    sendRequest();
}