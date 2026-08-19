#include "weather_manager.h"
#include "network_manager.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>

WeatherManager::WeatherManager()
    : currentWeather(WEATHER_SUNNY), currentTemp(26), syncedWithNetwork(false), lastCheckTime(0) {
}

void WeatherManager::begin() {
    currentWeather = WEATHER_SUNNY;
    currentTemp = 26;
    syncedWithNetwork = false;
    lastCheckTime = millis();
}

const char* WeatherManager::getWeatherName() const {
    switch (currentWeather) {
        case WEATHER_SUNNY: return "晴朗";
        case WEATHER_RAINY: return "小雨";
        case WEATHER_SNOWY: return "飞雪";
        case WEATHER_CLOUDY: return "多云";
        default: return "晴天";
    }
}

const char* WeatherManager::getWeatherIcon() const {
    switch (currentWeather) {
        case WEATHER_SUNNY: return "☀️";
        case WEATHER_RAINY: return "🌧️";
        case WEATHER_SNOWY: return "❄️";
        case WEATHER_CLOUDY: return "☁️";
        default: return "☀️";
    }
}

void WeatherManager::fetchWeatherFromNetwork() {
    if (!g_net.isConnected()) return;

    HTTPClient http;
    http.setTimeout(4500);

    float lat = 39.9f;
    float lon = 116.4f;

    // 1. 通过 IP 获取当地地理经纬度
    if (http.begin("http://ip-api.com/json/?fields=lat,lon")) {
        int code = http.GET();
        if (code == 200) {
            String res = http.getString();
            StaticJsonDocument<256> docGeo;
            if (!deserializeJson(docGeo, res)) {
                lat = docGeo["lat"] | 39.9f;
                lon = docGeo["lon"] | 116.4f;
            }
        }
        http.end();
    }

    // 2. 调用 Open-Meteo 免费气象 API 获取真实气温与天气代码
    char url[128];
    snprintf(url, sizeof(url), "http://api.open-meteo.com/v1/forecast?latitude=%.2f&longitude=%.2f&current_weather=true", lat, lon);

    if (http.begin(url)) {
        int code = http.GET();
        if (code == 200) {
            String res = http.getString();
            StaticJsonDocument<512> docW;
            if (!deserializeJson(docW, res)) {
                float t = docW["current_weather"]["temperature"] | 26.0f;
                int wCode = docW["current_weather"]["weathercode"] | 0;

                currentTemp = (int)round(t);
                syncedWithNetwork = true;

                // WMO 天气代码转换
                if (wCode == 0) {
                    currentWeather = WEATHER_SUNNY;
                } else if (wCode >= 1 && wCode <= 48) {
                    currentWeather = WEATHER_CLOUDY;
                } else if ((wCode >= 51 && wCode <= 67) || (wCode >= 80 && wCode <= 99)) {
                    currentWeather = WEATHER_RAINY;
                } else if (wCode >= 71 && wCode <= 77) {
                    currentWeather = WEATHER_SNOWY;
                } else {
                    currentWeather = WEATHER_SUNNY;
                }
            }
        }
        http.end();
    }
}

void WeatherManager::update() {
    uint32_t now = millis();
    // 每 15 分钟联网拉取一次天气
    if (now - lastCheckTime > 900000 || (lastCheckTime == 0 && g_net.isConnected())) {
        lastCheckTime = now;
        if (g_net.isConnected()) {
            fetchWeatherFromNetwork();
        } else if (!syncedWithNetwork) {
            // 未联网时根据时间段模拟轮换
            int cycle = (now / 180000) % 4;
            if (cycle == 0) { currentWeather = WEATHER_SUNNY; currentTemp = 26; }
            else if (cycle == 1) { currentWeather = WEATHER_RAINY; currentTemp = 20; }
            else if (cycle == 2) { currentWeather = WEATHER_CLOUDY; currentTemp = 22; }
            else { currentWeather = WEATHER_SNOWY; currentTemp = 2; }
        }
    }
}

