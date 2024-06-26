#include "relay.h"
#include "alarm.h"
#include "configManager.h"

uint Relay::idCounter = 0;

Relay::Relay(const uint8_t pin, const String& name) {
    this->id = idCounter++;
    this->name = name;
    this->pin = pin;

    pinMode(pin, OUTPUT);
    this->Off();
}

Relay::Relay(String json) {
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, json);

    uint id = doc["id"].as<uint>();
    if (id < idCounter)
    {
        id = idCounter++;
    } else {
        idCounter = id + 1;
    }

    this->id = id;
    this->name = doc["name"].as<String>();
    this->pin = doc["pin"];

    pinMode(pin, OUTPUT);
    this->Off();

    JsonArray alarmsArray = doc["alarms"];
    for (JsonVariant alarm : alarmsArray) {
        String alarmJson;
        serializeJson(alarm, alarmJson);
        Alarm* tempAlarm = new Alarm(alarmJson, this);
        this->alarms[tempAlarm->getId()] = tempAlarm;
    }
}

Relay::~Relay() {
    for (auto const& element : this->alarms) {
        delete element.second;
    }
}

uint Relay::getId() {
    return this->id;
}

String Relay::getName() {
    return this->name;
}

void Relay::setName(const String& name) {
    this->name = name;
}

uint8_t Relay::getPin() {
    return this->pin;
}

bool Relay::getState() {
    int state = digitalRead(this->pin);
    return state == LOW;
}

void Relay::On() {
    digitalWrite(this->pin, LOW);
}

void Relay::Off() {
    digitalWrite(this->pin, HIGH);
}

Alarm* Relay::addAlarm(uint hour, uint minute, uint second, std::array<bool, 7> weekdays, bool state) {
    Alarm* tempAlarm = new Alarm(hour, minute, second, weekdays, this, state);
    this->alarms[tempAlarm->getId()] = tempAlarm;

    return tempAlarm;
}

void Relay::removeAlarm(const uint id) {
    auto it = alarms.find(id);
    if (it != alarms.end()) {
        delete it->second;
        alarms.erase(it);
    }
}

vector<uint> Relay::getAlarmIDs() const {
    vector<uint> ids;
    for (auto const& element : alarms) {
        ids.push_back(element.first);
    }
    return ids;
}

Alarm* Relay::getAlarmByID(const uint id) const {
    auto it = alarms.find(id);
    if (it != alarms.end()) {
        return it->second;
    } else {
        return nullptr;
    }
}

String Relay::toJson() const {
    DynamicJsonDocument doc(1024);
    doc["id"] = this->id;
    doc["name"] = this->name;
    doc["pin"] = this->pin;

    JsonArray alarmsArray = doc.createNestedArray("alarms");
    for (auto const& element : this->alarms) {
        Alarm* alarm = element.second;
        DynamicJsonDocument alarmDoc(1024);
        alarmDoc["id"] = alarm->getId();
        alarmDoc["hour"] = alarm->getHour();
        alarmDoc["minute"] = alarm->getMinute();
        alarmDoc["second"] = alarm->getSecond();
        alarmDoc.createNestedArray("weekdays");
        for (int i = 0; i < 7; i++) {
            alarmDoc["weekdays"][i] = alarm->getWeekdays()[i];
        }
        alarmDoc["relay"] = alarm->getRelay()->getId();
        alarmDoc["state"] = alarm->getState();
        alarmsArray.add(alarmDoc);
    }

    String output;
    serializeJson(doc, output);
    return output;
}