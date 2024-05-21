#include "relay.h"

Relay::Relay(int id, String name, uint8_t pin) {
    this->id = id;
    this->name = name;
    this->pin = pin;

    pinMode(pin, OUTPUT);
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

void Relay::setState(bool state) {
    digitalWrite(this->pin, state ? HIGH : LOW);
}

String Relay::toJson() {
    bool state = this->getState();
    return "{\"id\": " + String(this->id) + ", \"name\": \"" + this->name + "\", \"state\": \"" + (state ? "on" : "off") + "\"}";
}