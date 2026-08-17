#pragma once
#include <Arduino.h>

// ─────────────────────────────────────────────────────────────
//  QQ 宠物 (M5Stack StickS3) 系统硬件与常量配置
// ─────────────────────────────────────────────────────────────

// 屏幕分辨率 (竖屏 135 x 240)
static constexpr int SCREEN_W = 135;
static constexpr int SCREEN_H = 240;

// 系统默认音量与背光亮度 (0 - 255)
static constexpr uint8_t DEFAULT_VOLUME     = 120;
static constexpr uint8_t DEFAULT_BRIGHTNESS = 200;

// 定时周期常量 (毫秒)
static constexpr uint32_t DECAY_INTERVAL_MS     = 60000; // 属性自然衰减周期 (60秒)
static constexpr uint32_t SAVE_INTERVAL_MS      = 30000; // 自动持久化周期 (30秒)
static constexpr uint32_t MENU_TIMEOUT_MS       = 6000;  // 菜单无操作自动关闭时间
static constexpr uint32_t BUBBLE_DISPLAY_MS     = 4500;  // 气泡对话显示时长
static constexpr uint32_t IDLE_BEHAVIOR_MS      = 5000;  // 自主日常动作切换周期 (5秒)

// IMU 传感器阈值
static constexpr float IMU_SHAKE_THRESHOLD      = 2.2f;  // 晃动检测阈值 (G)
static constexpr float IMU_TILT_THRESHOLD       = 0.45f; // 倾斜姿态阈值
