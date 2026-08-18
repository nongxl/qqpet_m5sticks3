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
#include "sound_manager.h"
#include "game_data.h"
#include "mini_game_manager.h"


// 计时变量
static uint32_t lastDecayTime = 0;
static uint32_t lastSaveTime = 0;
static uint32_t lastIdleQuoteTime = 0;
static uint32_t lastUserInteractTime = 0; // 用户最后一次操作时间


// 拖拽模式
static bool isDraggingMode = false;
static int dragOffsetX = 0;
static int dragOffsetY = 0;

void executeMenuAction(MenuOption opt) {
    String msg;
    switch (opt) {
        case MENU_FEED:
            // 进入全屏食物选择背包
            g_display.openSubScreen(SUB_SCREEN_FEED);
            g_haptics.trigger(HAPTIC_CLICK);
            break;

        case MENU_BATH:
            if (g_pet.bath(1000)) {
                g_haptics.trigger(HAPTIC_SUCCESS);
                g_display.showToast("洗香香 +1000 清洁", 2000);
                g_ai.requestDialog("dirty");
            } else {
                g_haptics.trigger(HAPTIC_ALERT);
                g_display.showToast("香皂不足！请前往商城购买。", 2000);
            }
            break;

        case MENU_PLAY:
            // 进入 5 大经典体感游戏大厅
            g_display.openSubScreen(SUB_SCREEN_GAMES);
            g_haptics.trigger(HAPTIC_CLICK);
            break;

        case MENU_WARDROBE:
            // 👗 进入全屏企鹅换装衣橱
            g_display.openSubScreen(SUB_SCREEN_WARDROBE);
            g_haptics.trigger(HAPTIC_CLICK);
            break;



        case MENU_WORK:
            if (g_pet.isTaskActive() && g_pet.getCurrentTask() == TASK_WORK) {
                String stopMsg;
                g_pet.stopTask(stopMsg, false);
                g_haptics.trigger(HAPTIC_SUCCESS);
                g_display.showToast(stopMsg, 5500);
            } else {
                g_display.openSubScreen(SUB_SCREEN_WORK);
                g_haptics.trigger(HAPTIC_CLICK);
            }
            break;

        case MENU_STUDY:
            if (g_pet.isTaskActive() && g_pet.getCurrentTask() == TASK_STUDY) {
                String stopMsg;
                g_pet.stopTask(stopMsg, false);
                g_haptics.trigger(HAPTIC_SUCCESS);
                g_display.showToast(stopMsg, 5500);
            } else {
                g_display.openSubScreen(SUB_SCREEN_STUDY);
                g_haptics.trigger(HAPTIC_CLICK);
            }
            break;

        case MENU_TRIP:
            if (g_pet.isTaskActive() && g_pet.getCurrentTask() == TASK_TRIP) {
                String stopMsg;
                g_pet.stopTask(stopMsg, false);
                g_haptics.trigger(HAPTIC_SUCCESS);
                g_display.showToast(stopMsg, 5500);
            } else {
                g_display.openSubScreen(SUB_SCREEN_TRIP);
                g_haptics.trigger(HAPTIC_CLICK);
            }
            break;

        case MENU_CURE:
            // 进入全屏对症药箱
            g_display.openSubScreen(SUB_SCREEN_CURE);
            g_haptics.trigger(HAPTIC_CLICK);
            break;

        case MENU_SHOP:
            // 进入全屏元宝道具商城
            g_display.openSubScreen(SUB_SCREEN_SHOP);
            g_haptics.trigger(HAPTIC_CLICK);
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

    // 初始化声音、网络与后台
    g_sound.begin();
    g_net.begin();
    g_webPortal.begin();
    g_ai.begin();

    lastDecayTime = millis();
    lastSaveTime = millis();
    lastIdleQuoteTime = millis();
    lastUserInteractTime = millis();

    // 启动问候气泡 (仅在已领养状态下)
    if (g_pet.isAdopted()) {
        g_sound.playSound(SOUND_HAPPY);
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
            g_display.showBubble((selectedAdoptGender == 1) ? "啪嗒！破壳诞生啦~ 我是您的甜美MM！" : "啪嗒！破壳诞生啦~ 我是您的帅气GG！", 5000);
        }


        g_display.drawAdoptionScreen(selectedAdoptGender);

        int elapsed = millis() - frameStart;
        if (elapsed < 33) {
            delay(33 - elapsed);
        }
        return;
    }

    uint32_t nowTime = millis();
    bool isSleeping = (nowTime - lastUserInteractTime > 90000) && !g_pet.isTaskActive() && 
                      !g_display.isMenuOpen() && !g_display.isSubScreenOpen() && !g_pet.isDead();

    if (isSleeping) {
        // 进入睡梦打呼噜状态
        g_pet.triggerTransientAnim(ANIM_SLEEP, 3000);
        static uint32_t lastSnoreSoundTime = 0;
        if (nowTime - lastSnoreSoundTime > 9000) {
            lastSnoreSoundTime = nowTime;
            g_sound.playSound(SOUND_SNORE);
        }
    }

    g_pet.updateAnimState();

    // 1. 处理按键 BtnA (前面板按键: 确认 / 抚摸 / 按住拖拽 / 游戏主键)
    if (M5.BtnA.wasClicked()) {
        lastUserInteractTime = nowTime; // 唤醒与重置待机
        if (g_display.isSubScreenOpen()) {

            SubScreenMode mode = g_display.getSubScreenMode();
            if (mode == SUB_SCREEN_GAMES) {
                MiniGameManager::getInstance().onBtnA();
                return;
            }

            int idx = g_display.getSubScreenIndex();
            int totalItems = (mode == SUB_SCREEN_FEED) ? (FOOD_COUNT + 1) : 
                             ((mode == SUB_SCREEN_CURE) ? (MEDICINE_COUNT + 1) : 
                             ((mode == SUB_SCREEN_SHOP) ? (SHOP_PRODUCT_COUNT + 1) : 
                             ((mode == SUB_SCREEN_WARDROBE) ? (COSTUME_COUNT + 1) : (4 + 1))));

            static const int TASK_DURATIONS[4] = {300, 900, 1800, 3600};

            // 如果点击了最后一项 [返回桌面]
            if (idx == totalItems - 1) {
                g_display.closeSubScreen();
                g_haptics.trigger(HAPTIC_CLICK);
            } else if (mode == SUB_SCREEN_FEED) {
                String feedMsg;
                if (g_pet.feedFood(idx, feedMsg)) {
                    g_haptics.trigger(HAPTIC_SUCCESS);
                    g_display.showToast(feedMsg, 5500);
                    g_display.closeSubScreen();
                    g_ai.requestDialog("hungry");
                } else {
                    g_haptics.trigger(HAPTIC_ALERT);
                    g_display.showToast(feedMsg, 5500);
                }
            } else if (mode == SUB_SCREEN_CURE) {
                String cureMsg;
                if (g_pet.cureWithMed(idx, cureMsg)) {
                    g_haptics.trigger(HAPTIC_SUCCESS);
                    g_display.showToast(cureMsg, 5500);
                    g_display.closeSubScreen();
                    g_ai.requestDialog("idle");
                } else {
                    g_haptics.trigger(HAPTIC_ALERT);
                    g_display.showToast(cureMsg, 5500);
                }
            } else if (mode == SUB_SCREEN_SHOP) {
                if (idx == 0) {
                    // 点击了第一项【👗 企鹅衣橱】，无缝进入全屏衣橱
                    g_display.openSubScreen(SUB_SCREEN_WARDROBE);
                    g_haptics.trigger(HAPTIC_CLICK);
                } else {
                    String buyMsg;
                    if (g_pet.buyShopProduct(idx, 1, buyMsg)) {
                        g_haptics.trigger(HAPTIC_SUCCESS);
                        g_display.showToast(buyMsg, 5500);
                    } else {
                        g_haptics.trigger(HAPTIC_ALERT);
                        g_display.showToast(buyMsg, 5500);
                    }
                }
            } else if (mode == SUB_SCREEN_WORK || mode == SUB_SCREEN_STUDY || mode == SUB_SCREEN_TRIP) {

                PetTaskType targetTask = (mode == SUB_SCREEN_WORK) ? TASK_WORK : ((mode == SUB_SCREEN_STUDY) ? TASK_STUDY : TASK_TRIP);
                String taskMsg;
                if (g_pet.startTask(targetTask, TASK_DURATIONS[idx], taskMsg)) {
                    g_haptics.trigger(HAPTIC_SUCCESS);
                    g_display.showToast(taskMsg, 5500);
                    g_display.closeSubScreen();
                } else {
                    g_haptics.trigger(HAPTIC_ALERT);
                    g_display.showToast(taskMsg, 5500);
                }
            } else if (mode == SUB_SCREEN_WARDROBE) {
                int costumeId = idx + 1;
                String cosMsg;
                if (!g_pet.ownsCostume(costumeId)) {
                    if (g_pet.buyCostume(costumeId, cosMsg)) {
                        g_haptics.trigger(HAPTIC_SUCCESS);
                        g_display.showToast(cosMsg, 4000);
                        String eqMsg;
                        g_pet.toggleEquipCostume(costumeId, eqMsg);
                    } else {
                        g_haptics.trigger(HAPTIC_ALERT);
                        g_display.showToast(cosMsg, 4000);
                    }
                } else {
                    if (g_pet.toggleEquipCostume(costumeId, cosMsg)) {
                        g_haptics.trigger(HAPTIC_CLICK);
                        g_display.showToast(cosMsg, 3000);
                    }
                }
            }

        } else if (g_display.isToastVisible()) {
            g_display.closeToast();
            g_haptics.trigger(HAPTIC_CLICK);
        } else if (g_display.isStatusCardOpen()) {
            g_display.closeStatusCard();
            g_haptics.trigger(HAPTIC_CLICK);
        } else if (g_display.isWebPortalOpen()) {
            g_display.closeWebPortalCard();
            g_haptics.trigger(HAPTIC_CLICK);
        } else if (g_display.isMenuOpen()) {
            // 菜单模式下: 确认选中项并执行
            executeMenuAction(g_display.getSelectedMenuOption());
            g_display.closeMenu();
        } else if (isSleeping) {
            // 睡眠状态下按键: 温柔唤醒小企鹅
            g_sound.playSound(SOUND_HAPPY);
            g_haptics.trigger(HAPTIC_CLICK);
            g_display.showBubble("唔... 主人把我叫醒啦~ 伸个懒腰！", 4000);
        } else {
            // 待机模式下: 如果正在作业，按 A 键召回结算；否则摸摸互动
            if (g_pet.isTaskActive()) {
                String stopMsg;
                g_pet.stopTask(stopMsg, false);
                g_haptics.trigger(HAPTIC_SUCCESS);
                g_display.showToast(stopMsg, 5500);
            } else {
                g_pet.play(50);
                g_haptics.trigger(HAPTIC_CLICK);
                g_ai.requestDialog("happy");
            }
        }
    }

    // 按住拖拽：按住 BtnA 时将企鹅悬空提起，并跟随手腕倾斜实时空中扑腾
    static bool dragStarted = false;
    static float baseTiltX = 0;
    static float baseTiltY = 0;
    static float smoothOffsetX = 0;
    static float smoothOffsetY = 0;

    if (M5.BtnA.isHolding() && !g_display.isMenuOpen() && !g_display.isStatusCardOpen() && !g_display.isSubScreenOpen() && !isSleeping) {
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

    // 2. 处理按键 BtnB (侧边按键: 呼出/顺时针轮换菜单 / 关闭状态卡片 / 子界面滚动)
    if (M5.BtnB.wasClicked()) {
        lastUserInteractTime = millis();
        if (g_display.isToastVisible()) {
            g_display.closeToast();
            g_haptics.trigger(HAPTIC_CLICK);
        } else if (g_display.isSubScreenOpen()) {
            if (g_display.getSubScreenMode() == SUB_SCREEN_GAMES) {
                MiniGameManager::getInstance().onBtnB();
            } else {
                g_display.nextSubScreenItem();
                g_haptics.trigger(HAPTIC_CLICK);
            }
        } else if (g_display.isStatusCardOpen()) {
            g_display.closeStatusCard();
            g_haptics.trigger(HAPTIC_CLICK);
        } else if (g_display.isMenuOpen()) {
            // 菜单已打开状态下: 短按 BtnB 顺时针选择下一项 (清脆微触感)
            g_display.nextMenuOption();
            g_haptics.trigger(HAPTIC_CLICK);
        } else {
            // 菜单未打开: 短按 BtnB 呼出环绕圆圈菜单
            isDraggingMode = false;
            g_display.setDragging(false);
            dragOffsetX = 0;
            dragOffsetY = 0;
            g_display.openMenu();
            g_haptics.trigger(HAPTIC_CLICK);
        }
    } else if (M5.BtnB.wasHold()) {
        lastUserInteractTime = millis();
        if (g_display.isSubScreenOpen()) {
            if (g_display.getSubScreenMode() == SUB_SCREEN_GAMES) {
                MiniGameManager::getInstance().stopGame();
            }
            g_display.closeSubScreen();
            g_haptics.trigger(HAPTIC_CLICK);
        } else if (g_display.isStatusCardOpen()) {
            g_display.closeStatusCard();
            g_haptics.trigger(HAPTIC_CLICK);
        } else if (g_display.isMenuOpen()) {
            // 菜单模式下长按 BtnB: 关闭菜单
            g_display.closeMenu();
            g_haptics.trigger(HAPTIC_CLICK);
        }
    }

    // 3. 处理 IMU 姿态与体感更新 (小游戏驱动 / 待机摇一摇)
    if (g_display.getSubScreenMode() == SUB_SCREEN_GAMES) {
        g_imu.update();
        MiniGameManager::getInstance().update(g_imu.getTiltX(), g_imu.getTiltY(), g_imu.getAccelZ());
    } else if (g_display.isMenuOpen()) {
        // 菜单模式已改回纯按键选择，不再调用 IMU 倾斜扰乱选项
        g_imu.update();
    } else {
        ImuEventType imuEvt = g_imu.update();
        if (imuEvt == IMU_EVENT_SHAKE) {
            lastUserInteractTime = millis();
            if (isSleeping) {
                g_sound.playSound(SOUND_HAPPY);
                g_haptics.trigger(HAPTIC_SUCCESS);
                g_display.showBubble("哇！被晃醒啦~ 揉揉眼睛！", 4000);
            } else if (!g_pet.isTaskActive()) {
                g_pet.play(80);
                g_haptics.trigger(HAPTIC_SUCCESS);
                g_ai.requestDialog("happy");
            }
        }
    }

    // 4. 定时状态衰减 (每 30 秒递减一次属性)
    uint32_t now = millis();
    if (now - lastDecayTime >= 30000) {
        uint32_t deltaSec = (now - lastDecayTime) / 1000;
        g_pet.tickDecay(deltaSec);
        lastDecayTime = now;
    }

    // 5. 待机状态下自主自言自语 (每 35~55 秒随机触发，严格对齐当前场景)
    if (now - lastIdleQuoteTime > (35000 + random(0, 20000))) {
        lastIdleQuoteTime = now;
        if (!g_display.isMenuOpen() && !g_display.isSubScreenOpen() && !g_pet.isDead()) {
            if (isSleeping) {
                // 睡梦打呼噜中：只显示可爱的梦呓
                static const char* SLEEP_QUOTES[] = {
                    "呼噜噜... (梦到大龙虾了)",
                    "zzZ... 呼呼...",
                    "梦里考了100分... 嘿嘿",
                    "唔... 别抢我的小鱼干... zzZ",
                    "呼... 呼... 睡得好香~"
                };
                g_display.showBubble(SLEEP_QUOTES[random(0, 5)], 3500);
            } else if (g_pet.isTaskActive()) {
                // 作业中根据具体任务情境发言
                if (g_pet.getState().current_task == TASK_WORK) {
                    g_display.showBubble("搬砖加油中！为了赚元宝~", 3500);
                } else if (g_pet.getState().current_task == TASK_STUDY) {
                    g_display.showBubble("书中自有黄金屋~ 正在专心自习！", 3500);
                } else if (g_pet.getState().current_task == TASK_TRIP) {
                    g_display.showBubble("神州大地的风景真美呀~", 3500);
                }
            } else if (g_pet.isSick()) {
                g_display.showBubble(getRandomClassicQuote("sick"), 4000);
            } else if (g_pet.isHungry() || g_pet.isDirty()) {
                g_display.showBubble(getRandomClassicQuote("idle"), 4000);
            } else {
                g_display.showBubble(getRandomClassicQuote("happy"), 4000);
            }
        }
    }

    // 6. 定时自动壁纸漫游换景 (每 10~15 分钟企鹅自动探索新场景，仅在清醒待机时漫游)
    static uint32_t lastAutoBgTime = millis();
    if (now - lastAutoBgTime > (10 * 60 * 1000 + random(0, 5 * 60 * 1000))) {
        lastAutoBgTime = now;
        if (!g_display.isMenuOpen() && !g_display.isSubScreenOpen() && !g_pet.isDead() && !isSleeping && !g_pet.isTaskActive()) {
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

