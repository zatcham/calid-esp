#include "config.h"
#include <EEPROM.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif

Config config;

void Config::load() {
    EEPROM.begin(1024); // Initialize EEPROM with size 1024 bytes
    EEPROM.get(0, *this);
    EEPROM.end();
}

void Config::save() {
    EEPROM.begin(1024); // Initialize EEPROM with size 1024 bytes
    EEPROM.put(0, *this);
    EEPROM.commit();
    EEPROM.end();
}

String Config::getAdoptionCode() {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char code[7];
    snprintf(code, sizeof(code), "%02X%02X%02X", mac[3], mac[4], mac[5]);
    return String(code);
}

void Config::writeStringToEEPROM(int addrOffset, const char* str, int length) {
    for (int i = 0; i < length; i++) {
        EEPROM.write(addrOffset + i, str[i]);
    }
    EEPROM.write(addrOffset + length, '\0'); // Ensure null termination
    EEPROM.commit();
}

void Config::readStringFromEEPROM(int addrOffset, char* buffer, int length) {
    for (int i = 0; i < length; i++) {
        buffer[i] = EEPROM.read(addrOffset + i);
        if (buffer[i] == '\0') {
            break; // Stop reading if null terminator is found
        }
    }
    buffer[length - 1] = '\0'; // Ensure null termination
}
