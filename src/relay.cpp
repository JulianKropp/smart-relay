#include "relay.h"
#include "configManager.h"



Relay::Relay(const uint8_t pin, const String& name, const uint id) {
    this->id = id;
    this->name = name;
    this->pin = pin;

    pinMode(pin, OUTPUT);
    this->Off();
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

String Relay::toJson() {
    bool state = this->getState();
    return "{\"id\": " + String(this->id) + ", \"name\": \"" + this->name + "\", \"state\": \"" + (state ? "on" : "off") + "\"}";
}