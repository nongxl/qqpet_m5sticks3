#include "game_catcher.h"
#include "haptics.h"

#include "pet_core.h"
#include "asset_manager.h"
#include <cmath>

extern PetCore g_pet;
extern HapticsEngine g_haptics;
extern AssetManager g_assets;

GameCatcher::GameCatcher() {
    init();
}

void GameCatcher::init() {
    playerX = 67.0f;
    lives = 3;
    score = 0;
    coinsGained = 0;
    gameStartTime = millis();
    totalDurationMs = 40000; // 40秒一局
    isShieldActive = false;
    shieldEndTime = 0;
    isGameOver = false;
    gameOverTime = 0;
    finished = false;
    resultMsg = "";
    items.clear();
    lastDropTime = millis();
}

void GameCatcher::update(float tiltX, float tiltY, float accelZ) {
    if (finished) return;
    uint32_t now = millis();

    if (isGameOver) {
        if (now - gameOverTime > 2800) {
            finished = true;
        }
        return;
    }

    // 检查时间耗尽
    if (now - gameStartTime >= totalDurationMs) {
        isGameOver = true;
        gameOverTime = now;
        g_haptics.trigger(HAPTIC_LEVELUP);

        PetState& st = const_cast<PetState&>(g_pet.getState());
        st.coins += coinsGained;
        g_pet.addGrowth(score * 0.2f + 15.0f);
        resultMsg = String("🪙 接宝大满贯！得分 ") + score + "，结算金元宝 +" + coinsGained + "，经验 +" + static_cast<int>(score * 0.2f + 15.0f) + "！";
        return;
    }

    // 检查护盾过期
    if (isShieldActive && now >= shieldEndTime) {
        isShieldActive = false;
    }

    // 1. 体感左右平滑滑行
    playerX += tiltX * 5.8f;
    if (playerX < 18.0f) playerX = 18.0f;
    if (playerX > 117.0f) playerX = 117.0f;

    // 2. 掉落物生成
    if (now - lastDropTime > 650) {
        lastDropTime = now;
        FallingItem it;
        it.x = static_cast<float>(random(15, 120));
        it.y = -10.0f;
        it.vy = static_cast<float>(random(22, 38)) / 10.0f;
        int r = random(0, 100);
        if (r < 45) it.type = 0; // 元宝
        else if (r < 70) it.type = 1; // 小鱼干
        else if (r < 85) it.type = 2; // 彩星
        else it.type = 3; // 炸弹
        items.push_back(it);
    }

    // 3. 掉落物下落与碰撞
    float playerCatchY = 175.0f;
    for (size_t i = 0; i < items.size(); ) {
        items[i].y += items[i].vy;

        float dx = std::abs(items[i].x - playerX);
        float dy = std::abs(items[i].y - playerCatchY);

        if (dx < 18.0f && dy < 14.0f) {
            // 接住了！
            if (items[i].type == 0) {
                // 元宝
                score += 10;
                coinsGained += 5;
                g_haptics.trigger(HAPTIC_CLICK);
            } else if (items[i].type == 1) {
                // 小鱼干
                score += 15;
                coinsGained += 2;
                PetState& st = const_cast<PetState&>(g_pet.getState());
                st.hunger = std::min(100.0f, st.hunger + 1.5f);
                g_haptics.trigger(HAPTIC_CLICK);
            } else if (items[i].type == 2) {
                // 幸运彩星
                score += 25;
                coinsGained += 8;
                PetState& st = const_cast<PetState&>(g_pet.getState());
                st.mood = std::min(100.0f, st.mood + 2.0f);
                g_haptics.trigger(HAPTIC_SUCCESS);
            } else {
                // 炸弹！
                if (!isShieldActive) {
                    lives--;
                    g_haptics.trigger(HAPTIC_ALERT);
                    if (lives <= 0) {
                        isGameOver = true;
                        gameOverTime = now;
                        PetState& st = const_cast<PetState&>(g_pet.getState());
                        st.coins += coinsGained;
                        g_pet.addGrowth(score * 0.1f);
                        resultMsg = String("💥 被炸飞啦！得分 ") + score + "，结算元宝 +" + coinsGained + "！";
                        return;
                    }
                } else {
                    g_haptics.trigger(HAPTIC_CLICK); // 护盾抵挡
                }
            }
            items.erase(items.begin() + i);
            continue;
        }

        if (items[i].y > 230.0f) {
            items.erase(items.begin() + i);
        } else {
            ++i;
        }
    }
}

void GameCatcher::onBtnA() {
    if (isGameOver) {
        finished = true;
        return;
    }
    // 开启 3 秒防炸小伞护盾
    if (!isShieldActive) {
        isShieldActive = true;
        shieldEndTime = millis() + 3000;
        g_haptics.trigger(HAPTIC_SUCCESS);
    }
}

void GameCatcher::onBtnB() {
    finished = true;
    resultMsg = "退出接元宝。";
}

