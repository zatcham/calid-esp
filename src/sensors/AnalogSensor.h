#ifndef ANALOG_SENSOR_H
#define ANALOG_SENSOR_H

#include "../../include/SensorInterface.h"

class AnalogSensor : public SensorInterface {
public:
    AnalogSensor(int pin, String type, String dataType, String unit);
    void begin() override;
    SensorReadings read() override;
private:
    int _pin;
    String _type;
    String _dataType;
    String _unit;
};

#endif
