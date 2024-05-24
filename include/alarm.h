#pragma once
#include "relay.h"
#include "rtc.h"
#include <array>
#include <cstdint>
#include <tuple>
#include <ArduinoJson.h>

// Forward declaration of Relay
class Relay;

class Alarm {
    private:
        static uint idCounter;
        uint id;

        Relay* relay = nullptr;
        uint hour = 0;
        uint minute = 0;
        uint second = 0;
        std::array<bool, 7> weekdays = {false, false, false, false, false, false, false}; // Sunday, Monday, Tuesday, Wednesday, Thursday, Friday, Saturday

    public:
        Alarm(uint hour, uint minute, uint second, std::array<bool, 7> weekdays, Relay* relay);

        // get methods
        int getId();
        uint getHour() const;
        uint getMinute() const;
        uint getSecond() const;
        std::array<bool, 7> getWeekdays() const;
        Relay* getRelay() const;

        // set methods
        void setHour(const uint hour);
        void setMinute(const uint minute);
        void setSecond(const uint second);
        void setWeekdays(const std::array<bool, 7> weekdays);
        void setRelay(Relay* relay);

        uint getNextAlarminSeconds(DateTime now) const; // This will return seconds from rtc now until this alarm will be executed

        String toJson() const;
};
