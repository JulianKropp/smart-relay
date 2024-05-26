#include "rtc.h"
#include "relayManager.h"
#include "configManager.h"

#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <Update.h>

// settings
String systemName = "Smart Relays";
String wifiName = "";
String wifiPassword = "";
bool syncTime = false;
String APssid = "ESP32-Access-Point";
String APpassword = "12345678";

// Create the WebServer (port 80)
WebServer server(80);

// RTC
RTC rtc(22, 23);

// Relay Manager
RelayManager relayManager(&rtc);

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
void handleNetworkInfo();      // - **Endpoint**: `/api/network-info` GET
void handleServerTime();       // - **Endpoint**: `/api/server-time` GET
void handleFirmwareUpdate();   // - **Endpoint**: `/api/update-firmware` POST
void sendJsonResponse(int status, const String &message);

// Setup function
void setup()
{
    Serial.begin(115200);

    // Initialize the RTC
    rtc.begin();

    // Initialize the Relay Manager
    relayManager.addRelay(32, "Relay 1");
    relayManager.addRelay(33, "Relay 2");
    relayManager.addRelay(25, "Relay 3");
    relayManager.addRelay(26, "Relay 4");

    // Initialize the SPIFFS
    if (!SPIFFS.begin(true))
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }

    // Start the Access Point
    WiFi.softAP(APssid, APpassword);
    Serial.println("Access Point started");

    // Define routes for the WebServer
    // Web
    server.onNotFound([]()
                      { handleFileRead(server.uri()); });
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
    server.on("/api/network-info", HTTP_GET, handleNetworkInfo);
    server.on("/api/update-firmware", HTTP_POST, handleFirmwareUpdate);

    // Start the server
    server.begin();
    Serial.println("HTTP server started");
}

