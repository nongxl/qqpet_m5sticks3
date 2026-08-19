#include "game_floor.h"
#include "pet_core.h"
#include "haptics.h"
#include "sound_manager.h"

extern PetCore g_pet;
extern HapticsEngine g_haptics;

GameFloor::GameFloor() {
    init();
}

void GameFloor::init() {
    petX = 67.0f;
    petY = 60.0f;
    vy = 0.0f;
    currentFloor = 1;
    maxFloor = 1;
    lives = 3;
    finished = false;
    resultMsg = "";
    gameOverTime = 0;

    platforms.clear();
    // 初始生成多层云梯
    for (int i = 0; i < 6; ++i) {
        CloudPlatform cp;
        cp.x = random(10, 80);
        cp.y = 80 + i * 32;
        cp.w = 46.0f;
        cp.type = (i == 0) ? 0 : random(0, 3);
        platforms.push_back(cp);
    }
}

void GameFloor::update(float tiltX, float tiltY, float accelZ) {
    if (finished) return;

    if (lives <= 0 || petY < 20 || currentFloor >= 100) {
        if (gameOverTime == 0) {
            gameOverTime = millis();
            int coinReward = maxFloor;
            if (coinReward > 100) coinReward = 100;
            if (coinReward < 10) coinReward = 10;
            int moodGain = 60 + maxFloor;
            if (moodGain > 250) moodGain = 250;

            PetState& st = const_cast<PetState&>(g_pet.getState());
            st.coins += coinReward;
            st.mood = std::min(1000, st.mood + moodGain);
            g_pet.addGrowth(25.0f);


            if (currentFloor >= 100) {
                resultMsg = "🏆 登峰造极！突破100层！+" + String(coinReward) + "元宝";
            } else {
                resultMsg = "☁️ 挑战至 " + String(maxFloor) + " 层！+" + String(coinReward) + "元宝";
            }
        }
        if (millis() - gameOverTime > 2000) {
            finished = true;
        }
        return;
    }

    // 1. 重力感应左右倾斜控制企鹅横向移动
    petX += tiltX * 4.5f;
    if (petX < 8) petX = 8;
    if (petX > 115) petX = 115;

    // 2. 企鹅下落重力物理
    vy += 0.18f;
    petY += vy;

    // 3. 画面向上匀速推进 (楼层不断加深)
    float scrollSpeed = 0.8f + (currentFloor * 0.01f);
    petY -= scrollSpeed;

    for (auto& cp : platforms) {
        cp.y -= scrollSpeed;
    }

    // 4. 碰撞检测 (企鹅下落踏上云梯)
    for (auto& cp : platforms) {
        if (vy > 0 && petY + 12 >= cp.y && petY + 8 <= cp.y + 6 &&
            petX + 12 >= cp.x && petX - 12 <= cp.x + cp.w) {
            
            if (cp.type == 2) {
                // 尖刺云：扣血并反弹
                lives--;
                vy = -3.2f;
                g_haptics.trigger(HAPTIC_CLICK);
                SoundManager::getInstance().playSound(SOUND_SICK);
            } else if (cp.type == 1) {
                // 弹簧云：强力起跳
                vy = -5.5f;
                g_haptics.trigger(HAPTIC_SUCCESS);
                SoundManager::getInstance().playSound(SOUND_HAPPY);
            } else {
                // 普通云
                vy = 0.0f;
                petY = cp.y - 12;
                g_haptics.trigger(HAPTIC_CLICK);
            }
            break;
        }
    }

    // 5. 移出顶部的云梯回收并在底部生成新云梯
    if (!platforms.empty() && platforms.front().y < 20) {
        platforms.erase(platforms.begin());
        currentFloor++;
        if (currentFloor > maxFloor) maxFloor = currentFloor;

        CloudPlatform newCp;
        newCp.x = random(8, 82);
        newCp.y = platforms.back().y + 32;
        newCp.w = 44.0f;
        int r = random(0, 100);
        if (r < 60) newCp.type = 0;       // 普通云
        else if (r < 80) newCp.type = 1;  // 弹簧云
        else newCp.type = 2;              // 尖刺云
        platforms.push_back(newCp);
    }

    // 企鹅坠落到底部
    if (petY > 230) {
        lives--;
        petY = 60;
        vy = -2.0f;
        g_haptics.trigger(HAPTIC_CLICK);
    }
}

