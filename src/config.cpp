#include "config.h"
#include <LittleFS.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <user_interface.h>
#else
#include <WiFi.h>
#include <rom/crc.h>
#endif

Config config;

bool Config::load() {
    if (!LittleFS.exists(CONFIG_FILE)) {
        Serial.println("Config file not found, using defaults.");
        return false;
    }

    File configFile = LittleFS.open(CONFIG_FILE, "r");
    if (!configFile) {
        Serial.println("Failed to open config file.");
        return false;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, configFile);
    configFile.close();

    if (error) {
        Serial.println("Failed to parse config file, using defaults.");
        return false;
    }

    strlcpy(ssid, doc["ssid"] | "", sizeof(ssid));
    strlcpy(password, doc["password"] | "", sizeof(password));
    strlcpy(apiEndpoint, doc["apiEndpoint"] | "", sizeof(apiEndpoint));
    strlcpy(sensorId, doc["sensorId"] | "ESP-Device", sizeof(sensorId));
    strlcpy(apiKey, doc["apiKey"] | "", sizeof(apiKey));
    testingMode = doc["testingMode"] | false;
    
    strlcpy(adminUser, doc["adminUser"] | "admin", sizeof(adminUser));
    strlcpy(adminPassword, doc["adminPassword"] | "admin", sizeof(adminPassword));
    utcOffset = doc["utcOffset"] | 0;
    strlcpy(ntpServer, doc["ntpServer"] | "pool.ntp.org", sizeof(ntpServer));
    strlcpy(firmwareUrl, doc["firmwareUrl"] | "", sizeof(firmwareUrl));

    JsonArray sensorsArr = doc["sensors"];
    int i = 0;
    for (JsonObject s : sensorsArr) {
        if (i >= MAX_SENSORS) break;
        sensors[i].type = s["type"] | 0;
        sensors[i].pin = s["pin"] | 0;
        sensors[i].i2cAddress = s["i2cAddress"] | 0x76;
        sensors[i].i2cMultiplexerChannel = s["i2cMultiplexerChannel"] | -1;
        sensors[i].tempOffset = s["tempOffset"] | 0.0f;
        sensors[i].humOffset = s["humOffset"] | 0.0f;
        i++;
    }

    strlcpy(mqttBroker, doc["mqttBroker"] | "mqtt.calid.io", sizeof(mqttBroker));
    mqttPort = doc["mqttPort"] | 1883;
    strlcpy(mqttUser, doc["mqttUser"] | "", sizeof(mqttUser));
    strlcpy(mqttPassword, doc["mqttPassword"] | "", sizeof(mqttPassword));
    strlcpy(mqttTopicPrefix, doc["mqttTopicPrefix"] | "calid", sizeof(mqttTopicPrefix));
    mqttEnabled = doc.containsKey("mqttEnabled") ? doc["mqttEnabled"].as<bool>() : true;

    return true;
}

bool Config::save() {
    JsonDocument doc;
    doc["ssid"] = ssid;
    doc["password"] = password;
    doc["apiEndpoint"] = apiEndpoint;
    doc["sensorId"] = sensorId;
    doc["apiKey"] = apiKey;
    doc["testingMode"] = testingMode;
    doc["adminUser"] = adminUser;
    doc["adminPassword"] = adminPassword;
    doc["utcOffset"] = utcOffset;
    doc["ntpServer"] = ntpServer;
    doc["firmwareUrl"] = firmwareUrl;

    JsonArray sensorsArr = doc["sensors"].to<JsonArray>();
    for (int i = 0; i < MAX_SENSORS; i++) {
        JsonObject s = sensorsArr.add<JsonObject>();
        s["type"] = sensors[i].type;
        s["pin"] = sensors[i].pin;
        s["i2cAddress"] = sensors[i].i2cAddress;
        s["i2cMultiplexerChannel"] = sensors[i].i2cMultiplexerChannel;
        s["tempOffset"] = sensors[i].tempOffset;
        s["humOffset"] = sensors[i].humOffset;
    }

    doc["mqttBroker"] = mqttBroker;
    doc["mqttPort"] = mqttPort;
    doc["mqttUser"] = mqttUser;
    doc["mqttPassword"] = mqttPassword;
    doc["mqttTopicPrefix"] = mqttTopicPrefix;
    doc["mqttEnabled"] = mqttEnabled;

    File configFile = LittleFS.open(CONFIG_FILE, "w");
    if (!configFile) {
        return false;
    }

    serializeJson(doc, configFile);
    configFile.close();
    return true;
}

String Config::getAdoptionCode() {
    uint32_t crc = 0;
    #ifdef ESP32
    uint64_t mac = ESP.getEfuseMac();
    crc = crc32_le(0, (uint8_t*)&mac, sizeof(mac));
    #else
    uint32_t id = ESP.getChipId();
    crc = id; // Simple fallback for 8266 or use a crc lib if needed
    #endif
    
    char code[9];
    snprintf(code, sizeof(code), "%08X", crc);
    return String(code);
}

SystemHealth Config::getSystemHealth() {
    SystemHealth health;
    health.rssi = WiFi.RSSI();
    health.uptime = millis() / 1000;
    health.freeHeap = ESP.getFreeHeap();
    
    #ifdef ESP32
    esp_reset_reason_t reason = esp_reset_reason();
    switch (reason) {
        case ESP_RST_POWERON: health.resetReason = "Power On"; break;
        case ESP_RST_SW: health.resetReason = "Software Reset"; break;
        case ESP_RST_PANIC: health.resetReason = "Exception"; break;
        case ESP_RST_INT_WDT: health.resetReason = "Interrupt WDT"; break;
        case ESP_RST_TASK_WDT: health.resetReason = "Task WDT"; break;
        case ESP_RST_WDT: health.resetReason = "Other WDT"; break;
        case ESP_RST_DEEPSLEEP: health.resetReason = "Deep Sleep"; break;
        case ESP_RST_BROWNOUT: health.resetReason = "Brownout"; break;
        default: health.resetReason = "Unknown"; break;
    }
    #else
    health.resetReason = ESP.getResetReason();
    #endif
    
    return health;
}
