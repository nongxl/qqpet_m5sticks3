#pragma once
#include <Arduino.h>
#include <M5GFX.h>
#include "pet_core.h"

enum MenuOption {
    MENU_FEED = 0,
    MENU_BATH,
    MENU_PLAY,
    MENU_WORK,
    MENU_STUDY,
    MENU_TRIP,
    MENU_CURE,
    MENU_SHOP,        // 🛍️ 元宝道具商城
    MENU_STATUS,
    MENU_WEB_CONFIG,
    MENU_COUNT
};

enum SubScreenMode {
    SUB_SCREEN_NONE = 0,
    SUB_SCREEN_FEED,   // 全屏食物选择背包
    SUB_SCREEN_CURE,   // 全屏对症药品药箱
    SUB_SCREEN_SHOP,   // 全屏元宝道具商城
    SUB_SCREEN_WORK,   // 打工时长选择
    SUB_SCREEN_STUDY,  // 学习时长选择
    SUB_SCREEN_TRIP    // 旅游时长选择
};


class DisplayEngine {
public:
    DisplayEngine();
    void begin();
    void update(int petOffsetX = 0, int petOffsetY = 0);
    void drawAdoptionScreen(uint8_t selectedGender);

    // 全屏子界面控制 (食物选择、看病药箱、元宝商城)
    void openSubScreen(SubScreenMode mode);
    void closeSubScreen();
    bool isSubScreenOpen() const { return subMode != SUB_SCREEN_NONE; }
    SubScreenMode getSubScreenMode() const { return subMode; }
    void nextSubScreenItem();
    void prevSubScreenItem();
    int getSubScreenIndex() const { return subIndex; }
    void renderSubScreen();

    // 气泡对话控制
    void showBubble(const String& text, uint32_t durationMs = 4500);
    
    // 菜单控制
    void toggleMenu();
    void nextMenuOption();
    void prevMenuOption();
    void closeMenu();
    bool isMenuOpen() const { return menuVisible; }
    MenuOption getSelectedMenuOption() const { return currentOption; }

    // 拖拽悬空状态
    void setDragging(bool dragging) { isDragging = dragging; }

    // 弹窗与状态卡片控制
    void showToast(const String& msg, uint32_t durationMs = 5500);
    void closeToast() { toastEndTime = 0; }
    bool isToastVisible() const { return millis() <= toastEndTime && toastText.length() > 0; }
    void showStatusCard();
    void closeStatusCard() { statusCardVisible = false; }
    bool isStatusCardOpen() const { return statusCardVisible; }
    void showWebPortalCard(uint32_t durationMs = 20000);
    void closeWebPortalCard() { webPortalCardEndTime = 0; }
    bool isWebPortalOpen() const { return millis() <= webPortalCardEndTime; }


private:
    M5Canvas canvas;
    bool menuVisible;
    float menuSlideProgress; // 0.0 ~ 1.0 平滑滑入滑出动画
    float wheelCurrentAngle; // 当前平滑旋转角度
    float wheelTargetAngle;  // 目标旋转角度
    uint32_t menuLastActiveTime;
    MenuOption currentOption;

    // 全屏子界面状态
    SubScreenMode subMode;
    int subIndex;


    // 气泡对话状态
    String bubbleText;
    uint32_t bubbleEndTime;
    int typewriterIndex;
    uint32_t lastTypewriterTime;

    // 提示 Toast、状态卡片与后台面板
    String toastText;
    uint32_t toastEndTime;
    bool statusCardVisible;
    uint32_t webPortalCardEndTime;


    // 电池缓存 (防止频繁 I2C 读取导致背光闪烁)
    int cachedBattery;
    uint32_t lastBatteryCheckTime;

    // 动画帧计数
    uint32_t animFrame;
    bool isDragging;

    // 绘制子函数
    void drawBackground();
    void drawTopBar();
    void drawPet(int centerX, int centerY, PetAnimState anim);
    void drawBubble();
    void drawRightBubbleMenu();
    void drawToast();
};

extern DisplayEngine g_display;
