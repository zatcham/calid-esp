#include "sensor.h"
#include "config.h"
#include "sensors/DHTSensor.h"
#include "sensors/BME280Sensor.h"

SensorReadings allSensorData[MAX_SENSORS];
int activeSensorCount = 0;

Sensor::Sensor() {}

void Sensor::begin() {
    // Clear existing
    for (auto s : sensors) {
        delete s;
    }
    sensors.clear();
    activeSensorCount = 0;

    for (int i = 0; i < MAX_SENSORS; i++) {
        int type = config.sensors[i].type;
        int pin = config.sensors[i].pin;

        if (type == 0) continue;

        SensorInterface* impl = nullptr;
        if (type == 11) {
            impl = new DHTSensor(pin, DHT11);
        } else if (type == 22) {
            impl = new DHTSensor(pin, DHT22);
        } else if (type == 2) {
            impl = new BME280Sensor(pin);
        }

        if (impl) {
            impl->begin();
            sensors.push_back(impl);
            allSensorData[activeSensorCount].pin = pin;
            allSensorData[activeSensorCount].valid = false;
            activeSensorCount++;
        }
    }

    // Legacy fallback if no sensors defined in array but defined in legacy fields
    if (activeSensorCount == 0 && config.sensorPin != 0) {
        SensorInterface* impl = nullptr;
        if (config.sensorType == 0 || config.sensorType == 11) {
            impl = new DHTSensor(config.sensorPin, DHT11);
        } else if (config.sensorType == 1 || config.sensorType == 22) {
            impl = new DHTSensor(config.sensorPin, DHT22);
        } else if (config.sensorType == 2) {
            impl = new BME280Sensor(config.sensorPin);
        }

        if (impl) {
            impl->begin();
            sensors.push_back(impl);
            allSensorData[activeSensorCount].pin = config.sensorPin;
            activeSensorCount++;
        }
    }
}

void Sensor::update() {
    for (size_t i = 0; i < sensors.size() && i < MAX_SENSORS; i++) {
        allSensorData[i] = sensors[i]->read();
    }
}
