#include "relayManager.h"
#include "relay.h"
#include "configManager.h"

RelayManager::RelayManager(RTC *rtc)
{
    this->rtc = rtc;
}

RelayManager::~RelayManager()
{
    for (auto const &element : this->relays)
    {
        delete element.second;
    }
}

Relay *RelayManager::addRelay(const uint8_t pin, const String &name)
{
    Relay* tempRelay = new Relay(pin, name);
    this->relays[tempRelay->getId()] = tempRelay;

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