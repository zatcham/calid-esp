#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <ArduinoJson.h>

#define MAX_SENSORS 4
#define CONFIG_FILE "/config.json"

struct SensorConfig {
    int type = 0; // 0: None, 11: DHT11, 22: DHT22, 2: BME280
    int pin = 0;
    int i2cAddress = 0x76; // For BME280
    int i2cMultiplexerChannel = -1; // -1: None, 0-7: TCA9548A channel
    float tempOffset = 0.0f;
    float humOffset = 0.0f;
};

struct SystemHealth {
    int rssi;
    uint32_t uptime;
    uint32_t freeHeap;
    String resetReason;
};

struct Config {
    char ssid[32] = "";
    char password[32] = "";
    char apiEndpoint[128] = "";
    char sensorId[32] = "ESP-Device";
    char apiKey[64] = "";
    bool testingMode = false;
    
    // Auth
    char adminUser[32] = "admin";
    char adminPassword[64] = "admin";

    // Time
    int utcOffset = 0;
    char ntpServer[64] = "pool.ntp.org";
    char firmwareUrl[128] = "";

    // Multi-sensor support
    SensorConfig sensors[MAX_SENSORS];

    char mqttBroker[64] = "mqtt.calid.io";
    int mqttPort = 1883;
    char mqttUser[32] = "";
    char mqttPassword[32] = "";
    char mqttTopicPrefix[32] = "calid";
    bool mqttEnabled = true;

    bool load();
    bool save();
    String getAdoptionCode();
    SystemHealth getSystemHealth();
};

extern Config config;

#endif // CONFIG_H
