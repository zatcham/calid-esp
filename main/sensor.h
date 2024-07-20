#ifndef SENSOR_H
#define SENSOR_H

#include <DHT.h>

// Define the SensorData struct
struct SensorData {
    float temperature;
    float humidity;
    bool valid;
};

// Declare sensorData as an external variable
extern SensorData sensorData;

class Sensor {
public:
    Sensor(int pin, int type);
    void begin();
    float readTemperature();
    float readHumidity();

private:
    DHT dht;
};

#endif // SENSOR_H
