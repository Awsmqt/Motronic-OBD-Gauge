#ifndef WEB_DASHBOARD_H
#define WEB_DASHBOARD_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "Config.h"

class WebDashboard {
public:
    WebDashboard();
    void begin();
    void update(SensorData &data);

private:
    WebServer server;
    SensorData *currentData;

    void setupRoutes();
    void handleRoot();
    void handleApiData();
    void handleDtcRead();
    void handleDtcClear();
    void handlePeakReset();
    void handleThemeSelect();
};

#endif // WEB_DASHBOARD_H
