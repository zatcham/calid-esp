#ifndef DHT_SENSOR_H
#define DHT_SENSOR_H

#include "../../include/SensorInterface.h"
#include <DHT.h>

class DHTSensor : public SensorInterface {
public:
    DHTSensor(int pin, int type);
    void begin() override;
    SensorReadings read() override;
private:
    DHT dht;
    int _pin;
    int _type;
};

#endif
