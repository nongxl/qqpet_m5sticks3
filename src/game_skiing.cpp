#include "game_skiing.h"
#include "haptics.h"

#include "pet_core.h"
#include "asset_manager.h"
#include <cmath>

extern PetCore g_pet;
extern HapticsEngine g_haptics;
extern AssetManager g_assets;

GameSkiing::GameSkiing() {
    init();
}

void GameSkiing::init() {
    playerX = 67.0f;
    playerY = 60.0f; // 企鹅位于屏幕偏上方，向下俯冲
    speed = 2.8f;
    distance = 0.0f;
    coinsCollected = 0;
    isJumping = false;
    jumpProgress = 0.0f;
    isCrashed = false;
    crashTime = 0;
    finished = false;
    resultMsg = "";
    obstacles.clear();
    lastSpawnTime = millis();
}

void GameSkiing::update(float tiltX, float tiltY, float accelZ) {
    if (finished) return;
    uint32_t now = millis();

    if (isCrashed) {
        if (now - crashTime > 2800) {
            finished = true;
        }
        return;
    }

    // 1. 体感控制左右横移 (倾斜灵敏度)
    playerX += tiltX * 5.2f;
    if (playerX < 18.0f) playerX = 18.0f;
    if (playerX > 117.0f) playerX = 117.0f;

    // 2. 距离与速度随时间自然提升
    distance += speed * 0.1f;
    speed += 0.0008f;
    if (speed > 6.5f) speed = 6.5f;

    // 3. 跳跃状态更新
    if (isJumping) {
        jumpProgress += 0.06f;
        if (jumpProgress >= 1.0f) {
            isJumping = false;
            jumpProgress = 0.0f;
        }
    }

    // 4. 生成新障碍物 / 金币 (从屏幕底部向上迎面而来，模拟俯冲)
    if (now - lastSpawnTime > static_cast<uint32_t>(750.0f / (speed * 0.35f))) {
        lastSpawnTime = now;
        SkiObstacle obs;
        obs.x = static_cast<float>(random(15, 120));
        obs.y = 250.0f;
        int r = random(0, 100);
        if (r < 40) obs.type = 2; // 金币
        else if (r < 75) obs.type = 0; // 松树
        else obs.type = 1; // 雪人
        obstacles.push_back(obs);
    }

    // 5. 障碍物移动与碰撞检测
    for (size_t i = 0; i < obstacles.size(); ) {
        obstacles[i].y -= speed * 1.8f;

        // 碰撞检测
        float dx = std::abs(obstacles[i].x - playerX);
        float dy = std::abs(obstacles[i].y - playerY);

        if (dx < 16.0f && dy < 16.0f) {
            if (obstacles[i].type == 2) {
                // 吃金币
                coinsCollected++;
                g_haptics.trigger(HAPTIC_CLICK);
                obstacles.erase(obstacles.begin() + i);
                continue;
            } else if (!isJumping) {
                // 撞击障碍物！
                isCrashed = true;
                crashTime = now;
                g_haptics.trigger(HAPTIC_ALERT);

                // 结算
                int finalScore = static_cast<int>(distance);
                int earnedCoins = coinsCollected * 5 + (finalScore / 10);
                float earnedExp = finalScore * 0.15f + 10.0f;

                PetState& st = const_cast<PetState&>(g_pet.getState());
                st.coins += earnedCoins;
                g_pet.addGrowth(earnedExp);

                resultMsg = String("⛷️ 滑雪挑战结束！滑行 ") + finalScore + "米，捕获 " + coinsCollected + "金币，结算元宝 +" + earnedCoins + "，经验 +" + static_cast<int>(earnedExp) + "！";
                return;
            }
        }

        if (obstacles[i].y < -20.0f) {
            obstacles.erase(obstacles.begin() + i);
        } else {
            ++i;
        }
    }
}

void GameSkiing::onBtnA() {
    if (isCrashed) {
        finished = true;
        return;
    }
    // 触发腾空跳跃！
    if (!isJumping) {
        isJumping = true;
        jumpProgress = 0.0f;
        g_haptics.trigger(HAPTIC_SUCCESS);
    }
}

void GameSkiing::onBtnB() {
    finished = true;
    resultMsg = "退出极速滑雪。";
}

