#include "BMP280Sensor.h"

BMP280Sensor::BMP280Sensor(int pin, int i2cAddress) : _pin(pin), _i2cAddress(i2cAddress) {}

void BMP280Sensor::begin() {
    if (!bmp.begin(_i2cAddress)) { 
        Serial.printf("Could not find a valid BMP280 sensor at 0x%02X!
", _i2cAddress);
    }
}

SensorReadings BMP280Sensor::read() {
    SensorReadings data;
    data.pin = _pin;
    data.sensorType = "BMP280";
    
    float t = bmp.readTemperature();
    float p = bmp.readPressure() / 100.0F;
    
    data.valid = !isnan(t); 
    
    if (data.valid) {
        data.readings.push_back({"Temperature", t, "C"});
        data.readings.push_back({"Pressure", p, "hPa"});
    } else {
        data.error = "Failed to read from BMP280 sensor";
    }
    return data;
}
