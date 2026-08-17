#include "haptics.h"
#include <M5Unified.h>

HapticsEngine g_haptics;

HapticsEngine::HapticsEngine() 
    : currentPattern(HAPTIC_NONE), stepStartTime(0), stepIndex(0), isRunning(false) {}

void HapticsEngine::begin() {
    M5.Speaker.setVolume(120);
    // 彻底关闭并禁用震动马达，确保引脚输出低电平断电
    pinMode(19, OUTPUT);
    digitalWrite(19, LOW);
}

void HapticsEngine::setVibration(bool on) {
    // 暂时完全禁用震动电机，避免产生任何电源抖动
    (void)on;
}

void HapticsEngine::playTone(uint16_t freq, uint16_t durationMs) {
    M5.Speaker.tone(freq, durationMs);
}




void HapticsEngine::trigger(HapticPattern pattern) {
    currentPattern = pattern;
    stepIndex = 0;
    stepStartTime = millis();
    isRunning = true;

    switch (pattern) {
        case HAPTIC_CLICK:
            setVibration(true);
            playTone(2800, 20);
            break;
        case HAPTIC_SUCCESS:
            setVibration(true);
            playTone(1800, 40);
            break;
        case HAPTIC_ALERT:
            setVibration(true);
            playTone(600, 100);
            break;
        case HAPTIC_LEVELUP:
            setVibration(true);
            playTone(1200, 80);
            break;
        default:
            setVibration(false);
            isRunning = false;
            break;
    }
}

void HapticsEngine::update() {
    if (!isRunning) return;
    uint32_t elapsed = millis() - stepStartTime;

    switch (currentPattern) {
        case HAPTIC_CLICK:
            if (elapsed >= 25) {
                setVibration(false);
                isRunning = false;
            }
            break;

        case HAPTIC_SUCCESS:
            if (stepIndex == 0 && elapsed >= 40) {
                setVibration(false);
                stepIndex = 1;
                stepStartTime = millis();
                playTone(2400, 50);
            } else if (stepIndex == 1 && elapsed >= 30) {
                setVibration(true);
                stepIndex = 2;
                stepStartTime = millis();
            } else if (stepIndex == 2 && elapsed >= 40) {
                setVibration(false);
                isRunning = false;
            }
            break;

        case HAPTIC_ALERT:
            if (stepIndex == 0 && elapsed >= 80) {
                setVibration(false);
                stepIndex = 1;
                stepStartTime = millis();
                playTone(450, 100);
            } else if (stepIndex == 1 && elapsed >= 60) {
                setVibration(true);
                stepIndex = 2;
                stepStartTime = millis();
            } else if (stepIndex == 2 && elapsed >= 80) {
                setVibration(false);
                isRunning = false;
            }
            break;

        case HAPTIC_LEVELUP:
            if (stepIndex == 0 && elapsed >= 80) {
                setVibration(false);
                stepIndex = 1;
                stepStartTime = millis();
                playTone(1600, 80);
            } else if (stepIndex == 1 && elapsed >= 60) {
                setVibration(true);
                stepIndex = 2;
                stepStartTime = millis();
                playTone(2200, 120);
            } else if (stepIndex == 2 && elapsed >= 120) {
                setVibration(false);
                isRunning = false;
            }
            break;

        default:
            setVibration(false);
            isRunning = false;
            break;
    }
}

