#include "imu_sensor.h"
#include "config.h"
#include <M5Unified.h>
#include <cmath>

ImuSensorManager g_imu;

ImuSensorManager::ImuSensorManager() 
    : lastAccX(0), lastAccY(0), lastAccZ(0), tiltX(0), tiltY(0),
      lastShakeDetectTime(0), lastEventTriggerTime(0), shakeCount(0), wasUpsideDown(false) {}

void ImuSensorManager::begin() {
    M5.Imu.init();
}

ImuEventType ImuSensorManager::update() {
    uint32_t now = millis();
    static uint32_t lastImuReadTime = 0;
    if (now - lastImuReadTime < 40) {
        return IMU_EVENT_NONE;
    }
    lastImuReadTime = now;

    float ax = 0, ay = 0, az = 0;
    if (!M5.Imu.getAccel(&ax, &ay, &az)) {
        return IMU_EVENT_NONE;
    }

    tiltX = ax;
    tiltY = ay;


    // 处于事件冷却期 (触发一次后 2.5 秒内不重复报告事件)
    if (now - lastEventTriggerTime < 2500) {
        lastAccX = ax;
        lastAccY = ay;
        lastAccZ = az;
        return IMU_EVENT_NONE;
    }

    // 计算加速度变化率
    float deltaX = std::abs(ax - lastAccX);
    float deltaY = std::abs(ay - lastAccY);
    float deltaZ = std::abs(az - lastAccZ);
    float totalDelta = deltaX + deltaY + deltaZ;

    lastAccX = ax;
    lastAccY = ay;
    lastAccZ = az;

    // 倒立边缘触发检测 (避免每帧持续触发)
    bool isUpsideDownNow = (ay < -0.92f);
    if (isUpsideDownNow && !wasUpsideDown) {
        wasUpsideDown = true;
        lastEventTriggerTime = now;
        return IMU_EVENT_UPSIDE_DOWN;
    }
    if (!isUpsideDownNow) {
        wasUpsideDown = false;
    }

    // 摇晃检测 (有效连续 2 次以上晃动才判定为 1 次摇晃事件)
    if (totalDelta > IMU_SHAKE_THRESHOLD) {
        if (now - lastShakeDetectTime > 150 && now - lastShakeDetectTime < 800) {
            shakeCount++;
            lastShakeDetectTime = now;
            if (shakeCount >= 4) {
                shakeCount = 0;
                lastEventTriggerTime = now;
                return IMU_EVENT_DIZZY;
            } else if (shakeCount >= 2) {
                shakeCount = 0;
                lastEventTriggerTime = now;
                return IMU_EVENT_SHAKE;
            }
        } else if (now - lastShakeDetectTime >= 800) {
            shakeCount = 1;
            lastShakeDetectTime = now;
        }
    }

    return IMU_EVENT_NONE;
}
