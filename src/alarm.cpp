#include "alarm.h"
#include <iostream>
#include <fstream>

uint Alarm::idCounter = 0;

Alarm::Alarm(uint hour, uint minute, uint second, std::array<bool, 7> weekdays, Relay *relay)
    : hour(hour), minute(minute), second(second), weekdays(weekdays), relay(relay)
{
    this->id = Alarm::idCounter++;
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

uint Alarm::getNextAlarminSeconds(DateTime now) const {
    // get weekday
    int weekday = now.dayOfTheWeek();

    for (int i = 0; i < 7; i++) {
        int day = (weekday + i) % 7;
        if (weekdays[day]) {
            // Calculate the next alarm time
            DateTime nextAlarm = DateTime(now.year(), now.month(), now.day() + i, hour, minute, second);

            // Check if the alarm is in the past
            if (nextAlarm < now) {
                continue;
            }

            return (nextAlarm - now).totalseconds();
        }
    }

    return -1;
}

String Alarm::toJson() const {
    JsonDocument doc;
    doc["id"] = id;
    doc["hour"] = hour;
    doc["minute"] = minute;
    doc["second"] = second;
    doc.createNestedArray("weekdays");
    for (bool weekday : weekdays) {
        doc["weekdays"].add(weekday);
    }
    doc["relay"] = relay->getId();
    String jsonString;
    serializeJson(doc, jsonString);
    return jsonString;
}