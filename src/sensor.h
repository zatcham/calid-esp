#ifndef SENSOR_H
#define SENSOR_H

#include "../include/SensorInterface.h"
#include "config.h"
#include <vector>

// Global storage for multiple sensors
extern SensorReadings allSensorData[MAX_SENSORS];
extern int activeSensorCount;

class Sensor {
public:
    Sensor();
    void begin();
    
    // Updates readings from all hardware sensors
    void update(); 

private:
    std::vector<SensorInterface*> sensors;
    void selectI2CChannel(int channel);
};

#endif // SENSOR_H
