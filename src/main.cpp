#include "rtc.h"
#include "relayManager.h"
#include "configManager.h"
#include "esp_heap_caps.h"
#include "esp_log.h"

#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <Update.h>
#include <DNSServer.h>
#include <algorithm>

#define SDA_PIN 15
#define SCL_PIN 2
#define RELAY1_PIN 32
#define RELAY2_PIN 33
#define RELAY3_PIN 25
#define RELAY4_PIN 26
#define BUTTON_PIN 5
#define LONG_PRESS_TIME 10000 // 10 seconds in milliseconds
#define WIFI_ON_TIME 3600000  // 1 hour in milliseconds

#define UPDATE_CHUNK_SIZE 1024

#define LOOP_SPEED 100 // in ms

// settings
const char *APssid = "Smart-Relays-"; // SSID + dynamic part
const char *APpassword = NULL;
IPAddress APip(192, 168, 4, 1);
IPAddress APsubnet(255, 255, 255, 0);

// DNS
static const byte DNS_PORT = 53;
static DNSServer dnsServer;
const String localIPURL = "http://" + APip.toString();

// Create the WebServer (port 80)
WebServer server(80);

// RTC
RTC *rtc = nullptr;

// ConfigManager
ConfigManager *configManager = nullptr;

// Relay Manager
RelayManager *relayManager;
std::queue<std::vector<Alarm *>> alarmQueue;

// Function declarations
String getContentType(String filename);
void handleFileRead(String path);
void handleGetAllRelays();     // - **Endpoint**: `/api/all-relays` GET
void handleRelayControl();     // - **Endpoint**: `/api/relay-control` POST
void handleSystemSettings();   // - **Endpoint**: `/api/settings` GET
void handleUpdateSettings();   // - **Endpoint**: `/api/settings` POST
void handleGetRelayAlarms();   // - **Endpoint**: `/api/relay-alarms?relayId=:relayId` GET
void handleCreateRelayAlarm(); // - **Endpoint**: `/api/relay-alarm` POST
void handleUpdateRelayAlarm(); // - **Endpoint**: `/api/relay-alarm?relayId=:relayId&alarmId=:alarmId` PUT
void handleDeleteRelayAlarm(); // - **Endpoint**: `/api/relay-alarm?relayId=:relayId&alarmId=:alarmId` DELETE
void handleServerTime();       // - **Endpoint**: `/api/server-time` GET
void handleUpdateServerTime(); // - **Endpoint**: `/api/server-time` POST
void handleFirmwareUpdate();   // - **Endpoint**: `/api/update-firmware` POST
void handleReset();            // - **Endpoint**: `/api/reset` POST
void sendJsonResponse(int status, const String &message);

void factoryreset();
void restart();
void calculateNextAlarm();
void toggleWifi();
void IRAM_ATTR handleButtonPress();

String LoadConfig();
void SaveConfig();