void GameFloor::render(M5Canvas& canvas) {
    int SCREEN_W = 135;
    int SCREEN_H = 240;

    // 天空渐变背景 (随层数加深渐变)
    uint16_t skyCol = (currentFloor < 30) ? canvas.color565(120, 190, 255) :
                      ((currentFloor < 60) ? canvas.color565(80, 130, 220) : canvas.color565(30, 40, 90));
    canvas.fillScreen(skyCol);

    // 1. 顶部状态栏
    canvas.fillRoundRect(4, 4, SCREEN_W - 8, 24, 4, canvas.color565(20, 30, 50));
    canvas.setFont(&fonts::efontCN_12);
    canvas.setTextSize(1);
    canvas.setTextColor(canvas.color565(255, 220, 50));
    canvas.drawString("第 " + String(currentFloor) + " 层", 8, 8);

    String lifeStr = "";
    for (int i = 0; i < lives; ++i) lifeStr += "♥ ";
    canvas.setTextColor(canvas.color565(255, 90, 100));
    canvas.drawRightString(lifeStr, SCREEN_W - 8, 8);

    // 2. 绘制云朵平台
    for (const auto& cp : platforms) {
        int cx = (int)cp.x;
        int cy = (int)cp.y;
        if (cy < 15 || cy > SCREEN_H) continue;

        if (cp.type == 0) {
            // 白色普通软云
            canvas.fillRoundRect(cx, cy, (int)cp.w, 8, 4, TFT_WHITE);
            canvas.drawRoundRect(cx, cy, (int)cp.w, 8, 4, canvas.color565(180, 210, 240));
        } else if (cp.type == 1) {
            // 绿色弹簧云
            canvas.fillRoundRect(cx, cy, (int)cp.w, 8, 4, canvas.color565(100, 240, 140));
            canvas.drawRoundRect(cx, cy, (int)cp.w, 8, 4, canvas.color565(20, 160, 60));
            canvas.drawCenterString("▲", cx + (int)cp.w / 2, cy - 2);
        } else {
            // 红色尖刺云
            canvas.fillRoundRect(cx, cy, (int)cp.w, 8, 4, canvas.color565(255, 100, 100));
            canvas.drawRoundRect(cx, cy, (int)cp.w, 8, 4, canvas.color565(180, 20, 20));
            for (int k = cx + 4; k < cx + (int)cp.w - 4; k += 6) {
                canvas.drawTriangle(k, cy, k + 4, cy, k + 2, cy - 4, canvas.color565(255, 230, 80));
            }
        }
    }

    // 3. 绘制企鹅小萌身 (带小翅膀与红围巾)
    int px = (int)petX;
    int py = (int)petY;
    canvas.fillCircle(px, py, 9, canvas.color565(30, 40, 60));
    canvas.fillCircle(px, py + 2, 6, TFT_WHITE); // 白肚皮
    canvas.fillCircle(px, py - 3, 2, canvas.color565(255, 140, 0)); // 小嘴
    canvas.fillCircle(px - 3, py - 5, 1, TFT_BLACK); // 眼睛
    canvas.fillCircle(px + 3, py - 5, 1, TFT_BLACK);

    // 4. 底部指引
    int botY = SCREEN_H - 20;
    canvas.fillRoundRect(8, botY, SCREEN_W - 16, 16, 3, canvas.color565(16, 24, 38));
    canvas.setTextColor(canvas.color565(255, 230, 100));
    canvas.drawCenterString("倾斜设备 左右踩云下坠", SCREEN_W / 2, botY + 2);
}

void GameFloor::onBtnA() {
    // 紧急微弱小滑翔
    if (vy > 0) {
        vy = -1.2f;
        g_haptics.trigger(HAPTIC_CLICK);
    }
}

void GameFloor::onBtnB() {
    finished = true;
}
