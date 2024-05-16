document.addEventListener("DOMContentLoaded", function () {
    const relayList = document.getElementById("relay-list");
    relayList.innerHTML = "<p>Loading...</p>";

    // Function to fetch relay data from the API
    function fetchRelays() {
        return fetch("/api/all-relays")
            .then(response => response.json())
            .catch(error => {
                console.error('Error fetching relay data:', error);
                relayList.innerHTML = "<p>Error fetching relay data</p>";
                throw error;
            });
    }

    // Function to update the UI with the fetched relay data
    function updateUI(relays) {
        if (relayList.innerHTML === "<p>Loading...</p>" || relayList.innerHTML === "<p>Error fetching relay data</p>") {
            relayList.innerHTML = "";
        }

        if (!relays || relays.length === 0) {
            relayList.innerHTML = "<p>No relays found</p>";
            return;
        }

        const currentRelayIds = new Set(relays.map(relay => relay.id));

        // Remove non-existent relays
        document.querySelectorAll('.relay-control').forEach(node => {
            const nodeId = node.id.replace('relay-control', '');
            if (!currentRelayIds.has(parseInt(nodeId))) {
                node.remove();
            }
        });

        relays.forEach(relay => {
            let relayControl = document.getElementById(`relay-control${relay.id}`);

            // If the relay element is not found, add it
            if (!relayControl) {
                relayControl = document.createElement("div");
                relayControl.classList.add("relay-control");
                relayControl.id = `relay-control${relay.id}`;
                relayControl.innerHTML = `
                    <h2 id="relaySwitchName${relay.id}">${relay.name}</h2>
                    <div class="switch">
                        <input type="checkbox" id="relaySwitch${relay.id}" class="relay-checkbox">
                        <label class="toggle-label" for="relaySwitch${relay.id}"></label>
                    </div>
                `;

                relayList.appendChild(relayControl);

                // Add event listener to the switch
                const relaySwitch = document.getElementById(`relaySwitch${relay.id}`);
                relaySwitch.addEventListener('change', function () {
                    const newState = this.checked ? 'on' : 'off';
                    const data = { relayId: relay.id, state: newState };

                    fetch('/api/relay-control', {
                        method: 'POST',
                        headers: {
                            'Content-Type': 'application/json',
                        },
                        body: JSON.stringify(data)
                    })
                        .then(response => response.json())
                        .then(data => console.log('Success:', data))
                        .catch(error => console.error('Error:', error));
                });
            }

            // Update the name of the switch
            const switchName = document.getElementById(`relaySwitchName${relay.id}`);
            if (switchName.textContent !== relay.name) {
                switchName.textContent = relay.name;
            }

            // Update the state of the switch if the relay already exists
            const switchElement = document.getElementById(`relaySwitch${relay.id}`);
            if (switchElement) {
                switchElement.checked = relay.state === "on";
            }
        });
    }

    // Fetch data initially when the page loads and update UI
    fetchRelays().then(updateUI);

    // Function to continuously update relay data
    function updateRelayData() {
        setInterval(() => {
            fetchRelays().then(updateUI);
        }, 1000);
    }

    // Call the function to start data updating
    updateRelayData();
});
