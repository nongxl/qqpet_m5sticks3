#pragma once
#include <Arduino.h>
#include <WebServer.h>

class WebServerPortal {
public:
    WebServerPortal();
    void begin();
    void update();

private:
    WebServer server;
    
    void handleRoot();
    void handleStatusApi();
    void handleActionApi();
    void handleConfigApi();
    void handleNotFound();
};

extern WebServerPortal g_webPortal;