// Setup function
void setup()
{
    Serial.begin(115200);

    Serial.println("LETS GOOOOO");

    // Initialize the RTC
    rtc = RTC::getInstance(SDA_PIN, SCL_PIN);

    // ConfigManager
    configManager = ConfigManager::getInstance();

    // Load config data
    String config = LoadConfig();

    // Initialize the Relay Manager
    Serial.println(config);
    if (config != "{}")
    {
        relayManager = new RelayManager(config);
    }
    else
    {
        // Load default relays
        relayManager = new RelayManager();
        relayManager->addRelay(RELAY1_PIN, "Relay 1");
        relayManager->addRelay(RELAY2_PIN, "Relay 2");
        relayManager->addRelay(RELAY3_PIN, "Relay 3");
        relayManager->addRelay(RELAY4_PIN, "Relay 4");

        // Save default relays
        try
        {
            SaveConfig();
        }
        catch (const std::exception &e)
        {
            Serial.println("Failed to save default config");
        }

        // Turn on wifi
        toggleWifi();
    }

    // calculate new alarm queue
    calculateNextAlarm();

    // Initialize the SPIFFS
    if (!SPIFFS.begin(true))
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }

    // // Start Web services
    dnsServer.setTTL(3600);

    // Define routes for the WebServer
    server.onNotFound([]()
                      { handleFileRead(server.uri()); });

    // Captive portal
    server.on("/connecttest.txt", []()
              { server.sendHeader("Location", "http://logout.net", true); server.send(302, "text/html", ""); }); // windows 11 captive portal workaround
    server.on("/wpad.dat", []()
              { server.send(404); }); // win 10
    server.on("/generate_204", []()
              { server.sendHeader("Location", localIPURL, true); server.send(302, "text/html", ""); }); // android captive portal redirect
    server.on("/redirect", []()
              { server.sendHeader("Location", localIPURL, true); server.send(302, "text/html", ""); }); // microsoft redirect
    server.on("/hotspot-detect.html", []()
              { server.sendHeader("Location", localIPURL, true); server.send(302, "text/html", ""); }); // apple call home
    server.on("/canonical.html", []()
              { server.sendHeader("Location", localIPURL, true); server.send(302, "text/html", ""); }); // firefox captive portal call home
    server.on("/success.txt", []()
              { server.send(200); }); // firefox captive portal call home
    server.on("/ncsi.txt", []()
              { server.sendHeader("Location", localIPURL, true); server.send(302, "text/html", ""); }); // windows call home

    // API
    server.on("/api/all-relays", HTTP_GET, handleGetAllRelays);
    server.on("/api/relay-control", HTTP_POST, handleRelayControl);
    server.on("/api/settings", HTTP_GET, handleSystemSettings);
    server.on("/api/settings", HTTP_POST, handleUpdateSettings);
    server.on("/api/relay-alarms", HTTP_GET, handleGetRelayAlarms);
    server.on("/api/relay-alarm", HTTP_POST, handleCreateRelayAlarm);
    server.on("/api/relay-alarm", HTTP_PUT, handleUpdateRelayAlarm);
    server.on("/api/relay-alarm", HTTP_DELETE, handleDeleteRelayAlarm);
    server.on("/api/server-time", HTTP_GET, handleServerTime);
    server.on("/api/server-time", HTTP_POST, handleUpdateServerTime);
    server.on("/api/update-firmware", HTTP_POST, handleFirmwareUpdate);
    server.on("/api/reset", HTTP_POST, handleReset);

    // Initialize the button pin as an input
    pinMode(BUTTON_PIN, INPUT_PULLDOWN); // Using pull-up resistor
    // Attach the interrupt to the button pin for both rising and falling edges
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleButtonPress, CHANGE);
}

// Main loop
uint counter = 0;
DateTime lastAlarmCalculation = DateTime(2020, 1, 1, 0, 0, 0);
volatile unsigned long buttonPressTime = 0;
volatile bool buttonPressed = false;
volatile bool buttonPressedShort = false;
volatile bool longPressDetected = false;
unsigned long timeWifiTurnedOn = 0;
bool wifiOn = false;
void loop()
{
    counter = (counter + 1) % LOOP_SPEED;

    if (wifiOn)
    {
        if (counter % 30 == 0)
        {
            dnsServer.processNextRequest();
        }
        server.handleClient();
    }

    if (counter == 0)
    {
        // Serial.println(alarmQueue.size());
        // Check alarms
        DateTime now = rtc->now();

        if (!alarmQueue.empty())
        {
            std::vector<Alarm *> alarms = alarmQueue.front();
            if (alarms.size() > 0)
            {
                Serial.println("Checking alarm(s) at " + String(alarms[0]->getHour()) + ":" + String(alarms[0]->getMinute()) + ":" + String(alarms[0]->getSecond()) + " and next alarm in " + String(alarms[0]->getNextAlarminSeconds(now)) + " seconds from now and there will be " + String(alarms.size()) + " alarm(s) and last calculation was " + String(now.unixtime() - lastAlarmCalculation.unixtime()) + " seconds ago");
                if (alarms[0]->checkAlarm(now))
                {
                    for (Alarm *alarm : alarms)
                    {
                        Relay *rel = alarm->getRelay();
                        if (alarm->getState())
                        {
                            alarm->turnOn();
                            Serial.println("Relay " + rel->getName() + " turned on");
                        }
                        else
                        {
                            alarm->turnOff();
                            Serial.println("Relay " + rel->getName() + " turned off");
                        }
                    }
                    alarmQueue.pop();
                    alarmQueue.push(alarms);
                }
            }
        }
    }

    // Check if a normal press was detected
    if (buttonPressedShort && !longPressDetected)
    {
        Serial.println("Button Pressed briefly. Wifi turned on/off");
        toggleWifi();
        buttonPressedShort = false;
    }

    // Check if a long press was detected
    if (longPressDetected)
    {
        Serial.println("Button Pressed for more than 10 seconds! Factory reset and restart");
        factoryreset();
        restart();
        longPressDetected = false; // Reset the long press detection
    }

    if (wifiOn && millis() - timeWifiTurnedOn > WIFI_ON_TIME)
    {
        Serial.println("Turning off wifi after 1 hour of inactivity");
        toggleWifi();
    }

    delay(1);
}

