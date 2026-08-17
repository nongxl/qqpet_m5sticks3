#pragma once
#include <Arduino.h>

enum ImuEventType {
    IMU_EVENT_NONE = 0,
    IMU_EVENT_SHAKE,     // 摇晃逗玩 (带严格冷却)
    IMU_EVENT_DIZZY,     // 剧烈晃动眩晕
    IMU_EVENT_UPSIDE_DOWN// 倒立
};

class ImuSensorManager {
public:
    ImuSensorManager();
    void begin();
    ImuEventType update();

    float getTiltX() const { return tiltX; }
    float getTiltY() const { return tiltY; }

private:
    float lastAccX, lastAccY, lastAccZ;
    float tiltX, tiltY;
    uint32_t lastShakeDetectTime;
    uint32_t lastEventTriggerTime;
    int shakeCount;
    bool wasUpsideDown;
};

extern ImuSensorManager g_imu;
