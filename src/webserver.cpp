#include "webserver.h"
#include "config.h"
#include "sensor.h" 
#include "logging.h"
#include <LittleFS.h>
#include <Update.h>
#include <ArduinoJson.h>
#include <Wire.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#else
#include <WiFi.h>
#include <ESPmDNS.h>
#endif

const byte DNS_PORT = 53;

WebServer::WebServer() : server(80) {}

void WebServer::begin() {
    if (WiFi.getMode() == WIFI_AP) {
        dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
    }

    // Start mDNS
    String dnsName = "calid-" + config.getAdoptionCode();
    dnsName.toLowerCase();
    if (MDNS.begin(dnsName.c_str())) {
        Serial.printf("mDNS responder started: http://%s.local\n", dnsName.c_str());
        MDNS.addService("http", "tcp", 80);
    }

    // API Endpoints
    server.on("/api/config", HTTP_POST, [this](AsyncWebServerRequest *request) { this->handleApiConfigSave(request); });
    server.on("/api/config", HTTP_GET, [this](AsyncWebServerRequest *request) { this->handleApiConfigGet(request); });
    server.on("/api/data", HTTP_GET, [this](AsyncWebServerRequest *request) { this->handleApiData(request); });
    server.on("/api/logs", HTTP_GET, [this](AsyncWebServerRequest *request) { this->handleApiLogs(request); });
    server.on("/api/system", HTTP_GET, [this](AsyncWebServerRequest *request) { this->handleApiSystem(request); });
    server.on("/api/system/scan-i2c", HTTP_GET, [this](AsyncWebServerRequest *request) { this->handleApiScanI2C(request); });

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

    // Catch-All for SPA
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
    #ifdef ESP8266
    MDNS.update();
    #endif
}

bool WebServer::authenticate(AsyncWebServerRequest *request) {
    if (strlen(config.adminPassword) == 0) return true; // No password set
    if (!request->authenticate(config.adminUser, config.adminPassword)) {
        request->requestAuthentication();
        return false;
    }
    return true;
}

void WebServer::handleApiConfigSave(AsyncWebServerRequest *request) {
    if (!authenticate(request)) return;

    if (request->hasParam("ssid", true)) strlcpy(config.ssid, request->getParam("ssid", true)->value().c_str(), sizeof(config.ssid));
    if (request->hasParam("password", true)) strlcpy(config.password, request->getParam("password", true)->value().c_str(), sizeof(config.password));
    if (request->hasParam("apiEndpoint", true)) strlcpy(config.apiEndpoint, request->getParam("apiEndpoint", true)->value().c_str(), sizeof(config.apiEndpoint));
    if (request->hasParam("sensorId", true)) strlcpy(config.sensorId, request->getParam("sensorId", true)->value().c_str(), sizeof(config.sensorId));
    if (request->hasParam("apiKey", true)) strlcpy(config.apiKey, request->getParam("apiKey", true)->value().c_str(), sizeof(config.apiKey));
    
    config.testingMode = (request->hasParam("testingMode", true) && (request->getParam("testingMode", true)->value() == "on" || request->getParam("testingMode", true)->value() == "true"));
    
    if (request->hasParam("utcOffset", true)) config.utcOffset = request->getParam("utcOffset", true)->value().toInt();
    if (request->hasParam("adminUser", true)) strlcpy(config.adminUser, request->getParam("adminUser", true)->value().c_str(), sizeof(config.adminUser));
    if (request->hasParam("adminPassword", true) && request->getParam("adminPassword", true)->value().length() > 0) {
        strlcpy(config.adminPassword, request->getParam("adminPassword", true)->value().c_str(), sizeof(config.adminPassword));
    }

    // Multi-sensor config
    for (int i = 0; i < MAX_SENSORS; i++) {
        String typeKey = "sensorType" + String(i);
        String pinKey = "sensorPin" + String(i);
        String i2cKey = "sensorI2C" + String(i);
        if (request->hasParam(typeKey, true)) config.sensors[i].type = request->getParam(typeKey, true)->value().toInt();
        if (request->hasParam(pinKey, true)) config.sensors[i].pin = request->getParam(pinKey, true)->value().toInt();
        if (request->hasParam(i2cKey, true)) config.sensors[i].i2cAddress = strtol(request->getParam(i2cKey, true)->value().c_str(), NULL, 0);
    }

    if(request->hasParam("mqttBroker", true)) strlcpy(config.mqttBroker, request->getParam("mqttBroker", true)->value().c_str(), sizeof(config.mqttBroker));
    if(request->hasParam("mqttPort", true)) config.mqttPort = request->getParam("mqttPort", true)->value().toInt();
    if(request->hasParam("mqttUser", true)) strlcpy(config.mqttUser, request->getParam("mqttUser", true)->value().c_str(), sizeof(config.mqttUser));
    if(request->hasParam("mqttPassword", true)) strlcpy(config.mqttPassword, request->getParam("mqttPassword", true)->value().c_str(), sizeof(config.mqttPassword));
    if(request->hasParam("mqttTopicPrefix", true)) strlcpy(config.mqttTopicPrefix, request->getParam("mqttTopicPrefix", true)->value().c_str(), sizeof(config.mqttTopicPrefix));
    config.mqttEnabled = (request->hasParam("mqttEnabled", true) && (request->getParam("mqttEnabled", true)->value() == "on" || request->getParam("mqttEnabled", true)->value() == "true"));

    config.save();
    request->send(200, "application/json", "{\"success\":true, \"message\":\"Configuration saved. Restarting...\"}");
    delay(1000);
    ESP.restart();
}

