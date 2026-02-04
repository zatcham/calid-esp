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

#if defined(ESP8266)
#include <LittleFS.h>
#else
#include <LittleFS.h> // Uses lorol/LittleFS_esp32
#endif

#include "config.h"
#include "webserver.h"
#include "sensor.h"
#include "logging.h"
#include "mqtt_manager.h"

#include <NTPClient.h>
#include <WiFiUdp.h>

// Define D2 for ESP32 if not defined (ESP8266 has it)
#ifndef D2
#define D2 4
#endif

const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 4, 1);
DNSServer dnsServer;
Sensor sensor;
WebServer webServer;
Logger logger("/log.txt");

WiFiUDP udp;
NTPClient timeClient(udp, "pool.ntp.org", 0, 60000); // NTP server, UTC offset, update interval
bool timeSynced = false;

// Function Prototypes
void startAP();
void connectWiFi();
String getTime();

void startAP() {
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP("ESP_Config");

    dnsServer.start(DNS_PORT, "*", apIP);

    Serial.println("Started Access Point 'ESP_Config'");
    Serial.print("IP address: ");
    Serial.println(WiFi.softAPIP());
}

void connectWiFi() {
    WiFi.mode(WIFI_STA);
    Serial.println("Configured SSID: ");
    Serial.print(config.ssid);
    WiFi.begin(config.ssid, config.password);
    Serial.print("Connecting to WiFi...");
    int retries = 30;
    while (WiFi.status() != WL_CONNECTED && retries > 0) {
        delay(500);
        Serial.print(".");
        retries--;
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Failed to connect.");
        startAP();
    } else {
        Serial.println(" connected.");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        // Initialize NTP client and fetch the time
        timeClient.begin();
        timeClient.update();
        timeSynced = timeClient.getEpochTime() > 0;
    }
}

void setup() {
    Serial.begin(115200);
    delay(10);
    Serial.println("Device starting..");
    
    // Initialize File System
    if(!LittleFS.begin()){
        Serial.println("LittleFS Mount Failed");
        return;
    }

    config.load();
    Serial.println(getTime());

    if (config.testingMode) {
        Serial.println("Testing mode enabled. Fake data will be generated.");
    } else {
        sensor.begin();
    }

    if (strlen(config.ssid) == 0 || strlen(config.password) == 0) {
        startAP();
    } else {
        connectWiFi();
        mqttManager.begin();
    }

    webServer.begin();
}

void loop() {
    webServer.handleClient(); // Handle AP DNS requests
    mqttManager.loop();

    if (WiFi.status() == WL_CONNECTED) {
        if (!timeSynced) {
            delay(15000); // Wait for NTP sync
        }

        if (config.testingMode) {
            // Generate fake data for multiple "sensors"
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

        // MQTT Publish all sensors
        for (int i = 0; i < activeSensorCount; i++) {
            if (allSensorData[i].valid) {
                String json = "{\"pin\":" + String(allSensorData[i].pin) + 
                              ",\"sensorType\":\"" + allSensorData[i].sensorType + "\",\"readings\":[";
                for (size_t j = 0; j < allSensorData[i].readings.size(); j++) {
                    json += "{\"type\":\"" + allSensorData[i].readings[j].type + 
                            "\",\"value\":" + String(allSensorData[i].readings[j].value) + 
                            ",\"unit\":\"" + allSensorData[i].readings[j].unit + "\"}";
                    if (j < allSensorData[i].readings.size() - 1) json += ",";
                }
                json += "]}";
                String topic = "data/pin" + String(allSensorData[i].pin);
                mqttManager.publish(topic.c_str(), json.c_str());
            }
        }

        // HTTP POST all sensors
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

        delay(60000);
    } else {
        delay(30000);
    }

        delay(60000);
    } else {
        delay(30000);
    }

            if (httpResponseCode > 0) {
                String response = http.getString();
                Serial.println(httpResponseCode);
                Serial.println(response);
                logger.log("Data sent successfully: " + payload);
            } else {
                Serial.print("Error on sending POST: ");
                Serial.println(httpResponseCode);
                logger.log("Error sending data: " + String(httpResponseCode));
            }

            http.end();
        }

        delay(60000); // Read and upload data every 60 seconds
    } else {
      delay(30000); // Wait 30s to see if connected and has time
    }
}

String getTime() {
  timeClient.update();
  time_t now = timeClient.getEpochTime();
  struct tm* timeinfo = localtime(&now);
  char buffer[20];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
  return String(buffer);
}