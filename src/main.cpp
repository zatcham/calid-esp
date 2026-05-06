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
#include <algorithm>

#if defined(ESP32)
#include <mbedtls/sha256.h>
#elif defined(ESP8266)
#include <Hash.h>
#endif

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
String getIsoTime();
String toCanonicalDataType(const String& rawType);
String normalizeSensorType(const String& rawType);
String generateMessageId();
String sha256Hex(const String& data);
int parsePinToInt(const String& rawPin);
void applyProvisioningConfig(const JsonVariantConst& payload);
void handleMqttCommand(const String& topic, const String& payload);
void publishTelemetryMqtt();
void runIdentify(int durationSeconds);

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

    if (strlen(config.currentFirmwareVersion) == 0) {
        strlcpy(config.currentFirmwareVersion, SW_VERSION.c_str(), sizeof(config.currentFirmwareVersion));
        config.save();
    }
    
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
    mqttManager.setCommandCallback(handleMqttCommand);
}

unsigned long lastDataLog = 0;
unsigned long lastHeartbeat = 0;
bool forceReadNow = false;

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

        unsigned long intervalMs = (unsigned long) max(5, config.reportingInterval) * 1000UL;
        if (!forceReadNow && now - lastDataLog < intervalMs && lastDataLog != 0) return;
        lastDataLog = now;
        forceReadNow = false;

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
            publishTelemetryMqtt();
        }

        String apiEndpoint = String(config.apiEndpoint);
        if (apiEndpoint.length() > 0) {
            bool isHttps = apiEndpoint.startsWith("https://");
            HTTPClient http;
            WiFiClient client;
            WiFiClientSecure secureClient;

            if (isHttps) {
                secureClient.setInsecure();
                http.begin(secureClient, apiEndpoint + "/api/v1/sensor/m2m/data");
            } else {
                http.begin(client, apiEndpoint + "/api/v1/sensor/m2m/data");
            }

            http.addHeader("Content-Type", "application/json");
            http.addHeader("X-Sensor-Id", config.sensorId);
            http.addHeader("X-Sensor-Api-Key", config.apiKey);

            String timeStr = getIsoTime();
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
                               "\",\"data_type\":\"" + toCanonicalDataType(r.type) + 
                               "\",\"value\":" + String(r.value, 4) + 
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

String getIsoTime() {
  if (WiFi.status() == WL_CONNECTED) {
    timeClient.update();
  }
  time_t now = timeClient.getEpochTime();
  struct tm* timeinfo = gmtime(&now);
  char buffer[30];
  strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", timeinfo);
  return String(buffer);
}

String toCanonicalDataType(const String& rawType) {
    String out = rawType;
    out.trim();
    out.toLowerCase();
    out.replace(" ", "_");

    if (out == "temp") return "temperature";
    if (out == "humid") return "humidity";
    return out;
}

String normalizeSensorType(const String& rawType) {
    String out = rawType;
    out.trim();
    out.toLowerCase();
    return out;
}

String generateMessageId() {
    char buf[37];
    const char* hex = "0123456789abcdef";
    uint8_t bytes[16];
    for (int i = 0; i < 16; i++) {
        bytes[i] = (uint8_t) random(0, 256);
    }

    int p = 0;
    for (int i = 0; i < 16; i++) {
        if (i == 4 || i == 6 || i == 8 || i == 10) buf[p++] = '-';
        buf[p++] = hex[(bytes[i] >> 4) & 0x0F];
        buf[p++] = hex[bytes[i] & 0x0F];
    }
    buf[p] = '\0';
    return String(buf);
}

String sha256Hex(const String& data) {
#if defined(ESP32)
    uint8_t hash[32];
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts_ret(&ctx, 0);
    mbedtls_sha256_update_ret(&ctx, (const unsigned char*) data.c_str(), data.length());
    mbedtls_sha256_finish_ret(&ctx, hash);
    mbedtls_sha256_free(&ctx);

    char out[65];
    for (int i = 0; i < 32; i++) {
        sprintf(out + (i * 2), "%02x", hash[i]);
    }
    out[64] = '\0';
    return String(out);
#elif defined(ESP8266)
    return sha256(data);
#else
    return "";
#endif
}

int parsePinToInt(const String& rawPin) {
    String pin = rawPin;
    pin.trim();
    pin.toUpperCase();
    pin.replace("GPIO", "");
    pin.replace("D", "");
    return pin.toInt();
}

void applyProvisioningConfig(const JsonVariantConst& payload) {
    if (payload.isNull()) {
        return;
    }

    if (!payload["device_id"].isNull()) {
        strlcpy(config.mqttClientId, payload["device_id"], sizeof(config.mqttClientId));
    }
    if (!payload["reporting_interval"].isNull()) {
        config.reportingInterval = max(5, (int) payload["reporting_interval"].as<int>());
    }

    if (!payload["config"].isNull()) {
        JsonObjectConst conf = payload["config"].as<JsonObjectConst>();
        if (!conf["api_endpoint"].isNull()) {
            strlcpy(config.apiEndpoint, conf["api_endpoint"], sizeof(config.apiEndpoint));
        }
        if (!conf["api_key"].isNull()) {
            strlcpy(config.apiKey, conf["api_key"], sizeof(config.apiKey));
        }
        if (!conf["sensor_id"].isNull()) {
            strlcpy(config.sensorId, conf["sensor_id"], sizeof(config.sensorId));
        }
    }

    for (int i = 0; i < MAX_SENSORS; i++) {
        strlcpy(config.sensors[i].type, "none", sizeof(config.sensors[i].type));
        config.sensors[i].pin = 0;
        config.sensors[i].i2cAddress = 0x76;
        config.sensors[i].i2cMultiplexerChannel = -1;
        config.sensors[i].tempOffset = 0.0f;
        config.sensors[i].humOffset = 0.0f;
        config.sensors[i].sensorId[0] = '\0';
        config.sensors[i].pinIdentifier[0] = '\0';
    }

    JsonArrayConst pins = payload["pins"].as<JsonArrayConst>();
    int idx = 0;
    for (JsonVariantConst pinVar : pins) {
        if (idx >= MAX_SENSORS) break;
        JsonObjectConst pinObj = pinVar.as<JsonObjectConst>();

        bool enabled = pinObj["enabled"].isNull() ? true : pinObj["enabled"].as<bool>();
        if (!enabled) {
            continue;
        }

        const char* sensorType = pinObj["sensor_type"] | "none";
        strlcpy(config.sensors[idx].type, normalizeSensorType(String(sensorType)).c_str(), sizeof(config.sensors[idx].type));

        String pinIdentifier = pinObj["pin"] | "";
        if (pinIdentifier.length() == 0 && !pinObj["pin_identifier"].isNull()) {
            pinIdentifier = String((const char*) pinObj["pin_identifier"]);
        }
        strlcpy(config.sensors[idx].pinIdentifier, pinIdentifier.c_str(), sizeof(config.sensors[idx].pinIdentifier));
        config.sensors[idx].pin = parsePinToInt(pinIdentifier);

        const char* sensorUniqueId = pinObj["sensor_id"] | "";
        if (strlen(sensorUniqueId) > 0) {
            strlcpy(config.sensors[idx].sensorId, sensorUniqueId, sizeof(config.sensors[idx].sensorId));
        }

        JsonObjectConst sensorCfg = pinObj["config"].as<JsonObjectConst>();
        if (!sensorCfg.isNull()) {
            if (!sensorCfg["i2c_address"].isNull()) {
                config.sensors[idx].i2cAddress = sensorCfg["i2c_address"].as<int>();
            }
            if (!sensorCfg["i2c_mux_channel"].isNull()) {
                config.sensors[idx].i2cMultiplexerChannel = sensorCfg["i2c_mux_channel"].as<int>();
            }
            if (!sensorCfg["temp_offset"].isNull()) {
                config.sensors[idx].tempOffset = sensorCfg["temp_offset"].as<float>();
            }
            if (!sensorCfg["hum_offset"].isNull()) {
                config.sensors[idx].humOffset = sensorCfg["hum_offset"].as<float>();
            }
        }

        idx++;
    }

    config.save();
    sensor.begin();
}

void runIdentify(int durationSeconds) {
    #ifdef LED_BUILTIN
    const int blinkPin = LED_BUILTIN;
    #else
    const int blinkPin = 2;
    #endif
    pinMode(blinkPin, OUTPUT);
    unsigned long endAt = millis() + ((unsigned long) max(1, durationSeconds) * 1000UL);
    while (millis() < endAt) {
        digitalWrite(blinkPin, LOW);
        delay(120);
        digitalWrite(blinkPin, HIGH);
        delay(120);
    }
}

void handleMqttCommand(const String& topic, const String& payload) {
    (void) topic;

    // Legacy plain-text commands
    if (payload == "restart") {
        mqttManager.publishAck("", "accepted", "restarting");
        delay(500);
        ESP.restart();
        return;
    }
    if (payload == "toggle_sim") {
        config.testingMode = !config.testingMode;
        config.save();
        mqttManager.publishAck("", "completed", config.testingMode ? "sim_on" : "sim_off");
        return;
    }
    if (payload == "update") {
        if (strlen(config.firmwareUrl) > 0) {
            mqttManager.publishAck("", "accepted", "starting_ota");
            OtaManager::triggerUpdate(config.firmwareUrl);
        } else {
            mqttManager.publishAck("", "failed", "missing_firmware_url");
        }
        return;
    }

    JsonDocument commandDoc;
    DeserializationError err = deserializeJson(commandDoc, payload);
    if (err) {
        mqttManager.publishAck("", "failed", "invalid_json");
        return;
    }

    // Raw config payload on /config topic (without wrapper command)
    if (commandDoc["command"].isNull()) {
        applyProvisioningConfig(commandDoc.as<JsonObjectConst>());
        mqttManager.publishAck("", "completed", "config_applied");
        return;
    }

    String command = String((const char*) commandDoc["command"]);
    command.toUpperCase();
    String commandId = commandDoc["command_id"] | "";
    JsonVariantConst cmdPayload = commandDoc["payload"];

    if (command == "CONFIG_UPDATE") {
        applyProvisioningConfig(cmdPayload);
        mqttManager.publishAck(commandId, "completed", "config_applied");
    } else if (command == "REBOOT") {
        mqttManager.publishAck(commandId, "accepted", "rebooting");
        delay(500);
        ESP.restart();
    } else if (command == "IDENTIFY") {
        int duration = cmdPayload["duration"] | 10;
        runIdentify(duration);
        mqttManager.publishAck(commandId, "completed", "identify_done");
    } else if (command == "SET_INTERVAL") {
        int interval = cmdPayload["interval"] | config.reportingInterval;
        config.reportingInterval = max(5, interval);
        config.save();
        mqttManager.publishAck(commandId, "completed", "interval_updated");
    } else if (command == "SET_PIN") {
        String pinRaw = cmdPayload["pin"] | "";
        bool state = cmdPayload["state"] | false;
        int pin = parsePinToInt(pinRaw);
        pinMode(pin, OUTPUT);
        digitalWrite(pin, state ? HIGH : LOW);
        mqttManager.publishAck(commandId, "completed", "pin_state_updated");
    } else if (command == "READ_NOW") {
        forceReadNow = true;
        mqttManager.publishAck(commandId, "accepted", "read_scheduled");
    } else if (command == "CALIBRATE") {
        String pinRaw = cmdPayload["pin"] | "";
        int pin = parsePinToInt(pinRaw);
        JsonObjectConst calibration = cmdPayload["calibration"].as<JsonObjectConst>();
        for (int i = 0; i < MAX_SENSORS; i++) {
            if (config.sensors[i].pin == pin) {
                if (!calibration["temp_offset"].isNull()) {
                    config.sensors[i].tempOffset = calibration["temp_offset"].as<float>();
                }
                if (!calibration["hum_offset"].isNull()) {
                    config.sensors[i].humOffset = calibration["hum_offset"].as<float>();
                }
            }
        }
        config.save();
        mqttManager.publishAck(commandId, "completed", "calibration_updated");
    } else if (command == "FIRMWARE_UPDATE") {
        String url = cmdPayload["url"] | "";
        String version = cmdPayload["version"] | "";
        if (url.length() == 0) {
            mqttManager.publishAck(commandId, "failed", "missing_url");
            return;
        }
        strlcpy(config.firmwareUrl, url.c_str(), sizeof(config.firmwareUrl));
        config.save();
        mqttManager.publishAck(commandId, "accepted", "ota_started");
        OtaManager::triggerUpdate(config.firmwareUrl, version.c_str());
    } else if (command == "FACTORY_RESET") {
        LittleFS.remove(CONFIG_FILE);
        mqttManager.publishAck(commandId, "accepted", "factory_resetting");
        delay(500);
        ESP.restart();
    } else {
        mqttManager.publishAck(commandId, "failed", "unsupported_command");
    }
}

void publishTelemetryMqtt() {
    int activeIdx = 0;

    for (int configIdx = 0; configIdx < MAX_SENSORS; configIdx++) {
        String configuredType = String(config.sensors[configIdx].type);
        if (configuredType == "none" || configuredType == "") {
            continue;
        }

        if (activeIdx >= activeSensorCount) {
            break;
        }

        const SensorReadings& sensorData = allSensorData[activeIdx];
        if (!sensorData.valid || sensorData.readings.empty()) {
            activeIdx++;
            continue;
        }

        String logicalSensorId = String(config.sensors[configIdx].sensorId);
        if (logicalSensorId.length() == 0) {
            logicalSensorId = String(config.sensorId);
        }

        JsonDocument dataDoc;
        JsonObject dataObj = dataDoc.to<JsonObject>();
        for (const auto& r : sensorData.readings) {
            String key = toCanonicalDataType(r.type);
            JsonObject reading = dataObj[key].to<JsonObject>();
            reading["value"] = r.value;
            reading["unit"] = r.unit;
        }

        String dataJson;
        serializeJson(dataObj, dataJson);

        JsonDocument payloadDoc;
        payloadDoc["messageId"] = generateMessageId();
        payloadDoc["timestamp"] = getIsoTime();
        payloadDoc["data"] = dataObj;
        payloadDoc["checksum"] = sha256Hex(dataJson);

        String payload;
        serializeJson(payloadDoc, payload);
        String topic = "sensors/" + logicalSensorId + "/telemetry";
        mqttManager.publishRaw(topic.c_str(), payload.c_str());

        activeIdx++;
    }
}
