#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>

// Netzwerkeinstellungen
const char *ssid = "ESP32-Access-Point";
const char *password = "12345678";

// Erstellen des WebServers (Port 80)
WebServer server(80);

String getContentType(String);

// Eine Funktion, die versucht, die angeforderte Datei aus SPIFFS zu senden
void handleFileRead(String path)
{
    Serial.println("Handling file read for: " + path);
    if (path.endsWith("/"))
    {
        path += "index.html"; // Standardmäßig index.html, wenn keine Datei angegeben ist
    }

    if (SPIFFS.exists(path))
    {
        File file = SPIFFS.open(path, "r");
        String contentType = getContentType(path); // Inhaltstyp basierend auf der Dateierweiterung bestimmen
        server.streamFile(file, contentType);
        file.close();
    }
    else if (SPIFFS.exists(path + "/index.html"))
    {
        path = path + "/index.html";
        File file = SPIFFS.open(path, "r");
        String contentType = getContentType(path); // Inhaltstyp basierend auf der Dateierweiterung bestimmen
        server.streamFile(file, contentType);
        file.close();
    }
    {
        // Send 404 if the file does not exist
        server.send(404, "text/plain", "404: Not Found");
    }
}

// Eine Hilfsfunktion, um den MIME-Typ einer Datei zu bestimmen
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

// Funktionen zur Handhabung der API-Anfragen
void handleGetAllRelays()
{
    String json = "[{\"id\": 1, \"name\": \"Relay 1\", \"state\": \"on\"}, {\"id\": 2, \"name\": \"Relay 2\", \"state\": \"off\"}]";
    server.send(200, "application/json", json);
}

void handleRelayControl()
{
    if (!server.hasArg("relayId") || !server.hasArg("state"))
    {
        server.send(400, "application/json", "{\"error\": \"Missing or invalid parameters\"}");
        return;
    }
    String response = "{\"message\": \"Relay state updated successfully\", \"relayId\": " + server.arg("relayId") + ", \"currentState\": \"" + server.arg("state") + "\"}";
    server.send(200, "application/json", response);
}

void handleSystemSettings()
{
    String json = "{\"systemName\": \"Smart Relays\", \"wifiName\": \"" + String(ssid) + "\", \"systemTime\": \"11:32:45\", \"systemDate\": \"2024-07-23\", \"syncTime\": true, \"relayNames\": {\"relay1\": \"Relay 1\", \"relay2\": \"Relay 2\"}}";
    server.send(200, "application/json", json);
}

void handleGetRelayAlarms()
{
    if (!server.hasArg("relayId"))
    {
        server.send(400, "application/json", "{\"error\": \"Missing relayId parameter\"}");
        return;
    }

    int relayId = server.arg("relayId").toInt();
    if (relayId <= 0)
    {
        server.send(404, "application/json", "{\"error\": \"Relay not found\"}");
        return;
    }

    String json = "["
                  "{\"id\": 1, \"state\": \"on\", \"time\": \"06:00:00\", \"days\": [\"mon\", \"wed\", \"fri\"]},"
                  "{\"id\": 2, \"state\": \"off\", \"time\": \"22:00:00\", \"days\": [\"tue\", \"thu\", \"sat\", \"sun\"]}"
                  "]";
    server.send(200, "application/json", json);
}

void setup()
{
    Serial.begin(115200);

    if (!SPIFFS.begin(true))
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }

    WiFi.softAP(ssid, password);
    Serial.println("Access Point gestartet");

    // Routen für den WebServer definieren
    // Web
    server.onNotFound([]()
    {
        handleFileRead(server.uri());
    });
    // API
    server.on("/api/all-relays", HTTP_GET, handleGetAllRelays);
    server.on("/api/relay-control", HTTP_POST, handleRelayControl);
    server.on("/api/settings", HTTP_GET, handleSystemSettings);
    server.on("/api/relay-alarms", HTTP_GET, handleGetRelayAlarms);

    // Starten des Servers
    server.begin();
    Serial.println("HTTP server gestartet");
}

void loop()
{
    server.handleClient();
}
