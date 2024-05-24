#include <Arduino.h>
#include <array>
#include "relay.h"
#include "alarm.h"
#include "configManager.h"
#include "relayManager.h"
#include "rtc.h"

RTC rtc(22, 23);

// Testfunktion für die Klasse Alarm
void testAlarm()
{
    std::array<bool, 7> weekdays1 = {true, true, true, true, true, false, false};
    std::array<bool, 7> weekdays2 = {false, true, true, true, true, true, false};
    Relay relay1(32, "Test Relay 1");
    Relay relay2(33, "Test Relay 2");

    Alarm alarm1(7, 30, 0, weekdays1, &relay1);
    Alarm alarm2(7, 30, 0, weekdays2, &relay2);
    Alarm alarm3(8, 0, 0, weekdays1, &relay1);

    Serial.print(alarm1.getNextAlarminSeconds(rtc.now()));
}

// Testfunktion für die Klasse ConfigManager
void testConfigManager()
{
    ConfigManager &config = ConfigManager::getInstance();

    config.setConfig("wifi_ssid", "MyNetwork");
    config.setConfig("wifi_password", "MyPassword");

    Serial.print("WiFi SSID: ");
    Serial.println(config.getConfig("wifi_ssid"));
    Serial.print("WiFi Password: ");
    Serial.println(config.getConfig("wifi_password"));
}

// Testfunktion für die Klasse Relay
void testRelay()
{
    Relay relay(33, "Living Room Light");

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
void testRelayManager()
{
    RelayManager relayManager(&rtc);

    Relay *relay1 = relayManager.addRelay(25, "Bedroom Light");
    Relay *relay2 = relayManager.addRelay(26, "Kitchen Light");

    std::vector<uint> relayIDs = relayManager.getRelayIDs();
    Serial.print("Relay IDs: ");
    for (uint id : relayIDs)
    {
        Serial.print(id);
        Serial.print(" ");
    }
    Serial.println();

    Relay *retrievedRelay = relayManager.getRelayByID(3);
    if (retrievedRelay)
    {
        Serial.print("Retrieved Relay Name: ");
        Serial.println(retrievedRelay->getName());
    }

    relayManager.removeRelayByID(4);
    relayIDs = relayManager.getRelayIDs();
    Serial.print("Relay IDs after removal: ");
    for (uint id : relayIDs)
    {
        Serial.print(id);
        Serial.print(" ");
    }
    Serial.println();
}

// void setup() {
//     Serial.begin(115200);

//     rtc.begin();

//     Serial.println("Starting tests...");

//     testAlarm();
//     testConfigManager();
//     testRelay();
//     testRelayManager();
// }

void setup()
{
    Serial.begin(115200);

    rtc.begin();
    DateTime now(2024, 1, 1, 0, 0, 0);
    rtc.setDateTime(now);

    RelayManager rm(&rtc);
    Relay *r1 = rm.addRelay(32, "Test Relay 1");
    Relay *r2 = rm.addRelay(33, "Test Relay 2");
    Relay *r3 = rm.addRelay(25, "Test Relay 3");
    Relay *r4 = rm.addRelay(26, "Test Relay 4");

    std::array<bool, 7> weekdays1 = {false, true, false, false, false, false, false};
    std::array<bool, 7> weekdays2 = {false, false, true, false, false, false, false};
    std::array<bool, 7> weekdays3 = {false, false, false, true, false, false, false};
    std::array<bool, 7> weekdays4 = {true, false, false, false, false, false, false};

    Alarm* a1 = r1->addAlarm(1, 0, 0, weekdays1);
    r2->addAlarm(2, 0, 0, weekdays2);
    r3->addAlarm(3, 0, 0, weekdays3);
    r4->addAlarm(4, 0, 0, weekdays4);

    Serial.println(a1->toJson());
    Serial.println(r4->toJson());

    r2->removeAlarm(3);

    rm.removeRelayByID(4);

    std::queue<std::vector<Alarm *>> alarmQueue = rm.getNextAlarm();
    // get size of the queue
    Serial.println("Size of the queue: " + String(alarmQueue.size()));
    while (!alarmQueue.empty())
    {
        std::vector<Alarm *> alarms = alarmQueue.front();
        alarmQueue.pop();
        Serial.println("Size of the vector: " + String(alarms.size()));
        for (Alarm *alarm : alarms)
        {
            Serial.println("Alarm: " + String(String(alarm->getHour()) + ":" + String(alarm->getMinute()) + ":" + String(alarm->getSecond())));
        }
    }
}

// std::array<bool, 7> weekdays1 = {false, true, false, false, false, false, false};
// Relay relay1(32, "Test Relay 1", 1);

// Alarm alarm1(20, 22, 0, weekdays1, &relay1, 1);

void loop()
{
    DateTime now = rtc.now();
    uint day = now.dayOfTheWeek();
    uint hour = now.hour();
    uint minute = now.minute();
    uint second = now.second();

    // Serial.println("Next alarm: " + String(alarm1.getNextAlarminSeconds(rtc.now())));
    Serial.println("Time now: " + String(day) + ":" + String(hour) + ":" + String(minute) + ":" + String(second));
    delay(1000);
}
