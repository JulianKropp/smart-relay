#include "rtc.h"

RTC *RTC::instance = nullptr; // Initialize pointer to nullptr

RTC *RTC::getInstance(uint8_t sdaPin, uint8_t sclPin)
{
    if (instance == nullptr)
    {
        instance = new RTC(sdaPin, sclPin);

        Wire.begin(sdaPin, sclPin);
        while (true)
        {
            if (instance->rtc.begin())
            {
                break;
            }
            Serial.println("Couldn't find RTC");
            delay(5000);
        }

        if (instance->rtc.lostPower())
        {
            Serial.println("RTC lost power, setting the time!");
            // Set the date and time at compile time
            instance->rtc.adjust(DateTime(2020, 2, 1, 0, 0, 0));
        }
    }
    return instance;
}

DateTime RTC::now()
{
    return rtc.now();
}

void RTC::setDateTime(const DateTime &dt)
{
    rtc.adjust(dt);
}
