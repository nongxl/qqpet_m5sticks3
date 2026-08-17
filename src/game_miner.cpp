#include "game_miner.h"
#include "haptics.h"

#include "pet_core.h"
#include "asset_manager.h"
#include <cmath>

extern PetCore g_pet;
extern HapticsEngine g_haptics;
extern AssetManager g_assets;

GameMiner::GameMiner() {
    init();
}

void GameMiner::init() {
    clawAngle = 0.0f;
    swingSpeed = 0.038f;
    clawLength = 18.0f;
    clawState = CLAW_SWINGING;
    grabbedItemIdx = -1;
    score = 0;
    totalGold = 0;
    gameStartTime = millis();
    durationMs = 45000; // 45 秒一局
    isGameOver = false;
    gameOverTime = 0;
    finished = false;
    resultMsg = "";
    items.clear();

    // 生成地底矿物
    // 1. 2 个大金块
    items.push_back({30.0f, 130.0f, 12.0f, 50, 1.4f, 0});
    items.push_back({105.0f, 185.0f, 12.0f, 50, 1.4f, 0});
    // 2. 3 个小金块
    items.push_back({70.0f, 110.0f, 7.0f, 25, 0.7f, 1});
    items.push_back({25.0f, 195.0f, 7.0f, 25, 0.7f, 1});
    items.push_back({110.0f, 125.0f, 7.0f, 25, 0.7f, 1});
    // 3. 2 颗璀璨钻石 (小巧极值钱)
    items.push_back({67.0f, 160.0f, 5.0f, 80, 0.3f, 2});
    items.push_back({85.0f, 215.0f, 5.0f, 80, 0.3f, 2});
    // 4. 2 个笨重石头
    items.push_back({50.0f, 200.0f, 11.0f, 5, 2.0f, 3});
    items.push_back({90.0f, 140.0f, 10.0f, 5, 1.9f, 3});
    // 5. 1 个神秘问号宝箱
    items.push_back({45.0f, 155.0f, 9.0f, 60, 0.8f, 4});
}

void GameMiner::update(float tiltX, float tiltY, float accelZ) {
    if (finished) return;
    uint32_t now = millis();

    if (isGameOver) {
        if (now - gameOverTime > 2800) {
            finished = true;
        }
        return;
    }

    // 检查时间耗尽
    if (now - gameStartTime >= durationMs) {
        isGameOver = true;
        gameOverTime = now;
        g_haptics.trigger(HAPTIC_LEVELUP);

        PetState& st = const_cast<PetState&>(g_pet.getState());
        st.coins += totalGold;
        g_pet.addGrowth(score * 0.2f + 15.0f);
        resultMsg = String("⛏️ 黄金矿工大丰收！挖掘价值 ") + score + "，结算元宝 +" + totalGold + "，经验 +" + static_cast<int>(score * 0.2f + 15.0f) + "！";
        return;
    }

    float originX = 67.0f;
    float originY = 48.0f;

    if (clawState == CLAW_SWINGING) {
        // 钟摆摆动 (-1.2 ~ +1.2 弧度，约 -70度 到 +70度)
        clawAngle += swingSpeed;
        if (clawAngle > 1.15f || clawAngle < -1.15f) {
            swingSpeed = -swingSpeed;
        }
    } else if (clawState == CLAW_EXTENDING) {
        // 飞爪向前射出
        clawLength += 4.5f;

        float clawTipX = originX + sin(clawAngle) * clawLength;
        float clawTipY = originY + cos(clawAngle) * clawLength;

        // 检查碰撞矿物
        for (size_t i = 0; i < items.size(); ++i) {
            float dx = items[i].x - clawTipX;
            float dy = items[i].y - clawTipY;
            float dist = sqrt(dx * dx + dy * dy);
            if (dist < items[i].radius + 5.0f) {
                // 抓到了！
                grabbedItemIdx = static_cast<int>(i);
                clawState = CLAW_RETRACTING;
                g_haptics.trigger((items[i].type == 0 || items[i].type == 3) ? HAPTIC_ALERT : HAPTIC_SUCCESS);
                break;
            }
        }

        // 碰触屏幕边界回缩
        if (clawTipX < 4.0f || clawTipX > 131.0f || clawTipY > 235.0f) {
            clawState = CLAW_RETRACTING;
            grabbedItemIdx = -1;
        }
    } else if (clawState == CLAW_RETRACTING) {
        // 根据矿物重量决定拉回速度
        float pullSpeed = 4.8f;
        if (grabbedItemIdx >= 0 && grabbedItemIdx < static_cast<int>(items.size())) {
            pullSpeed = 3.6f / items[grabbedItemIdx].weight;
            // 抓大金块时持续微震
            if (items[grabbedItemIdx].weight > 1.2f && ((now / 120) % 2 == 0)) {
                g_haptics.trigger(HAPTIC_CLICK);
            }
        }

        clawLength -= pullSpeed;
        if (clawLength <= 18.0f) {
            clawLength = 18.0f;
            clawState = CLAW_SWINGING;

            // 成功拉回结算该物品
            if (grabbedItemIdx >= 0 && grabbedItemIdx < static_cast<int>(items.size())) {
                score += items[grabbedItemIdx].value;
                totalGold += items[grabbedItemIdx].value;
                g_haptics.trigger(HAPTIC_SUCCESS);
                items.erase(items.begin() + grabbedItemIdx);
                grabbedItemIdx = -1;
            }
        }
    }
}

