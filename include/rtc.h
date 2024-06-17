#pragma once
#include "RTClib.h"
#include <Wire.h>
#include <Arduino.h>

class RTC
{
private:
    uint8_t sdaPin, sclPin;
    RTC_DS3231 rtc;
    static RTC* instance; // Singleton instance

    // Private constructor
    RTC(uint8_t sdaPin, uint8_t sclPin) : sdaPin(sdaPin), sclPin(sclPin) {};

public:
    RTC(const RTC& other) = delete; // Disable copy constructor
    void operator=(const RTC&) = delete; // Disable assignment operator

    static RTC* getInstance(uint8_t sdaPin = 21, uint8_t sclSclPin = 22); // Method to get the instance

    DateTime now();

    void setDateTime(const DateTime& dt);
};
