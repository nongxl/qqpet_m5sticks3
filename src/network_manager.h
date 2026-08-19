#pragma once
#include <Arduino.h>
#include <WiFi.h>

class NetworkManager {
public:
    NetworkManager();
    void begin();
    void update();
    
    bool isConnected() const { return WiFi.status() == WL_CONNECTED; }
    String getIPAddress() const { return isConnected() ? WiFi.localIP().toString() : WiFi.softAPIP().toString(); }
    
    void startAP();
    void connectWiFi(const char* ssid, const char* pwd);

private:
    bool apMode;
    bool wasConnected;
    uint32_t lastConnectAttempt;
};


extern NetworkManager g_net;
