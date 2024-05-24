#include <Arduino.h>
#include <array>
#include "relay.h"
#include "alarm.h"
#include "configManager.h"
#include "relayManager.h"
#include "rtc.h"

RTC rtc(15, 2);

// Testfunktion für die Klasse Alarm
void testAlarm() {
    std::array<bool, 7> weekdays1 = {true, true, true, true, true, false, false};
    std::array<bool, 7> weekdays2 = {false, true, true, true, true, true, false};
    Relay relay1(32, "Test Relay 1", 1);
    Relay relay2(33, "Test Relay 2", 2);

    Alarm alarm1(7, 30, 0, weekdays1, &relay1, 1);
    Alarm alarm2(7, 30, 0, weekdays2, &relay2, 2);
    Alarm alarm3(8, 0, 0, weekdays1, &relay1, 3);

    Serial.print(alarm1.getNextAlarminSeconds(rtc.now()));
}

// Testfunktion für die Klasse ConfigManager
void testConfigManager() {
    ConfigManager& config = ConfigManager::getInstance();

    config.setConfig("wifi_ssid", "MyNetwork");
    config.setConfig("wifi_password", "MyPassword");

    Serial.print("WiFi SSID: ");
    Serial.println(config.getConfig("wifi_ssid"));
    Serial.print("WiFi Password: ");
    Serial.println(config.getConfig("wifi_password"));
}

// Testfunktion für die Klasse Relay
void testRelay() {
    Relay relay(33, "Living Room Light", 2);

    Serial.print("Relay ID: ");
    Serial.println(relay.getId());
    Serial.print("Relay Name: ");
    Serial.println(relay.getName());
    Serial.print("Relay Pin: ");
    Serial.println(relay.getPin());

    relay.On();
    Serial.print("Relay State after On(): ");
    Serial.println(relay.getState());
    relay.Off();
    Serial.print("Relay State after Off(): ");
    Serial.println(relay.getState());
}

// Testfunktion für die Klasse RelayManager
void testRelayManager() {
    RelayManager relayManager(&rtc);

    Relay* relay1 = relayManager.addRelay(25, "Bedroom Light", 3);
    Relay* relay2 = relayManager.addRelay(26, "Kitchen Light", 4);

    std::vector<uint> relayIDs = relayManager.getRelayIDs();
    Serial.print("Relay IDs: ");
    for (uint id : relayIDs) {
        Serial.print(id);
        Serial.print(" ");
    }
    Serial.println();

    Relay* retrievedRelay = relayManager.getRelayByID(3);
    if (retrievedRelay) {
        Serial.print("Retrieved Relay Name: ");
        Serial.println(retrievedRelay->getName());
    }

    relayManager.removeRelayByID(4);
    relayIDs = relayManager.getRelayIDs();
    Serial.print("Relay IDs after removal: ");
    for (uint id : relayIDs) {
        Serial.print(id);
        Serial.print(" ");
    }
    Serial.println();
}

void setup() {
    Serial.begin(115200);

    rtc.begin();

    Serial.println("Starting tests...");

    testAlarm();
    testConfigManager();
    testRelay();
    testRelayManager();
}

std::array<bool, 7> weekdays1 = {false, true, false, false, false, false, false};
Relay relay1(32, "Test Relay 1", 1);

Alarm alarm1(20, 22, 0, weekdays1, &relay1, 1);

void loop() {
    DateTime now = rtc.now();
    uint day = now.dayOfTheWeek();
    uint hour = now.hour();
    uint minute = now.minute();
    uint second = now.second();

    Serial.println("Next alarm: " + String(alarm1.getNextAlarminSeconds(rtc.now())));
    Serial.println("Time now: " + String(day) + ":" + String(hour) + ":" + String(minute) + ":" + String(second));
    delay(1000);
}
