#pragma once
#include "alarm.h"
#include <Arduino.h>
#include <map>
#include <vector>
#include <ArduinoJson.h>


// Forward declaration of Relay
class Alarm;

using std::vector;

class Relay {
    private:
        static uint idCounter;

        uint id;
        String name;
        uint8_t pin;

        std::map<uint, Alarm*> alarms;

    public:
        Relay(const uint8_t pin, const String& name);
        Relay(String json);
        ~Relay();
        int getId();
        String getName();
        void setName(const String& name);
        uint8_t getPin();
        bool getState();
        void On();
        void Off();

        Alarm* addAlarm(uint hour, uint minute, uint second, std::array<bool, 7> weekdays);
        void removeAlarm(const uint id);
        vector<uint> getAlarmIDs() const;
        Alarm* getAlarmByID(const uint id) const;

        String toJson() const;
};
