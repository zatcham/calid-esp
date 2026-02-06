#include "sensor.h"
#include "config.h"
#include "sensors/DHTSensor.h"
#include "sensors/BME280Sensor.h"
#include "sensors/BMP280Sensor.h"
#include "sensors/DS18B20Sensor.h"
#include "sensors/SHT31Sensor.h"
#include "sensors/AnalogSensor.h"
#include "sensors/AirQualityI2C.h"
#include "sensors/LightProximityI2C.h"
#include "sensors/DigitalSensor.h"

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
        String type = String(config.sensors[i].type);
        int pin = config.sensors[i].pin;
        int addr = config.sensors[i].i2cAddress;
        int muxChannel = config.sensors[i].i2cMultiplexerChannel;

        if (type == "none" || type == "") continue;

        bool isI2C = (type == "bme280" || type == "bmp280" || type == "sht31" || 
                      type == "ccs811" || type == "scd40" || type == "bh1750" || 
                      type == "tsl2561" || type == "vl53l0x");

        if (isI2C && muxChannel >= 0) {
            selectI2CChannel(muxChannel);
        }

        SensorInterface* impl = nullptr;
        
        // Temperature & Humidity
        if (type == "dht11") impl = new DHTSensor(pin, 11);
        else if (type == "dht22") impl = new DHTSensor(pin, 22);
        else if (type == "ds18b20") impl = new DS18B20Sensor(pin);
        else if (type == "bme280") impl = new BME280Sensor(pin, addr);
        else if (type == "bmp280") impl = new BMP280Sensor(pin, addr);
        else if (type == "sht31") impl = new SHT31Sensor(pin, addr);
        
        // Analog
        else if (type == "lm35") impl = new AnalogSensor(pin, type, "Temperature", "C");
        else if (type == "tmp36") impl = new AnalogSensor(pin, type, "Temperature", "C");
        else if (type == "mq2") impl = new AnalogSensor(pin, type, "Smoke", "raw");
        else if (type == "mq135") impl = new AnalogSensor(pin, type, "Air Quality", "raw");
        else if (type == "ldr") impl = new AnalogSensor(pin, type, "Light", "raw");
        else if (type == "soil_moisture") impl = new AnalogSensor(pin, type, "Moisture", "%");
        else if (type == "water_level") impl = new AnalogSensor(pin, type, "Level", "raw");
        else if (type == "ph_sensor") impl = new AnalogSensor(pin, type, "pH", "raw");
        else if (type == "tds_meter") impl = new AnalogSensor(pin, type, "TDS", "ppm");
        
        // I2C Special
        else if (type == "ccs811" || type == "scd40") impl = new AirQualityI2C(pin, type, addr);
        else if (type == "bh1750" || type == "tsl2561" || type == "vl53l0x") impl = new LightProximityI2C(pin, type, addr);
        
        // Digital
        else if (type == "pir" || type == "relay") impl = new DigitalSensor(pin, type);

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
        String type = String(config.sensors[i].type);
        if (type == "none" || type == "") continue;
        
        bool isI2C = (type == "bme280" || type == "bmp280" || type == "sht31" || 
                      type == "ccs811" || type == "scd40" || type == "bh1750" || 
                      type == "tsl2561" || type == "vl53l0x");

        if (isI2C && config.sensors[i].i2cMultiplexerChannel >= 0) {
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