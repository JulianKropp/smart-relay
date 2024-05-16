document.addEventListener("DOMContentLoaded", function() {
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

    // Function to create rule elements for a given relay and rule data
    function createRuleElement(relayId, ruleData, ruleIndex) {
        const templateHtml = document.getElementById('relay-alarm-rule-template').innerHTML;
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

        return ruleDiv;
    }

    // Function to update the relay-alarm-rules section
    function updateRelayAlarmRules(relays) {
        const relayAlarmRulesDiv = document.getElementById('relay-alarm-rules');

        relays.forEach((relay, relayIndex) => {
            const relayDiv = document.createElement('div');
            relayDiv.id = `relay-${relay.id}-alarm-rules`;

            const relayHeading = document.createElement('h2');
            relayHeading.id = `relay-${relay.id}-heading`;
            relayHeading.textContent = `${relay.name} - Alarm rules`;

            const relayForm = document.createElement('form');
            relayForm.id = `relay-${relay.id}-form`;

            const rulesHeading = document.createElement('h3');
            rulesHeading.id = `relay-${relay.id}-rules-heading`;
            rulesHeading.textContent = `Rules: ${relay.name}`;

            relayForm.appendChild(rulesHeading);

            let ruleIndexTemp = 0;
            fetch(`/api/relay-alarms?relayId=${relay.id}`)
                .then(response => response.json())
                .then(alarmRules => {
                    alarmRules.forEach((rule, ruleIndex) => {
                        const ruleElement = createRuleElement(relay.id, rule, ruleIndex + 1);
                        relayForm.appendChild(ruleElement);
                        ruleIndexTemp = ruleIndex + 1; // Increment ruleIndexTemp correctly
                    });

                    // Create a new rule element for the relay for adding a new rule
                    const newRuleElement = createRuleElement(relay.id, { state: 'on', time: '00:00:00', days: [] }, ruleIndexTemp + 1);

                    // Edit Delete button to Add Rule button with class add-rule
                    const newButton = newRuleElement.querySelector(`#relay-${relay.id}-delete-rule-${ruleIndexTemp + 1}`);
                    newButton.textContent = 'Add Rule';
                    newButton.classList.add('add-rule');

                    // Append the new rule element to the relay form
                    relayForm.appendChild(newRuleElement);

                    // Create save button
                    const saveButton = document.createElement('button');
                    saveButton.type = 'submit';
                    saveButton.id = `save-relay-${relay.id}-rules`;
                    saveButton.classList.add('submit-button');
                    saveButton.textContent = 'Save Changes';

                    relayForm.appendChild(saveButton);
                    relayDiv.appendChild(relayForm);
                    relayAlarmRulesDiv.appendChild(relayDiv);
                });
        });
    }

    // Fetch and load the HTML template, then fetch relays and update the relay-alarm-rules section
    loadTemplate()
        .then(() => {
            fetch('/api/all-relays')
                .then(response => response.json())
                .then(relays => {
                    updateRelayAlarmRules(relays);
                })
                .catch(error => console.error('Error fetching relays:', error));
        })
        .catch(error => console.error('Error loading template:', error));
});
