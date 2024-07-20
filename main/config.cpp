#include "config.h"
#include <EEPROM.h>

Config config;

void Config::load() {
    EEPROM.begin(512); // Initialize EEPROM with size 512 bytes
    EEPROM.get(0, *this);
    EEPROM.end();
}

void Config::save() {
    EEPROM.begin(512); // Initialize EEPROM with size 512 bytes
    EEPROM.put(0, *this);
    EEPROM.commit();
    EEPROM.end();
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
