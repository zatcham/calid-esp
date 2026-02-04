#include "DHTSensor.h"

DHTSensor::DHTSensor(int pin, int type) : dht(pin, type), _pin(pin), _type(type) {}

void DHTSensor::begin() {
    dht.begin();
}

SensorReadings DHTSensor::read() {
    SensorReadings data;
    data.pin = _pin;
    data.sensorType = (_type == 22) ? "DHT22" : "DHT11";
    
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    
    data.valid = !isnan(t) && !isnan(h);
    
    if (data.valid) {
        data.readings.push_back({"Temperature", t, "C"});
        data.readings.push_back({"Humidity", h, "%"});
    } else {
        data.error = "Failed to read from DHT sensor";
    }
    
    return data;
}
