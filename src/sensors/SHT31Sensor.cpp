#include "SHT31Sensor.h"

SHT31Sensor::SHT31Sensor(int pin, int i2cAddress) : _pin(pin), _i2cAddress(i2cAddress) {}

void SHT31Sensor::begin() {
    if (!sht.begin(_i2cAddress)) {
        Serial.printf("Could not find a valid SHT31 sensor at 0x%02X!
", _i2cAddress);
    }
}

SensorReadings SHT31Sensor::read() {
    SensorReadings data;
    data.pin = _pin;
    data.sensorType = "SHT31";
    
    float t = sht.readTemperature();
    float h = sht.readHumidity();
    
    data.valid = !isnan(t);
    
    if (data.valid) {
        data.readings.push_back({"Temperature", t, "C"});
        data.readings.push_back({"Humidity", h, "%"});
    } else {
        data.error = "Failed to read from SHT31 sensor";
    }
    return data;
}
