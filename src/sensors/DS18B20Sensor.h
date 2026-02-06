#ifndef DS18B20_SENSOR_H
#define DS18B20_SENSOR_H

#include "../../include/SensorInterface.h"
#include <OneWire.h>
#include <DallasTemperature.h>

class DS18B20Sensor : public SensorInterface {
public:
    DS18B20Sensor(int pin);
    void begin() override;
    SensorReadings read() override;
private:
    OneWire oneWire;
    DallasTemperature sensors;
    int _pin;
};

#endif
