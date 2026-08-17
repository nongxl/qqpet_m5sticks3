#include <Arduino.h>
#include <M5Unified.h>
#include "config.h"
#include "pet_core.h"
#include "storage_manager.h"
#include "display_engine.h"
#include "haptics.h"
#include "imu_sensor.h"
#include "network_manager.h"
#include "web_server_portal.h"
#include "ai_client.h"

// 计时变量
static uint32_t lastDecayTime = 0;
static uint32_t lastSaveTime = 0;
static uint32_t lastIdleQuoteTime = 0;

// 拖拽模式
static bool isDraggingMode = false;
static int dragOffsetX = 0;
static int dragOffsetY = 0;

void executeMenuAction(MenuOption opt) {
    String msg;
    switch (opt) {
        case MENU_FEED:
            if (g_pet.feed(1000)) {
                g_haptics.trigger(HAPTIC_SUCCESS);
                g_display.showToast("喂食 +1000 饥饿", 2000);
                g_ai.requestDialog("hungry");
            } else {
                g_haptics.trigger(HAPTIC_ALERT);
                g_display.showToast("食物不足/无法喂食", 2000);
            }
            break;

        case MENU_BATH:
            if (g_pet.bath(1000)) {
                g_haptics.trigger(HAPTIC_SUCCESS);
                g_display.showToast("洗香香 +1000 清洁", 2000);
                g_ai.requestDialog("dirty");
            } else {
                g_haptics.trigger(HAPTIC_ALERT);
                g_display.showToast("香皂不足/无法洗澡", 2000);
            }
            break;

        case MENU_PLAY:
            if (g_pet.play(150)) {
                g_haptics.trigger(HAPTIC_SUCCESS);
                g_display.showToast("逗玩开怀大笑 +150 心情", 2000);
                g_ai.requestDialog("happy");
            }
            break;

        case MENU_WORK:
            {
                String workMsg;
                if (g_pet.work(workMsg)) {
                    g_haptics.trigger(HAPTIC_SUCCESS);
                    g_display.showToast(workMsg, 3000);
                    g_ai.requestDialog("idle");
                } else {
                    g_haptics.trigger(HAPTIC_ALERT);
                    g_display.showToast(workMsg, 3000);
                }
            }
            break;

        case MENU_STUDY:
            {
                String studyMsg;
                if (g_pet.study(studyMsg)) {
                    g_haptics.trigger(HAPTIC_SUCCESS);
                    g_display.showToast(studyMsg, 3000);
                    g_ai.requestDialog("idle");
                } else {
                    g_haptics.trigger(HAPTIC_ALERT);
                    g_display.showToast(studyMsg, 3000);
                }
            }
            break;

        case MENU_TRIP:
            {
                String tripMsg;
                if (g_pet.trip(tripMsg)) {
                    g_haptics.trigger(HAPTIC_SUCCESS);
                    g_display.showToast(tripMsg, 3000);
                    g_ai.requestDialog("happy");
                } else {
                    g_haptics.trigger(HAPTIC_ALERT);
                    g_display.showToast(tripMsg, 3000);
                }
            }
            break;



        case MENU_CURE:
            if (g_pet.autoHeal(msg)) {
                g_haptics.trigger(HAPTIC_SUCCESS);
                g_display.showToast(msg, 3000);
                g_ai.requestDialog("idle");
            } else {
                g_haptics.trigger(HAPTIC_ALERT);
                g_display.showToast(msg, 3000);
            }
            break;

        case MENU_STATUS:
            g_display.showStatusCard(5000);
            g_haptics.trigger(HAPTIC_CLICK);
            break;


        case MENU_WEB_CONFIG:
            g_net.startAP(); // 确保 AP 热点 100% 开启广播
            g_display.showWebPortalCard(20000);
            g_haptics.trigger(HAPTIC_SUCCESS);
            break;

        default:
            break;
    }
}

#include "asset_manager.h"

void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);
    
    // 初始化文件系统与资源引擎
    g_assets.begin();

    // 初始化外设与模块
    g_haptics.begin();
    g_display.begin();
    g_imu.begin();


    // 加载持久化数据
    if (g_storage.begin()) {
        PetState savedState;
        if (g_storage.loadPetState(savedState)) {
            g_pet.getState() = savedState;
        }
    }

    // 初始化网络与后台
    g_net.begin();
    g_webPortal.begin();
    g_ai.begin();

    lastDecayTime = millis();
    lastSaveTime = millis();
    lastIdleQuoteTime = millis();

    // 预热加载当前阶段核心动作到 8MB PSRAM (彻底消除动画切换时的卡顿)
    g_assets.preloadCoreActions(g_pet.getState().gender, g_pet.getLevel());

    // 启动问候气泡
    g_haptics.trigger(HAPTIC_CLICK);
    g_display.showBubble("主人早上好！我是你的QQ小桌宠~", 4000);
}

