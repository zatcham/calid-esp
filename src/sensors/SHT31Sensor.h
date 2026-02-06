#ifndef SHT31_SENSOR_H
#define SHT31_SENSOR_H

#include "../../include/SensorInterface.h"
#include <Adafruit_SHT31.h>

class SHT31Sensor : public SensorInterface {
public:
    SHT31Sensor(int pin, int i2cAddress = 0x44);
    void begin() override;
    SensorReadings read() override;
private:
    Adafruit_SHT31 sht;
    int _pin;
    int _i2cAddress;
};

#endif