void WebServer::handleApiConfigGet(AsyncWebServerRequest *request) {
    if (!authenticate(request)) return;

    JsonDocument doc;
    doc["ssid"] = config.ssid;
    doc["password"] = config.password;
    doc["apiEndpoint"] = config.apiEndpoint;
    doc["sensorId"] = config.sensorId;
    doc["apiKey"] = config.apiKey;
    doc["testingMode"] = config.testingMode;
    doc["utcOffset"] = config.utcOffset;
    doc["adminUser"] = config.adminUser;

    JsonArray sensorsArr = doc["sensors"].to<JsonArray>();
    for (int i = 0; i < MAX_SENSORS; i++) {
        JsonObject s = sensorsArr.add<JsonObject>();
        s["type"] = config.sensors[i].type;
        s["pin"] = config.sensors[i].pin;
        s["i2cAddress"] = config.sensors[i].i2cAddress;
    }

    doc["mqttBroker"] = config.mqttBroker;
    doc["mqttPort"] = config.mqttPort;
    doc["mqttUser"] = config.mqttUser;
    doc["mqttTopicPrefix"] = config.mqttTopicPrefix;
    doc["mqttEnabled"] = config.mqttEnabled;

    String json;
    serializeJson(doc, json);
    request->send(200, "application/json", json);
}

void WebServer::handleApiData(AsyncWebServerRequest *request) {
    JsonDocument doc;
    JsonArray sensorsArr = doc["sensors"].to<JsonArray>();
    for (int i = 0; i < activeSensorCount; i++) {
        JsonObject s = sensorsArr.add<JsonObject>();
        s["pin"] = allSensorData[i].pin;
        s["sensorType"] = allSensorData[i].sensorType;
        s["valid"] = allSensorData[i].valid;
        if (!allSensorData[i].valid) s["error"] = allSensorData[i].error;
        
        JsonArray readingsArr = s["readings"].to<JsonArray>();
        for (const auto& r : allSensorData[i].readings) {
            JsonObject ro = readingsArr.add<JsonObject>();
            ro["type"] = r.type;
            ro["value"] = r.value;
            ro["unit"] = r.unit;
        }
    }
    String json;
    serializeJson(doc, json);
    request->send(200, "application/json", json);
}

void WebServer::handleApiLogs(AsyncWebServerRequest *request) {
    if (!authenticate(request)) return;
    if (LittleFS.exists("/log.txt")) {
        request->send(LittleFS, "/log.txt", "text/plain");
    } else {
        request->send(200, "text/plain", "No logs found.");
    }
}

void WebServer::handleApiSystem(AsyncWebServerRequest *request) {
    JsonDocument doc;
    doc["adoptionCode"] = config.getAdoptionCode();
    doc["macAddress"] = WiFi.macAddress();
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["sdkVersion"] = ESP.getSdkVersion();
    #ifdef ESP32
    doc["chipModel"] = ESP.getChipModel();
    doc["chipRevision"] = ESP.getChipRevision();
    #else
    doc["chipId"] = ESP.getChipId();
    #endif
    
    String json;
    serializeJson(doc, json);
    request->send(200, "application/json", json);
}

void WebServer::handleApiScanI2C(AsyncWebServerRequest *request) {
    if (!authenticate(request)) return;
    JsonDocument doc;
    JsonArray addresses = doc.to<JsonArray>();
    
    byte error, address;
    for(address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();
        if (error == 0) {
            addresses.add(address);
        }
    }
    
    String json;
    serializeJson(doc, json);
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
        if (Update.write(data, len) != len) Update.printError(Serial);
    }
    if (final) {
        if (Update.end(true)) Serial.printf("Update Success: %uB\n", index + len);
        else Update.printError(Serial);
    }
}
