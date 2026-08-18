#include "haptics.h"
#include <M5Unified.h>

HapticsEngine g_haptics;

// 参考 sandtimer_for_StickS3 硬件配置：StickS3 扩展口 Hat Vibrator 控制线为 GPIO 0
static constexpr int VIBR_PIN         = 0;
static constexpr int VIBR_PWM_CHANNEL = 2;
static constexpr int VIBR_PWM_FREQ    = 10000; // 10kHz PWM 载波，消除电机电感噪音
static constexpr int VIBR_PWM_BITS    = 8;

// 精细微震强度配置 (PWM 0~255)
static constexpr uint8_t PWM_CLICK    = 75;   // 轻巧点击 (清脆微震感)
static constexpr uint8_t PWM_SUCCESS  = 90;   // 确认/成功 (饱满适中)
static constexpr uint8_t PWM_ALERT    = 105;  // 警报 (醒目有力)
static constexpr uint8_t PWM_LEVELUP  = 100;  // 升级庆祝 (连击欢快)

HapticsEngine::HapticsEngine() 
    : currentPattern(HAPTIC_NONE), stepStartTime(0), stepIndex(0), isRunning(false) {}

void HapticsEngine::begin() {
    // 1. 初始化引脚为输出并立即拉低
    pinMode(VIBR_PIN, OUTPUT);
    digitalWrite(VIBR_PIN, LOW);

    // 2. 配置 LEDC PWM 通道并绑定 GPIO 0
    ledcSetup(VIBR_PWM_CHANNEL, VIBR_PWM_FREQ, VIBR_PWM_BITS);
    ledcAttachPin(VIBR_PIN, VIBR_PWM_CHANNEL);

    // 3. 立即将占空比置 0，彻底根除开机自激/长震问题
    ledcWrite(VIBR_PWM_CHANNEL, 0);
}

void HapticsEngine::setVibrationPWM(uint8_t pwmLevel) {
    ledcWrite(VIBR_PWM_CHANNEL, pwmLevel);
}

void HapticsEngine::playTone(uint16_t freq, uint16_t durationMs) {
    if (!M5.Speaker.isPlaying()) {
        M5.Speaker.tone(freq, durationMs);
    }
}

void HapticsEngine::trigger(HapticPattern pattern) {
    currentPattern = pattern;
    stepIndex = 0;
    stepStartTime = millis();
    isRunning = true;

    switch (pattern) {
        case HAPTIC_CLICK:
            setVibrationPWM(PWM_CLICK);
            playTone(3200, 15);
            break;
        case HAPTIC_SUCCESS:
            setVibrationPWM(PWM_SUCCESS);
            playTone(2200, 35);
            break;
        case HAPTIC_ALERT:
            setVibrationPWM(PWM_ALERT);
            playTone(750, 60);
            break;
        case HAPTIC_LEVELUP:
            setVibrationPWM(PWM_LEVELUP);
            playTone(1600, 50);
            break;
        default:
            setVibrationPWM(0);
            isRunning = false;
            break;
    }
}

void HapticsEngine::update() {
    if (!isRunning) {
        // 非运行状态绝对输出 0，彻底停机
        setVibrationPWM(0);
        return;
    }

    uint32_t elapsed = millis() - stepStartTime;

    // 安全保护：单次模式超时 500ms 强制停机
    if (elapsed > 500) {
        setVibrationPWM(0);
        isRunning = false;
        return;
    }

    switch (currentPattern) {
        case HAPTIC_CLICK:
            if (elapsed >= 30) { // 30ms 极轻微清脆震感
                setVibrationPWM(0);
                isRunning = false;
            }
            break;

        case HAPTIC_SUCCESS:
            // 节奏: 35ms 震 -> 30ms 停 -> 40ms 震
            if (elapsed < 35) {
                setVibrationPWM(PWM_SUCCESS);
            } else if (elapsed < 65) {
                setVibrationPWM(0);
            } else if (elapsed < 105) {
                setVibrationPWM(PWM_SUCCESS);
            } else {
                setVibrationPWM(0);
                isRunning = false;
            }
            break;

        case HAPTIC_ALERT:
            // 节奏: 50ms 震 -> 35ms 停 -> 50ms 震 -> 35ms 停 -> 50ms 震
            if (elapsed < 50) {
                setVibrationPWM(PWM_ALERT);
            } else if (elapsed < 85) {
                setVibrationPWM(0);
            } else if (elapsed < 135) {
                setVibrationPWM(PWM_ALERT);
            } else if (elapsed < 170) {
                setVibrationPWM(0);
            } else if (elapsed < 220) {
                setVibrationPWM(PWM_ALERT);
            } else {
                setVibrationPWM(0);
                isRunning = false;
            }
            break;

        case HAPTIC_LEVELUP:
            // 欢快 4 连击节奏
            if (elapsed < 40) {
                setVibrationPWM(PWM_LEVELUP);
            } else if (elapsed < 75) {
                setVibrationPWM(0);
            } else if (elapsed < 115) {
                setVibrationPWM(PWM_LEVELUP);
            } else if (elapsed < 150) {
                setVibrationPWM(0);
            } else if (elapsed < 195) {
                setVibrationPWM(PWM_LEVELUP);
            } else {
                setVibrationPWM(0);
                isRunning = false;
            }
            break;

        default:
            setVibrationPWM(0);
            isRunning = false;
            break;
    }
}