void GameMiner::onBtnA() {
    if (isGameOver) {
        finished = true;
        return;
    }
    if (clawState == CLAW_SWINGING) {
        clawState = CLAW_EXTENDING;
        g_haptics.trigger(HAPTIC_CLICK);
    }
}

void GameMiner::onBtnB() {
    finished = true;
    resultMsg = "退出黄金矿工。";
}

void GameMiner::render(M5Canvas& canvas) {
    int SCREEN_W = 135;
    int SCREEN_H = 240;

    // 1. 绘制矿洞背景 (顶部阳光草地，下部深邃矿洞泥土)
    canvas.fillScreen(canvas.color565(75, 45, 25)); // 矿洞深褐土壤
    // 顶部地面草坪
    canvas.fillRect(0, 0, SCREEN_W, 46, canvas.color565(60, 160, 60));
    canvas.drawFastHLine(0, 46, SCREEN_W, canvas.color565(40, 100, 30));

    // 2. 绘制地底矿物
    float originX = 67.0f;
    float originY = 48.0f;
    float clawTipX = originX + sin(clawAngle) * clawLength;
    float clawTipY = originY + cos(clawAngle) * clawLength;

    for (size_t i = 0; i < items.size(); ++i) {
        float ix = items[i].x;
        float iy = items[i].y;

        // 如果该矿物正被抓取拉回，跟随爪子坐标
        if (static_cast<int>(i) == grabbedItemIdx) {
            ix = clawTipX;
            iy = clawTipY;
        }

        int sx = static_cast<int>(ix);
        int sy = static_cast<int>(iy);
        int rad = static_cast<int>(items[i].radius);

        if (items[i].type == 0) {
            // 大金块 (不规则金黄闪亮)
            canvas.fillRoundRect(sx - rad, sy - rad, rad * 2, rad * 2, 4, canvas.color565(255, 205, 0));
            canvas.fillCircle(sx - 2, sy - 2, 3, canvas.color565(255, 240, 100));
            canvas.drawRoundRect(sx - rad, sy - rad, rad * 2, rad * 2, 4, canvas.color565(200, 140, 0));
        } else if (items[i].type == 1) {
            // 小金块
            canvas.fillCircle(sx, sy, rad, canvas.color565(255, 215, 20));
            canvas.fillCircle(sx - 1, sy - 1, 2, canvas.color565(255, 245, 120));
        } else if (items[i].type == 2) {
            // 璀璨钻石 (青蓝闪亮多边形)
            canvas.fillTriangle(sx, sy - rad, sx - rad, sy, sx + rad, sy, canvas.color565(120, 235, 255));
            canvas.fillTriangle(sx - rad, sy, sx + rad, sy, sx, sy + rad, canvas.color565(60, 180, 245));
        } else if (items[i].type == 3) {
            // 灰色大石头
            canvas.fillCircle(sx, sy, rad, canvas.color565(130, 135, 140));
            canvas.drawCircle(sx, sy, rad, canvas.color565(90, 95, 100));
        } else {
            // 神秘问号宝箱
            canvas.fillRoundRect(sx - rad, sy - rad + 2, rad * 2, rad * 2 - 4, 3, canvas.color565(180, 100, 40));
            canvas.drawRoundRect(sx - rad, sy - rad + 2, rad * 2, rad * 2 - 4, 3, canvas.color565(240, 200, 60));
            canvas.setFont(&fonts::efontCN_10);
            canvas.setTextColor(canvas.color565(255, 230, 80));
            canvas.drawCenterString("?", sx, sy - 4);
        }
    }

    // 3. 绘制飞爪钢索与铁爪
    canvas.drawLine(static_cast<int>(originX), static_cast<int>(originY), static_cast<int>(clawTipX), static_cast<int>(clawTipY), canvas.color565(210, 215, 220));
    // 铁爪头部
    canvas.fillCircle(static_cast<int>(clawTipX), static_cast<int>(clawTipY), 4, canvas.color565(190, 80, 40));
    // 铁爪分叉
    float clawLeftX = clawTipX - cos(clawAngle) * 5.0f + sin(clawAngle) * 4.0f;
    float clawLeftY = clawTipY + sin(clawAngle) * 5.0f + cos(clawAngle) * 4.0f;
    float clawRightX = clawTipX + cos(clawAngle) * 5.0f + sin(clawAngle) * 4.0f;
    float clawRightY = clawTipY - sin(clawAngle) * 5.0f + cos(clawAngle) * 4.0f;
    canvas.drawLine(static_cast<int>(clawTipX), static_cast<int>(clawTipY), static_cast<int>(clawLeftX), static_cast<int>(clawLeftY), canvas.color565(230, 120, 60));
    canvas.drawLine(static_cast<int>(clawTipX), static_cast<int>(clawTipY), static_cast<int>(clawRightX), static_cast<int>(clawRightY), canvas.color565(230, 120, 60));

    // 4. 绘制地面上努力拉绳索的企鹅
    const PetState& st = g_pet.getState();
    PetAnimState anim = (clawState == CLAW_RETRACTING) ? ANIM_WORK : ANIM_IDLE_STAND;
    g_assets.drawPetFrame(canvas, 19, 0, anim, st.gender, g_pet.getLevel(), millis());

    // 5. 顶部 HUD
    canvas.setFont(&fonts::efontCN_12);
    canvas.setTextSize(1);
    canvas.fillRoundRect(3, 3, SCREEN_W - 6, 22, 4, canvas.color565(20, 40, 70));

    int remainSec = (durationMs - (millis() - gameStartTime)) / 1000;
    if (remainSec < 0) remainSec = 0;
    canvas.setTextColor(canvas.color565(255, 220, 80));
    canvas.drawString("⏱️ " + String(remainSec) + "s", 8, 7);

    canvas.setTextColor(canvas.color565(255, 210, 0));
    canvas.drawRightString("💰 " + String(score), SCREEN_W - 8, 7);

    // 6. 游戏结束弹窗
    if (isGameOver) {
        canvas.fillRoundRect(8, 100, SCREEN_W - 16, 75, 6, canvas.color565(255, 250, 230));
        canvas.drawRoundRect(8, 100, SCREEN_W - 16, 75, 6, canvas.color565(255, 180, 0));
        canvas.setTextColor(canvas.color565(200, 90, 0));
        canvas.drawCenterString("🏆 矿洞开采完毕！", SCREEN_W / 2, 108);
        canvas.setTextColor(canvas.color565(20, 120, 40));
        canvas.drawCenterString("总开采金: " + String(score), SCREEN_W / 2, 128);
        canvas.setTextColor(canvas.color565(255, 140, 0));
        canvas.drawCenterString("结算元宝: +" + String(totalGold), SCREEN_W / 2, 146);
    }
}
