#include "ota_manager.h"
#include "logging.h"
#include "mqtt_manager.h"
#include "config.h"
#if defined(ESP8266)
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <Updater.h>
#else
#include <HTTPClient.h>
#include <WiFi.h>
#include <Update.h>
#endif

extern Logger logger;

bool OtaManager::_updatePending = false;
String OtaManager::_updateUrl = "";
String OtaManager::_targetVersion = "";

void OtaManager::triggerUpdate(const char* url, const char* version) {
    _updateUrl = String(url);
    _targetVersion = version ? String(version) : "";
    _updatePending = true;
}

void OtaManager::loop() {
    if (_updatePending) {
        _updatePending = false;
        performUpdate(_updateUrl);
    }
}

void OtaManager::performUpdate(String url) {
    logger.log("OTA: Starting update from " + url);
    Serial.println("OTA: Starting update...");
    mqttManager.publishOtaStatus("in_progress", 0, _targetVersion);

    HTTPClient http;
    WiFiClient client;
    WiFiClientSecure sclient;

    if (url.startsWith("https")) {
        sclient.setInsecure();
        http.begin(sclient, url);
    } else {
        http.begin(client, url);
    }

    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK) {
        logger.log("OTA: HTTP GET failed, code: " + String(httpCode));
        mqttManager.publishOtaStatus("failed", 0, _targetVersion);
        http.end();
        return;
    }

    int contentLength = http.getSize();
    if (contentLength <= 0) {
        logger.log("OTA: Invalid content length");
        mqttManager.publishOtaStatus("failed", 0, _targetVersion);
        http.end();
        return;
    }

    bool canBegin = false;
    #ifdef ESP32
    canBegin = Update.begin(contentLength, U_FLASH);
    #else
    canBegin = Update.begin(contentLength, U_FLASH);
    #endif

    if (canBegin) {
        WiFiClient* stream = http.getStreamPtr();
        size_t written = Update.writeStream(*stream);
        int progress = (contentLength > 0) ? (int) ((written * 100U) / (size_t) contentLength) : 0;
        mqttManager.publishOtaStatus("in_progress", progress, _targetVersion);

        if (written == (size_t)contentLength) {
            Serial.println("OTA: Written " + String(written) + " successfully");
        } else {
            Serial.println("OTA: Written only " + String(written) + "/" + String(contentLength));
        }

        if (Update.end()) {
            Serial.println("OTA: Update finished!");
            if (Update.isFinished()) {
                logger.log("OTA: Success. Rebooting.");
                Serial.println("OTA: Success. Rebooting.");
                if (_targetVersion.length() > 0) {
                    strlcpy(config.currentFirmwareVersion, _targetVersion.c_str(), sizeof(config.currentFirmwareVersion));
                    config.save();
                }
                mqttManager.publishOtaStatus("completed", 100, _targetVersion);
                delay(1000);
                ESP.restart();
            } else {
                logger.log("OTA: Not finished?");
                mqttManager.publishOtaStatus("failed", progress, _targetVersion);
            }
        } else {
            logger.log("OTA: Error occurred #: " + String(Update.getError()));
            mqttManager.publishOtaStatus("failed", progress, _targetVersion);
        }
    } else {
        logger.log("OTA: Not enough space");
        mqttManager.publishOtaStatus("failed", 0, _targetVersion);
    }

    http.end();
}
