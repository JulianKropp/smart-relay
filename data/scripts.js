document.addEventListener("DOMContentLoaded", function () {
    const relayList = document.getElementById("relay-list");
    relayList.innerHTML = "<p>Loading...</p>";

    // Function to fetch relay data from the API and update the UI
    function fetchRelaysAndUpdateUI() {
        fetch("/api/all-relays")
            .then(response => response.json())
            .then(relays => {
                if(relayList.innerHTML === "<p>Loading...</p>") {
                    relayList.innerHTML = "";
                }

                if (!relays) {
                    // set there are no relays setup.
                    const relayList = document.getElementById("relay-list");
                    relayList.innerHTML = "<p>No relays found</p>";
                }

                relays.forEach(relay => {
                    const switchElement = document.getElementById(`relaySwitch${relay.id}`);

                    // if the switch element is not found, add it
                    if (!switchElement) {
                        /*
                            <div class="relay-control" id="relay-control1">
                                <h2>Relay 1</h2>
                                <div class="switch">
                                    <input type="checkbox" id="relaySwitch1" class="relay-checkbox">
                                    <label class="toggle-label" for="relaySwitch1"></label>
                                </div>
                            </div>
                        */ 
                        const relayControl = document.createElement("div");
                        relayControl.classList.add("relay-control");
                        relayControl.id = `relay-control${relay.id}`;
                        relayControl.innerHTML = `
                            <h2>Relay ${relay.id}</h2>
                            <div class="switch">
                                <input type="checkbox" id="relaySwitch${relay.id}" class="relay-checkbox">
                                <label class="toggle-label" for="relaySwitch${relay.id}"></label>
                            </div>
                        `;

                        relayList.appendChild(relayControl);
                    }


                    if (switchElement) {
                        switchElement.checked = (relay.state === "on");
                    }
                });
            })
            .catch(error => {
                console.error('Error fetching relay data:');

                // clear all relay data
                relayList.innerHTML = "<p>Error fetching relay data</p>";
            });
    }

    // Initial fetch on page load
    fetchRelaysAndUpdateUI();

    // Function to continuously update relay data every second
    function updateRelayData() {
        setInterval(fetchRelaysAndUpdateUI, 1000);
    }

    // Call the function to start updating relay data
    updateRelayData();
});
