#include "mqtt_manager.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <time.h>
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
    const String deviceId = getDeviceIdentifier();
    String statusTopic = "devices/" + deviceId + "/status";
    
    // Connect with LWT (Last Will and Testament)
    if (client.connect(clientId.c_str(), config.mqttUser, config.mqttPassword, 
                       statusTopic.c_str(), 1, true, "offline")) {
        Serial.println("connected");
        
        // Publish online status
        publishStatus("online");
        
        // Subscribe to device commands/config (API protocol)
        String commandTopic = "devices/" + deviceId + "/commands";
        String configTopic = "devices/" + deviceId + "/config";
        client.subscribe(commandTopic.c_str());
        client.subscribe(configTopic.c_str());

        // Backward compatibility with legacy topic
        String legacyCommandTopic = "sensors/" + getLegacySensorId() + "/commands";
        client.subscribe(legacyCommandTopic.c_str());
        
        Serial.printf("Subscribed to %s, %s and %s\n", commandTopic.c_str(), configTopic.c_str(), legacyCommandTopic.c_str());
    } else {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");
    }
}

void MqttManager::publishTelemetry(const char* payload) {
    String topic = "sensors/" + getLegacySensorId() + "/telemetry";
    publishRaw(topic.c_str(), payload);
}

void MqttManager::publishStatus(const char* status) {
    JsonDocument statusDoc;
    statusDoc["status"] = status;
    statusDoc["timestamp"] = time(nullptr);
    statusDoc["device_id"] = getDeviceIdentifier();
    statusDoc["firmware_version"] = config.currentFirmwareVersion;

    String payload;
    serializeJson(statusDoc, payload);

    String topic = "devices/" + getDeviceIdentifier() + "/status";
    publishRaw(topic.c_str(), payload.c_str(), true);

    // Legacy status topic
    String legacyTopic = "sensors/" + getLegacySensorId() + "/status";
    publishRaw(legacyTopic.c_str(), status, true);
}

void MqttManager::publishAck(const String& commandId, const String& status, const String& details) {
    JsonDocument ackDoc;
    ackDoc["command_id"] = commandId;
    ackDoc["status"] = status;
    ackDoc["details"] = details;
    ackDoc["timestamp"] = time(nullptr);

    String payload;
    serializeJson(ackDoc, payload);

    String topic = "devices/" + getDeviceIdentifier() + "/ack";
    publishRaw(topic.c_str(), payload.c_str());
}

void MqttManager::publishOtaStatus(const String& status, int progress, const String& version) {
    JsonDocument otaDoc;
    otaDoc["status"] = status;
    if (progress >= 0) {
        otaDoc["progress"] = progress;
    }
    if (version.length() > 0) {
        otaDoc["version"] = version;
    }
    otaDoc["timestamp"] = time(nullptr);

    String payload;
    serializeJson(otaDoc, payload);
    String topic = "devices/" + getDeviceIdentifier() + "/ota/status";
    publishRaw(topic.c_str(), payload.c_str());
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

String MqttManager::getLegacySensorId() const {
    return String(config.sensorId);
}

String MqttManager::getDeviceIdentifier() const {
    if (strlen(config.mqttClientId) > 0) {
        return String(config.mqttClientId);
    }

    if (strlen(config.sensorId) > 0) {
        return String(config.sensorId);
    }

    return config.getAdoptionCode();
}
