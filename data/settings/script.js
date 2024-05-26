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
        label.textContent = `Relay ${relay.id + 1} Name:`;

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

        if (type === "add") {
            ruleIndex = "new";
        }

        const replacedHtml = templateHtml
            .replace(/\$\{relayId\}/g, relayId)
            .replace(/\$\{ruleIndex\}/g, ruleIndex);

        const template = document.createElement('div');
        template.innerHTML = replacedHtml;
        const ruleDiv = template.firstElementChild;

        const select = ruleDiv.querySelector('.select-style');
        select.value = ruleData.state ? "on" : "off"; // Correctly set the state

        const timeInput = ruleDiv.querySelector('.time-input');
        timeInput.value = ruleData.time;

        const days = ["mon", "tue", "wed", "thu", "fri", "sat", "sun"];
        const daysChecked = {
            "mon": 1,
            "tue": 2,
            "wed": 3,
            "thu": 4,
            "fri": 5,
            "sat": 6,
            "sun": 0
        }
        days.forEach(day => {
            const checkbox = ruleDiv.querySelector(`input[value="${day}"]`);
            checkbox.checked = ruleData.weekdays[daysChecked[day]];
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
        // Get state, time, and days values
        let state = document.getElementById(`relay-${relayId}-select-new`).value === "on" ? true : false;
        let time = document.getElementById(`relay-${relayId}-time-new`).value;
        let weekdays = [false, false, false, false, false, false, false];
        let checkboxes = document.getElementById(`relay-${relayId}-weekdays-new`).querySelectorAll('input[type="checkbox"]');
        const daysChecked = {
            "mon": 1,
            "tue": 2,
            "wed": 3,
            "thu": 4,
            "fri": 5,
            "sat": 6,
            "sun": 0
        };
        checkboxes.forEach(checkbox => {
            let dayname = checkbox.value;
            if (checkbox.checked) {
                weekdays[daysChecked[dayname]] = true;
            }
        });

        // Make API request to add rule
        fetch(`/api/relay-alarm`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({
                relayId: relayId,
                state: state,
                time: time,
                weekdays: weekdays
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
            fetch(`/api/relay-alarm?relayId=${relayId}&alarmId=${ruleIndex}`, {
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
            .then(alarmRulesob => {
                let alarmRules = alarmRulesob.alarms;
    
                // Clear existing rules before updating
                while (relayDiv.firstChild) {
                    relayDiv.removeChild(relayDiv.firstChild);
                }
    
                const rulesHeading = document.createElement('h3');
                rulesHeading.id = `relay-${relay.id}-rules-heading`;
                rulesHeading.textContent = `Rules: ${relay.name}`;
                relayDiv.appendChild(rulesHeading);
    
                alarmRules.forEach((rule) => {
                    const ruleId = rule.id;
                    let ruleElement = createRuleElement(relay.id, rule, ruleId, "delete");
                    relayDiv.appendChild(ruleElement);
    
                    // Attach event listeners properly
                    document.getElementById(`relay-${relay.id}-select-${ruleId}`).addEventListener('change', () => updateRelayRule(relay.id, ruleId));
                    document.getElementById(`relay-${relay.id}-time-${ruleId}`).addEventListener('input', () => updateRelayRule(relay.id, ruleId));
                    
                    const days = ["sun", "mon", "tue", "wed", "thu", "fri", "sat"];
                    days.forEach(day => {
                        document.getElementById(`relay-${relay.id}-${day}-${ruleId}`).addEventListener('change', () => updateRelayRule(relay.id, ruleId));
                    });
                });
    
                // Ensure the "Add" button is always at the bottom
                const newRuleElement = createRuleElement(relay.id, { state: true, time: '00:00:00', weekdays: [false, false, false, false, false, false, false] }, alarmRules.length + 1, "add");
                relayDiv.appendChild(newRuleElement);
            });
    }

    function updateRelayRule(relayID, ruleID) {
        // /api/relay-alarm?relayId=0&ruleId=0 PUT
        let rs = document.getElementById(`relay-${relayID}-select-${ruleID}`);
        let ti = document.getElementById(`relay-${relayID}-time-${ruleID}`);
        let ch = document.getElementById(`relay-${relayID}-weekdays-${ruleID}`);

        if (!rs || !ti || !ch) {
            return;
        }

        let state = rs.value === "on" ? true : false;
        let time = ti.value;
        let weekdays = [false, false, false, false, false, false, false];
        let checkboxes = ch.querySelectorAll('input[type="checkbox"]');

        const daysChecked = {
            "mon": 1,
            "tue": 2,
            "wed": 3,
            "thu": 4,
            "fri": 5,
            "sat": 6,
            "sun": 0
        };
        checkboxes.forEach(checkbox => {
            let dayname = checkbox.value;
            if (checkbox.checked) {
                weekdays[daysChecked[dayname]] = true;
            }
        });

        fetch(`/api/relay-alarm?relayId=${relayID}&alarmId=${ruleID}`, {
            method: 'PUT',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({
                state: state,
                time: time,
                weekdays: weekdays
            })
        })
            .then(response => {
                if (response.ok) {
                    console.log('Rule updated');
                } else {
                    alert('Failed to update the rule.');
                }
            })
            .catch(error => {
                console.error('Error:', error);
                alert('An error occurred.');
            });
    }


    // Function to update the relay-alarm-rules section
    function updateAllRelayAlarmRules(relays) {
        const relayAlarmRulesDiv = document.getElementById('relay-alarm-rules');
        if (!relayAlarmRulesDiv) {
            console.error('relay-alarm-rules div not found');
            return;
        }

        // Clear existing relay rules before updating
        while (relayAlarmRulesDiv.firstChild) {
            relayAlarmRulesDiv.removeChild(relayAlarmRulesDiv.firstChild);
        }

        relays.forEach((relay) => {
            const relayMainDiv = document.createElement('div');
            relayMainDiv.id = `relay-${relay.id}-alarm-rules`;

            const relayDiv = document.createElement('div');
            relayDiv.id = `relay-${relay.id}-form`;

            relayMainDiv.appendChild(relayDiv);

            relayAlarmRulesDiv.appendChild(relayMainDiv);

            updateRelayAlarmRules(relay, relayDiv);
        });
    }

    function loadRelays() {
        fetch('/api/all-relays')
            .then(response => response.json())
            .then(relaysob => {
                updateAllRelayAlarmRules(relaysob.relays);
            })
            .catch(error => console.error('Error fetching relays:', error));
    }

    // Load settings on page load
    Relays = [];
    fetch("/api/settings")
        .then(response => response.json())
        .then(data => {
            document.getElementById("system-name").value = data.systemName;
            document.getElementById("title").textContent = data.systemName;

            const relayNamesDiv = document.getElementById('relay-names');
            if (!relayNamesDiv) {
                console.error('relay-names div not found');
                return;
            }

            Relays = data.relays;

            data.relays.forEach(element => {
                const nameElement = createNameElement(element);
                relayNamesDiv.appendChild(nameElement);
            });
        })
        .then(() => {
            Relays.forEach(relay => {
                document.getElementById(`relay-name-${relay.id}`).addEventListener("input", saveGeneralSettings);
            });
            document.getElementById("system-name").addEventListener("input", saveGeneralSettings);
        });

    // Save settings
    function saveGeneralSettings() {
        let systemName = document.getElementById("system-name").value;

        // Change title of the page
        document.getElementById("title").textContent = systemName;

        const settings = {
            systemName: systemName,
            relays: []
        };

        Relays.forEach(relay => {
            const nameInput = document.getElementById(`relay-name-${relay.id}`);
            settings.relays.push({
                id: relay.id,
                name: nameInput.value
            });

            document.getElementById(`relay-${relay.id}-rules-heading`).textContent = "Rules: " + nameInput.value;
        });

        fetch("/api/settings", {
            method: "POST",
            headers: {
                "Content-Type": "application/json"
            },
            body: JSON.stringify(settings)
        })
            .then(response => response.json())
            .catch(error => {
                console.error("Failed to save settings.");
            });
    }

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
                .then(relaysob => {
                    updateAllRelayAlarmRules(relaysob.relays);
                })
                .catch(error => console.error('Error fetching relays:', error));
        })
        .catch(error => console.error('Error loading template:', error));



    // Time
    // Function to update the time every second
    function fetchServerTime() {
        fetch('/api/server-time')
            .then(response => response.json())
            .then(data => {
                document.getElementById('system-time-hour').textContent = data.hour;
                document.getElementById('system-time-minute').textContent = data.minute;
                document.getElementById('system-time-second').textContent = data.second;
                document.getElementById('system-date').value = `${data.year}-${String(data.month).padStart(2, '0')}-${String(data.day).padStart(2, '0')}`;
            })
            .catch(error => console.error('Error fetching server time:', error));
    }

    // Call fetchServerTime every second
    setInterval(fetchServerTime, 1000);

    // Function to adjust time
    function adjustTime(unit, value) {
        let adjustments = {
            hourAdjustment: 0,
            minuteAdjustment: 0,
            secondAdjustment: 0,
            date: document.getElementById('system-date').value
        };

        if (unit === 'hour') {
            adjustments.hourAdjustment = value;
        } else if (unit === 'minute') {
            adjustments.minuteAdjustment = value;
        } else if (unit === 'second') {
            adjustments.secondAdjustment = value;
        }

        fetch('/api/server-time', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify(adjustments)
        })
            .then(response => response.json())
            .then(data => {
                console.log('Time adjusted:', data);
                fetchServerTime(); // Update the displayed time
            })
            .catch(error => console.error('Error adjusting time:', error));
    }

    // Attach event listeners to the buttons
    document.querySelectorAll('.time-adjust button').forEach(button => {
        button.addEventListener('click', (event) => {
            const unit = event.target.parentElement.id.split('-')[1];
            const value = event.target.textContent === '+' ? 1 : -1;
            adjustTime(unit, value);
        });
    });

    // Function to adjust date
    document.getElementById('system-date').addEventListener('change', (event) => {
        const adjustments = {
            hourAdjustment: 0,
            minuteAdjustment: 0,
            secondAdjustment: 0,
            date: document.getElementById('system-date').value
        };

        fetch('/api/server-time', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify(adjustments)
        })
            .then(response => response.json())
            .then(data => {
                console.log('Date adjusted:', data);
                fetchServerTime(); // Update the displayed date and time
            })
            .catch(error => console.error('Error adjusting date:', error));
    });

    // Initial fetch to set the time and date on page load
    fetchServerTime();
});
