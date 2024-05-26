document.addEventListener("DOMContentLoaded", function () {
    // Function to load the HTML template
    function loadTemplate() {
        return fetch('/settings/rart.html')
            .then(response => response.text())
            .then(templateText => {
                const template = document.createElement('div');
                template.innerHTML = templateText;
                document.body.appendChild(template);
            });
    }

    // Function to create name elements for a given relay
    function createNameElement(relay) {
        const nameDiv = document.createElement('div');
        nameDiv.id = `relay-name-${relay.id}-container`;

        const label = document.createElement('label');
        label.htmlFor = `relay-name-${relay.id}`;
        label.textContent = `Relay ${relay.id} Name:`;

        const input = document.createElement('input');
        input.type = 'text';
        input.id = `relay-name-${relay.id}`;
        input.name = `relayName${relay.id}`;
        input.value = relay.name;

        nameDiv.appendChild(label);
        nameDiv.appendChild(input);

        return nameDiv;
    }


    // Function to create rule elements for a given relay and rule data
    function createRuleElement(relayId, ruleData, ruleIndex, type = "delete") {
        const templateHtml = document.getElementById('relay-alarm-rule-template')?.innerHTML;
        if (!templateHtml) {
            console.error('relay-alarm-rule-template not found');
            return;
        }

        const replacedHtml = templateHtml
            .replace(/\$\{relayId\}/g, relayId)
            .replace(/\$\{ruleIndex\}/g, ruleIndex);

        const template = document.createElement('div');
        template.innerHTML = replacedHtml;
        const ruleDiv = template.firstElementChild;

        const select = ruleDiv.querySelector('.select-style');
        select.value = ruleData.state;

        const timeInput = ruleDiv.querySelector('.time-input');
        timeInput.value = ruleData.time;

        const days = ["mon", "tue", "wed", "thu", "fri", "sat", "sun"];
        days.forEach(day => {
            const checkbox = ruleDiv.querySelector(`input[value="${day}"]`);
            checkbox.checked = ruleData.days.includes(day);
        });

        // Add event listener for delete button
        if (type === "delete") {
            const deleteButton = ruleDiv.querySelector('.delete-rule');
            deleteButton.addEventListener('click', function () {
                handleDeleteButtonClick(relayId, ruleIndex, ruleDiv);
            });
        }

        // Add event listener for add button
        if (type === "add") {
            const addButton = ruleDiv.querySelector('.delete-rule');
            addButton.classList.remove('delete-rule');
            addButton.classList.add('add-rule');
            addButton.textContent = 'Add Rule';
            addButton.addEventListener('click', function () {
                handleAddButtonClick(relayId);
            });
        }

        return ruleDiv;
    }

    // Function to handle add button click
    function handleAddButtonClick(relayId) {
        // make api request to add rule
        fetch(`/api/relay-alarm/${relayId}`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({
                state: 'on',
                time: '00:00:00',
                days: []
            })
        })
            .then(response => {
                if (response.ok) {
                    // Add new rule element
                    loadRelays();
                } else {
                    alert('Failed to add the rule.');
                }
            });
    }

    // Function to handle delete button click
    function handleDeleteButtonClick(relayId, ruleIndex, ruleDiv) {
        const deleteButton = ruleDiv.querySelector('.delete-rule');
        deleteButton.style.display = 'none';

        const confirmButton = document.createElement('button');
        confirmButton.classList.add('confirm-delete');
        confirmButton.textContent = 'Confirm';

        const cancelButton = document.createElement('button');
        cancelButton.classList.add('cancel-delete');
        cancelButton.textContent = 'Cancel';

        const buttonContainer = document.createElement('div');
        buttonContainer.classList.add('confirm-cancel-buttons');
        buttonContainer.appendChild(confirmButton);
        buttonContainer.appendChild(cancelButton);

        ruleDiv.appendChild(buttonContainer);

        confirmButton.addEventListener('click', function () {
            fetch(`/api/relay-alarm/${relayId}/${ruleIndex}`, {
                method: 'DELETE',
            })
                .then(response => {
                    if (response.ok) {
                        loadRelays();
                    } else {
                        alert('Failed to delete the rule.');
                    }
                })
                .catch(error => {
                    console.error('Error:', error);
                    alert('An error occurred.');
                });
        });

        cancelButton.addEventListener('click', function () {
            buttonContainer.remove();
            deleteButton.style.display = 'block';
        });
    }

    function updateRelayAlarmRules(relay, relayDiv = null) {
        if (!relayDiv) {
            relayDiv = document.getElementById(`relay-${relay.id}-form`);
        }

        if (!relayDiv) {
            console.error(`relay-${relay.id}-form not found`);
            return;
        }

        fetch(`/api/relay-alarms?relayId=${relay.id}`)
            .then(response => response.json())
            .then(alarmRules => {
                const existingRuleElements = relayDiv.querySelectorAll('.relay-alarm-rule');
                const existingRuleIds = Array.from(existingRuleElements).map(el => parseInt(el.dataset.ruleIndex, 10));
                const newRuleIds = alarmRules.map((_, index) => index + 1);

                // Remove old elements that are no longer present
                existingRuleElements.forEach(el => {
                    const ruleIndex = parseInt(el.dataset.ruleIndex, 10);
                    if (!newRuleIds.includes(ruleIndex)) {
                        el.remove();
                    }
                });

                // Update existing elements and add new ones
                alarmRules.forEach((rule, ruleIndex) => {
                    const ruleId = ruleIndex + 1;
                    let ruleElement = relayDiv.querySelector(`.relay-alarm-rule[data-rule-index="${ruleId}"]`);

                    if (ruleElement) {
                        // Update existing rule element
                        const select = ruleElement.querySelector('.select-style');
                        select.value = rule.state;

                        const timeInput = ruleElement.querySelector('.time-input');
                        timeInput.value = rule.time;

                        const days = ["mon", "tue", "wed", "thu", "fri", "sat", "sun"];
                        days.forEach(day => {
                            const checkbox = ruleElement.querySelector(`input[value="${day}"]`);
                            checkbox.checked = rule.days.includes(day);
                        });
                    } else {
                        // Create new rule element
                        ruleElement = createRuleElement(relay.id, rule, ruleId);
                        relayDiv.appendChild(ruleElement);
                    }
                });

                // Ensure the "Add" button is always at the bottom
                const addButtonElement = relayDiv.querySelector('.add-rule');
                if (addButtonElement) {
                    addButtonElement.remove();
                }

                const newRuleElement = createRuleElement(relay.id, { state: 'on', time: '00:00:00', days: [] }, alarmRules.length + 1, "add");
                relayDiv.appendChild(newRuleElement);
            });
    }


    // Function to update the relay-alarm-rules section
    function updateAllRelayAlarmRules(relays) {
        const relayAlarmRulesDiv = document.getElementById('relay-alarm-rules');
        if (!relayAlarmRulesDiv) {
            console.error('relay-alarm-rules div not found');
            return;
        }

        relays.forEach((relay, relayIndex) => {
            const relayMainDiv = document.createElement('div');
            relayMainDiv.id = `relay-${relay.id}-alarm-rules`;

            const relayDiv = document.createElement('div');
            relayDiv.id = `relay-${relay.id}-form`;

            const rulesHeading = document.createElement('h3');
            rulesHeading.id = `relay-${relay.id}-rules-heading`;
            rulesHeading.textContent = `Rules: ${relay.name}`;

            relayDiv.appendChild(rulesHeading);

            relayMainDiv.appendChild(relayDiv);

            relayAlarmRulesDiv.appendChild(relayMainDiv);

            updateRelayAlarmRules(relay, relayDiv);
        });
    }

    function loadRelays() {
        fetch('/api/all-relays')
            .then(response => response.json())
            .then(relays => {
                updateAllRelayAlarmRules(relays);
            })
            .catch(error => console.error('Error fetching relays:', error));
    }

    // Load settings on page load
    fetch("/api/settings")
        .then(response => response.json())
        .then(data => {
            document.getElementById("system-name").value = data.systemName;
            document.getElementById("wifi-name").value = data.wifiName;
            document.getElementById("system-time").value = data.systemTime;
            document.getElementById("system-date").value = data.systemDate;
            document.getElementById("sync-time-checkbox").checked = data.syncTime;
            onChangeSyncTimeCheckbox();

            const relayNamesDiv = document.getElementById('relay-names');
            if (!relayNamesDiv) {
                console.error('relay-names div not found');
                return;
            }

            data.relays.forEach(element => {
                const nameElement = createNameElement(element);
                relayNamesDiv.appendChild(nameElement);
            });
        });

    // Load network information on page load
    fetch("/api/network-info")
        .then(response => response.json())
        .then(data => {
            document.getElementById("wifi-status").textContent = data.wifiMode;
            document.getElementById("ip-status").textContent = data.ipAddress;
            document.getElementById("gateway-status").textContent = data.gateway;
            document.getElementById("dns-status").textContent = data.dns;
        });

    // Save settings when the save button is clicked
    Relays = [];
    document.getElementById("save-general-settings").addEventListener("click", function () {
        const settings = {
            systemName: document.getElementById("system-name").value,
            wifiName: document.getElementById("wifi-name").value,
            wifiPassword: document.getElementById("wifi-password").value,
            systemTime: document.getElementById("system-time").value,
            systemDate: document.getElementById("system-date").value,
            syncTime: document.getElementById("sync-time-checkbox").checked,
            relayNames: []
        };

        Relays.forEach(relay => {
            const nameInput = document.getElementById(`relay-name-${relay.id}`);
            settings.relayNames.push({
                id: relay.id,
                name: nameInput.value
            });
        });

        fetch("/api/settings", {
            method: "POST",
            headers: {
                "Content-Type": "application/json"
            },
            body: JSON.stringify(settings)
        })
            .then(response => response.json())
            .then(data => {
                alert(data.message);
            })
            .catch(error => {
                alert("Failed to save settings.");
            });
    });


    function onChangeSyncTimeCheckbox() {
        if (document.getElementById("sync-time-checkbox").checked) {
            //  when checked make the time and date uneditable
            document.getElementById("system-time").readOnly = true;
            document.getElementById("system-date").readOnly = true;

            // Sync time every second
            this.syncTimeInterval = setInterval(() => {
                fetch("/api/server-time")
                    .then(response => response.json())
                    .then(data => {
                        /*
                            ```json
                            {
                                "time": "11:32:45", 
                                "date": "2024-07-23"
                            }
                            ```
                        */
                        time = data.time.split(":");
                        date = data.date.split("-");

                        document.getElementById("system-time").value = `${time[0]}:${time[1]}:${time[2]}`;
                        document.getElementById("system-date").value = `${date[0]}-${date[1]}-${date[2]}`;
                    });
            }, 1000);
        } else {
            // when unchecked make the time and date editable
            document.getElementById("system-time").readOnly = false;
            document.getElementById("system-date").readOnly = false;

            clearInterval(this.syncTimeInterval);
        }
    }

    // Sync time automatically when the checkbox is clicked
    document.getElementById("sync-time-checkbox").addEventListener("change", function () {
        onChangeSyncTimeCheckbox();
    });

    // Upload a file when the upload button is clicked
    document.getElementById("update-button").addEventListener("click", function () {
        const fileInput = document.getElementById("update-file");
        const formData = new FormData();
        formData.append("firmware", fileInput.files[0]);

        fetch("/api/update-firmware", {
            method: "POST",
            body: formData
        })
            .then(response => response.json())
            .then(data => {
                alert(data.message);
            })
            .catch(error => {
                alert("Failed to update firmware.");
            });
    });

    // Fetch and load the HTML template, then fetch relays and update the relay-alarm-rules section
    loadTemplate()
        .then(() => {
            fetch('/api/all-relays')
                .then(response => response.json())
                .then(relays => {
                    updateAllRelayAlarmRules(relays);
                    Relays = relays;
                })
                .catch(error => console.error('Error fetching relays:', error));
        })
        .catch(error => console.error('Error loading template:', error));
});
