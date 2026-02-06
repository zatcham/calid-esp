#include "LightProximityI2C.h"

LightProximityI2C::LightProximityI2C(int pin, String type, int i2cAddress) 
    : _pin(pin), _type(type), _i2cAddress(i2cAddress), tsl(i2cAddress, 12345) {}

void LightProximityI2C::begin() {
    if (_type == "bh1750") {
        bh1750.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, _i2cAddress);
    } else if (_type == "tsl2561") {
        tsl.begin();
        tsl.enableAutoRange(true);
    } else if (_type == "vl53l0x") {
        vl.begin(_i2cAddress);
    }
}

SensorReadings LightProximityI2C::read() {
    SensorReadings data;
    data.pin = _pin;
    data.sensorType = _type;
    data.valid = false;

    if (_type == "bh1750") {
        float lux = bh1750.readLightLevel();
        if (lux >= 0) {
            data.readings.push_back({"Light", lux, "lx"});
            data.valid = true;
        }
    } else if (_type == "tsl2561") {
        sensors_event_t event;
        tsl.getEvent(&event);
        if (event.light) {
            data.readings.push_back({"Light", event.light, "lx"});
            data.valid = true;
        }
    } else if (_type == "vl53l0x") {
        VL53L0X_RangingMeasurementData_t measure;
        vl.rangingTest(&measure, false);
        if (measure.RangeStatus != 4) {
            data.readings.push_back({"Distance", (float)measure.RangeMilliMeter, "mm"});
            data.valid = true;
        }
    }

    if (!data.valid) data.error = "Read failed";
    return data;
}
