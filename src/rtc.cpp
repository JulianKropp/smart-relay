#include "rtc.h"

void RTC::begin()
{
    Wire.begin(sdaPin, sclPin);
    while(true) {
        if (rtc.begin()) {
            break;
        }
        Serial.println("Couldn't find RTC");
        delay(5000);
    }

    if (rtc.lostPower())
    {
        Serial.println("RTC lost power, setting the time!");
        // Set the date and time at compile time
        rtc.adjust(DateTime(2020, 2, 1, 0, 0, 0));
    }
}

DateTime RTC::now()
{
    return rtc.now();
}

void RTC::setDateTime(const DateTime &dt)
{
    rtc.adjust(dt);
}