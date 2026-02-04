#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <WiFiClient.h>
#include "config.h"

class MqttManager {
public:
    MqttManager();
    void begin();
    void loop();
    void publish(const char* topic, const char* payload);
    bool isConnected();

private:
    WiFiClient espClient;
    WiFiClientSecure espClientSecure;
    PubSubClient client;
    long lastReconnectAttempt;
    
    void reconnect();
};

extern MqttManager mqttManager;

#endif
