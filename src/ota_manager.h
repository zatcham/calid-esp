#ifndef CALID_OTA_MANAGER_H
#define CALID_OTA_MANAGER_H

#include <Arduino.h>

class OtaManager {
public:
    static void triggerUpdate(const char* url, const char* version = nullptr);
    static void loop();

private:
    static bool _updatePending;
    static String _updateUrl;
    static String _targetVersion;
    static void performUpdate(String url);
};

#endif