void toggleWifi()
{
    Serial.println("Toggling wifi. Status: " + String(wifiOn ? "APon" : "APoff"));
    if (wifiOn)
    {
        // Stop dns and http server
        dnsServer.stop();
        Serial.println("DNS server stopped");
        server.stop();
        Serial.println("HTTP server stopped");

        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        Serial.println("Wifi turned off");

        timeWifiTurnedOn = 0;
        wifiOn = false;
    }
    else
    {
        // Load ssid from config or generate a new one
        String fullSSID = configManager->getConfig("ssid", "");
        if (fullSSID == "")
        {
            DateTime now = rtc->now();
            String dynamicPart = String(now.hour()) + String(now.minute()) + String(now.second()) + String(now.day()) + String(now.month()) + String(now.year());
            fullSSID = String(APssid + dynamicPart);
            try
            {
                configManager->setConfig("ssid", fullSSID);
            }
            catch (const std::exception &e)
            {
                Serial.println("Failed to save ssid");
            }
        }

        WiFi.mode(WIFI_MODE_APSTA);
        WiFi.softAP(fullSSID, APpassword);
        WiFi.softAPConfig(APip, APip, APsubnet);
        Serial.println("Wifi turned on");

        // Start the DNS server
        dnsServer.start(DNS_PORT, "*", APip);
        Serial.println("DNS server started");

        // Start the server
        server.begin();
        Serial.println("HTTP server started");

        timeWifiTurnedOn = millis();
        wifiOn = true;
    }
}

// Interrupt service routine (ISR) for the button press
void IRAM_ATTR handleButtonPress()
{
    if (digitalRead(BUTTON_PIN) == HIGH)
    {                               // Check if button is actually pressed
        buttonPressTime = millis(); // Record the time the button was pressed
        buttonPressed = true;
    }
    else
    {
        if (buttonPressed)
        {
            buttonPressed = false; // Reset the button press flag

            if (millis() - buttonPressTime > 10000)
            {                             // Check if the button was pressed for more than 10 seconds
                longPressDetected = true; // Set the long press flag
            }
            else
            {
                if (millis() - buttonPressTime > 50)
                {
                    buttonPressedShort = true; // Set the normal press flag
                }
            }
        }
    }
}

void calculateNextAlarm()
{
    Serial.println("Calculating next alarm");
    DateTime now = rtc->now();
    alarmQueue = relayManager->getNextAlarm();
    lastAlarmCalculation = now;
}

void SaveConfig()
{
    configManager->setConfig("config", relayManager->toJson());
}

String LoadConfig()
{
    return configManager->getConfig("config", "{}");
}

// Utility functions
String getContentType(String filename)
{
    if (server.hasArg("download"))
        return "application/octet-stream";
    else if (filename.endsWith(".htm"))
        return "text/html";
    else if (filename.endsWith(".html"))
        return "text/html";
    else if (filename.endsWith(".css"))
        return "text/css";
    else if (filename.endsWith(".js"))
        return "application/javascript";
    else if (filename.endsWith(".png"))
        return "image/png";
    else if (filename.endsWith(".gif"))
        return "image/gif";
    else if (filename.endsWith(".jpg"))
        return "image/jpeg";
    else if (filename.endsWith(".ico"))
        return "image/x-icon";
    else if (filename.endsWith(".xml"))
        return "text/xml";
    else if (filename.endsWith(".pdf"))
        return "application/pdf";
    else if (filename.endsWith(".zip"))
        return "application/zip";
    return "text/plain";
}

