#include "relayManager.h"
#include "relay.h"
#include "configManager.h"

RelayManager::RelayManager()
{
}

RelayManager::RelayManager(String json)
{
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, json);

    this->name = doc["name"].as<String>();

    JsonArray relaysArray = doc["relays"];
    for (JsonVariant relay : relaysArray)
    {
        String relayJson;
        serializeJson(relay, relayJson);
        Relay *tempRelay = new Relay(relayJson);
        this->relays[tempRelay->getId()] = tempRelay;
    }
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
    Relay *tempRelay = new Relay(pin, name);
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

std::queue<std::vector<Alarm *>> RelayManager::getNextAlarm() const
{

    std::vector<Alarm *> alarms;
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

    DateTime now = RTC::getInstance()->now();

    // Sort alarms
    std::sort(alarms.begin(), alarms.end(), [now](Alarm *a, Alarm *b)
              { return a->getNextAlarminSeconds(now) < b->getNextAlarminSeconds(now); });

    std::queue<std::vector<Alarm *>> alarmQueue;

    // group alarms which have the same next alarm time together
    std::vector<Alarm *> groupedAlarms;
    for (auto const &element : alarms)
    {
        if (groupedAlarms.size() == 0)
        {
            groupedAlarms.push_back(element);
        }
        else
        {
            if (element->getNextAlarminSeconds(now) == groupedAlarms[0]->getNextAlarminSeconds(now))
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
    if (groupedAlarms.size() > 0) {
        alarmQueue.push(groupedAlarms);
    }

    return alarmQueue;
}

 String RelayManager::getName() const
{
    return this->name;
}

void RelayManager::setName(const String &name)
{
    this->name = name;
}

String RelayManager::toJson() const
{
    DynamicJsonDocument doc(1024);

    doc["name"] = this->name;

    JsonArray relaysArray = doc.createNestedArray("relays");
    for (auto const &element : this->relays)
    {
        Relay *relay = element.second;
        DynamicJsonDocument relayDoc(1024);
        relayDoc["id"] = relay->getId();
        relayDoc["name"] = relay->getName();
        relayDoc["pin"] = relay->getPin();
        JsonArray alarmsArray = relayDoc.createNestedArray("alarms");
        for (auto const &element : relay->getAlarmIDs())
        {
            Alarm *alarm = relay->getAlarmByID(element);
            DynamicJsonDocument alarmDoc(1024);
            alarmDoc["id"] = alarm->getId();
            alarmDoc["hour"] = alarm->getHour();
            alarmDoc["minute"] = alarm->getMinute();
            alarmDoc["second"] = alarm->getSecond();
            alarmDoc.createNestedArray("weekdays");
            for (int i = 0; i < 7; i++)
            {
                alarmDoc["weekdays"][i] = alarm->getWeekdays()[i];
            }
            alarmDoc["relay"] = alarm->getRelay()->getId();
            alarmDoc["state"] = alarm->getState();
            alarmsArray.add(alarmDoc);
        }
        relaysArray.add(relayDoc);
    }

    String output;
    serializeJson(doc, output);
    return output;
}