void GameSkiing::render(M5Canvas& canvas) {
    int SCREEN_W = 135;
    int SCREEN_H = 240;

    // 1. 绘制雪白雪山背景与雪道极速线条
    canvas.fillScreen(canvas.color565(240, 248, 255));
    // 左右雪道边界
    canvas.drawFastVLine(6, 0, SCREEN_H, canvas.color565(180, 215, 245));
    canvas.drawFastVLine(SCREEN_W - 7, 0, SCREEN_H, canvas.color565(180, 215, 245));

    // 动态向下飞驰的雪痕线条
    uint32_t now = millis();
    for (int i = 0; i < 6; ++i) {
        int ly = (static_cast<int>(now * speed * 0.2f + i * 45)) % SCREEN_H;
        canvas.drawFastVLine(25 + i * 16, ly, 18, canvas.color565(210, 230, 250));
    }

    // 2. 绘制障碍物与金币
    for (const auto& obs : obstacles) {
        int ox = static_cast<int>(obs.x);
        int oy = static_cast<int>(obs.y);
        if (obs.type == 0) {
            // 绿色松树 (三角形树冠 + 棕色树干)
            canvas.fillTriangle(ox, oy - 14, ox - 9, oy + 4, ox + 9, oy + 4, canvas.color565(20, 130, 40));
            canvas.fillTriangle(ox, oy - 8, ox - 7, oy + 8, ox + 7, oy + 8, canvas.color565(30, 160, 50));
            canvas.fillRect(ox - 2, oy + 8, 4, 5, canvas.color565(120, 70, 30));
        } else if (obs.type == 1) {
            // 憨态雪人 (双层白球 + 红帽子)
            canvas.fillCircle(ox, oy + 4, 7, TFT_WHITE);
            canvas.drawCircle(ox, oy + 4, 7, canvas.color565(160, 190, 220));
            canvas.fillCircle(ox, oy - 4, 5, TFT_WHITE);
            canvas.drawCircle(ox, oy - 4, 5, canvas.color565(160, 190, 220));
            canvas.fillRoundRect(ox - 4, oy - 10, 8, 4, 1, TFT_RED);
        } else {
            // 闪闪发光大金币
            canvas.fillCircle(ox, oy, 6, canvas.color565(255, 200, 0));
            canvas.fillCircle(ox, oy, 4, canvas.color565(255, 240, 90));
            canvas.drawCircle(ox, oy, 6, canvas.color565(210, 140, 0));
        }
    }

    // 3. 绘制企鹅滑雪者 (两块滑雪双板 + 企鹅)
    int px = static_cast<int>(playerX);
    int py = static_cast<int>(playerY);

    if (isJumping) {
        // 跳跃时向上腾空位移与阴影
        float jumpHeight = sin(jumpProgress * 3.14159f) * 16.0f;
        canvas.fillEllipse(px, py + 24, 16, 5, canvas.color565(180, 200, 220)); // 地面阴影
        py -= static_cast<int>(jumpHeight);
    } else {
        // 正常滑雪双板
        canvas.fillRect(px - 14, py + 20, 8, 4, canvas.color565(220, 50, 50)); // 左滑雪板
        canvas.fillRect(px + 6, py + 20, 8, 4, canvas.color565(220, 50, 50));  // 右滑雪板
        canvas.fillEllipse(px, py + 22, 14, 4, canvas.color565(190, 215, 235));
    }

    const PetState& st = g_pet.getState();
    PetAnimState anim = isCrashed ? ANIM_SAD : (isJumping ? ANIM_HAPPY : ANIM_PLAY);
    g_assets.drawPetFrame(canvas, px - 48, py - 30, anim, st.gender, g_pet.getLevel(), millis());

    // 4. 顶部 HUD 计分条
    canvas.setFont(&fonts::efontCN_12);
    canvas.setTextSize(1);
    canvas.fillRoundRect(3, 3, SCREEN_W - 6, 22, 4, canvas.color565(20, 50, 90));
    canvas.setTextColor(canvas.color565(255, 220, 80));
    canvas.drawString(String(static_cast<int>(distance)) + "m", 8, 7);

    canvas.setTextColor(canvas.color565(255, 200, 0));
    canvas.drawRightString("🪙 " + String(coinsCollected), SCREEN_W - 8, 7);

    // 5. 撞击结算弹窗
    if (isCrashed) {
        canvas.fillRoundRect(8, 140, SCREEN_W - 16, 70, 6, canvas.color565(255, 245, 245));
        canvas.drawRoundRect(8, 140, SCREEN_W - 16, 70, 6, TFT_RED);
        canvas.setTextColor(TFT_RED);
        canvas.drawCenterString("💥 哎呀，撞树翻车啦！", SCREEN_W / 2, 148);
        canvas.setTextColor(canvas.color565(20, 100, 40));
        canvas.drawCenterString("滑行: " + String(static_cast<int>(distance)) + "m  金币: +" + String(coinsCollected * 5), SCREEN_W / 2, 168);
        canvas.setTextColor(canvas.color565(120, 130, 140));
        canvas.drawCenterString("按【BtnA】返回大厅", SCREEN_W / 2, 188);
    }
}