void handleFileRead(String path)
{
    Serial.println("Handling file read for: " + path);
    if (path.endsWith("/"))
        path += "index.html"; // Default to index.html

    if (SPIFFS.exists(path))
    {
        File file = SPIFFS.open(path, "r");
        String contentType = getContentType(path);
        server.streamFile(file, contentType);
        file.close();
    }
    else if (SPIFFS.exists(path + "/index.html"))
    {
        path = path + "/index.html";
        File file = SPIFFS.open(path, "r");
        String contentType = getContentType(path);
        server.streamFile(file, contentType);
        file.close();
    }
    else
    {
        server.send(404, "text/plain", "404: Not Found");
    }
}

void sendJsonResponse(int status, const String &message)
{
    server.send(status, "application/json", message);
}

// Create JSON from string and validate required keys and their types
String CreateJsonFromString(const String &body, const std::map<String, String> &requiredKeys, StaticJsonDocument<256> &doc)
{
    // Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, body);

    // Check if deserialization was successful
    if (error)
    {
        return "Failed to parse JSON";
    }

    // Validate required keys and their types
    for (const auto &keyTypePair : requiredKeys)
    {
        const String &key = keyTypePair.first;
        const String &type = keyTypePair.second;

        if (!doc.containsKey(key))
        {
            return "Missing key: " + key;
        }

        if (type == "uint" && !doc[key].is<uint>())
        {
            return "Invalid type for key: " + key;
        }
        else if (type == "int" && !doc[key].is<int>())
        {
            return "Invalid type for key: " + key;
        }
        else if (type == "bool" && !doc[key].is<bool>())
        {
            return "Invalid type for key: " + key;
        }
        else if (type == "string" && !doc[key].is<String>())
        {
            return "Invalid type for key: " + key;
        }
        else if (type == "array_bool_7")
        {
            if (!doc[key].is<JsonArray>() || doc[key].size() != 7)
            {
                return "Invalid array size for key: " + key;
            }
            for (int i = 0; i < 7; i++)
            {
                if (!doc[key][i].is<bool>())
                {
                    return "Invalid array element type for key: " + key;
                }
            }
        }
    }

    return "";
}

// - **Endpoint**: `/api/all-relays` GET
void handleGetAllRelays()
{
    try
    {
        StaticJsonDocument<500> doc; // Adjust size as needed
        doc["systemName"] = relayManager->getName();
        JsonArray relaysArray = doc.createNestedArray("relays");
        std::vector<uint> relayIDs = relayManager->getRelayIDs();
        for (uint id : relayIDs)
        {
            Relay *relay = relayManager->getRelayByID(id);
            if (relay != nullptr)
            {
                JsonObject relayDoc = relaysArray.createNestedObject();
                relayDoc["id"] = id;
                relayDoc["name"] = relay->getName();
                relayDoc["state"] = relay->getState();
            }
        }
        String response;
        serializeJson(doc, response);
        sendJsonResponse(200, response);
    }
    catch (const std::exception &e)
    {
        sendJsonResponse(500, "{ \"error\": \"" + String(e.what()) + "\"}");
    }
}

// - **Endpoint**: `/api/relay-control` POST
void handleRelayControl()
{
    try
    {
        // Get body
        String body = server.arg("plain");

        // Define required keys and their types
        std::map<String, String> requiredKeys = {
            {"relayId", "uint"},
            {"state", "bool"}};

        // Allocate memory for the JsonDocument
        StaticJsonDocument<256> doc; // Adjust size as needed

        // Validate and create JSON document from string
        String validationError = CreateJsonFromString(body, requiredKeys, doc);
        if (validationError != "")
        {
            sendJsonResponse(400, "{ \"error\": \"" + validationError + "\"}");
            return;
        }

        // Get relay id and state
        uint relayId = doc["relayId"].as<uint>();
        bool state = doc["state"].as<bool>();

        Relay *relay = relayManager->getRelayByID(relayId);
        if (relay != nullptr)
        {
            if (state)
            {
                relay->On();
            }
            else
            {
                relay->Off();
            }
            StaticJsonDocument<200> responseDoc;
            responseDoc["message"] = "Relay state updated successfully";
            responseDoc["relayId"] = relayId;
            responseDoc["state"] = state;

            String response;
            serializeJson(responseDoc, response);
            sendJsonResponse(200, response);
        }
        else
        {
            sendJsonResponse(404, "{ \"error\": \"Relay not found\"}");
        }
    }
    catch (const std::exception &e)
    {
        sendJsonResponse(500, "{ \"error\": \"" + String(e.what()) + "\"}");
    }
}

