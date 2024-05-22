#include <Arduino.h>
#include "relayManager.h"
#include "configManager.h"

#include <map>

std::map<uint, uint8_t> relays;
RelayManager relayManager;

void setup() {
    Serial.begin(115200);
    

    relayManager.loadRelays();
    Serial.println(relayManager.toJSON());
    // This will add a relay if it doesnt exist
    relayManager.addRelay(26, String("Relay 1"), 1);
    relayManager.addRelay(25, String("Relay 2"), 2);
    relayManager.addRelay(33, String("Relay 3"), 3);
    relayManager.addRelay(32, String("Relay 4"), 4);
    relayManager.saveRelays();

    Serial.println(relayManager.toJSON());
}

void loop() {
    // The main logic is in the setup function in this example

}