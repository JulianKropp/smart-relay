#include "alarm.h"
#include <iostream>
#include <fstream>

uint Alarm::idCounter = 0;

Alarm::Alarm(uint hour, uint minute, uint second, std::array<bool, 7> weekdays, Relay *relay, bool state)
    : hour(hour), minute(minute), second(second), weekdays(weekdays), relay(relay), state(state)
{
    this->id = Alarm::idCounter++;
}

Alarm::Alarm(String json, Relay *relay)
{
    DynamicJsonDocument doc(256);
    DeserializationError error = deserializeJson(doc, json);
    if (error)
    {
        std::cerr << "Failed to deserialize alarm json: " << error.c_str() << std::endl;
        return;
    }

    uint id = doc["id"].as<uint>();
    if (id < idCounter)
    {
        id = idCounter++;
    } else {
        idCounter = id + 1;
    }

    id = id;
    hour = doc["hour"];
    minute = doc["minute"];
    second = doc["second"];
    JsonArray weekdaysArray = doc["weekdays"].as<JsonArray>();
    int counter = 0;
    for(JsonVariant v : weekdaysArray) {
        weekdays[counter++] = v.as<bool>();
    }
    this->relay = relay;
    state = doc["state"].as<bool>();

    // Set last alarm to 0
    lastAlarm = DateTime(0, 0, 0, 0, 0, 0);
}

int Alarm::getId()
{
    return id;
}

uint Alarm::getHour() const
{
    return hour;
}

uint Alarm::getMinute() const
{
    return minute;
}

uint Alarm::getSecond() const
{
    return second;
}

std::array<bool, 7> Alarm::getWeekdays() const
{
    return weekdays;
}

Relay *Alarm::getRelay() const
{
    return relay;
}

bool Alarm::getState() const
{
    return state;
}

DateTime Alarm::lastTimeTriggert() const
{
    return lastAlarm;
}

void Alarm::turnOn()
{
    relay->On();
    this->lastAlarm = RTC::getInstance()->now();
}

void Alarm::turnOff()
{
    relay->Off();
    this->lastAlarm = RTC::getInstance()->now();
}

void Alarm::setHour(const uint hour)
{
    this->hour = hour;
}

void Alarm::setMinute(const uint minute)
{
    this->minute = minute;
}

void Alarm::setSecond(const uint second)
{
    this->second = second;
}

void Alarm::setWeekdays(const std::array<bool, 7> weekdays)
{
    this->weekdays = weekdays;
}

void Alarm::setRelay(Relay *relay)
{
    this->relay = relay;
}

void Alarm::setState(bool state)
{
    this->state = state;
}

// This will check if alarm is in time range between now and the past 24h from now
bool Alarm::checkAlarm(DateTime now) const {

    uint sec = this->getNextAlarminSeconds(now);
    DateTime last = this->lastAlarm;

    if (sec == 0 || sec < 7*24*59*60 && last > now - TimeSpan(0, 0, 1, 0)) {
        return true;
    }

    return false;
}



uint Alarm::getNextAlarminSeconds(DateTime now) const
{
    // get weekday
    int weekday = now.dayOfTheWeek();

    for (int i = 0; i < 7; i++)
    {
        int day = (weekday + i) % 7;
        if (weekdays[day])
        {
            // Calculate the next alarm time
            DateTime nextAlarm = DateTime(now.year(), now.month(), now.day() + i, hour, minute, second);

            // Check if the alarm is in the past
            if (nextAlarm < now)
            {
                continue;
            }

            return (nextAlarm - now).totalseconds();
        }
    }

    return -1;
}

String Alarm::toJson() const
{
    JsonDocument doc;
    doc["id"] = id;
    doc["hour"] = hour;
    doc["minute"] = minute;
    doc["second"] = second;
    doc.createNestedArray("weekdays");
    for (bool weekday : weekdays)
    {
        doc["weekdays"].add(weekday);
    }
    doc["relay"] = relay->getId();
    doc["state"] = state;
    String jsonString;
    serializeJson(doc, jsonString);
    return jsonString;
}