// - **Endpoint**: `/api/settings` GET
void handleSystemSettings()
{
    try
    {
        DateTime now = rtc->now();

        StaticJsonDocument<200> doc;
        doc["systemName"] = relayManager->getName();

        // Format month and day with leading zeros
        String month = String(now.month());
        if (month.length() == 1)
            month = "0" + month;
        String day = String(now.day());
        if (day.length() == 1)
            day = "0" + day;

        JsonArray relaysArray = doc.createNestedArray("relays");
        std::vector<uint> relayIDs = relayManager->getRelayIDs();
        for (uint id : relayIDs)
        {
            Relay *relay = relayManager->getRelayByID(id);
            if (relay != nullptr)
            {
                JsonObject relayDoc = relaysArray.createNestedObject();
                relayDoc["id"] = id;
                relayDoc["name"] = relay->getName();
                relayDoc["state"] = relay->getState();
            }
        }

        String response;
        serializeJson(doc, response);
        sendJsonResponse(200, response);
    }
    catch (const std::exception &e)
    {
        sendJsonResponse(500, "{ \"error\": \"" + String(e.what()) + "\"}");
    }
}

// - **Endpoint**: `/api/settings` POST
void handleUpdateSettings()
{
    try
    {
        // Get body
        String body = server.arg("plain");

        // Define required keys and their types
        std::map<String, String> requiredKeys = {
            {"systemName", "string"},
            {"relays", "array"}};

        // Allocate memory for the JsonDocument
        StaticJsonDocument<256> doc; // Adjust size as needed

        // Validate and create JSON document from string
        String validationError = CreateJsonFromString(body, requiredKeys, doc);
        if (validationError != "")
        {
            sendJsonResponse(400, "{ \"error\": \"" + validationError + "\"}");
            return;
        }

        relayManager->setName(doc["systemName"].as<String>());

        // Update relays
        for (auto relay : doc["relays"].as<JsonArray>())
        {
            uint id = relay["id"].as<uint>();
            String name = relay["name"].as<String>();
            Relay *r = relayManager->getRelayByID(id);
            if (r != nullptr)
            {
                r->setName(name);
            }
        }

        // Save config
        SaveConfig();

        StaticJsonDocument<200> responseDoc;
        responseDoc["message"] = "Settings updated successfully";

        String response;
        serializeJson(responseDoc, response);
        sendJsonResponse(200, response);
    }
    catch (const std::exception &e)
    {
        sendJsonResponse(500, "{ \"error\": \"" + String(e.what()) + "\"}");
    }
}

// - **Endpoint**: `/api/relay-alarms?relayId=relayId` GET
void handleGetRelayAlarms()
{
    try
    {
        // get relay id
        uint relayId = server.arg("relayId").toInt();

        // get relay
        Relay *relay = relayManager->getRelayByID(relayId);
        if (relay != nullptr)
        {
            StaticJsonDocument<500> doc; // Adjust size as needed
            JsonArray alarmsArray = doc.createNestedArray("alarms");

            std::vector<uint> alarmIDs = relay->getAlarmIDs();
            for (uint id : alarmIDs)
            {
                Alarm *alarm = relay->getAlarmByID(id);
                if (alarm != nullptr)
                {
                    JsonObject alarmDoc = alarmsArray.createNestedObject();
                    alarmDoc["id"] = id;
                    alarmDoc["state"] = alarm->getState();

                    String hour = String(alarm->getHour());
                    String minute = String(alarm->getMinute());
                    String second = String(alarm->getSecond());
                    if (hour.length() == 1)
                        hour = "0" + hour;
                    if (minute.length() == 1)
                        minute = "0" + minute;
                    if (second.length() == 1)
                        second = "0" + second;
                    alarmDoc["hour"] = hour;
                    alarmDoc["minute"] = minute;
                    alarmDoc["second"] = second;

                    JsonArray weekdaysArray = alarmDoc.createNestedArray("weekdays");
                    std::array<bool, 7> weekdays = alarm->getWeekdays();
                    for (int i = 0; i < 7; i++)
                    {
                        weekdaysArray.add(weekdays[i]);
                    }
                }
            }
            String response;
            serializeJson(doc, response);
            sendJsonResponse(200, response);
        }
        else
        {
            sendJsonResponse(404, "{ \"error\": \"Relay not found\"}");
        }
    }
    catch (const std::exception &e)
    {
        sendJsonResponse(500, "{ \"error\": \"" + String(e.what()) + "\"}");
    }
}

