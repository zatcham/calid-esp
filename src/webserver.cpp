#include "webserver.h"
#include "config.h"
#include "sensor.h" 
#include "logging.h"
#include <LittleFS.h>
#include <Update.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif

const byte DNS_PORT = 53;

WebServer::WebServer() : server(80) {}

void WebServer::begin() {
    if (WiFi.getMode() == WIFI_AP) {
        dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
    }

    // API Endpoints
    server.on("/api/config", HTTP_POST, [this](AsyncWebServerRequest *request) { this->handleApiConfigSave(request); });
    server.on("/api/config", HTTP_GET, [this](AsyncWebServerRequest *request) { this->handleApiConfigGet(request); });
    server.on("/api/data", HTTP_GET, [this](AsyncWebServerRequest *request) { this->handleApiData(request); });
    server.on("/api/logs", HTTP_GET, [this](AsyncWebServerRequest *request) { this->handleApiLogs(request); });
    server.on("/api/system", HTTP_GET, [this](AsyncWebServerRequest *request) { this->handleApiSystem(request); });

    // Serve Static Files (Frontend)
    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
    
    // OTA Update
    server.on("/api/update", HTTP_POST, [&](AsyncWebServerRequest *request){
        bool shouldReboot = !Update.hasError();
        AsyncWebServerResponse *response = request->beginResponse(200, "application/json", shouldReboot ? "{\"success\":true}" : "{\"success\":false}");
        response->addHeader("Connection", "close");
        request->send(response);
        if (shouldReboot) {
            delay(100);
            ESP.restart();
        }
    }, [this](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
        this->handleUpdateUpload(request, filename, index, data, len, final);
    });

    // Catch-All for SPA (History API) - Redirect to index.html for non-API requests
    server.onNotFound([](AsyncWebServerRequest *request) {
        if (request->method() == HTTP_OPTIONS) {
            request->send(200);
        } else {
            request->send(LittleFS, "/index.html");
        }
    });

    server.begin();
}

void WebServer::handleClient() {
    if (WiFi.getMode() == WIFI_AP) {
        dnsServer.processNextRequest();
    }
}

void WebServer::handleApiConfigSave(AsyncWebServerRequest *request) {
    if (request->hasParam("ssid", true)) {
        strncpy(config.ssid, request->getParam("ssid", true)->value().c_str(), sizeof(config.ssid) - 1);
        strncpy(config.password, request->getParam("password", true)->value().c_str(), sizeof(config.password) - 1);
        strncpy(config.apiEndpoint, request->getParam("apiEndpoint", true)->value().c_str(), sizeof(config.apiEndpoint) - 1);
        strncpy(config.sensorId, request->getParam("sensorId", true)->value().c_str(), sizeof(config.sensorId) - 1);
        strncpy(config.apiKey, request->getParam("apiKey", true)->value().c_str(), sizeof(config.apiKey) - 1);
        
        config.testingMode = (request->hasParam("testingMode", true) && request->getParam("testingMode", true)->value() == "on");
        
        if(request->hasParam("sensorType", true)) config.sensorType = request->getParam("sensorType", true)->value().toInt();
        if(request->hasParam("sensorPin", true)) config.sensorPin = request->getParam("sensorPin", true)->value().toInt();

        // Multi-sensor config
        for (int i = 0; i < MAX_SENSORS; i++) {
            String typeKey = "sensorType" + String(i);
            String pinKey = "sensorPin" + String(i);
            if (request->hasParam(typeKey, true)) {
                config.sensors[i].type = request->getParam(typeKey, true)->value().toInt();
            }
            if (request->hasParam(pinKey, true)) {
                config.sensors[i].pin = request->getParam(pinKey, true)->value().toInt();
            }
        }

        if(request->hasParam("mqttBroker", true)) strncpy(config.mqttBroker, request->getParam("mqttBroker", true)->value().c_str(), sizeof(config.mqttBroker) - 1);
        if(request->hasParam("mqttPort", true)) config.mqttPort = request->getParam("mqttPort", true)->value().toInt();
        if(request->hasParam("mqttUser", true)) strncpy(config.mqttUser, request->getParam("mqttUser", true)->value().c_str(), sizeof(config.mqttUser) - 1);
        if(request->hasParam("mqttPassword", true)) strncpy(config.mqttPassword, request->getParam("mqttPassword", true)->value().c_str(), sizeof(config.mqttPassword) - 1);
        if(request->hasParam("mqttTopicPrefix", true)) strncpy(config.mqttTopicPrefix, request->getParam("mqttTopicPrefix", true)->value().c_str(), sizeof(config.mqttTopicPrefix) - 1);
        
        config.mqttEnabled = (request->hasParam("mqttEnabled", true) && request->getParam("mqttEnabled", true)->value() == "on");

        config.save();

        request->send(200, "application/json", "{\"success\":true, \"message\":\"Configuration saved. Restarting...\"}");

        delay(1000);
        ESP.restart();
    } else {
        request->send(400, "application/json", "{\"success\":false, \"message\":\"Bad Request\"}");
    }
}

