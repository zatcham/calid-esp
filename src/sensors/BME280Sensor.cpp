#include "BME280Sensor.h"

BME280Sensor::BME280Sensor(int pin, int i2cAddress) : _pin(pin), _i2cAddress(i2cAddress) {}

void BME280Sensor::begin() {
    if (!bme.begin(_i2cAddress)) { 
        Serial.printf("Could not find a valid BME280 sensor at 0x%02X!\n", _i2cAddress);
    }
}

SensorReadings BME280Sensor::read() {
    SensorReadings data;
    data.pin = _pin;
    data.sensorType = "BME280";
    
    float t = bme.readTemperature();
    float h = bme.readHumidity();
    float p = bme.readPressure() / 100.0F;
    
    data.valid = !isnan(t); 
    
    if (data.valid) {
        data.readings.push_back({"Temperature", t, "C"});
        data.readings.push_back({"Humidity", h, "%"});
        data.readings.push_back({"Pressure", p, "hPa"});
    } else {
        data.error = "Failed to read from BME280 sensor";
    }
    return data;
}
