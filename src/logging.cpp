#include "logging.h"

Logger::Logger(const char* logFileName) : logFileName(logFileName) {}

void Logger::begin() {
    #ifdef ESP32
    if (!LittleFS.begin(true)) {
    #else
    if (!LittleFS.begin()) {
    #endif
        Serial.println("Failed to mount file system");
        return;
    }
    File file = LittleFS.open(logFileName, "r");
    if (!file) {
      file = LittleFS.open(logFileName, "w");
      if (!file) {
        Serial.println("Failed to create log file");
        return;
      }
      file.println("Started logging");
      file.close();
    }
}

void Logger::log(const String& message) {
    appendToFile(message);
}

void Logger::appendToFile(const String& message) {
    // Basic rotation: if file > 50KB, clear it
    File checkFile = LittleFS.open(logFileName, "r");
    if (checkFile && checkFile.size() > 51200) {
        checkFile.close();
        File clearFile = LittleFS.open(logFileName, "w");
        if (clearFile) {
            clearFile.println("--- Log Rotated ---");
            clearFile.close();
        }
    } else if (checkFile) {
        checkFile.close();
    }

    File file = LittleFS.open(logFileName, "a");
    if (!file) {
        Serial.println("Failed to open log file");
        return;
    }
    file.println(message);
    file.close();
}

void Logger::printLogs() {
    File file = LittleFS.open(logFileName, "r");
    if (!file) {
        Serial.println("Failed to open log file");
        return;
    }
    while (file.available()) {
        Serial.write(file.read());
    }
    file.close();
}
