#include "AnalogSensor.h"

AnalogSensor::AnalogSensor(int pin, String type, String dataType, String unit) 
    : _pin(pin), _type(type), _dataType(dataType), _unit(unit) {}

void AnalogSensor::begin() {
    pinMode(_pin, INPUT);
}

SensorReadings AnalogSensor::read() {
    SensorReadings data;
    data.pin = _pin;
    data.sensorType = _type;
    
    int raw = analogRead(_pin);
    float value = (float)raw;

    // Basic scaling logic for specific common types
    if (_type == "lm35") {
        #ifdef ESP32
        value = (raw * 330.0f) / 4095.0f;
        #else
        value = (raw * 100.0f) / 1024.0f;
        #endif
    } else if (_type == "tmp36") {
        #ifdef ESP32
        float voltage = (raw * 3300.0f) / 4095.0f;
        #else
        float voltage = (raw * 1000.0f) / 1024.0f;
        #endif
        value = (voltage - 500.0f) / 10.0f;
    } else if (_type == "soil_moisture") {
        // Map 0-1023 (or 4095) to 0-100% inverse
        #ifdef ESP32
        value = 100.0f - ((raw * 100.0f) / 4095.0f);
        #else
        value = 100.0f - ((raw * 100.0f) / 1024.0f);
        #endif
    }

    data.valid = true;
    data.readings.push_back({_dataType, value, _unit});
    
    return data;
}