void GameCatcher::render(M5Canvas& canvas) {
    int SCREEN_W = 135;
    int SCREEN_H = 240;

    // 1. 绘制唯美果园蓝天草地背景
    canvas.fillScreen(canvas.color565(175, 225, 255));
    // 底部绿草地
    canvas.fillRect(0, 195, SCREEN_W, 45, canvas.color565(80, 190, 80));
    canvas.drawFastHLine(0, 195, SCREEN_W, canvas.color565(40, 150, 40));

    // 2. 绘制掉落物品
    for (const auto& it : items) {
        int ix = static_cast<int>(it.x);
        int iy = static_cast<int>(it.y);
        if (it.type == 0) {
            // 金元宝 (金黄小梯形元宝)
            canvas.fillRoundRect(ix - 7, iy - 4, 14, 8, 2, canvas.color565(255, 210, 0));
            canvas.fillCircle(ix, iy - 4, 3, canvas.color565(255, 235, 80));
        } else if (it.type == 1) {
            // 小鱼干 (蓝色小鱼)
            canvas.fillEllipse(ix, iy, 7, 4, canvas.color565(40, 160, 240));
            canvas.fillTriangle(ix - 6, iy, ix - 10, iy - 4, ix - 10, iy + 4, canvas.color565(40, 160, 240));
            canvas.fillCircle(ix + 4, iy - 1, 1, TFT_WHITE);
        } else if (it.type == 2) {
            // 幸运彩星 (粉红/金黄五角星)
            canvas.fillCircle(ix, iy, 6, canvas.color565(255, 100, 180));
            canvas.fillCircle(ix, iy, 3, canvas.color565(255, 240, 120));
        } else {
            // 黑色炸弹 (黑圆球 + 引信)
            canvas.fillCircle(ix, iy, 7, canvas.color565(40, 40, 40));
            canvas.drawLine(ix + 4, iy - 6, ix + 7, iy - 9, canvas.color565(200, 100, 30));

            canvas.fillCircle(ix + 7, iy - 9, 2, TFT_RED); // 火星
        }
    }

    // 3. 绘制企鹅与接宝竹篮/小伞
    int px = static_cast<int>(playerX);
    int py = 150;

    // 企鹅头顶接宝小竹篮
    canvas.fillRoundRect(px - 14, py - 4, 28, 7, 2, canvas.color565(210, 140, 70));
    canvas.drawRoundRect(px - 14, py - 4, 28, 7, 2, canvas.color565(150, 90, 40));

    // 护盾光罩特效
    if (isShieldActive) {
        canvas.drawCircle(px, py + 16, 26, canvas.color565(80, 220, 255));
        canvas.drawCircle(px, py + 16, 27, canvas.color565(200, 245, 255));
    }

    const PetState& st = g_pet.getState();
    PetAnimState anim = isGameOver ? ANIM_SAD : ANIM_HAPPY;
    g_assets.drawPetFrame(canvas, px - 48, py - 20, anim, st.gender, g_pet.getLevel(), millis());

    // 4. 顶部 HUD (倒计时 + 生命心 + 得分)
    canvas.setFont(&fonts::efontCN_12);
    canvas.setTextSize(1);
    canvas.fillRoundRect(3, 3, SCREEN_W - 6, 22, 4, canvas.color565(20, 50, 90));

    // 倒计时
    int remainSec = (totalDurationMs - (millis() - gameStartTime)) / 1000;
    if (remainSec < 0) remainSec = 0;
    canvas.setTextColor(canvas.color565(255, 220, 80));
    canvas.drawString(String(remainSec) + "s", 8, 7);

    // 生命红心
    for (int i = 0; i < 3; ++i) {
        if (i < lives) {
            canvas.fillCircle(50 + i * 11, 14, 4, TFT_RED);
        } else {
            canvas.drawCircle(50 + i * 11, 14, 4, canvas.color565(120, 130, 140));
        }
    }

    // 分数
    canvas.setTextColor(canvas.color565(255, 200, 0));
    canvas.drawRightString("🪙 " + String(score), SCREEN_W - 8, 7);

    // 5. 游戏结束弹窗
    if (isGameOver) {
        canvas.fillRoundRect(8, 90, SCREEN_W - 16, 75, 6, canvas.color565(255, 250, 235));
        canvas.drawRoundRect(8, 90, SCREEN_W - 16, 75, 6, canvas.color565(255, 180, 0));
        canvas.setTextColor(canvas.color565(200, 100, 0));
        canvas.drawCenterString("🏆 摘果挑战结束！", SCREEN_W / 2, 98);
        canvas.setTextColor(canvas.color565(20, 120, 40));
        canvas.drawCenterString("最终得分: " + String(score), SCREEN_W / 2, 118);
        canvas.setTextColor(canvas.color565(255, 140, 0));
        canvas.drawCenterString("获得元宝: +" + String(coinsGained), SCREEN_W / 2, 136);
    }
}
