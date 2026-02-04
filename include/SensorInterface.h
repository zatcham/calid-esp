#ifndef SENSOR_INTERFACE_H
#define SENSOR_INTERFACE_H

#include <Arduino.h>
#include <vector>

struct Reading {
    String type;  // e.g. "Temperature", "Humidity", "Pressure"
    float value;
    String unit;  // e.g. "C", "%", "hPa"
};

struct SensorReadings {
    int pin;
    String sensorType; // e.g. "DHT22", "BME280"
    std::vector<Reading> readings;
    bool valid;
    String error;
};

class SensorInterface {
public:
    virtual ~SensorInterface() {}
    virtual void begin() = 0;
    virtual SensorReadings read() = 0;
};

#endif