// - **Endpoint**: `/api/relay-alarm` POST
void handleCreateRelayAlarm()
{
    try
    {
        // Get body
        String body = server.arg("plain");

        // Define required keys and their types
        std::map<String, String> requiredKeys = {
            {"relayId", "uint"},
            {"state", "bool"},
            {"hour", "uint"},
            {"minute", "uint"},
            {"second", "uint"},
            {"weekdays", "array_bool_7"}};

        // Allocate memory for the JsonDocument
        StaticJsonDocument<256> doc; // Adjust size as needed

        // Validate and create JSON document from string
        String validationError = CreateJsonFromString(body, requiredKeys, doc);
        if (validationError != "")
        {
            sendJsonResponse(400, "{ \"error\": \"" + validationError + "\"}");
            return;
        }

        // Get relayId, state, time, and weekdays
        uint relayId = doc["relayId"].as<uint>();
        bool state = doc["state"].as<bool>();
        uint hour = doc["hour"].as<uint>();
        uint minute = doc["minute"].as<uint>();
        uint second = doc["second"].as<uint>();
        std::array<bool, 7> weekdays;
        for (int i = 0; i < 7; i++)
        {
            weekdays[i] = doc["weekdays"][i].as<bool>();
        }

        // Get relay
        Relay *relay = relayManager->getRelayByID(relayId);
        if (relay == nullptr)
        {
            sendJsonResponse(404, "{ \"error\": \"Relay not found\"}");
            return;
        }

        // Parse time
        if (hour > 23 || minute > 59 || second > 59)
        {
            sendJsonResponse(400, "{ \"error\": \"Invalid time values\"}");
            return;
        }

        // Create alarm
        Alarm *alarm = relay->addAlarm(hour, minute, second, weekdays, state);

        // Calculate new alarm queue
        calculateNextAlarm();

        // Save config
        SaveConfig();

        // Create response
        StaticJsonDocument<200> responseDoc;
        responseDoc["message"] = "Relay alarm rule created successfully";
        responseDoc["ruleId"] = alarm->getId();

        String response;
        serializeJson(responseDoc, response);
        sendJsonResponse(200, response);
    }
    catch (const std::exception &e)
    {
        sendJsonResponse(500, "{ \"error\": \"" + String(e.what()) + "\"}");
    }
}