// Main loop
void loop()
{
    server.handleClient();
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
        JsonDocument doc;
        JsonArray relaysArray = doc.createNestedArray("relays");

        std::vector<uint> relayIDs = relayManager.getRelayIDs();
        for (uint id : relayIDs)
        {
            Relay *relay = relayManager.getRelayByID(id);
            if (relay != nullptr)
            {
                String name = relay->getName();
                bool state = relay->getState();

                JsonDocument relayDoc;
                relayDoc["id"] = id;
                relayDoc["name"] = name;
                relayDoc["state"] = state;

                relaysArray.add(relayDoc);
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
        // get body
        String body = server.arg("plain");
        Serial.println("Body: " + body);

        // Allocate memory for the JsonDocument
        StaticJsonDocument<200> doc; // Adjust size as needed

        // Deserialize the JSON document
        DeserializationError error = deserializeJson(doc, body);

        // Check if deserialization was successful
        if (error)
        {
            sendJsonResponse(400, "{ \"error\": \"Failed to parse JSON\"}");
            return;
        }

        // get relay id and state
        uint relayId = doc["relayId"].as<uint>();
        bool state = doc["state"].as<bool>();

        Serial.println("Relay ID: " + String(relayId));
        Serial.println("State: " + String(state));

        // control relay
        Relay *relay = relayManager.getRelayByID(relayId);
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
        DateTime now = rtc.now();

        StaticJsonDocument<200> doc;
        doc["systemName"] = systemName;
        doc["wifiName"] = wifiName;
        doc["wifiPassword"] = wifiPassword;
        doc["systemTime"] = String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second());

        // Format month and day with leading zeros
        String month = String(now.month());
        if (month.length() == 1)
            month = "0" + month;
        String day = String(now.day());
        if (day.length() == 1)
            day = "0" + day;

        doc["systemDate"] = String(now.year()) + "-" + month + "-" + day;
        doc["syncTime"] = syncTime;

        std::vector<uint> relayIDs = relayManager.getRelayIDs();
        JsonArray relaysArray = doc.createNestedArray("relays");
        for (uint id : relayIDs)
        {
            Relay *relay = relayManager.getRelayByID(id);
            if (relay != nullptr)
            {
                String name = relay->getName();
                bool state = relay->getState();

                JsonObject relayDoc = relaysArray.createNestedObject();
                relayDoc["id"] = id;
                relayDoc["name"] = name;
                relayDoc["state"] = state;
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
        // get body
        String body = server.arg("plain");
        Serial.println("Body: " + body);

        // Allocate memory for the JsonDocument
        StaticJsonDocument<200> doc; // Adjust size as needed

        // Deserialize the JSON document
        DeserializationError error = deserializeJson(doc, body);

        // Check if deserialization was successful
        if (error)
        {
            sendJsonResponse(400, "{ \"error\": \"Failed to parse JSON\"}");
            return;
        }

        systemName = doc["systemName"].as<String>();
        wifiName = doc["wifiName"].as<String>();
        wifiPassword = doc["wifiPassword"].as<String>();
        String systemTime = doc["systemTime"].as<String>();
        String systemDate = doc["systemDate"].as<String>();
        syncTime = doc["syncTime"].as<bool>();

        // update relays
        for (auto relay : doc["relays"].as<JsonArray>())
        {
            uint id = relay["id"].as<uint>();
            String name = relay["name"].as<String>();
            Relay *r = relayManager.getRelayByID(id);
            if (r != nullptr)
            {
                r->setName(name);
            }
        }

        // update wifi settings
        // TODO: IF wifi name & password != empty: Then Stop AP and try connecting to wifi. If it doesnt work then start AP again

        // If syncTime is false, then set the time
        if (!syncTime)
        {
            // Set the time
            int year = systemDate.substring(0, 4).toInt();
            int month = systemDate.substring(5, 7).toInt();
            int day = systemDate.substring(8, 10).toInt();
            int hour = systemTime.substring(0, 2).toInt();
            int minute = systemTime.substring(3, 5).toInt();
            int second = systemTime.substring(6, 8).toInt();

            rtc.setDateTime(DateTime(year, month, day, hour, minute, second));
        }
        else
        {
            // Sync time over NTP
            // TODO: Implement NTP sync
        }

        // Save data to NVS
        ConfigManager &cm = ConfigManager::getInstance();
        cm.setConfig("systemName", systemName);
        cm.setConfig("wifiName", wifiName);
        cm.setConfig("wifiPassword", wifiPassword);
        cm.setConfig("syncTime", String(syncTime));

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
        Relay *relay = relayManager.getRelayByID(relayId);
        if (relay != nullptr)
        {
            JsonDocument doc;
            JsonArray alarmsArray = doc.createNestedArray("alarms");

            std::vector<uint> alarmIDs = relay->getAlarmIDs();
            for (uint id : alarmIDs)
            {
                Alarm *alarm = relay->getAlarmByID(id);
                if (alarm != nullptr)
                {
                    String hour = String(alarm->getHour());
                    String minute = String(alarm->getMinute());
                    String second = String(alarm->getSecond());
                    if (hour.length() == 1)
                        hour = "0" + hour;
                    if (minute.length() == 1)
                        minute = "0" + minute;
                    if (second.length() == 1)
                        second = "0" + second;

                    std::array<bool, 7> weekdays = alarm->getWeekdays();

                    JsonDocument alarmDoc;
                    alarmDoc["id"] = id;
                    alarmDoc["state"] = alarm->getState();
                    alarmDoc["time"] = hour + ":" + minute + ":" + second;
                    alarmDoc.createNestedArray("weekdays");
                    JsonArray weekdaysArray = alarmDoc.createNestedArray("weekdays");
                    for (int i = 0; i < 7; i++)
                    {
                        weekdaysArray.add(weekdays[i]);
                    }

                    alarmsArray.add(alarmDoc);
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
        Serial.println("Body: " + body);

        // Define required keys and their types
        std::map<String, String> requiredKeys = {
            {"relayId", "uint"},
            {"state", "bool"},
            {"time", "string"},
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
        String time = doc["time"].as<String>();
        std::array<bool, 7> weekdays;
        for (int i = 0; i < 7; i++)
        {
            weekdays[i] = doc["weekdays"][i].as<bool>();
        }

        // Get relay
        Relay *relay = relayManager.getRelayByID(relayId);
        if (relay == nullptr)
        {
            sendJsonResponse(404, "{ \"error\": \"Relay not found\"}");
            return;
        }

        // Parse time
        std::vector<String> timeParts;
        String temp = "";
        for (int i = 0; i < time.length(); i++)
        {
            if (i == time.length() - 1)
            {
                temp += time[i];
                timeParts.push_back(temp);
                break;
            }

            if (time[i] == ':')
            {
                timeParts.push_back(temp);
                temp = "";
            }
            else
            {
                temp += time[i];
            }
        }
        if (timeParts.size() != 3)
        {
            sendJsonResponse(400, "{ \"error\": \"Invalid 'time' format\"}");
            return;
        }
        // Convert time parts to integers and check if they are valid
        uint hour;
        uint minute;
        uint second;
        try
        {
            hour = timeParts[0].toInt();
            minute = timeParts[1].toInt();
            second = timeParts[2].toInt();
            if (hour > 23 || minute > 59 || second > 59)
            {
                sendJsonResponse(400, "{ \"error\": \"Invalid time values\"}");
                return;
            }
        }
        catch (const std::exception &e)
        {
            sendJsonResponse(400, "{ \"error\": \"Invalid time values\"}");
            return;
        }

        // Create alarm
        Alarm *alarm = relay->addAlarm(hour, minute, second, weekdays, state);

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
        Serial.println("Body: " + body);

        // Define required keys and their types
        std::map<String, String> requiredKeys = {
            {"state", "bool"},
            {"time", "string"},
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
        Relay *relay = relayManager.getRelayByID(relayId);
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
        String time = doc["time"].as<String>();
        std::array<bool, 7> weekdays;
        for (int i = 0; i < 7; i++)
        {
            weekdays[i] = doc["weekdays"][i].as<bool>();
        }

        // Parse time
        std::vector<String> timeParts;
        String temp = "";
        for (int i = 0; i < time.length(); i++)
        {
            if (i == time.length() - 1)
            {
                temp += time[i];
                timeParts.push_back(temp);
                break;
            }

            if (time[i] == ':')
            {
                timeParts.push_back(temp);
                temp = "";
            }
            else
            {
                temp += time[i];
            }
        }

        if (timeParts.size() != 3)
        {
            sendJsonResponse(400, "{ \"error\": \"Invalid 'time' format\"}");
            return;
        }

        // Convert time parts to integers and check if they are valid
        uint hour;
        uint minute;
        uint second;
        try
        {
            hour = timeParts[0].toInt();
            minute = timeParts[1].toInt();
            second = timeParts[2].toInt();
            if (hour > 23 || minute > 59 || second > 59)
            {
                sendJsonResponse(400, "{ \"error\": \"Invalid time values\"}");
                return;
            }
        }
        catch (const std::exception &e)
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
        Relay *relay = relayManager.getRelayByID(relayId);
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

// - **Endpoint**: `/api/network-info` GET
void handleNetworkInfo()
{
    try
    {
        StaticJsonDocument<200> doc;
        doc["ssid"] = WiFi.SSID();
        doc["ipAddress"] = WiFi.localIP().toString();
        doc["gateway"] = WiFi.macAddress();
        doc["dns"] = WiFi.dnsIP().toString();

        String response;
        serializeJson(doc, response);
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
        DateTime now = rtc.now();

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
