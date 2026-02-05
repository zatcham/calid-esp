#include "sensor.h"
#include "config.h"
#include "sensors/DHTSensor.h"
#include "sensors/BME280Sensor.h"

#include <Wire.h>

SensorReadings allSensorData[MAX_SENSORS];
int activeSensorCount = 0;

void Sensor::selectI2CChannel(int channel) {
    if (channel < 0 || channel > 7) return;
    Wire.beginTransmission(0x70); // TCA9548A address
    Wire.write(1 << channel);
    Wire.endTransmission();
}

Sensor::Sensor() {}

void Sensor::begin() {
    for (auto s : sensors) {
        delete s;
    }
    sensors.clear();
    activeSensorCount = 0;

    for (int i = 0; i < MAX_SENSORS; i++) {
        int type = config.sensors[i].type;
        int pin = config.sensors[i].pin;
        int muxChannel = config.sensors[i].i2cMultiplexerChannel;

        if (type == 0) continue;

        if (type == 2 && muxChannel >= 0) {
            selectI2CChannel(muxChannel);
        }

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
    int activeIdx = 0;
    for (int i = 0; i < MAX_SENSORS; i++) {
        if (config.sensors[i].type == 0) continue;
        
        if (config.sensors[i].type == 2 && config.sensors[i].i2cMultiplexerChannel >= 0) {
            selectI2CChannel(config.sensors[i].i2cMultiplexerChannel);
        }
        
        if (activeIdx < (int)sensors.size()) {
            allSensorData[activeIdx] = sensors[activeIdx]->read();
            
            // Apply Offsets
            for (auto& r : allSensorData[activeIdx].readings) {
                if (r.type == "Temperature") r.value += config.sensors[i].tempOffset;
                if (r.type == "Humidity") r.value += config.sensors[i].humOffset;
            }
            
            activeIdx++;
        }
    }
}
