#pragma once
#include <Arduino.h>

enum HapticPattern {
    HAPTIC_NONE = 0,
    HAPTIC_CLICK,      // 单击轻按
    HAPTIC_SUCCESS,    // 成功操作
    HAPTIC_ALERT,      // 异常警报
    HAPTIC_LEVELUP     // 升级庆祝
};

class HapticsEngine {
public:
    HapticsEngine();
    void begin();
    void update();
    void trigger(HapticPattern pattern);
    void playTone(uint16_t freq, uint16_t durationMs);

private:
    void setVibration(bool on);

    HapticPattern currentPattern;
    uint32_t stepStartTime;
    int stepIndex;
    bool isRunning;
};


extern HapticsEngine g_haptics;
