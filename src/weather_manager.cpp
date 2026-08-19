#include "weather_manager.h"
#include "network_manager.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

WeatherManager::WeatherManager()
    : currentWeather(WEATHER_SUNNY), currentTemp(26), syncedWithNetwork(false), lastCheckTime(0) {
}

void WeatherManager::begin() {
    currentWeather = WEATHER_SUNNY;
    currentTemp = 26;
    syncedWithNetwork = false;
    lastCheckTime = 0;
}

const char* WeatherManager::getWeatherName() const {
    switch (currentWeather) {
        case WEATHER_SUNNY: return "晴朗";
        case WEATHER_RAINY: return "小雨";
        case WEATHER_SNOWY: return "飞雪";
        case WEATHER_CLOUDY: return "多云";
        default: return "晴朗";
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
    if (!g_net.isConnected()) {
        syncedWithNetwork = false;
        return;
    }

    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(6);

    HTTPClient http;
    http.setTimeout(5000);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    bool fetched = false;

    // 1. 优先尝试 wttr.in 自动基于 IP 获取当前真实温度与天气
    if (http.begin(client, "https://wttr.in/?format=j1")) {
        http.setUserAgent("curl/7.88.1");
        int code = http.GET();
        if (code == 200) {
            String res = http.getString();
            DynamicJsonDocument docW(4096);
            if (!deserializeJson(docW, res)) {
                if (docW.containsKey("current_condition") && docW["current_condition"].size() > 0) {
                    JsonObject cur = docW["current_condition"][0];
                    int t = cur["temp_C"].as<int>();
                    int wCode = cur["weatherCode"].as<int>();
                    currentTemp = t;
                    syncedWithNetwork = true;
                    fetched = true;

                    if (wCode == 113) {
                        currentWeather = WEATHER_SUNNY;
                    } else if (wCode == 116 || wCode == 119 || wCode == 122 || wCode == 143 || wCode == 248) {
                        currentWeather = WEATHER_CLOUDY;
                    } else if (wCode == 179 || wCode == 182 || wCode == 185 || wCode == 227 || wCode == 230 || 
                               (wCode >= 317 && wCode <= 377) || (wCode >= 392 && wCode <= 395)) {
                        currentWeather = WEATHER_SNOWY;
                    } else {
                        currentWeather = WEATHER_RAINY;
                    }
                }
            }
        }
        http.end();
    }

    // 2. 回退备选方案：Open-Meteo
    if (!fetched) {
        if (http.begin(client, "https://api.open-meteo.com/v1/forecast?latitude=39.90&longitude=116.40&current_weather=true")) {
            http.setUserAgent("ESP32");
            int code = http.GET();
            if (code == 200) {
                String res = http.getString();
                DynamicJsonDocument docW(512);
                if (!deserializeJson(docW, res)) {
                    float t = docW["current_weather"]["temperature"] | 26.0f;
                    int wCode = docW["current_weather"]["weathercode"] | 0;

                    currentTemp = (int)round(t);
                    syncedWithNetwork = true;
                    fetched = true;

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
}

void WeatherManager::update() {
    uint32_t now = millis();
    // 每 1 小时 (3600000 毫秒) 联网自动同步一次真实天气
    if (now - lastCheckTime >= 3600000 || (lastCheckTime == 0 && g_net.isConnected())) {
        lastCheckTime = now;
        if (g_net.isConnected()) {
            fetchWeatherFromNetwork();
        } else {
            syncedWithNetwork = false;
        }
    }
}


