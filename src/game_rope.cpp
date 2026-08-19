#include "game_rope.h"
#include "pet_core.h"
#include "haptics.h"
#include "sound_manager.h"

extern PetCore g_pet;
extern HapticsEngine g_haptics;

GameRope::GameRope() {
    init();
}

void GameRope::init() {
    ropeAngle = 0.0f;
    ropeSpeed = 0.08f;
    petJumpY = 0.0f;
    petJumpVy = 0.0f;
    isJumping = false;
    jumpCount = 0;
    maxJumps = 0;
    lives = 3;
    feedback = "";
    feedbackTime = 0;
    finished = false;
    resultMsg = "";
    gameOverTime = 0;
}

void GameRope::update(float tiltX, float tiltY, float accelZ) {
    if (finished) return;

    if (lives <= 0) {
        if (gameOverTime == 0) {
            gameOverTime = millis();
            int coinReward = 15 + maxJumps * 2;
            if (coinReward > 80) coinReward = 80;
            int moodGain = 50 + maxJumps * 8;
            if (moodGain > 200) moodGain = 200;

            PetState& st = const_cast<PetState&>(g_pet.getState());
            st.coins += coinReward;
            st.mood = std::min(1000, st.mood + moodGain);
            g_pet.addGrowth(20.0f);


            resultMsg = "🪢 跳绳挑战 " + String(maxJumps) + " 次！+" + String(coinReward) + "元宝";
        }
        if (millis() - gameOverTime > 2000) {
            finished = true;
        }
        return;
    }

    // 1. 摇绳旋转
    float prevAngle = ropeAngle;
    ropeAngle += ropeSpeed;
    if (ropeAngle >= 2.0f * PI) {
        ropeAngle -= 2.0f * PI;
    }

    // 2. 绳子过脚判定 (当角度扫过 3*PI/2 ~ 2*PI 时，绳子位于最低点过脚)
    bool ropeAtFeet = (prevAngle < 1.5f * PI && ropeAngle >= 1.5f * PI);
    if (ropeAtFeet) {
        if (isJumping && petJumpY > 12.0f) {
            // 成功跳跃过绳！
            jumpCount++;
            if (jumpCount > maxJumps) maxJumps = jumpCount;
            feedback = "JUMP! " + String(jumpCount);
            feedbackTime = millis();
            g_haptics.trigger(HAPTIC_CLICK);
            SoundManager::getInstance().playSound(SOUND_CLICK);

            // 越跳越快增加挑战乐趣
            ropeSpeed = min(0.16f, 0.08f + (jumpCount * 0.003f));
        } else {
            // 绊绳失误
            lives--;
            jumpCount = 0;
            feedback = "绊到绳啦！";
            feedbackTime = millis();
            g_haptics.trigger(HAPTIC_CLICK);
            SoundManager::getInstance().playSound(SOUND_SICK);
            ropeSpeed = 0.08f;
        }
    }

    // 3. 企鹅跳跃物理
    if (isJumping) {
        petJumpY += petJumpVy;
        petJumpVy -= 1.2f; // 重力
        if (petJumpY <= 0.0f) {
            petJumpY = 0.0f;
            petJumpVy = 0.0f;
            isJumping = false;
        }
    }
}

void GameRope::render(M5Canvas& canvas) {
    int SCREEN_W = 135;
    int SCREEN_H = 240;

    canvas.fillScreen(canvas.color565(220, 245, 230)); // 清新青草绿背景

    // 1. 顶部状态栏
    canvas.fillRoundRect(4, 4, SCREEN_W - 8, 24, 4, canvas.color565(30, 60, 45));
    canvas.setFont(&fonts::efontCN_12);
    canvas.setTextSize(1);
    canvas.setTextColor(canvas.color565(255, 220, 50));
    canvas.drawString("连跳: " + String(jumpCount), 8, 8);

    String lifeStr = "";
    for (int i = 0; i < lives; ++i) lifeStr += "♥ ";
    canvas.setTextColor(canvas.color565(255, 80, 90));
    canvas.drawRightString(lifeStr, SCREEN_W - 8, 8);

    // 2. 地面与阴影
    int groundY = 185;
    canvas.fillRoundRect(0, groundY, SCREEN_W, SCREEN_H - groundY, 0, canvas.color565(80, 160, 90));
    canvas.fillEllipse(67, groundY + 8, 28, 6, canvas.color565(60, 130, 70)); // 地面投影

    // 3. 绘制摇绳 (正弦空间透视弧线)
    int ropeCenterY = groundY - 18;
    int ropeHeight = (int)(sin(ropeAngle) * 36.0f);
    int rx1 = 18, rx2 = SCREEN_W - 18;
    int ryMid = ropeCenterY + ropeHeight;

    // 绘制粗麻绳
    canvas.drawLine(rx1, ropeCenterY, 67, ryMid, canvas.color565(180, 110, 40));
    canvas.drawLine(67, ryMid, rx2, ropeCenterY, canvas.color565(180, 110, 40));
    canvas.drawLine(rx1, ropeCenterY + 1, 67, ryMid + 1, canvas.color565(220, 150, 60));
    canvas.drawLine(67, ryMid + 1, rx2, ropeCenterY + 1, canvas.color565(220, 150, 60));

    // 4. 绘制起跳中的企鹅
    int px = 67;
    int py = groundY - 22 - (int)petJumpY;

    canvas.fillCircle(px, py, 16, canvas.color565(30, 45, 65));
    canvas.fillCircle(px, py + 3, 11, TFT_WHITE);
    canvas.fillCircle(px, py - 6, 3, canvas.color565(255, 140, 0));
    canvas.fillCircle(px - 5, py - 9, 2, TFT_BLACK);
    canvas.fillCircle(px + 5, py - 9, 2, TFT_BLACK);
    // 飞扬小围巾
    canvas.fillRoundRect(px - 8, py + 1, 16, 4, 2, canvas.color565(240, 50, 50));

    // 5. 判定反馈文字
    if (millis() - feedbackTime < 600 && feedback.length() > 0) {
        canvas.setTextColor(canvas.color565(200, 40, 20));
        canvas.drawCenterString(feedback, SCREEN_W / 2, 50);
    }

    // 6. 底部操作指引
    int botY = SCREEN_H - 22;
    canvas.fillRoundRect(8, botY, SCREEN_W - 16, 18, 4, canvas.color565(20, 40, 30));
    canvas.setTextColor(canvas.color565(255, 230, 80));
    canvas.drawCenterString("绳子到底时 按A起跳！", SCREEN_W / 2, botY + 3);
}

void GameRope::onBtnA() {
    if (!isJumping && lives > 0) {
        isJumping = true;
        petJumpVy = 8.5f; // 爆发起跳
        g_haptics.trigger(HAPTIC_CLICK);
    }
}

void GameRope::onBtnB() {
    finished = true;
}
