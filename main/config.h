#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

struct Config {
    char ssid[32];
    char password[32];
    char apiEndpoint[128];
    char sensorId[32];
    char apiKey[64];
    bool testingMode;

    void load();
    void save();
    void writeStringToEEPROM(int addrOffset, const char* str, int length);
    void readStringFromEEPROM(int addrOffset, char* buffer, int length);
};

extern Config config;

#endif // CONFIG_H
