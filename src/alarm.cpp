#include "alarm.h"
#include <iostream>
#include <fstream>

DateTime Alarm::calculateLastAlarm() const {
    DateTime lastA = DateTime(0, 0, 0, 0, 0, 0);

    // get last weeeks alarm
    DateTime now = RTC::getInstance()->now();
    // get weekday
    uint8_t weekday = now.dayOfTheWeek();

    for (int i = 0; i < 8; i++)
    {
        int day = ((weekday - i) % 7 + 7) % 7;
        Serial.println("Checking day last: " + String(day) + " " + String(this->weekdays[day]));
        if (this->weekdays[day]) {
            // Calculate the last alarm time
            DateTime la = DateTime(now.year(), now.month(), now.day(), this->hour, this->minute, this->second);
            la = la - TimeSpan(i, 0, 0, 0);

            if (la < now) {
                lastA = la;
                break;
            }
        }
    }

    Serial.println("Last alarm: " + String(lastA.year()) + "-" + String(lastA.month()) + "-" + String(lastA.day()) + " " + String(lastA.hour()) + ":" + String(lastA.minute()) + ":" + String(lastA.second()));
    return lastA;
}

uint Alarm::idCounter = 0;

Alarm::Alarm(uint hour, uint minute, uint second, std::array<bool, 7> weekdays, Relay *relay, bool state)
    : hour(hour), minute(minute), second(second), weekdays(weekdays), relay(relay), state(state)
{
    this->id = Alarm::idCounter++;

    this->lastAlarm = this->calculateLastAlarm();
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
    this->lastAlarm = this->calculateLastAlarm();
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
    return this->lastAlarm;
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

    Serial.println("if ((" + String(sec) + " == 0 || (" + String(sec) + " > 594720 && " + String(sec) + " <= 604800)) && " + String(last < (now - TimeSpan(0, 0, 1, 0))) + "))");
    if ((sec == 0 || ((sec > (7*24*60*60-60)) && (sec <= (7*24*60*60)))) && (last < (now - TimeSpan(0, 0, 1, 0)))) {
        Serial.println("Alarm is in time range");
        return true;
    }
    Serial.println("Alarm is not in time range");

    return false;
}



uint Alarm::getNextAlarminSeconds(DateTime now) const
{
    // get weekday
    uint8_t weekday = now.dayOfTheWeek();

    for (int i = 0; i < 8; i++)
    {
        int day = (weekday + i) % 7;
        Serial.println("Checking day: " + String(day) + " " + String(weekdays[day]));
        if (weekdays[day])
        {
            // Calculate the next alarm time
            DateTime nextAlarm = DateTime(now.year(), now.month(), now.day(), hour, minute, second);
            nextAlarm = nextAlarm + TimeSpan(i, 0, 0, 0);

            // Check if the alarm is in the past
            if (nextAlarm < now)
            {
                continue;
            }

            Serial.println("Now:        " + String(now.year()) + "-" + String(now.month()) + "-" + String(now.day()) + " " + String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second()));
            Serial.println("Next alarm: " + String(nextAlarm.year()) + "-" + String(nextAlarm.month()) + "-" + String(nextAlarm.day()) + " " + String(nextAlarm.hour()) + ":" + String(nextAlarm.minute()) + ":" + String(nextAlarm.second()));

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