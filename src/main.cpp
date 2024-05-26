#include "rtc.h"
#include "relayManager.h"
#include "configManager.h"

#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

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
void handleGetRelayAlarms();   // - **Endpoint**: `/api/relay-alarms?relayId=relayId` GET
void handleCreateRelayAlarm(); // - **Endpoint**: `/api/relay-alarm` POST
void handleUpdateRelayAlarm(); // - **Endpoint**: `/api/relay-alarm?relayId=relayId&alarmId=alarmId` PUT
void handleDeleteRelayAlarm(); // - **Endpoint**: `/api/relay-alarm?relayId=relayId&alarmId=alarmId` DELETE
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
    // server.on("/api/relay-alarm", HTTP_POST, handleCreateRelayAlarm);
    // server.on("/api/relay-alarm", HTTP_PUT, handleUpdateRelayAlarm);
    // server.on("/api/relay-alarm", HTTP_DELETE, handleDeleteRelayAlarm);
    // server.on("/api/server-time", HTTP_GET, handleServerTime);
    // server.on("/api/network-info", HTTP_GET, handleNetworkInfo);
    // server.on("/api/update-firmware", HTTP_POST, handleFirmwareUpdate);

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
        Relay* relay = relayManager.getRelayByID(relayId);
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
        if (month.length() == 1) month = "0" + month;
        String day = String(now.day());
        if (day.length() == 1) day = "0" + day;

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
            Relay* r = relayManager.getRelayByID(id);
            if (r != nullptr)
            {
                r->setName(name);
            }
        }

        // update wifi settings
        //TODO: IF wifi name & password != empty: Then Stop AP and try connecting to wifi. If it doesnt work then start AP again

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
        } else {
            // Sync time over NTP
            // TODO: Implement NTP sync
        }

        // Save data to NVS
        ConfigManager& cm = ConfigManager::getInstance();
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
void handleGetRelayAlarms() {
    try {
        // get relay id
        uint relayId = server.arg("relayId").toInt();
        
        // get relay
        Relay* relay = relayManager.getRelayByID(relayId);
        if (relay != nullptr) {
            JsonDocument doc;
            JsonArray alarmsArray = doc.createNestedArray("alarms");

            std::vector<uint> alarmIDs = relay->getAlarmIDs();
            for (uint id : alarmIDs) {
                Alarm* alarm = relay->getAlarmByID(id);
                if (alarm != nullptr) {
                    uint hour = alarm->getHour();
                    uint minute = alarm->getMinute();
                    uint second = alarm->getSecond();
                    std::array<bool, 7> weekdays = alarm->getWeekdays();

                    JsonDocument alarmDoc;
                    alarmDoc["id"] = id;
                    alarmDoc["state"] = alarm->getState();
                    alarmDoc["time"] = String(hour) + ":" + String(minute) + ":" + String(second);
                    alarmDoc.createNestedArray("weekdays");
                    JsonArray weekdaysArray = alarmDoc.createNestedArray("weekdays");
                    for (int i = 0; i < 7; i++) {
                        weekdaysArray.add(weekdays[i]);
                    }

                    alarmsArray.add(alarmDoc);
                }
            }

            String response;
            serializeJson(doc, response);
            sendJsonResponse(200, response);
        } else {
            sendJsonResponse(404, "{ \"error\": \"Relay not found\"}");
        }
    } catch (const std::exception& e) {
        sendJsonResponse(500, "{ \"error\": \"" + String(e.what()) + "\"}");
    }
}