#ifndef BME280_SENSOR_H
#define BME280_SENSOR_H

#include "../../include/SensorInterface.h"
#include <Adafruit_BME280.h>

class BME280Sensor : public SensorInterface {
public:
    BME280Sensor(int pin, int i2cAddress = 0x76); 
    void begin() override;
    SensorReadings read() override;
private:
    Adafruit_BME280 bme;
    int _pin;
    int _i2cAddress;
};

#endif
