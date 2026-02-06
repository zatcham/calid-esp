#include "DS18B20Sensor.h"

DS18B20Sensor::DS18B20Sensor(int pin) : oneWire(pin), sensors(&oneWire), _pin(pin) {}

void DS18B20Sensor::begin() {
    sensors.begin();
}

SensorReadings DS18B20Sensor::read() {
    SensorReadings data;
    data.pin = _pin;
    data.sensorType = "DS18B20";
    
    sensors.requestTemperatures();
    float t = sensors.getTempCByIndex(0);
    
    data.valid = (t != DEVICE_DISCONNECTED_C);
    
    if (data.valid) {
        data.readings.push_back({"Temperature", t, "C"});
    } else {
        data.error = "Failed to read from DS18B20 sensor";
    }
    return data;
}
