#pragma once
#include <Arduino.h>

enum WeatherType : uint8_t {
    WEATHER_SUNNY = 0,  // ☀️ 晴朗
    WEATHER_RAINY = 1,  // 🌧️ 细雨
    WEATHER_SNOWY = 2,  // ❄️ 飞雪
    WEATHER_CLOUDY = 3  // ☁️ 多云
};

class WeatherManager {
public:
    static WeatherManager& getInstance() {
        static WeatherManager instance;
        return instance;
    }

    void begin();
    void update();
    void fetchWeatherFromNetwork();

    WeatherType getCurrentWeather() const { return currentWeather; }
    int getCurrentTemp() const { return currentTemp; }
    const char* getWeatherName() const;
    const char* getWeatherIcon() const;

    bool isSyncedWithNetwork() const { return syncedWithNetwork; }

    void setWeatherManual(WeatherType w, int temp) {
        currentWeather = w;
        currentTemp = temp;
    }

private:
    WeatherManager();
    WeatherType currentWeather;
    int currentTemp;
    bool syncedWithNetwork;
    uint32_t lastCheckTime;
};

