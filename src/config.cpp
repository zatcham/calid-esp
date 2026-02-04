#include "config.h"
#include <LittleFS.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif

Config config;

bool Config::load() {
    if (!LittleFS.exists(CONFIG_FILE)) {
        return false;
    }

    File configFile = LittleFS.open(CONFIG_FILE, "r");
    if (!configFile) {
        return false;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, configFile);
    configFile.close();

    if (error) {
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

    JsonArray sensorsArr = doc["sensors"];
    int i = 0;
    for (JsonObject s : sensorsArr) {
        if (i >= MAX_SENSORS) break;
        sensors[i].type = s["type"] | 0;
        sensors[i].pin = s["pin"] | 0;
        sensors[i].i2cAddress = s["i2cAddress"] | 0x76;
        i++;
    }

    strlcpy(mqttBroker, doc["mqttBroker"] | "", sizeof(mqttBroker));
    mqttPort = doc["mqttPort"] | 1883;
    strlcpy(mqttUser, doc["mqttUser"] | "", sizeof(mqttUser));
    strlcpy(mqttPassword, doc["mqttPassword"] | "", sizeof(mqttPassword));
    strlcpy(mqttTopicPrefix, doc["mqttTopicPrefix"] | "calid", sizeof(mqttTopicPrefix));
    mqttEnabled = doc["mqttEnabled"] | false;

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

    JsonArray sensorsArr = doc["sensors"].to<JsonArray>();
    for (int i = 0; i < MAX_SENSORS; i++) {
        JsonObject s = sensorsArr.add<JsonObject>();
        s["type"] = sensors[i].type;
        s["pin"] = sensors[i].pin;
        s["i2cAddress"] = sensors[i].i2cAddress;
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
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char code[7];
    snprintf(code, sizeof(code), "%02X%02X%02X", mac[3], mac[4], mac[5]);
    return String(code);
}