#ifndef CALID_WEBSERVER_H // Renamed to solve conflict w AsyncWebServer
#define CALID_WEBSERVER_H

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>

class WebServer {
public:
    WebServer();
    void begin();
    void handleClient();

private:
    AsyncWebServer server;
    DNSServer dnsServer;
    void handleRoot(AsyncWebServerRequest *request);
    void handleConfig(AsyncWebServerRequest *request);
    void handleCurrentConfig(AsyncWebServerRequest *request);
    void handleLogs(AsyncWebServerRequest *request);
    void handleCurrentData(AsyncWebServerRequest *request);
    void handleEditConfig(AsyncWebServerRequest *request);
};

#endif // CALID_WEBSERVER_H
