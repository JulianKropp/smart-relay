#pragma once
#include <Arduino.h>


class Relay {
    private:
        int id;
        String name;
        uint8_t pin;

    public:
        Relay(int id, String name, uint8_t pin);
        int getId();
        String getName();
        uint8_t getPin();
        bool getState();
        void setState(bool state);
        String toJson();
};
