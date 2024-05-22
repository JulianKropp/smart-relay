#pragma once
#include <map>
#include "relay.h"
#include <vector>


using std::vector;

class RelayManager {
    private:
        static uint idCounter;
        static bool isCounterLoaded;
        void loadIdCounter();
        void saveIdCounter();

        std::map<uint, Relay*> relays; // Add the std:: namespace qualifier

        public:
            RelayManager();

            Relay* addRelay(const uint8_t pin, const String& name, uint id = 0);
            vector<uint> getRelayIDs() const;
            Relay* getRelayByID(const uint id) const;
            void removeRelay(Relay* relay);

            void loadRelays();
            void saveRelays();
            String toJSON() const;
};