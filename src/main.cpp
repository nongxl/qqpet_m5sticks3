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
#include "game_data.h"


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
            g_display.showStatusCard();
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

    // 启动问候气泡 (仅在已领养状态下)
    if (g_pet.isAdopted()) {
        g_haptics.trigger(HAPTIC_CLICK);
        g_display.showBubble("主人早上好！我是你的QQ小桌宠~", 4000);
    }
}

void loop() {
    uint32_t frameStart = millis();

    M5.update();
    g_haptics.update();
    g_net.update();
    g_webPortal.update();

    // 0. 如果尚未领养 (首次开机或重置)，进入双企鹅并排展示选性别领养仪式
    if (!g_pet.isAdopted()) {
        static uint8_t selectedAdoptGender = 0; // 0: GG 帅哥, 1: MM 妹子

        if (M5.BtnB.wasClicked()) {
            selectedAdoptGender = 1 - selectedAdoptGender;
            g_haptics.trigger(HAPTIC_CLICK);
        }

        if (M5.BtnA.wasClicked()) {
            g_pet.adopt(selectedAdoptGender);
            g_storage.savePetState(g_pet.getState());
            g_haptics.trigger(HAPTIC_SUCCESS);
            g_display.showBubble((selectedAdoptGender == 1) ? "领养成功！我是你的甜美MM~" : "领养成功！我是你的帅气GG~", 4500);
        }

        g_display.drawAdoptionScreen(selectedAdoptGender);

        int elapsed = millis() - frameStart;
        if (elapsed < 33) {
            delay(33 - elapsed);
        }
        return;
    }

    g_pet.updateAnimState();

    // 1. 处理按键 BtnA (前面板按键: 确认 / 抚摸 / 按住拖拽)
    if (M5.BtnA.wasClicked()) {

        if (g_display.isStatusCardOpen()) {
            g_display.closeStatusCard();
            g_haptics.trigger(HAPTIC_CLICK);
        } else if (g_display.isWebPortalOpen()) {
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

    // 按住拖拽：按住 BtnA 时将企鹅悬空提起，并跟随手腕倾斜实时空中扑腾
    static bool dragStarted = false;
    static float baseTiltX = 0;
    static float baseTiltY = 0;
    static float smoothOffsetX = 0;
    static float smoothOffsetY = 0;

    if (M5.BtnA.isHolding() && !g_display.isMenuOpen() && !g_display.isStatusCardOpen()) {
        if (!dragStarted) {
            dragStarted = true;
            baseTiltX = g_imu.getTiltX();
            baseTiltY = g_imu.getTiltY();
            g_haptics.trigger(HAPTIC_CLICK);
        }
        isDraggingMode = true;
        g_display.setDragging(true);

        // 相对握持姿态的倾斜偏量 (限制在 ±22px 内)，默认悬空提起 -16px
        float relX = (g_imu.getTiltX() - baseTiltX) * 28.0f;
        float relY = ((g_imu.getTiltY() - baseTiltY) * 20.0f) - 16.0f;
        relX = constrain(relX, -22.0f, 22.0f);
        relY = constrain(relY, -32.0f, -4.0f);

        smoothOffsetX += (relX - smoothOffsetX) * 0.3f;
        smoothOffsetY += (relY - smoothOffsetY) * 0.3f;
        dragOffsetX = static_cast<int>(smoothOffsetX);
        dragOffsetY = static_cast<int>(smoothOffsetY);
    } else if (dragStarted || (isDraggingMode && !M5.BtnA.isPressed())) {
        dragStarted = false;
        isDraggingMode = false;
        g_display.setDragging(false);
        smoothOffsetX = 0;
        smoothOffsetY = 0;
        dragOffsetX = 0;
        dragOffsetY = 0;
    }

    // 2. 处理按键 BtnB (侧边按键: 呼出菜单 / 轮换切换 / 关闭状态卡片)
    if (M5.BtnB.wasClicked()) {
        if (g_display.isStatusCardOpen()) {
            g_display.closeStatusCard();
            g_haptics.trigger(HAPTIC_CLICK);
        } else {
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
        }
    } else if (M5.BtnB.wasHold()) {
        if (g_display.isStatusCardOpen()) {
            g_display.closeStatusCard();
            g_haptics.trigger(HAPTIC_CLICK);
        } else if (g_display.isMenuOpen()) {
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
                g_display.showBubble(getRandomClassicQuote("sick"), 4000);
            } else if (g_pet.isHungry() || g_pet.isDirty()) {
                g_display.showBubble(getRandomClassicQuote("idle"), 4000);
            } else {
                g_display.showBubble(getRandomClassicQuote("happy"), 4000);
            }
        }
    }


    // 6. 定时自动壁纸漫游换景 (每 10~15 分钟企鹅自动探索新场景，完全脱离手机)
    static uint32_t lastAutoBgTime = millis();
    if (now - lastAutoBgTime > (10 * 60 * 1000 + random(0, 5 * 60 * 1000))) {
        lastAutoBgTime = now;
        if (!g_display.isMenuOpen() && !g_pet.isDead()) {
            static const char* SCENE_NAMES[] = {
                "经典桌面", "阳光草地", "森林小道", "浪漫海滩", "夜幕星空",
                "企鹅客厅", "梦幻冰屋", "落叶枫林", "童话乐园", "蔚蓝深海",
                "飞舞樱花", "魔法城堡", "农场庄园", "太空星云", "暖冬雪景",
                "新春庭阁", "都市天际"
            };
            uint8_t nextBg = static_cast<uint8_t>(random(1, 17));
            if (nextBg == g_pet.getState().bg_id) nextBg = (nextBg % 16) + 1;
            g_pet.getState().bg_id = nextBg;
            g_storage.savePetState(g_pet.getState());
            const char* sName = (nextBg <= 16) ? SCENE_NAMES[nextBg] : "美丽风景";
            g_display.showBubble(String("企鹅散步来到了【") + sName + "】~", 4000);
        }
    }

    // 7. 定时持久化存档 (每 60 秒自动写入 Flash)
    if (now - lastSaveTime >= 60000) {
        g_storage.savePetState(g_pet.getState());
        lastSaveTime = now;
    }

    // 8. 屏幕渲染输出 (将企鹅、壁纸、气泡与轮盘推送到屏幕)
    g_display.update(dragOffsetX, dragOffsetY);

    // 9. 物理帧率节拍控制 (锁定 30 FPS，丝滑无抖动)
    int elapsed = millis() - frameStart;
    if (elapsed < 33) {
        delay(33 - elapsed);
    }
}

