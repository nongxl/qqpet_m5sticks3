#include "network_manager.h"
#include "pet_core.h"
#include <time.h>

NetworkManager g_net;

NetworkManager::NetworkManager() : apMode(true), lastConnectAttempt(0) {}

void NetworkManager::begin() {
    // 默认开启 AP_STA 双模，确保手机随时可以搜到热点进行配置
    startAP();

    const PetState& st = g_pet.getState();
    if (strlen(st.wifi_ssid) > 0) {
        connectWiFi(st.wifi_ssid, st.wifi_pwd);
    }
}

void NetworkManager::startAP() {
    apMode = true;
    WiFi.mode(WIFI_AP_STA);
    // 开启无密码的开放热点，手机连接最方便
    WiFi.softAP("QQPet-StickS3", nullptr);
}

void NetworkManager::connectWiFi(const char* ssid, const char* pwd) {
    if (!ssid || strlen(ssid) == 0) return;
    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(ssid, pwd);
    lastConnectAttempt = millis();
}

void NetworkManager::update() {
    // 如果配置了 Wi-Fi 但未连接，后台尝试重连
    if (WiFi.status() != WL_CONNECTED) {
        if (millis() - lastConnectAttempt > 15000) {
            lastConnectAttempt = millis();
            const PetState& st = g_pet.getState();
            if (strlen(st.wifi_ssid) > 0) {
                WiFi.begin(st.wifi_ssid, st.wifi_pwd);
            }
        }
    }
}
