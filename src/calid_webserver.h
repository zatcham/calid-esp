#ifndef CALID_WEBSERVER_H
#define CALID_WEBSERVER_H

#if defined(ESP8266)
#include <ESPAsyncTCP.h>
#else
#include <AsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>

class CalidWebServer {
public:
    CalidWebServer();
    void begin();
    void handleClient();

private:
    AsyncWebServer server;
    DNSServer dnsServer;
    
    // API Handlers
    void handleApiConfigSave(AsyncWebServerRequest *request);
    void handleApiConfigGet(AsyncWebServerRequest *request);
    void handleApiLogs(AsyncWebServerRequest *request);
    void handleApiData(AsyncWebServerRequest *request);
    void handleApiSystem(AsyncWebServerRequest *request);
    void handleApiScanI2C(AsyncWebServerRequest *request);
    void handleApiWifiScan(AsyncWebServerRequest *request);
    
    // Auth
    bool authenticate(AsyncWebServerRequest *request);
    
    // OTA Handlers
    void handleUpdateUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
};

#endif // CALID_WEBSERVER_H