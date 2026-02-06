#include "calid_webserver.h"
#include "config.h"
#include "sensor.h" 
#include "logging.h"
#include <LittleFS.h>
#if defined(ESP8266)
#include <Updater.h>
#else
#include <Update.h>
#endif
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

CalidWebServer::CalidWebServer() : server(80) {}

void CalidWebServer::begin() {
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

    // Captive Portal Redirect
    server.onNotFound([this](AsyncWebServerRequest *request) {
        if (WiFi.getMode() == WIFI_AP) {
            String url = request->url();
            String host = request->host();
            
            // Common OS check paths for Captive Portal detection
            if (url.indexOf("generate_204") > -1 || 
                url.indexOf("hotspot-detect.html") > -1 || 
                url.indexOf("connectivity-check") > -1 ||
                url.indexOf("ncsi.txt") > -1 ||
                (host != "192.168.4.1" && !host.endsWith(".local"))) {
                
                request->redirect("http://192.168.4.1/config");
                return;
            }
        }
        
        if (request->method() == HTTP_OPTIONS) {
            request->send(200);
        } else {
            request->send(LittleFS, "/index.html");
        }
    });

    // API Endpoints
    server.on("/api/config", HTTP_POST, [this](AsyncWebServerRequest *request) { this->handleApiConfigSave(request); });
    server.on("/api/config", HTTP_GET, [this](AsyncWebServerRequest *request) { this->handleApiConfigGet(request); });
    server.on("/api/data", HTTP_GET, [this](AsyncWebServerRequest *request) { this->handleApiData(request); });
    server.on("/api/logs", HTTP_GET, [this](AsyncWebServerRequest *request) { this->handleApiLogs(request); });
    server.on("/api/system", HTTP_GET, [this](AsyncWebServerRequest *request) { this->handleApiSystem(request); });
    server.on("/api/system/scan-i2c", HTTP_GET, [this](AsyncWebServerRequest *request) { this->handleApiScanI2C(request); });
    server.on("/api/wifi/scan", HTTP_GET, [this](AsyncWebServerRequest *request) { this->handleApiWifiScan(request); });

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

    server.begin();
}

void CalidWebServer::handleClient() {
    if (WiFi.getMode() == WIFI_AP) {
        dnsServer.processNextRequest();
    }
    #ifdef ESP8266
    MDNS.update();
    #endif
}

bool CalidWebServer::authenticate(AsyncWebServerRequest *request) {
    if (strlen(config.adminPassword) == 0) return true; // No password set
    if (!request->authenticate(config.adminUser, config.adminPassword)) {
        request->requestAuthentication();
        return false;
    }
    return true;
}

void CalidWebServer::handleApiConfigSave(AsyncWebServerRequest *request) {
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
    if (request->hasParam("ntpServer", true)) strlcpy(config.ntpServer, request->getParam("ntpServer", true)->value().c_str(), sizeof(config.ntpServer));
    if (request->hasParam("firmwareUrl", true)) strlcpy(config.firmwareUrl, request->getParam("firmwareUrl", true)->value().c_str(), sizeof(config.firmwareUrl));

    // Multi-sensor config
    for (int i = 0; i < MAX_SENSORS; i++) {
        String typeKey = "sensorType" + String(i);
        String pinKey = "sensorPin" + String(i);
        String i2cKey = "sensorI2C" + String(i);
        String muxKey = "sensorMux" + String(i);
        String tOffKey = "sensorTOff" + String(i);
        String hOffKey = "sensorHOff" + String(i);
        if (request->hasParam(typeKey, true)) strlcpy(config.sensors[i].type, request->getParam(typeKey, true)->value().c_str(), sizeof(config.sensors[i].type));
        if (request->hasParam(pinKey, true)) config.sensors[i].pin = request->getParam(pinKey, true)->value().toInt();
        if (request->hasParam(i2cKey, true)) config.sensors[i].i2cAddress = strtol(request->getParam(i2cKey, true)->value().c_str(), NULL, 0);
        if (request->hasParam(muxKey, true)) config.sensors[i].i2cMultiplexerChannel = request->getParam(muxKey, true)->value().toInt();
        if (request->hasParam(tOffKey, true)) config.sensors[i].tempOffset = request->getParam(tOffKey, true)->value().toFloat();
        if (request->hasParam(hOffKey, true)) config.sensors[i].humOffset = request->getParam(hOffKey, true)->value().toFloat();
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

void CalidWebServer::handleApiConfigGet(AsyncWebServerRequest *request) {
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
    doc["ntpServer"] = config.ntpServer;
    doc["firmwareUrl"] = config.firmwareUrl;

    JsonArray sensorsArr = doc["sensors"].to<JsonArray>();
    for (int i = 0; i < MAX_SENSORS; i++) {
        JsonObject s = sensorsArr.add<JsonObject>();
        s["type"] = config.sensors[i].type;
        s["pin"] = config.sensors[i].pin;
        s["i2cAddress"] = config.sensors[i].i2cAddress;
        s["i2cMultiplexerChannel"] = config.sensors[i].i2cMultiplexerChannel;
        s["tempOffset"] = config.sensors[i].tempOffset;
        s["humOffset"] = config.sensors[i].humOffset;
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

void CalidWebServer::handleApiData(AsyncWebServerRequest *request) {
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

void CalidWebServer::handleApiLogs(AsyncWebServerRequest *request) {
    if (!authenticate(request)) return;
    if (LittleFS.exists("/log.txt")) {
        request->send(LittleFS, "/log.txt", "text/plain");
    } else {
        request->send(200, "text/plain", "No logs found.");
    }
}

void CalidWebServer::handleApiSystem(AsyncWebServerRequest *request) {
    SystemHealth health = config.getSystemHealth();
    JsonDocument doc;
    doc["adoptionCode"] = config.getAdoptionCode();
    doc["macAddress"] = WiFi.macAddress();
    doc["freeHeap"] = health.freeHeap;
    doc["uptime"] = health.uptime;
    doc["rssi"] = health.rssi;
    doc["resetReason"] = health.resetReason;
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

void CalidWebServer::handleApiScanI2C(AsyncWebServerRequest *request) {
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

void CalidWebServer::handleApiWifiScan(AsyncWebServerRequest *request) {
    if (!authenticate(request)) return;
    
    int n = WiFi.scanNetworks();
    JsonDocument doc;
    JsonArray networks = doc.to<JsonArray>();
    
    for (int i = 0; i < n; ++i) {
        JsonObject net = networks.add<JsonObject>();
        net["ssid"] = WiFi.SSID(i);
        net["rssi"] = WiFi.RSSI(i);
        #ifdef ESP32
        net["secure"] = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
        #else
        net["secure"] = WiFi.encryptionType(i) != ENC_TYPE_NONE;
        #endif
    }
    
    String json;
    serializeJson(doc, json);
    request->send(200, "application/json", json);
    WiFi.scanDelete();
}

void CalidWebServer::handleUpdateUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    if (!index) {
        Serial.printf("Update Start: %s\n", filename.c_str());
        
        #ifdef ESP32
        int cmd = (filename.indexOf("fs") > -1 || filename.indexOf("data") > -1) ? U_SPIFFS : U_FLASH;
        #else
        int cmd = (filename.indexOf("fs") > -1 || filename.indexOf("data") > -1) ? U_FS : U_FLASH;
        #endif

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