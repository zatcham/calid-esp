#include "sensor.h"

// Define the sensorData variable
SensorData sensorData;

Sensor::Sensor(int pin, int type) : dht(pin, type) {}

void Sensor::begin() {
    dht.begin();
}

float Sensor::readTemperature() {
    float temperature = dht.readTemperature();
    if (isnan(temperature)) {
        sensorData.valid = false;
    } else {
        sensorData.temperature = temperature;
        sensorData.valid = true;
    }
    return temperature;
}

float Sensor::readHumidity() {
    float humidity = dht.readHumidity();
    if (isnan(humidity)) {
        sensorData.valid = false;
    } else {
        sensorData.humidity = humidity;
        sensorData.valid = true;
    }
    return humidity;
}