// - **Endpoint**: `/api/relay-alarm?relayId=:relayId&alarmId=:alarmId` PUT
void handleUpdateRelayAlarm()
{
    try
    {
        // Get body
        String body = server.arg("plain");

        // Define required keys and their types
        std::map<String, String> requiredKeys = {
            {"state", "bool"},
            {"hour", "uint"},
            {"minute", "uint"},
            {"second", "uint"},
            {"weekdays", "array_bool_7"}};

        // Allocate memory for the JsonDocument
        StaticJsonDocument<256> doc; // Adjust size as needed

        // Validate and create JSON document from string
        String validationError = CreateJsonFromString(body, requiredKeys, doc);
        if (validationError != "")
        {
            sendJsonResponse(400, "{ \"error\": \"" + validationError + "\"}");
            return;
        }

        // Get relayId, alarmId
        uint relayId = server.arg("relayId").toInt();
        uint alarmId = server.arg("alarmId").toInt();

        // Get relay
        Relay *relay = relayManager->getRelayByID(relayId);
        if (relay == nullptr)
        {
            sendJsonResponse(404, "{ \"error\": \"Relay not found\"}");
            return;
        }

        // Get alarm
        Alarm *alarm = relay->getAlarmByID(alarmId);
        if (alarm == nullptr)
        {
            sendJsonResponse(404, "{ \"error\": \"Alarm not found\"}");
            return;
        }

        // Get state, time and weekdays
        bool state = doc["state"].as<bool>();
        uint hour = doc["hour"].as<uint>();
        uint minute = doc["minute"].as<uint>();
        uint second = doc["second"].as<uint>();
        std::array<bool, 7> weekdays;
        for (int i = 0; i < 7; i++)
        {
            weekdays[i] = doc["weekdays"][i].as<bool>();
        }

        // Validate time
        if (hour > 23 || minute > 59 || second > 59)
        {
            sendJsonResponse(400, "{ \"error\": \"Invalid time values\"}");
            return;
        }

        // Update alarm
        alarm->setHour(hour);
        alarm->setMinute(minute);
        alarm->setSecond(second);
        alarm->setWeekdays(weekdays);
        alarm->setState(state);

        // Calculate new alarm queue
        calculateNextAlarm();

        Serial.println("Updated alarm: " + String(alarm->getHour()) + ":" + String(alarm->getMinute()) + ":" + String(alarm->getSecond()));

        // Save config
        SaveConfig();

        // Create response
        StaticJsonDocument<200> responseDoc;
        responseDoc["message"] = "Relay alarm rule updated successfully";

        String response;
        serializeJson(responseDoc, response);
        sendJsonResponse(200, response);
    }
    catch (const std::exception &e)
    {
        sendJsonResponse(500, "{ \"error\": \"" + String(e.what()) + "\"}");
    }
}

// - **Endpoint**: `/api/relay-alarm?relayId=:relayId&alarmId=:alarmId` DELETE
void handleDeleteRelayAlarm()
{
    try
    {
        // Get relayId, alarmId
        uint relayId = server.arg("relayId").toInt();
        uint alarmId = server.arg("alarmId").toInt();

        // Get relay
        Relay *relay = relayManager->getRelayByID(relayId);
        if (relay == nullptr)
        {
            sendJsonResponse(404, "{ \"error\": \"Relay not found\"}");
            return;
        }

        // Get alarm
        Alarm *alarm = relay->getAlarmByID(alarmId);
        if (alarm == nullptr)
        {
            sendJsonResponse(404, "{ \"error\": \"Alarm not found\"}");
            return;
        }

        // Delete alarm
        relay->removeAlarm(alarmId);

        // Calculate new alarm queue
        calculateNextAlarm();

        // Create response
        StaticJsonDocument<200> responseDoc;
        responseDoc["message"] = "Relay alarm rule deleted successfully";

        String response;
        serializeJson(responseDoc, response);
        sendJsonResponse(200, response);
    }
    catch (const std::exception &e)
    {
        sendJsonResponse(500, "{ \"error\": \"" + String(e.what()) + "\"}");
    }
}

// - **Endpoint**: `/api/server-time` GET
void handleServerTime()
{
    try
    {
        DateTime now = rtc->now();

        StaticJsonDocument<200> doc;
        doc["hour"] = now.hour();
        doc["minute"] = now.minute();
        doc["second"] = now.second();
        doc["day"] = now.day();
        doc["month"] = now.month();
        doc["year"] = now.year();
        doc["weekday"] = now.dayOfTheWeek();

        String response;
        serializeJson(doc, response);
        sendJsonResponse(200, response);
    }
    catch (const std::exception &e)
    {
        sendJsonResponse(500, "{ \"error\": \"" + String(e.what()) + "\"}");
    }
}

