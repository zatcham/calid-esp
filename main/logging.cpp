#include "logging.h"

Logger::Logger(const char* logFileName) : logFileName(logFileName) {}

void Logger::begin() {
    if (!LittleFS.begin()) {
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
