#pragma once
#include "relay.h"
#include <array>
#include <cstdint>

// Forward declaration of Relay
class Relay;

class Alarm {
    private:
        uint id;

        Relay* relay = nullptr;
        uint hour = 0;
        uint minute = 0;
        uint second = 0;
        std::array<bool, 7> weekdays = {false, false, false, false, false, false, false};

    public:
        Alarm(uint hour, uint minute, uint second, std::array<bool, 7> weekdays, Relay* relay, uint id = 0);

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
};
