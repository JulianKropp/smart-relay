#include "relayManager.h"
#include "relay.h"
#include "configManager.h"

uint RelayManager::idCounter = 0;
bool RelayManager::isCounterLoaded = false;

void RelayManager::loadIdCounter()
{
    if (!RelayManager::isCounterLoaded)
    {
        ConfigManager& cm = ConfigManager::getInstance();

        String idCounter = cm.getConfig("relay_id_counter", "1");

        RelayManager::idCounter = (uint)idCounter.toInt();

        isCounterLoaded = true;
    }
}

void RelayManager::saveIdCounter()
{
    String idCounter = String(this->idCounter);
    ConfigManager::getInstance().setConfig("relay_id_counter", idCounter);
}

RelayManager::RelayManager() {
}

Relay* RelayManager::addRelay(const uint8_t pin, const String& name, uint id) {
    if (id == 0) {
        this->loadIdCounter();
        id = this->idCounter++;
        this->saveIdCounter();
    } else {
        if (id > this->idCounter) {
            this->idCounter = id;
            this->saveIdCounter();
        }
    }

    Relay* tempRelay = this->getRelayByID(id);
    if (tempRelay != nullptr) {
        return tempRelay;
    } else {
        Relay* newRelay = new Relay(pin, name, id);
        this->relays[newRelay->getId()] = newRelay;
    }

    return tempRelay;
}

vector<uint> RelayManager::getRelayIDs() const {
    vector<uint> ids;
    for (auto const& element : relays) {
        ids.push_back(element.first);
    }
    return ids;
}

Relay* RelayManager::getRelayByID(const uint id) const {
    return relays.at(id);
}

void RelayManager::removeRelay(Relay* relay) {
    relays.erase(relay->getId());
}

void RelayManager::loadRelays() {
    ConfigManager& cm = ConfigManager::getInstance();
    String relayCount = cm.getConfig("relay_id_counter", "0");
    uint count = (uint)relayCount.toInt();

    for (uint i = 1; i <= count; i++) {
        String relayName = cm.getConfig("relay_" + String(i) + "_name", "");
        String relayPin = cm.getConfig("relay_" + String(i) + "_pin", "");

        if (relayName != "" || relayPin != "") {
            continue;
        }

        if (relayName != "" && relayPin != 0) {
            this->addRelay(((uint8_t) (relayPin.toInt())), relayName, i);
        }
    }
}

void RelayManager::saveRelays() {
    this->saveIdCounter();

    ConfigManager& cm = ConfigManager::getInstance();
    for (auto const& element : this->relays) {
        cm.setConfig("relay_" + String(element.first) + "_name", element.second->getName());
        cm.setConfig("relay_" + String(element.first) + "_pin", String(element.second->getPin()));
    }
}


String RelayManager::toJSON() const
{
    String json = "[";
    for (auto const &element : this->relays)
    {
        json += "{\"id\": " + String(element.first) + ", \"name\": \"" + element.second->getName() + "\", \"pin\": " + String(element.second->getPin()) + ", \"state\": " + String(element.second->getState()) + "},";
    }
    json.remove(json.length() - 1);
    json += "]";
    return json;
}