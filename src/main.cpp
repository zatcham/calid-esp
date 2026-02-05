#include <Arduino.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESPAsyncTCP.h>
#else
#include <WiFi.h>
#include <HTTPClient.h>
#include <AsyncTCP.h>
#endif

#include <WiFiClientSecure.h> 
#include <DNSServer.h>

#include <LittleFS.h>

#include "config.h"
#include "calid_webserver.h"
#include "sensor.h"
#include "logging.h"
#include "mqtt_manager.h"
#include "wifi_setup.h"
#include "ota_manager.h"

#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <ArduinoJson.h>

#ifndef D2
#define D2 4
#endif

const String SW_VERSION = "1.2.0";
const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 4, 1);
DNSServer dnsServer;
Sensor sensor;
CalidWebServer webServer;
Logger logger("/log.txt");

WiFiUDP udp;
NTPClient timeClient(udp, "pool.ntp.org", 0, 60000);
bool timeSynced = false;

void connectWiFi();
String getTime();

void connectWiFi() {
    runWifiSetup();

    Serial.println("WiFi Connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    
    timeClient.begin();
    timeClient.setPoolServerName(config.ntpServer);
    timeClient.update();
    timeSynced = timeClient.getEpochTime() > 0;
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n\n===============================");
    Serial.println("Calid ESP Sensor Gateway");
    Serial.printf("Version: %s\n", SW_VERSION.c_str());
    Serial.println("===============================\n");
    
    #ifdef ESP32
    if(!LittleFS.begin(true)){
    #else
    if(!LittleFS.begin()){
    #endif
        Serial.println("LittleFS Mount Failed");
    }

    config.load();
    logger.begin();
    logger.log("System starting v" + SW_VERSION + " [AdoptionCode: " + config.getAdoptionCode() + "]");
    
    Wire.begin(); 
    
    timeClient.setTimeOffset(config.utcOffset);

    if (config.testingMode) {
        Serial.println("Testing mode enabled.");
    } else {
        sensor.begin();
    }

    connectWiFi();
    mqttManager.begin();
    webServer.begin();

    mqttManager.setCommandCallback([](String topic, String payload) {
        String ackTopic = "sensors/" + String(config.sensorId) + "/ack";
        
        if (payload == "restart") {
            Serial.println("Remote restart command received");
            mqttManager.publishRaw(ackTopic.c_str(), "restarting");
            delay(500);
            ESP.restart();
        } else if (payload == "toggle_sim") {
            config.testingMode = !config.testingMode;
            config.save();
            mqttManager.publishRaw(ackTopic.c_str(), config.testingMode ? "sim_on" : "sim_off");
        } else if (payload == "update") {
            if (strlen(config.firmwareUrl) > 0) {
                mqttManager.publishRaw(ackTopic.c_str(), "updating");
                OtaManager::triggerUpdate(config.firmwareUrl);
            } else {
                mqttManager.publishRaw(ackTopic.c_str(), "update_failed_no_url");
            }
        } else if (payload.startsWith("{")) {
            JsonDocument updateDoc;
            DeserializationError err = deserializeJson(updateDoc, payload);
            if (!err) {
                if (!updateDoc["sensorId"].isNull()) strlcpy(config.sensorId, updateDoc["sensorId"], sizeof(config.sensorId));
                if (!updateDoc["utcOffset"].isNull()) config.utcOffset = updateDoc["utcOffset"];
                if (!updateDoc["ntpServer"].isNull()) strlcpy(config.ntpServer, updateDoc["ntpServer"], sizeof(config.ntpServer));
                if (!updateDoc["mqttTopicPrefix"].isNull()) strlcpy(config.mqttTopicPrefix, updateDoc["mqttTopicPrefix"], sizeof(config.mqttTopicPrefix));
                
                config.save();
                mqttManager.publishRaw(ackTopic.c_str(), "config_updated");
                Serial.println("Remote configuration updated via MQTT");
            }
        }
    });
}

unsigned long lastDataLog = 0;
unsigned long lastHeartbeat = 0;

void loop() {
    webServer.handleClient(); 
    mqttManager.loop();
    OtaManager::loop();

    unsigned long now = millis();

    // Periodic Heartbeat / Status
    if (now - lastHeartbeat > 300000 || lastHeartbeat == 0) { // 5 mins
        lastHeartbeat = now;
        if (mqttManager.isConnected()) {
            mqttManager.publishStatus("online");
        }
    }

    if (WiFi.status() == WL_CONNECTED) {
        if (!timeSynced) {
            timeClient.update();
            timeSynced = timeClient.getEpochTime() > 28800; // After 1970
        }

        // Only run data collection every 60s
        if (now - lastDataLog < 60000 && lastDataLog != 0) return;
        lastDataLog = now;

        if (config.testingMode) {
            activeSensorCount = 2;
            allSensorData[0].pin = 4;
            allSensorData[0].sensorType = "DHT22";
            allSensorData[0].valid = true;
            allSensorData[0].readings.clear();
            allSensorData[0].readings.push_back({"Temperature", 22.5f + (random(-20, 20) / 10.0f), "C"});
            allSensorData[0].readings.push_back({"Humidity", 45.0f + (random(-50, 50) / 10.0f), "%"});

            allSensorData[1].pin = 5;
            allSensorData[1].sensorType = "BME280";
            allSensorData[1].valid = true;
            allSensorData[1].readings.clear();
            allSensorData[1].readings.push_back({"Temperature", 24.1f + (random(-20, 20) / 10.0f), "C"});
            allSensorData[1].readings.push_back({"Humidity", 40.0f + (random(-50, 50) / 10.0f), "%"});
            allSensorData[1].readings.push_back({"Pressure", 1012.5f + (random(-100, 100) / 10.0f), "hPa"});
        } else {
            sensor.update();
        }

        if (mqttManager.isConnected()) {
            JsonDocument mqttDoc;
            mqttDoc["sensorId"] = config.sensorId;
            mqttDoc["adoptionCode"] = config.getAdoptionCode();
            
            SystemHealth health = config.getSystemHealth();
            JsonObject sys = mqttDoc["system"].to<JsonObject>();
            sys["rssi"] = health.rssi;
            sys["uptime"] = health.uptime;
            sys["freeHeap"] = health.freeHeap;
            sys["resetReason"] = health.resetReason;

            JsonArray sensorsArr = mqttDoc["sensors"].to<JsonArray>();

            for (int i = 0; i < activeSensorCount; i++) {
                if (allSensorData[i].valid) {
                    JsonObject s = sensorsArr.add<JsonObject>();
                    s["pin"] = allSensorData[i].pin;
                    s["type"] = allSensorData[i].sensorType;
                    JsonArray rd = s["readings"].to<JsonArray>();
                    for (const auto& r : allSensorData[i].readings) {
                        JsonObject ro = rd.add<JsonObject>();
                        ro["type"] = r.type;
                        ro["value"] = r.value;
                        ro["unit"] = r.unit;
                    }
                }
            }
            String mqttPayload;
            serializeJson(mqttDoc, mqttPayload);
            mqttManager.publishTelemetry(mqttPayload.c_str());
        }

        String apiEndpoint = String(config.apiEndpoint);
        if (apiEndpoint.length() > 0) {
            bool isHttps = apiEndpoint.startsWith("https://");
            HTTPClient http;
            WiFiClient client;
            WiFiClientSecure secureClient;

            if (isHttps) {
                secureClient.setInsecure();
                http.begin(secureClient, apiEndpoint + "/sensor/data/write");
            } else {
                http.begin(client, apiEndpoint + "/sensor/data/write");
            }

            http.addHeader("Content-Type", "application/json");
            http.addHeader("X-Sensor-Id", config.sensorId);
            http.addHeader("X-Sensor-Api-Key", config.apiKey);

            String timeStr = getTime();
            String payload = "[";
            bool first = true;
            for (int i = 0; i < activeSensorCount; i++) {
                if (!allSensorData[i].valid) continue;

                for (const auto& r : allSensorData[i].readings) {
                    if (!first) payload += ",";
                    payload += "{\"time\":\"" + timeStr + 
                               "\",\"sensor_id\":\"" + String(config.sensorId) + 
                               "\",\"pin\":" + String(allSensorData[i].pin) + 
                               ",\"sensor_type\":\"" + allSensorData[i].sensorType +
                               "\",\"data_type\":\"" + r.type + 
                               "\",\"value\":\"" + String(r.value) + 
                               "\",\"unit\":\"" + r.unit + "\"}";
                    first = false;
                }
            }
            payload += "]";

            if (!first) {
                int httpResponseCode = http.POST(payload);
                if (httpResponseCode > 0) {
                    Serial.println("HTTP Success: " + String(httpResponseCode));
                } else {
                    Serial.println("HTTP Error: " + String(httpResponseCode));
                }
            }
            http.end();
        }
    }
}

String getTime() {
  if (WiFi.status() == WL_CONNECTED) {
    timeClient.update();
  }
  time_t now = timeClient.getEpochTime();
  struct tm* timeinfo = localtime(&now);
  char buffer[20];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
  return String(buffer);
}