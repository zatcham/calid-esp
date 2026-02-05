#ifndef CALID_OTA_MANAGER_H
#define CALID_OTA_MANAGER_H

#include <Arduino.h>

class OtaManager {
public:
    static void triggerUpdate(const char* url);
    static void loop();

private:
    static bool _updatePending;
    static String _updateUrl;
    static void performUpdate(String url);
};

#endif
