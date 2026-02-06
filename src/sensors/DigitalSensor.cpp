#include "DigitalSensor.h"

DigitalSensor::DigitalSensor(int pin, String type) : _pin(pin), _type(type) {}

void DigitalSensor::begin() {
    if (_type == "relay") {
        pinMode(_pin, OUTPUT);
    } else {
        pinMode(_pin, INPUT);
    }
}

SensorReadings DigitalSensor::read() {
    SensorReadings data;
    data.pin = _pin;
    data.sensorType = _type;
    data.valid = true;

    float val = (float)digitalRead(_pin);
    String dataType = (_type == "pir") ? "Motion" : "State";
    String unit = "bool";

    data.readings.push_back({dataType, val, unit});
    return data;
}
