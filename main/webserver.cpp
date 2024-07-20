#include "webserver.h"
#include "config.h"
#include "sensor.h" // so we can get sensor data
#include "logging.h"

const byte DNS_PORT = 53;

WebServer::WebServer() : server(80) {}

void WebServer::begin() {
    if (WiFi.getMode() == WIFI_AP) {
        dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
    }

    server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) { this->handleRoot(request); });
    server.on("/config", HTTP_POST, [this](AsyncWebServerRequest *request) { this->handleConfig(request); });
    server.on("/edit_config", HTTP_GET, [this](AsyncWebServerRequest *request) { this->handleEditConfig(request); });
    server.on("/current_config", HTTP_GET, [this](AsyncWebServerRequest *request) { this->handleCurrentConfig(request); });
    server.on("/current_data", HTTP_GET, [this](AsyncWebServerRequest *request) { this->handleCurrentData(request); });
    server.on("/logs", HTTP_GET, [this](AsyncWebServerRequest *request) { this->handleLogs(request); });

    server.begin();
}

void WebServer::handleClient() {
    if (WiFi.getMode() == WIFI_AP) {
        dnsServer.processNextRequest();
    }
    // AsyncWebServer handles requests asynchronously
}

void WebServer::handleRoot(AsyncWebServerRequest *request) {
    String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'>"
                  "<link rel='stylesheet' href='https://maxcdn.bootstrapcdn.com/bootstrap/4.5.2/css/bootstrap.min.css'>"
                  "<style>body { padding: 20px; } .container { max-width: 800px; margin: auto; } .btn { margin-top: 20px; }</style>"
                  "</head><body><div class='container'>"
                  "<h1>Calid - Sensor Dashboard</h1>"
                  "<div id='currentData'></div>"
                  "<a class='btn btn-primary' href='/edit_config'>Go to Configuration</a>"
                  "<a class='btn btn-secondary' href='/logs'>View Logs</a>"
                  "<script>fetch('/current_data').then(response => response.json()).then(data => {"
                  "document.getElementById('currentData').innerHTML = '<h2>Current Data</h2>' +"
                  "'<p>Temperature: ' + data.temperature + '°C</p>' +"
                  "'<p>Humidity: ' + data.humidity + '%</p>';"
                  "});</script>"
                  "</div></body></html>";
    request->send(200, "text/html", html);
}

void WebServer::handleConfig(AsyncWebServerRequest *request) {
    if (request->hasParam("ssid", true) && request->hasParam("password", true) &&
        request->hasParam("apiEndpoint", true) && request->hasParam("sensorId", true) &&
        request->hasParam("apiKey", true) && request->hasParam("testingMode", true)) {

        String ssid = request->getParam("ssid", true)->value();
        String password = request->getParam("password", true)->value();
        String apiEndpoint = request->getParam("apiEndpoint", true)->value();
        String sensorId = request->getParam("sensorId", true)->value();
        String apiKey = request->getParam("apiKey", true)->value();
        bool testingMode = request->getParam("testingMode", true)->value() == "on";

        strncpy(config.ssid, ssid.c_str(), sizeof(config.ssid) - 1);
        strncpy(config.password, password.c_str(), sizeof(config.password) - 1);
        strncpy(config.apiEndpoint, apiEndpoint.c_str(), sizeof(config.apiEndpoint) - 1);
        strncpy(config.sensorId, sensorId.c_str(), sizeof(config.sensorId) - 1);
        strncpy(config.apiKey, apiKey.c_str(), sizeof(config.apiKey) - 1);
        config.testingMode = testingMode;

        config.save();

        request->send(200, "text/html", "<html><body><h1>Configuration Saved</h1><p>Device will restart to apply new settings.</p></body></html>");

        delay(2000);
        ESP.restart();
    } else {
        request->send(400, "text/plain", "Bad Request");
    }
}

void WebServer::handleCurrentConfig(AsyncWebServerRequest *request) {
    String json = "{";
    json += "\"ssid\":\"" + String(config.ssid) + "\",";
    json += "\"password\":\"" + String(config.password) + "\",";
    json += "\"apiEndpoint\":\"" + String(config.apiEndpoint) + "\",";
    json += "\"sensorId\":\"" + String(config.sensorId) + "\",";
    json += "\"apiKey\":\"" + String(config.apiKey) + "\",";
    json += "\"testingMode\":" + String(config.testingMode ? "true" : "false");
    json += "}";
    request->send(200, "application/json", json);
}

void WebServer::handleLogs(AsyncWebServerRequest *request) {
    File file = LittleFS.open("/log.txt", "r");
    if (!file) {
        request->send(500, "text/plain", "Failed to open log file");
        return;
    }
    String logs;
    while (file.available()) {
        logs += (char)file.read();
    }
    file.close();
    String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'>"
                  "<link rel='stylesheet' href='https://maxcdn.bootstrapcdn.com/bootstrap/4.5.2/css/bootstrap.min.css'>"
                  "<style>body { padding: 20px; } .container { max-width: 800px; margin: auto; }</style>"
                  "</head><body><div class='container'>"
                  "<h1>Logs</h1><pre>" + logs + "</pre>"
                  "<a class='btn btn-primary' href='/'>Back to Dashboard</a>"
                  "</div></body></html>";
    request->send(200, "text/html", html);
}

void WebServer::handleCurrentData(AsyncWebServerRequest *request) {
    String json = "{"
                  "\"temperature\":" + String(sensorData.temperature) + ","
                  "\"humidity\":" + String(sensorData.humidity) + ","
                  "\"valid\":" + (sensorData.valid ? "true" : "false") +
                  "}";
    request->send(200, "application/json", json);
}

void WebServer::handleEditConfig(AsyncWebServerRequest *request) {
    String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'>"
                  "<link rel='stylesheet' href='https://maxcdn.bootstrapcdn.com/bootstrap/4.5.2/css/bootstrap.min.css'>"
                  "<style>body { padding: 20px; } .container { max-width: 800px; margin: auto; }</style>"
                  "</head><body><div class='container'>"
                  "<h1>Calid - Sensor Configuration</h1><form action='/config' method='post'>"
                  "<div class='mb-3'><label for='ssid' class='form-label'>WiFi SSID:</label><input type='text' class='form-control' id='ssid' name='ssid' value='" + String(config.ssid) + "'></div>"
                  "<div class='mb-3'><label for='password' class='form-label'>WiFi Password:</label><input type='password' class='form-control' id='password' name='password' value='" + String(config.password) + "'></div>"
                  "<div class='mb-3'><label for='apiEndpoint' class='form-label'>API Endpoint:</label><input type='text' class='form-control' id='apiEndpoint' name='apiEndpoint' value='" + String(config.apiEndpoint) + "'></div>"
                  "<div class='mb-3'><label for='sensorId' class='form-label'>Sensor ID:</label><input type='text' class='form-control' id='sensorId' name='sensorId' value='" + String(config.sensorId) + "'></div>"
                  "<div class='mb-3'><label for='apiKey' class='form-label'>API Key:</label><input type='text' class='form-control' id='apiKey' name='apiKey' value='" + String(config.apiKey) + "'></div>"
                  "<div class='mb-3'><label for='testingMode' class='form-label'>Testing Mode:</label><input type='checkbox' id='testingMode' name='testingMode' " + (config.testingMode ? "checked" : "") + "></div>"
                  "<button type='submit' class='btn btn-primary'>Save</button></form></div></body></html>";
    request->send(200, "text/html", html);
}
