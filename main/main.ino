#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h> // Wifi modules for sending data
#include <DNSServer.h>
#include <LittleFS.h> // Fs for logging

#include "config.h" // imports
#include "webserver.h"
#include "sensor.h"
#include "logging.h"

#include <NTPClient.h> // Time
#include <WiFiUdp.h>


// Define D2 as 4 if not already defined
#ifndef D2
#define D2 4
#endif

const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 4, 1);
DNSServer dnsServer;
Sensor sensor(D2, DHT11);
WebServer webServer;
Logger logger("/log.txt");

WiFiUDP udp;
NTPClient timeClient(udp, "pool.ntp.org", 0, 60000); // NTP server, UTC offset, update interval
bool timeSynced = false;

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
    }

    webServer.begin();
}

void loop() {
    webServer.handleClient(); // Handle incoming client requests
    dnsServer.processNextRequest(); // Process DNS requests

    if (WiFi.status() == WL_CONNECTED) {
      if (!timeSynced) {
        delay(15000);
      }
        float humidity;
        float temperature;

        if (config.testingMode) {
            // Generate fake data
            humidity = random(0, 100);
            temperature = random(15, 30);
        } else {
            // Read data from the sensor
            humidity = sensor.readHumidity();
            temperature = sensor.readTemperature();
        }

        // Create a client based on protocol
        String apiEndpoint = String(config.apiEndpoint); // Convert c-type str to String obj  
        bool isHttps = apiEndpoint.startsWith("https://");
        HTTPClient http;
        WiFiClient client;
        WiFiClientSecure secureClient;

        if (isHttps) {
          // HTTPS connection
          secureClient.setInsecure(); // For simplicity, disable certificate validation (not recommended for production)
          http.begin(secureClient, apiEndpoint + "/sensor/data/write");
        } else {
          // HTTP connection
          http.begin(client, apiEndpoint + "/sensor/data/write");
        }

        if (!isnan(humidity) && !isnan(temperature)) {
            http.addHeader("Content-Type", "application/json");
            http.addHeader("X-Sensor-Id", config.sensorId);
            http.addHeader("X-Sensor-Api-Key", config.apiKey);

            String payload = "[{\"time\":\"" + getTime() + "\",\"sensor_id\":\"" + String(config.sensorId) + "\",\"data_type\":\"Temperature\",\"value\":\"" + String(temperature) + "\",\"unit\":\"C\"},";
            payload += "{\"time\":\"" + getTime() + "\",\"sensor_id\":\"" + String(config.sensorId) + "\",\"data_type\":\"Humidity\",\"value\":\"" + String(humidity) + "\",\"unit\":\"%\"}]";

            int httpResponseCode = http.POST(payload);

            if (httpResponseCode > 0) {
                String response = http.getString();
                Serial.println(httpResponseCode);
                Serial.println(response);
                logger.log("Data sent successfully: " + payload); // Log successful data send
            } else {
                Serial.print("Error on sending POST: ");
                Serial.println(httpResponseCode);
                logger.log("Error sending data: " + String(httpResponseCode)); // Log error
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
