#include "rtc.h"

void RTC::begin()
{
    Wire.begin(sdaPin, sclPin);
    if (!rtc.begin())
    {
        Serial.println("Couldn't find RTC");
        while (1)
            ;
    }

    if (rtc.lostPower())
    {
        Serial.println("RTC lost power, setting the time!");
        // Set the date and time at compile time
        rtc.adjust(DateTime(2000, 1, 1, 0, 0, 0));
    }
}

DateTime RTC::now()
{
    return rtc.now();
}