void loop() {
    uint32_t frameStart = millis();

    M5.update();
    g_haptics.update();
    g_pet.updateAnimState();
    g_net.update();
    g_webPortal.update();

    // 1. 处理按键 BtnA (前面板按键: 确认 / 抚摸 / 按住拖拽)
    if (M5.BtnA.wasClicked()) {
        if (g_display.isWebPortalOpen()) {
            g_display.closeWebPortalCard();
            g_haptics.trigger(HAPTIC_CLICK);
        } else if (g_display.isMenuOpen()) {
            // 菜单模式下: 确认选中项
            executeMenuAction(g_display.getSelectedMenuOption());
            g_display.closeMenu();
        } else {
            // 待机模式下: 摸摸互动
            g_pet.play(50);
            g_haptics.trigger(HAPTIC_CLICK);
            g_ai.requestDialog("happy");
        }
    }


    // 按住拖拽：按住 BtnA 时进入拖拽模式，松开按键立即平滑复位
    if (M5.BtnA.isHolding() && !g_display.isMenuOpen()) {
        isDraggingMode = true;
        g_display.setDragging(true);
        dragOffsetX = static_cast<int>(g_imu.getTiltX() * 24.0f);
        dragOffsetY = static_cast<int>(g_imu.getTiltY() * 18.0f);
    } else if (isDraggingMode && !M5.BtnA.isPressed()) {
        isDraggingMode = false;
        g_display.setDragging(false);
        dragOffsetX = 0;
        dragOffsetY = 0;
    }

    // 2. 处理按键 BtnB (侧边按键: 呼出菜单 / 轮换切换 / 关闭)
    if (M5.BtnB.wasClicked()) {
        isDraggingMode = false;
        g_display.setDragging(false);
        dragOffsetX = 0;
        dragOffsetY = 0;

        if (!g_display.isMenuOpen()) {
            g_display.toggleMenu();
        } else {
            g_display.nextMenuOption();
        }
        g_haptics.trigger(HAPTIC_CLICK);
    } else if (M5.BtnB.wasHold()) {
        if (g_display.isMenuOpen()) {
            g_display.closeMenu();
            g_haptics.trigger(HAPTIC_CLICK);
        }
    }

    // 3. 处理 IMU 姿态与体感 (菜单打开时不响应晃动)
    if (!g_display.isMenuOpen()) {
        ImuEventType imuEvt = g_imu.update();
        if (imuEvt == IMU_EVENT_SHAKE) {
            g_pet.play(80);
            g_haptics.trigger(HAPTIC_SUCCESS);
            g_ai.requestDialog("happy");
        }
    }

    // 4. 定时状态衰减 (每 30 秒递减一次属性)
    uint32_t now = millis();
    if (now - lastDecayTime >= 30000) {
        uint32_t deltaSec = (now - lastDecayTime) / 1000;
        g_pet.tickDecay(deltaSec);
        lastDecayTime = now;
    }

    // 5. 待机状态下自主自言自语 (每 35~55 秒随机触发)
    if (now - lastIdleQuoteTime > (35000 + random(0, 20000))) {
        lastIdleQuoteTime = now;
        if (!g_display.isMenuOpen() && !g_pet.isDead()) {
            if (g_pet.isSick()) {
                g_ai.requestDialog("sick");
            } else if (g_pet.isHungry()) {
                g_ai.requestDialog("hungry");
            } else if (g_pet.isDirty()) {
                g_ai.requestDialog("dirty");
            } else {
                g_ai.requestDialog("idle");
            }
        }
    }

    // 6. 状态自动保存 (每 60 秒持久化到 Flash)
    if (now - lastSaveTime >= 60000) {
        g_storage.savePetState(g_pet.getState());
        lastSaveTime = now;
    }

    // 7. 渲染输出
    g_display.update(dragOffsetX, dragOffsetY);

    // 精准锁定 30 FPS 物理帧率节拍
    uint32_t elapsed = millis() - frameStart;
    if (elapsed < 33) {
        delay(33 - elapsed);
    }
}

