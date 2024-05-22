#pragma once
#include <Arduino.h>


class Relay {
    private:
        uint id;
        String name;
        uint8_t pin;

    public:
        Relay(const uint8_t pin, const String& name, const uint id);
        int getId();
        String getName();
        uint8_t getPin();
        bool getState();
        void On();
        void Off();
        String toJson();
};
