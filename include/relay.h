#pragma once
#include "alarm.h"
#include <Arduino.h>
#include <map>
#include <vector>


// Forward declaration of Relay
class Alarm;

using std::vector;

class Relay {
    private:
        static uint idCounter;
        static bool isCounterLoaded;
        void loadIdCounter();
        void saveIdCounter();

        uint id;
        String name;
        uint8_t pin;

        std::map<uint, Alarm*> alarms;

    public:
        Relay(const uint8_t pin, const String& name, const uint id);
        ~Relay();
        int getId();
        String getName();
        uint8_t getPin();
        bool getState();
        void On();
        void Off();

        Alarm* addAlarm(uint hour, uint minute, uint second, std::array<bool, 7> weekdays, uint id = 0);
        void removeAlarm(const uint id);
        vector<uint> getAlarmIDs() const;
        Alarm* getAlarmByID(const uint id) const;
};
