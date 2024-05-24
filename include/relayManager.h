#pragma once
#include "relay.h"
#include "rtc.h"
#include <map>
#include <vector>
#include <tuple>
#include <algorithm>
#include <queue>


using std::vector;

class RelayManager {
    private:
        static uint idCounter;
        static bool isCounterLoaded;
        void loadIdCounter();
        void saveIdCounter();

        std::map<uint, Relay*> relays; // Add the std:: namespace qualifier
        RTC* rtc;

        public:
            RelayManager(RTC* rtc);

            Relay* addRelay(const uint8_t pin, const String& name, uint id = 0);
            vector<uint> getRelayIDs() const;
            Relay* getRelayByID(const uint id) const;
            void removeRelayByID(const uint id);

            void loadRelays();
            void saveRelays();
            String toJSON() const;

            std::queue<std::vector<Alarm*>> getNextAlarm() const;
};