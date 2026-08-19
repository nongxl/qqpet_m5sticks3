#include "network_manager.h"
#include "pet_core.h"
#include "weather_manager.h"
#include "display_engine.h"
#include <time.h>

NetworkManager g_net;

NetworkManager::NetworkManager() : apMode(true), wasConnected(false), lastConnectAttempt(0) {}

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
    bool connected = (WiFi.status() == WL_CONNECTED);

    // 刚连接上 WiFi 时触发一次全量初始化 (NTP 对时 + 真实天气拉取 + 屏幕提示)
    if (connected && !wasConnected) {
        wasConnected = true;
        configTime(8 * 3600, 0, "pool.ntp.org", "ntp.aliyun.com");
        WeatherManager::getInstance().fetchWeatherFromNetwork();
        g_display.showToast("📶 WiFi已连接! 天气已同步", 3500);
    } else if (!connected && wasConnected) {
        wasConnected = false;
    }

    // 如果配置了 Wi-Fi 但未连接，后台尝试重连
    if (!connected) {
        if (millis() - lastConnectAttempt > 15000) {
            lastConnectAttempt = millis();
            const PetState& st = g_pet.getState();
            if (strlen(st.wifi_ssid) > 0) {
                WiFi.begin(st.wifi_ssid, st.wifi_pwd);
            }
        }
    }
}

