#include "relay.h"
#include "alarm.h"
#include "configManager.h"

uint Relay::idCounter = 0;

Relay::Relay(const uint8_t pin, const String& name, const uint id) {
    this->id = id;
    this->name = name;
    this->pin = pin;

    pinMode(pin, OUTPUT);
    this->Off();
}

Relay::~Relay() {
    for (auto const& element : this->alarms) {
        delete element.second;
    }
}

int Relay::getId() {
    return this->id;
}

String Relay::getName() {
    return this->name;
}

uint8_t Relay::getPin() {
    return this->pin;
}

bool Relay::getState() {
    int state = digitalRead(this->pin);
    return state == HIGH;
}

void Relay::On() {
    digitalWrite(this->pin, HIGH);
}

void Relay::Off() {
    digitalWrite(this->pin, LOW);
}

Alarm* Relay::addAlarm(uint hour, uint minute, uint second, std::array<bool, 7> weekdays, uint id) {
    id = id == 0 ? Relay::idCounter++ : id;

    Alarm* tempAlarm = this->getAlarmByID(id);
    if (tempAlarm != nullptr) {
        return tempAlarm;
    } else {
        tempAlarm = new Alarm(hour, minute, second, weekdays, this, id);
        this->alarms[tempAlarm->getId()] = tempAlarm;
    }

    return tempAlarm;
}

void Relay::removeAlarm(const uint id) {
    auto it = alarms.find(id);
    if (it != alarms.end()) {
        alarms.erase(it);
        delete it->second;
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