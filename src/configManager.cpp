#include "configManager.h"

void ConfigManager::setConfig(const String &key, const String &value)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &handle);
    if (err == ESP_OK)
    {
        nvs_set_str(handle, key.c_str(), value.c_str());
        nvs_commit(handle);
        nvs_close(handle);
    }
}

String ConfigManager::getConfig(const String &key, const String &default_value = "")
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &handle);
    String value = default_value;
    if (err == ESP_OK)
    {
        size_t required_size;
        err = nvs_get_str(handle, key.c_str(), nullptr, &required_size);
        if (err == ESP_OK)
        {
            char *buffer = new char[required_size];
            nvs_get_str(handle, key.c_str(), buffer, &required_size);
            value = String(buffer);
            delete[] buffer;
        }
        nvs_close(handle);
    }
    return value;
}

ConfigManager::ConfigManager()
{
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        nvs_flash_erase();
        nvs_flash_init();
    }
}