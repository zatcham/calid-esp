#include "mqtt_manager.h"
#include <Arduino.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif

MqttManager mqttManager;

MqttManager::MqttManager() : lastReconnectAttempt(0), _commandCallback(nullptr) {}

void MqttManager::begin() {
    if (!config.mqttEnabled) return;

    bool secure = (config.mqttPort == 8883);

    if (secure) {
        #ifdef ESP8266
        espClientSecure.setInsecure();
        #else
        espClientSecure.setInsecure();
        #endif
        client.setClient(espClientSecure);
    } else {
        client.setClient(espClient);
    }

    client.setServer(config.mqttBroker, config.mqttPort);
    client.setCallback([this](char* topic, byte* payload, unsigned int length) {
        this->internalCallback(topic, payload, length);
    });
}

void MqttManager::loop() {
    if (!config.mqttEnabled) return;

    if (!client.connected()) {
        long now = millis();
        if (now - lastReconnectAttempt > 5000) {
            lastReconnectAttempt = now;
            reconnect();
        }
    } else {
        client.loop();
    }
}

void MqttManager::reconnect() {
    if (WiFi.status() != WL_CONNECTED) return;

    Serial.print("Attempting MQTT connection...");
    
    String clientId = "CalidESP-";
    clientId += config.getAdoptionCode();

    String statusTopic = "sensors/" + String(config.sensorId) + "/status";
    
    // Connect with LWT (Last Will and Testament)
    if (client.connect(clientId.c_str(), config.mqttUser, config.mqttPassword, 
                       statusTopic.c_str(), 1, true, "offline")) {
        Serial.println("connected");
        
        // Publish online status
        client.publish(statusTopic.c_str(), "online", true);
        
        // Subscribe to commands
        String commandTopic = "sensors/" + String(config.sensorId) + "/commands";
        client.subscribe(commandTopic.c_str());
        
        Serial.printf("Subscribed to %s\n", commandTopic.c_str());
    } else {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");
    }
}

void MqttManager::publishTelemetry(const char* payload) {
    String topic = "sensors/" + String(config.sensorId) + "/telemetry";
    publishRaw(topic.c_str(), payload);
}

void MqttManager::publishStatus(const char* status) {
    String topic = "sensors/" + String(config.sensorId) + "/status";
    publishRaw(topic.c_str(), status, true);
}

void MqttManager::publishRaw(const char* topic, const char* payload, bool retained) {
    if (!config.mqttEnabled || !client.connected()) return;
    client.publish(topic, payload, retained);
}

void MqttManager::setCommandCallback(CommandCallback cb) {
    _commandCallback = cb;
}

void MqttManager::internalCallback(char* topic, byte* payload, unsigned int length) {
    String payloadStr = "";
    for (unsigned int i = 0; i < length; i++) {
        payloadStr += (char)payload[i];
    }
    
    Serial.printf("MQTT Message [%s]: %s\n", topic, payloadStr.c_str());
    
    if (_commandCallback) {
        _commandCallback(String(topic), payloadStr);
    }
}

bool MqttManager::isConnected() {
    return client.connected();
}