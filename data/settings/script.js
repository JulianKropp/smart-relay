document.addEventListener("DOMContentLoaded", function() {
    // Fetch current system settings
    fetch('/api/settings')
        .then(response => response.json())
        .then(data => {
            document.getElementById('systemName').value = data.systemName;
            document.getElementById('wifiName').value = data.wifiName;
            document.getElementById('systemTime').value = data.systemTime;
            document.getElementById('systemDate').value = data.systemDate;
            document.getElementById('syncTime').checked = data.syncTime;
            document.getElementById('relayName').value = data.relayNames.relay1;
        })
        .catch(error => console.error('Error fetching settings:', error));

    // Handle general settings form submission
    document.querySelector('.settings-form form').addEventListener('submit', function(event) {
        event.preventDefault();
        const settingsData = {
            systemName: document.getElementById('systemName').value,
            wifiName: document.getElementById('wifiName').value,
            wifiPassword: document.getElementById('wifiPassword').value,
            systemTime: document.getElementById('systemTime').value,
            systemDate: document.getElementById('systemDate').value,
            syncTime: document.getElementById('syncTime').checked,
            relayNames: {
                relay1: document.getElementById('relayName').value,
                relay2: "Relay 2" // Assuming there's another relay as per the API structure
            }
        };

        fetch('/api/settings', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify(settingsData)
        })
        .then(response => response.json())
        .then(data => alert(data.message))
        .catch(error => console.error('Error updating settings:', error));
    });

    // Handle firmware update submission
    document.querySelector('#file').addEventListener('change', function(event) {
        const fileInput = event.target;
        const formData = new FormData();
        formData.append('firmware', fileInput.files[0]);

        fetch('/api/update-firmware', {
            method: 'POST',
            body: formData
        })
        .then(response => response.json())
        .then(data => alert(data.message))
        .catch(error => console.error('Error updating firmware:', error));
    });

    // Fetch and display network information
    fetch('/api/network-info')
        .then(response => response.json())
        .then(data => {
            document.querySelector('.settings-form h2 + label + p').textContent = data.wifiMode;
            document.querySelector('.settings-form h2 + label + p + label + p').textContent = data.ipAddress;
            document.querySelector('.settings-form h2 + label + p + label + p + label + p').textContent = data.gateway;
            document.querySelector('.settings-form h2 + label + p + label + p + label + p + label + p').textContent = data.dns;
        })
        .catch(error => console.error('Error fetching network info:', error));

    // Fetch and display all relays
    fetch('/api/all-relays')
        .then(response => response.json())
        .then(data => {
            // Assuming there's a place in the DOM to display relay info
            const relaysContainer = document.createElement('div');
            data.forEach(relay => {
                const relayElement = document.createElement('div');
                relayElement.textContent = `Relay ${relay.id}: ${relay.name} is ${relay.state}`;
                relaysContainer.appendChild(relayElement);
            });
            document.body.appendChild(relaysContainer);
        })
        .catch(error => console.error('Error fetching relays:', error));

    // Fetch relay 1 alarms and populate the form
    fetch('/api/relay-alarms/1')
        .then(response => response.json())
        .then(data => {
            const rulesList = document.querySelector('.rules-list');
            rulesList.innerHTML = ''; // Clear existing rules
            data.forEach(rule => {
                const ruleElement = document.createElement('div');
                ruleElement.className = 'rule';
                ruleElement.innerHTML = `
                    <div class="rule-detail">
                        <div class="input-group">
                            <select class="select-style">
                                <option value="on" ${rule.state === 'on' ? 'selected' : ''}>ON</option>
                                <option value="off" ${rule.state === 'off' ? 'selected' : ''}>OFF</option>
                            </select>
                            <input type="time" name="ruleTime" value="${rule.time}" class="time-input">
                        </div>
                        <div class="weekdays">
                            ${['mon', 'tue', 'wed', 'thu', 'fri', 'sat', 'sun'].map(day => `
                                <input type="checkbox" name="ruleDay" value="${day}" ${rule.days.includes(day) ? 'checked' : ''}>${day.charAt(0).toUpperCase() + day.slice(1)}
                            `).join('')}
                        </div>
                    </div>
                    <button class="delete-rule">Delete</button>
                `;
                rulesList.appendChild(ruleElement);
            });
        })
        .catch(error => console.error('Error fetching relay alarms:', error));

    // Handle alarm rules form submission
    document.querySelector('.settings-form form').addEventListener('submit', function(event) {
        event.preventDefault();
        const rules = [];
        document.querySelectorAll('.rules-list .rule').forEach(ruleElement => {
            const state = ruleElement.querySelector('.select-style').value;
            const time = ruleElement.querySelector('.time-input').value;
            const days = Array.from(ruleElement.querySelectorAll('input[name="ruleDay"]:checked')).map(input => input.value);
            rules.push({ state, time, days });
        });

        fetch('/api/relay-alarm/1', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify(rules)
        })
        .then(response => response.json())
        .then(data => alert(data.message))
        .catch(error => console.error('Error updating relay alarms:', error));
    });
});
