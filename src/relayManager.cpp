#include "relayManager.h"
#include "relay.h"
#include "configManager.h"

uint RelayManager::idCounter = 0;
bool RelayManager::isCounterLoaded = false;

void RelayManager::loadIdCounter()
{
    if (!RelayManager::isCounterLoaded)
    {
        ConfigManager &cm = ConfigManager::getInstance();

        String idCounter = cm.getConfig("relay_id_counter", "1");

        RelayManager::idCounter = (uint)idCounter.toInt();

        RelayManager::isCounterLoaded = true;
    }
}

void RelayManager::saveIdCounter()
{
    String idCounter = String(RelayManager::idCounter);
    ConfigManager::getInstance().setConfig("relay_id_counter", idCounter);
}

RelayManager::RelayManager(RTC *rtc)
{
    this->rtc = rtc;
}

Relay *RelayManager::addRelay(const uint8_t pin, const String &name, uint id)
{
    if (id == 0)
    {
        this->loadIdCounter();
        id = RelayManager::idCounter++;
        this->saveIdCounter();
    }
    else
    {
        if (id > RelayManager::idCounter)
        {
            RelayManager::idCounter = id;
            this->saveIdCounter();
        }
    }

    Relay *tempRelay = this->getRelayByID(id);
    if (tempRelay != nullptr)
    {
        return tempRelay;
    }
    else
    {
        tempRelay = new Relay(pin, name, id);
        this->relays[tempRelay->getId()] = tempRelay;
    }

    return tempRelay;
}

vector<uint> RelayManager::getRelayIDs() const
{
    vector<uint> ids;
    for (auto const &element : relays)
    {
        ids.push_back(element.first);
    }
    return ids;
}

Relay *RelayManager::getRelayByID(const uint id) const
{
    auto it = relays.find(id);
    if (it != relays.end())
    {
        return it->second;
    }
    else
    {
        return nullptr;
    }
}

void RelayManager::removeRelayByID(const uint id)
{
    auto it = relays.find(id);
    if (it != relays.end())
    {
        relays.erase(it);
        delete it->second;
    }
}

void RelayManager::loadRelays()
{
    ConfigManager &cm = ConfigManager::getInstance();
    String relayCount = cm.getConfig("relay_id_counter", "0");
    uint count = (uint)relayCount.toInt();

    for (uint i = 1; i <= count; i++)
    {
        String relayName = cm.getConfig("relay_" + String(i) + "_name", "");
        String relayPin = cm.getConfig("relay_" + String(i) + "_pin", "");

        if (relayName != "" || relayPin != "")
        {
            continue;
        }

        if (relayName != "" && relayPin != 0)
        {
            this->addRelay(((uint8_t)(relayPin.toInt())), relayName, i);
        }
    }
}

void RelayManager::saveRelays()
{
    this->saveIdCounter();

    ConfigManager &cm = ConfigManager::getInstance();
    for (auto const &element : this->relays)
    {
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

std::queue<std::vector<Alarm*>> RelayManager::getNextAlarm() const {

    std::vector<Alarm*> alarms;
    vector<uint> relayIDs = this->getRelayIDs();
    for (auto const &element : relayIDs)
    {
        Relay *relay = this->getRelayByID(element);
        if (relay != nullptr)
        {
            vector<uint> alarmIDs = relay->getAlarmIDs();
            for (auto const &alarmID : alarmIDs)
            {
                Alarm *alarm = relay->getAlarmByID(alarmID);
                if (alarm != nullptr)
                {
                    alarms.push_back(alarm);
                }
            }
        }
    }

    DateTime now = this->rtc->now();

    std::queue<std::vector<Alarm*>> alarmQueue;
    for (int i = 0; i < 7; i++)
    {
        std::vector<Alarm*> alarmsOfDay;

        
        int weekday = (now.dayOfTheWeek() + i) % 7;
        for (auto const &element : alarms)
        {
            if (element->getWeekdays()[weekday])
            {
                alarmsOfDay.push_back(element);
            }
        }


        // sort alarmsOfDay by element->getNextAlarminSeconds(midnight);
        DateTime midnight = DateTime(now.year(), now.month(), now.day() + i, 0, 0, 0);
        std::sort(alarmsOfDay.begin(), alarmsOfDay.end(), [midnight](Alarm* a, Alarm* b) {
            return a->getNextAlarminSeconds(midnight) < b->getNextAlarminSeconds(midnight);
        });

        // group alarms which have the same next alarm time together
        std::vector<Alarm*> groupedAlarms;
        for (auto const &element : alarmsOfDay)
        {
            if (groupedAlarms.size() == 0)
            {
                groupedAlarms.push_back(element);
                // if last element add to queue
                if (element == alarmsOfDay.back())
                {
                    alarmQueue.push(groupedAlarms);
                }
            }
            else
            {
                if (element->getNextAlarminSeconds(midnight) == groupedAlarms[0]->getNextAlarminSeconds(midnight))
                {
                    groupedAlarms.push_back(element);
                }
                else
                {
                    alarmQueue.push(groupedAlarms);
                    groupedAlarms.clear();
                    groupedAlarms.push_back(element);
                }
            }
        }
    }

    return alarmQueue;
}