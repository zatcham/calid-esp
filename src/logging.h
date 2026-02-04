#ifndef LOGGING_H
#define LOGGING_H

#include <Arduino.h>
#include <LittleFS.h>

class Logger {
public:
    Logger(const char* logFileName = "/log.txt");
    void begin();
    void log(const String& message);
    void printLogs();

private:
    String logFileName;
    void appendToFile(const String& message);
};

#endif // LOGGING_H