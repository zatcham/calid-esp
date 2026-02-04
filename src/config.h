#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

#define MAX_SENSORS 4

struct SensorConfig {
    int type; // 0: None, 11: DHT11, 22: DHT22, 2: BME280
    int pin;
};

struct Config {
    char ssid[32];
    char password[32];
    char apiEndpoint[128];
    char sensorId[32];
    char apiKey[64];
    bool testingMode;
    
    // Legacy support (to be migrated or removed eventually)
    int sensorType; 
    int sensorPin;

    // Multi-sensor support
    SensorConfig sensors[MAX_SENSORS];

    char mqttBroker[64];
    int mqttPort;
    char mqttUser[32];
    char mqttPassword[32];
    char mqttTopicPrefix[32];
    bool mqttEnabled;

    void load();
    void save();
    String getAdoptionCode();
    void writeStringToEEPROM(int addrOffset, const char* str, int length);
    void readStringFromEEPROM(int addrOffset, char* buffer, int length);
};

extern Config config;

#endif // CONFIG_H