#ifndef BMP280_SENSOR_H
#define BMP280_SENSOR_H

#include "../../include/SensorInterface.h"
#include <Adafruit_BMP280.h>

class BMP280Sensor : public SensorInterface {
public:
    BMP280Sensor(int pin, int i2cAddress = 0x77); 
    void begin() override;
    SensorReadings read() override;
private:
    Adafruit_BMP280 bmp;
    int _pin;
    int _i2cAddress;
};

#endif
