#include "game_jumper.h"
#include "haptics.h"

#include "pet_core.h"
#include "asset_manager.h"
#include <cmath>

extern PetCore g_pet;
extern HapticsEngine g_haptics;
extern AssetManager g_assets;

GameJumper::GameJumper() {
    init();
}

void GameJumper::init() {
    playerX = 67.0f;
    playerY = 170.0f;
    playerVy = -7.5f; // 起跳初始速度
    cameraY = 0.0f;
    maxHeight = 0.0f;
    canGlide = true;
    isGameOver = false;
    gameOverTime = 0;
    finished = false;
    resultMsg = "";
    platforms.clear();

    // 初始踏板生成
    JumperPlatform basePlat = {67.0f, 210.0f, 60.0f, 0, 0.0f};
    platforms.push_back(basePlat);

    highestSpawnY = 210.0f;
    for (int i = 0; i < 8; ++i) {
        highestSpawnY -= random(28, 42);
        JumperPlatform p;
        p.x = static_cast<float>(random(25, 110));
        p.y = highestSpawnY;
        p.w = static_cast<float>(random(32, 48));
        p.type = (random(0, 10) > 7) ? 1 : 0;
        p.vx = (p.type == 1) ? ((random(0, 2) == 0) ? 0.8f : -0.8f) : 0.0f;
        platforms.push_back(p);
    }
}

void GameJumper::update(float tiltX, float tiltY, float accelZ) {
    if (finished) return;
    uint32_t now = millis();

    if (isGameOver) {
        if (now - gameOverTime > 2800) {
            finished = true;
        }
        return;
    }

    // 1. 体感控制左右横移 (屏幕边缘穿越机制，左进右出，经典趣味)
    playerX += tiltX * 5.2f;
    if (playerX < 0.0f) playerX = 135.0f;
    if (playerX > 135.0f) playerX = 0.0f;

    // 2. 物理重力加速度
    playerVy += 0.26f;
    playerY += playerVy;

    // 3. 摄像机跟随向上滚动
    if (playerY < cameraY + 95.0f) {
        cameraY = playerY - 95.0f;
    }

    // 记录最大攀登高度
    float currentH = -playerY / 8.0f + 25.0f;
    if (currentH > maxHeight) maxHeight = currentH;

    // 4. 踏板逻辑与踩踏碰撞
    for (auto& p : platforms) {
        if (p.type == 1) {
            p.x += p.vx;
            if (p.x < 20.0f || p.x > 115.0f) p.vx = -p.vx;
        }

        // 下落状态下才判定踩踏板
        if (playerVy > 0.0f) {
            if (playerY >= p.y - 6.0f && playerY <= p.y + 10.0f) {
                if (std::abs(playerX - p.x) < p.w / 2.0f + 8.0f) {
                    // 弹跳！
                    playerVy = (p.type == 2) ? -10.5f : -7.5f;
                    canGlide = true; // 踩到踏板刷新滑翔次数
                    g_haptics.trigger(HAPTIC_CLICK);
                }
            }
        }
    }

    // 5. 动态补全上方新踏板
    while (highestSpawnY > cameraY - 120.0f) {
        highestSpawnY -= random(30, 45);
        JumperPlatform p;
        p.x = static_cast<float>(random(25, 110));
        p.y = highestSpawnY;
        p.w = static_cast<float>(random(30, 44));
        int r = random(0, 100);
        if (r < 60) p.type = 0; // 普通
        else if (r < 88) p.type = 1; // 移动
        else p.type = 2; // 弹簧强力踏板
        p.vx = (p.type == 1) ? 1.0f : 0.0f;
        platforms.push_back(p);
    }

    // 6. 清理下方出界过远的踏板
    for (size_t i = 0; i < platforms.size(); ) {
        if (platforms[i].y > cameraY + 260.0f) {
            platforms.erase(platforms.begin() + i);
        } else {
            ++i;
        }
    }

    // 7. 踩空坠落死亡检测
    if (playerY > cameraY + 245.0f) {
        isGameOver = true;
        gameOverTime = now;
        g_haptics.trigger(HAPTIC_ALERT);

        int finalScore = static_cast<int>(maxHeight);
        int earnedCoins = finalScore / 2;
        float earnedExp = finalScore * 0.25f + 10.0f;

        PetState& st = const_cast<PetState&>(g_pet.getState());
        st.coins += earnedCoins;
        g_pet.addGrowth(earnedExp);

        resultMsg = String("☁️ 步步高升攀登结束！高度 ") + finalScore + "米，结算元宝 +" + earnedCoins + "，经验 +" + static_cast<int>(earnedExp) + "！";
    }
}

