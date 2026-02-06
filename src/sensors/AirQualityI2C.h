#ifndef AIR_QUALITY_I2C_H
#define AIR_QUALITY_I2C_H

#include "../../include/SensorInterface.h"
#include <Adafruit_CCS811.h>
#include <SensirionI2CScd4x.h>

class AirQualityI2C : public SensorInterface {
public:
    AirQualityI2C(int pin, String type, int i2cAddress);
    void begin() override;
    SensorReadings read() override;
private:
    int _pin;
    String _type;
    int _i2cAddress;
    Adafruit_CCS811 ccs;
    SensirionI2CScd4x scd;
};

#endif