void WebServer::handleApiConfigGet(AsyncWebServerRequest *request) {
    String json = "{";
    json += "\"ssid\":\"" + String(config.ssid) + "\",";
    json += "\"password\":\"" + String(config.password) + "\",";
    json += "\"apiEndpoint\":\"" + String(config.apiEndpoint) + "\",";
    json += "\"sensorId\":\"" + String(config.sensorId) + "\",";
    json += "\"apiKey\":\"" + String(config.apiKey) + "\",";
    json += "\"testingMode\":" + String(config.testingMode ? "true" : "false") + ",";
    json += "\"sensorType\":" + String(config.sensorType) + ",";
    json += "\"sensorPin\":" + String(config.sensorPin) + ",";
    
    json += "\"sensors\":[";
    for (int i = 0; i < MAX_SENSORS; i++) {
        json += "{\"type\":" + String(config.sensors[i].type) + ",\"pin\":" + String(config.sensors[i].pin) + "}";
        if (i < MAX_SENSORS - 1) json += ",";
    }
    json += "],";

    json += "\"mqttBroker\":\"" + String(config.mqttBroker) + "\",";
    json += "\"mqttPort\":" + String(config.mqttPort) + ",";
    json += "\"mqttUser\":\"" + String(config.mqttUser) + "\",";
    json += "\"mqttPassword\":\"" + String(config.mqttPassword) + "\",";
    json += "\"mqttTopicPrefix\":\"" + String(config.mqttTopicPrefix) + "\",";
    json += "\"mqttEnabled\":" + String(config.mqttEnabled ? "true" : "false");
    json += "}";
    request->send(200, "application/json", json);
}

void WebServer::handleApiData(AsyncWebServerRequest *request) {
    String json = "{\"sensors\":[";
    for (int i = 0; i < activeSensorCount; i++) {
        json += "{";
        json += "\"pin\":" + String(allSensorData[i].pin) + ",";
        json += "\"sensorType\":\"" + allSensorData[i].sensorType + "\",";
        json += "\"valid\":" + String(allSensorData[i].valid ? "true" : "false") + ",";
        json += "\"error\":\"" + allSensorData[i].error + "\",";
        json += "\"readings\":[";
        for (size_t j = 0; j < allSensorData[i].readings.size(); j++) {
            json += "{";
            json += "\"type\":\"" + allSensorData[i].readings[j].type + "\",";
            json += "\"value\":" + String(allSensorData[i].readings[j].value) + ",";
            json += "\"unit\":\"" + allSensorData[i].readings[j].unit + "\"";
            json += "}";
            if (j < allSensorData[i].readings.size() - 1) json += ",";
        }
        json += "]";
        json += "}";
        if (i < activeSensorCount - 1) json += ",";
    }
    json += "]}";
    request->send(200, "application/json", json);
}

void WebServer::handleApiLogs(AsyncWebServerRequest *request) {
    if (LittleFS.exists("/log.txt")) {
        request->send(LittleFS, "/log.txt", "text/plain");
    } else {
        request->send(200, "text/plain", "No logs found.");
    }
}

void WebServer::handleApiSystem(AsyncWebServerRequest *request) {
    String json = "{";
    json += "\"adoptionCode\":\"" + config.getAdoptionCode() + "\",";
    json += "\"macAddress\":\"" + WiFi.macAddress() + "\",";
    json += "\"freeHeap\":" + String(ESP.getFreeHeap()) + ",";
    json += "\"sdkVersion\":\"" + String(ESP.getSdkVersion()) + "\",";
    #ifdef ESP32
    json += "\"chipModel\":\"" + String(ESP.getChipModel()) + "\",";
    json += "\"chipRevision\":" + String(ESP.getChipRevision());
    #else
    json += "\"chipId\":\"" + String(ESP.getChipId()) + "\"";
    #endif
    json += "}";
    request->send(200, "application/json", json);
}

void WebServer::handleUpdateUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    if (!index) {
        Serial.printf("Update Start: %s\n", filename.c_str());
        int cmd = (filename.indexOf("fs") > -1 || filename.indexOf("data") > -1) ? U_FS : U_FLASH;
        #ifdef ESP8266
        Update.runAsync(true);
        if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000, cmd)) {
        #else
        if (!Update.begin(UPDATE_SIZE_UNKNOWN, cmd)) {
        #endif
            Update.printError(Serial);
        }
    }
    
    if (!Update.hasError()) {
        if (Update.write(data, len) != len) {
            Update.printError(Serial);
        }
    }
    
    if (final) {
        if (Update.end(true)) {
            Serial.printf("Update Success: %uB\n", index + len);
        } else {
            Update.printError(Serial);
        }
    }
}