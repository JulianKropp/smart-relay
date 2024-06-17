#pragma once
#include "relay.h"
#include "rtc.h"
#include <map>
#include <vector>
#include <tuple>
#include <algorithm>
#include <queue>

using std::vector;

class RelayManager
{
private:
    std::map<uint, Relay *> relays; // Add the std:: namespace qualifier

public:
    RelayManager();
    RelayManager(String json);
    ~RelayManager();

    Relay *addRelay(const uint8_t pin, const String &name);
    vector<uint> getRelayIDs() const;
    Relay *getRelayByID(const uint id) const;
    void removeRelayByID(const uint id);

    std::queue<std::vector<Alarm *>> getNextAlarm() const;

    String toJson() const;
};