#ifndef BME280_SENSOR_H
#define BME280_SENSOR_H

#include "../../include/SensorInterface.h"
#include <Adafruit_BME280.h>

class BME280Sensor : public SensorInterface {
public:
    BME280Sensor(int pin); // pin here represents SDA for I2C usually
    void begin() override;
    SensorReadings read() override;
private:
    Adafruit_BME280 bme;
    int _pin;
};

#endif
