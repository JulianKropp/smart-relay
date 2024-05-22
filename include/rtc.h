#pragma once
#include "RTClib.h"
#include <Wire.h>
#include <Arduino.h>

class RTC
{
private:
    uint8_t sdaPin, sclPin;
    RTC_DS3231 rtc;

public:
    RTC(uint8_t sdaPin, uint8_t sclPin) : sdaPin(sdaPin), sclPin(sclPin) {};

    void begin();

    DateTime now();
};