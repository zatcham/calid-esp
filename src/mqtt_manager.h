#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <WiFiClient.h>
#include "config.h"
#include <functional>

class MqttManager {
public:
    typedef std::function<void(String topic, String payload)> CommandCallback;

    MqttManager();
    void begin();
    void loop();
    void publishTelemetry(const char* payload);
    void publishStatus(const char* status);
    void publishRaw(const char* topic, const char* payload, bool retained = false);
    void setCommandCallback(CommandCallback cb);
    bool isConnected();

private:
    WiFiClient espClient;
    WiFiClientSecure espClientSecure;
    PubSubClient client;
    long lastReconnectAttempt;
    CommandCallback _commandCallback;
    
    void reconnect();
    void internalCallback(char* topic, byte* payload, unsigned int length);
};

extern MqttManager mqttManager;

#endif