#include "mqtt_manager.h"
#include <Arduino.h>

MqttManager mqttManager;

MqttManager::MqttManager() : lastReconnectAttempt(0) {}

void MqttManager::begin() {
    if (!config.mqttEnabled) return;

    bool secure = (config.mqttPort == 8883);

    if (secure) {
        #ifdef ESP8266
        espClientSecure.setInsecure();
        #else
        espClientSecure.setInsecure(); // Disable cert check for simplicity
        #endif
        client.setClient(espClientSecure);
    } else {
        client.setClient(espClient);
    }

    client.setServer(config.mqttBroker, config.mqttPort);
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
    
    String clientId = "ESPClient-";
    clientId += String(config.sensorId); // Use sensor ID as client ID if possible

    if (client.connect(clientId.c_str(), config.mqttUser, config.mqttPassword)) {
        Serial.println("connected");
    } else {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");
    }
}

void MqttManager::publish(const char* topic, const char* payload) {
    if (!config.mqttEnabled) return;
    
    if (!client.connected()) {
       // Optionally try to reconnect immediately or just drop?
       // relying on loop() to reconnect.
       return;
    }
    
    String fullTopic = String(config.mqttTopicPrefix);
    if (fullTopic.length() > 0 && !fullTopic.endsWith("/")) fullTopic += "/";
    fullTopic += topic;
    
    client.publish(fullTopic.c_str(), payload);
}

bool MqttManager::isConnected() {
    return client.connected();
}
