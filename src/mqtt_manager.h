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
    void publishAck(const String& commandId, const String& status, const String& details);
    void publishOtaStatus(const String& status, int progress = -1, const String& version = "");
    void publishRaw(const char* topic, const char* payload, bool retained = false);
    void setCommandCallback(CommandCallback cb);
    bool isConnected();
    String getDeviceIdentifier() const;

private:
    WiFiClient espClient;
    WiFiClientSecure espClientSecure;
    PubSubClient client;
    long lastReconnectAttempt;
    CommandCallback _commandCallback;
    
    void reconnect();
    void internalCallback(char* topic, byte* payload, unsigned int length);
    String getLegacySensorId() const;
};

extern MqttManager mqttManager;

#endif
