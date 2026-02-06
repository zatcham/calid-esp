#ifndef LIGHT_PROXIMITY_I2C_H
#define LIGHT_PROXIMITY_I2C_H

#include "../../include/SensorInterface.h"
#include <BH1750.h>
#include <Adafruit_TSL2561_U.h>
#include <Adafruit_VL53L0X.h>

class LightProximityI2C : public SensorInterface {
public:
    LightProximityI2C(int pin, String type, int i2cAddress);
    void begin() override;
    SensorReadings read() override;
private:
    int _pin;
    String _type;
    int _i2cAddress;
    BH1750 bh1750;
    Adafruit_TSL2561_Unified tsl;
    Adafruit_VL53L0X vl;
};

#endif
