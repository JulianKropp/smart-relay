#include <Arduino.h>
#include "relayManager.h"
#include "configManager.h"
#include "alarm.h"

// // Define relay instances
// Relay relay1 = Relay(26, "relay1", 1);
// Relay relay2 = Relay(25, "relay2", 1);
// Relay relay3 = Relay(33, "relay3", 1);
// Relay relay4 = Relay(32, "relay4", 1);

// // Define an array of relay objects
// Relay relays[4] = {relay1, relay2, relay3, relay4};

RelayManager rm = RelayManager();
ConfigManager& cm = ConfigManager::getInstance();

void setup() {
    Serial.begin(115200);

    ConfigManager& cm = ConfigManager::getInstance();

    // Set the configuration value
    String name = cm.getConfig("device_name", "");

    Serial.println("Device name: " + name);

    if (name == "") {
        cm.setConfig("device_name", "ESP32");
    }

    Relay* r1 = rm.addRelay(32, String("Relay 1"), 1);
    Relay* r2 = rm.addRelay(33, String("Relay 2"), 2);
    Relay* r3 = rm.addRelay(25, String("Relay 3"), 3);
    Relay* r4 = rm.addRelay(26, String("Relay 4"), 4);

    // Create test alarms
    r1->addAlarm(8, 0, 0, {true, true, true, true, true, true, true}, 1);

    std::vector<uint> alarmids = r1->getAlarmIDs();
    for (auto const& id : alarmids) {
        Alarm* alarm = r1->getAlarmByID(id);
        Serial.println("Alarm ID: " + String(alarm->getId()));
        Serial.println("Alarm Hour: " + String(alarm->getHour()));
        Serial.println("Alarm Minute: " + String(alarm->getMinute()));
        Serial.println("Alarm Second: " + String(alarm->getSecond()));
        std::array<bool, 7> weekdays = alarm->getWeekdays();
        for (int i = 0; i < 7; i++) {
            Serial.println("Weekday " + String(i) + ": " + String(weekdays[i]));
        }
    }

}

void loop() {
    // Iterate through the relays array and turn each relay on and off
    vector<uint> relayids = rm.getRelayIDs();
    for (auto const& id : relayids) {
        Relay* relay = rm.getRelayByID(id);
        Serial.println("Relay ID: " + String(relay->getId()));
        Serial.println("Relay Name: " + relay->getName());
        Serial.println("Relay Pin: " + String(relay->getPin()));
        Serial.println("Relay State: " + String(relay->getState()));
        relay->On();
        delay(1000);
        relay->Off();
        delay(1000);
    }
}
