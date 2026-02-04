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
            impl = new BME280Sensor(pin, config.sensors[i].i2cAddress);
        }

        if (impl) {
            impl->begin();
            sensors.push_back(impl);
            allSensorData[activeSensorCount].pin = pin;
            allSensorData[activeSensorCount].valid = false;
            activeSensorCount++;
        }
    }
}

void Sensor::update() {
    for (size_t i = 0; i < sensors.size() && i < (size_t)activeSensorCount; i++) {
        allSensorData[i] = sensors[i]->read();
    }
}