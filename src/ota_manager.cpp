#include "ota_manager.h"
#include "logging.h"
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

void OtaManager::triggerUpdate(const char* url) {
    _updateUrl = String(url);
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
        http.end();
        return;
    }

    int contentLength = http.getSize();
    if (contentLength <= 0) {
        logger.log("OTA: Invalid content length");
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
                delay(1000);
                ESP.restart();
            } else {
                logger.log("OTA: Not finished?");
            }
        } else {
            logger.log("OTA: Error occurred #: " + String(Update.getError()));
        }
    } else {
        logger.log("OTA: Not enough space");
    }

    http.end();
}
