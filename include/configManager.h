#pragma once
#include <nvs_flash.h>
#include <nvs.h>
#include <Arduino.h>

class ConfigManager {
private:
    // Private constructor
    ConfigManager();

    // Disable copy constructor and assignment operator
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    // Private static instance pointer
    static ConfigManager* instance;

public:
    // Get the singleton instance
    static ConfigManager* getInstance() {
        if (instance == nullptr) {
            instance = new ConfigManager();
        }
        return instance;
    }

    // Store a string value
    void setConfig(const String& key, const String& value);

    // Retrieve a string value
    String getConfig(const String& key, const String& default_value = "") const;
};