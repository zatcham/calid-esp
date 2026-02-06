#ifndef DIGITAL_SENSOR_H
#define DIGITAL_SENSOR_H

#include "../../include/SensorInterface.h"

class DigitalSensor : public SensorInterface {
public:
    DigitalSensor(int pin, String type);
    void begin() override;
    SensorReadings read() override;
private:
    int _pin;
    String _type;
};

#endif
