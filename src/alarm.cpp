#include "alarm.h"
#include <iostream>
#include <fstream>

Alarm::Alarm(uint hour, uint minute, uint second, std::array<bool, 7> weekdays, Relay* relay, uint id)
    : hour(hour), minute(minute), second(second), weekdays(weekdays), relay(relay), id(id) {
}

int Alarm::getId() {
    return id;
}

uint Alarm::getHour() const {
    return hour;
}

uint Alarm::getMinute() const {
    return minute;
}

uint Alarm::getSecond() const {
    return second;
}

std::array<bool, 7> Alarm::getWeekdays() const {
    return weekdays;
}

Relay* Alarm::getRelay() const {
    return relay;
}

void Alarm::setHour(const uint hour) {
    this->hour = hour;
}

void Alarm::setMinute(const uint minute) {
    this->minute = minute;
}

void Alarm::setSecond(const uint second) {
    this->second = second;
}

void Alarm::setWeekdays(const std::array<bool, 7> weekdays) {
    this->weekdays = weekdays;
}

void Alarm::setRelay(Relay* relay) {
    this->relay = relay;
}
