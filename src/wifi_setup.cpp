#include "wifi_setup.h"
#include "config.h"
#include <WiFiManager.h>

#ifdef ESP32
#include <WebServer.h>
#else
#include <ESP8266WebServer.h>
#endif

void runWifiSetup() {
    WiFiManager wm;
    
    // Set custom hostname
    String hostname = "calid-" + config.getAdoptionCode();
    #ifdef ESP32
    WiFi.setHostname(hostname.c_str());
    #else
    WiFi.hostname(hostname);
    #endif

    // Professional looking setup page
    wm.setParamsPage(true);
    std::vector<const char *> menu = {"wifi","info","param","sep","restart"};
    wm.setMenu(menu);
    wm.setTitle("Calid Sensor Hub Setup");
    
    // Non-blocking wait for connection
    if (!wm.autoConnect("Calid_Setup")) {
        Serial.println("Failed to connect or hit timeout");
        ESP.restart();
    }

    // Capture the newly configured credentials
    strlcpy(config.ssid, WiFi.SSID().c_str(), sizeof(config.ssid));
    strlcpy(config.password, WiFi.psk().c_str(), sizeof(config.password));
    config.save();
}