// - **Endpoint**: `/api/server-time` POST
void handleUpdateServerTime()
{
    try
    {
        // Get body
        String body = server.arg("plain");

        // Define required keys and their types
        std::map<String, String> requiredKeys = {
            {"hourAdjustment", "int"},
            {"minuteAdjustment", "int"},
            {"secondAdjustment", "int"},
            {"date", "string"}};

        // Allocate memory for the JsonDocument
        StaticJsonDocument<256> doc; // Adjust size as needed

        // Validate and create JSON document from string
        String validationError = CreateJsonFromString(body, requiredKeys, doc);
        if (validationError != "")
        {
            sendJsonResponse(400, "{ \"error\": \"" + validationError + "\"}");
            return;
        }

        DateTime now = rtc->now();

        // Get hour, minute, second, day, month, year
        int hourAdjustment = (doc["hourAdjustment"].as<int>() + now.hour()) % 24;
        int minuteAdjustment = (doc["minuteAdjustment"].as<int>() + now.minute()) % 60;
        int secondAdjustment = (doc["secondAdjustment"].as<int>() + now.second()) % 60;
        String systemDate = doc["date"].as<String>();
        int year = systemDate.substring(0, 4).toInt();
        int month = systemDate.substring(5, 7).toInt();
        int day = systemDate.substring(8, 10).toInt();

        // Set the time
        rtc->setDateTime(DateTime(year, month, day, hourAdjustment, minuteAdjustment, secondAdjustment));

        // Calculate new alarm queue
        calculateNextAlarm();

        // Create response
        StaticJsonDocument<200> responseDoc;
        responseDoc["message"] = "Server time updated successfully";

        String response;
        serializeJson(responseDoc, response);
        sendJsonResponse(200, response);
    }
    catch (const std::exception &e)
    {
        sendJsonResponse(500, "{ \"error\": \"" + String(e.what()) + "\"}");
    }
}

void handleFirmwareUpdate()
{
    size_t freeHeap = ESP.getFreeHeap();
    Serial.printf("Free Heap: %u\n", freeHeap);

    HTTPUpload &upload = server.upload();
    static bool isFSUpdate = false;

    if (upload.status == UPLOAD_FILE_START)
    {
        Serial.printf("Update: %s\n", upload.filename.c_str());
        if (upload.filename.endsWith(".bin"))
        {
            isFSUpdate = false;
            Serial.println("Firmware update started");
            if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH))
            {
                Update.printError(Serial);
            }
        }
        else if (upload.filename.endsWith(".spiffs") || upload.filename.endsWith(".littlefs"))
        {
            isFSUpdate = true;
            Serial.println("Filesystem update started");
            if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_SPIFFS))
            {
                Update.printError(Serial);
            }
        }
        else
        {
            Serial.println("Unknown file type");
            server.send(400, "application/json", "{\"error\": \"Invalid file type\"}");
            return;
        }
    }
    else if (upload.status == UPLOAD_FILE_WRITE)
    {
        size_t written = 0;
        Serial.printf("Writing to update: %u bytes\n", upload.currentSize);
        while (written < upload.currentSize)
        {
            size_t chunkSize = min(UPDATE_CHUNK_SIZE, (int)(upload.currentSize - written));
            size_t bytesWritten = Update.write(upload.buf + written, chunkSize);
            Serial.printf("Bytes written: %u/%u\n", written, upload.currentSize);
            if (bytesWritten != chunkSize)
            {
                Update.printError(Serial);
                return;
            }
            written += bytesWritten;
        }
    }
    else if (upload.status == UPLOAD_FILE_END)
    {
        if (Update.end(true))
        {
            Serial.printf("Update Success: %u bytes\n", upload.totalSize);
        }
        else
        {
            Update.printError(Serial);
        }
    }
    else
    {
        Serial.println("Invalid file upload status");
        server.send(400, "application/json", "{\"error\": \"Invalid file upload\"}");
    }
}

void handleReset()
{
    Serial.println("Resetting device");

    try
    {
        factoryreset();
        server.send(200, "application/json", "{\"message\": \"Device reset\"}");
        restart();
    }
    catch (const std::exception &e)
    {
        server.send(500, "application/json", "{\"error\": \"Failed to reset device\"}");
    }
}

void factoryreset()
{
    ConfigManager *cm = ConfigManager::getInstance();
    cm->setConfig("config", "{}");
}

void restart()
{
    Serial.println("Restarting device");
    delay(1000);
    ESP.restart();
}