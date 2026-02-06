#include "AirQualityI2C.h"

AirQualityI2C::AirQualityI2C(int pin, String type, int i2cAddress) 
    : _pin(pin), _type(type), _i2cAddress(i2cAddress) {}

void AirQualityI2C::begin() {
    if (_type == "ccs811") {
        if (!ccs.begin(_i2cAddress)) {
            Serial.println("CCS811 not found");
        }
    } else if (_type == "scd40") {
        scd.begin(Wire);
        scd.startPeriodicMeasurement();
    }
}

SensorReadings AirQualityI2C::read() {
    SensorReadings data;
    data.pin = _pin;
    data.sensorType = _type;
    data.valid = false;

    if (_type == "ccs811") {
        if (ccs.available()) {
            if (!ccs.readData()) {
                data.readings.push_back({"CO2", (float)ccs.geteCO2(), "ppm"});
                data.readings.push_back({"TVOC", (float)ccs.getTVOC(), "ppb"});
                data.valid = true;
            }
        }
    } else if (_type == "scd40") {
        uint16_t co2;
        float t, h;
        if (scd.readMeasurement(co2, t, h) == 0) {
            data.readings.push_back({"CO2", (float)co2, "ppm"});
            data.readings.push_back({"Temperature", t, "C"});
            data.readings.push_back({"Humidity", h, "%"});
            data.valid = true;
        }
    }

    if (!data.valid) data.error = "No data available";
    return data;
}