void GameJumper::onBtnA() {
    if (isGameOver) {
        finished = true;
        return;
    }
    // 拍翅膀二次滞空救援！
    if (canGlide && playerVy > 0.0f) {
        playerVy = -6.2f;
        canGlide = false;
        g_haptics.trigger(HAPTIC_SUCCESS);
    }
}

void GameJumper::onBtnB() {
    finished = true;
    resultMsg = "退出跳一跳。";
}

void GameJumper::render(M5Canvas& canvas) {
    int SCREEN_W = 135;
    int SCREEN_H = 240;

    // 1. 随着高度逐渐变深的天空背景 (从蔚蓝渐变至高空深蓝/繁星)
    int skyFactor = static_cast<int>(maxHeight * 0.4f);
    if (skyFactor > 120) skyFactor = 120;
    canvas.fillScreen(canvas.color565(130 - skyFactor, 180 - skyFactor, 255 - skyFactor / 2));

    // 2. 绘制云朵跳板 (转换为屏幕相对坐标)
    for (const auto& p : platforms) {
        int sx = static_cast<int>(p.x);
        int sy = static_cast<int>(p.y - cameraY);
        int sw = static_cast<int>(p.w);

        if (sy > -20 && sy < SCREEN_H + 20) {
            if (p.type == 0) {
                // 普通白色蓬松云朵
                canvas.fillRoundRect(sx - sw / 2, sy - 3, sw, 8, 4, TFT_WHITE);
                canvas.drawRoundRect(sx - sw / 2, sy - 3, sw, 8, 4, canvas.color565(180, 210, 240));
            } else if (p.type == 1) {
                // 移动蓝色浮冰踏板
                canvas.fillRoundRect(sx - sw / 2, sy - 3, sw, 8, 3, canvas.color565(80, 200, 255));
                canvas.drawRoundRect(sx - sw / 2, sy - 3, sw, 8, 3, canvas.color565(30, 130, 220));
            } else {
                // 弹簧金色弹跳板
                canvas.fillRoundRect(sx - sw / 2, sy - 4, sw, 9, 4, canvas.color565(255, 210, 40));
                canvas.drawRoundRect(sx - sw / 2, sy - 4, sw, 9, 4, canvas.color565(210, 130, 0));
            }
        }
    }

    // 3. 绘制企鹅跳跃者
    int px = static_cast<int>(playerX);
    int py = static_cast<int>(playerY - cameraY);

    const PetState& st = g_pet.getState();
    PetAnimState anim = isGameOver ? ANIM_SAD : ((playerVy < 0) ? ANIM_HAPPY : ANIM_PLAY);
    g_assets.drawPetFrame(canvas, px - 48, py - 35, anim, st.gender, g_pet.getLevel(), millis());

    // 4. 顶部 HUD
    canvas.setFont(&fonts::efontCN_12);
    canvas.setTextSize(1);
    canvas.fillRoundRect(3, 3, SCREEN_W - 6, 22, 4, canvas.color565(20, 50, 90));
    canvas.setTextColor(canvas.color565(255, 220, 80));
    canvas.drawString("☁️ " + String(static_cast<int>(maxHeight)) + "m", 8, 7);

    // 翅膀滑翔保底标识
    if (canGlide) {
        canvas.setTextColor(canvas.color565(80, 240, 140));
        canvas.drawRightString("🕊️滑翔就绪", SCREEN_W - 8, 7);
    } else {
        canvas.setTextColor(canvas.color565(160, 170, 180));
        canvas.drawRightString("滑翔耗尽", SCREEN_W - 8, 7);
    }

    // 5. 坠落结算弹窗
    if (isGameOver) {
        canvas.fillRoundRect(8, 120, SCREEN_W - 16, 70, 6, canvas.color565(255, 245, 240));
        canvas.drawRoundRect(8, 120, SCREEN_W - 16, 70, 6, canvas.color565(240, 120, 40));
        canvas.setTextColor(canvas.color565(210, 90, 0));
        canvas.drawCenterString("哎呀，踩空掉下来啦！", SCREEN_W / 2, 128);
        canvas.setTextColor(canvas.color565(20, 100, 40));
        canvas.drawCenterString("攀登高度: " + String(static_cast<int>(maxHeight)) + "m", SCREEN_W / 2, 148);
        canvas.setTextColor(canvas.color565(120, 130, 140));
        canvas.drawCenterString("按【BtnA】返回大厅", SCREEN_W / 2, 168);
    }
}
