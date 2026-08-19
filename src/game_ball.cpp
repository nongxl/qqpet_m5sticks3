#include "game_ball.h"
#include "pet_core.h"
#include "haptics.h"
#include "sound_manager.h"

extern PetCore g_pet;
extern HapticsEngine g_haptics;

GameBall::GameBall() {
    init();
}

void GameBall::init() {
    ballX = 67.0f;
    ballY = 40.0f;
    ballSpeedY = 2.0f;
    comboCount = 0;
    maxCombo = 0;
    score = 0;
    lives = 3;
    hitFeedback = "";
    feedbackTime = 0;
    finished = false;
    resultMsg = "";
    gameOverTime = 0;
}

void GameBall::update(float tiltX, float tiltY, float accelZ) {
    if (finished) return;

    if (lives <= 0) {
        if (gameOverTime == 0) {
            gameOverTime = millis();
            int coinReward = score / 2;
            if (coinReward > 80) coinReward = 80;
            if (coinReward < 10) coinReward = 10;
            int moodGain = 50 + maxCombo * 10;
            if (moodGain > 200) moodGain = 200;

            PetState& st = const_cast<PetState&>(g_pet.getState());
            st.coins += coinReward;
            st.mood = std::min(1000, st.mood + moodGain);
            g_pet.addGrowth(20.0f);
            resultMsg = "🏀 颠球结束! +" + String(coinReward) + "元宝 / +" + String(moodGain) + "心情";

        }
        if (millis() - gameOverTime > 2000) {
            finished = true;
        }
        return;
    }


    // 体感快速向上颠动触发拍球 (当 Z 轴加速度突变时)
    static float lastAccelZ = 1.0f;
    if (fabs(accelZ - lastAccelZ) > 1.3f) {
        onBtnA();
    }
    lastAccelZ = accelZ;

    // 皮球重力运动
    ballY += ballSpeedY;
    ballSpeedY += 0.15f; // 重力加速度

    // 皮球掉落到底部越界 (Miss)
    if (ballY > 215.0f) {
        lives--;
        comboCount = 0;
        hitFeedback = "MISS!";
        feedbackTime = millis();
        g_haptics.trigger(HAPTIC_CLICK);

        // 重置皮球到上方重新掉落
        ballY = 35.0f;
        ballSpeedY = 2.2f;
    }
}

void GameBall::render(M5Canvas& canvas) {
    int SCREEN_W = 135;
    int SCREEN_H = 240;

    canvas.fillScreen(canvas.color565(16, 24, 38));

    // 1. 顶部状态栏 (得分、连击、生命)
    canvas.fillRoundRect(4, 4, SCREEN_W - 8, 26, 4, canvas.color565(26, 38, 60));
    canvas.setFont(&fonts::efontCN_12);
    canvas.setTextSize(1);
    canvas.setTextColor(canvas.color565(255, 215, 60));
    canvas.drawString("得分:" + String(score), 10, 9);

    // 生命红心
    String lifeStr = "";
    for (int i = 0; i < lives; ++i) lifeStr += "♥ ";
    canvas.setTextColor(canvas.color565(255, 80, 100));
    canvas.drawRightString(lifeStr, SCREEN_W - 10, 9);

    // 2. 拍球判定区 (Y=175 ~ 205，绿色舒适光带)
    int targetTop = 175;
    int targetBottom = 205;
    int perfectLine = 190;

    canvas.fillRect(10, targetTop, SCREEN_W - 20, targetBottom - targetTop, canvas.color565(20, 60, 45));
    canvas.drawRect(10, targetTop, SCREEN_W - 20, targetBottom - targetTop, canvas.color565(50, 180, 100));

    // 金色 Perfect 判定中线
    canvas.drawFastHLine(15, perfectLine, SCREEN_W - 30, canvas.color565(255, 220, 50));
    canvas.drawFastHLine(15, perfectLine + 1, SCREEN_W - 30, canvas.color565(255, 220, 50));

    canvas.setFont(&fonts::efontCN_10);
    canvas.setTextColor(canvas.color565(80, 220, 140));
    canvas.drawRightString("拍球判定区", SCREEN_W - 14, targetTop + 2);

    // 3. 绘制彩色皮球 (带高光与运动残影)
    int bx = (int)ballX;
    int by = (int)ballY;

    // 皮球本体 (黄蓝相间经典儿童皮球)
    canvas.fillCircle(bx, by, 10, canvas.color565(255, 140, 20));
    canvas.fillCircle(bx - 3, by - 3, 4, canvas.color565(255, 230, 80)); // 高光
    canvas.drawCircle(bx, by, 10, canvas.color565(180, 80, 0));
    canvas.drawFastHLine(bx - 8, by, 16, canvas.color565(40, 120, 255)); // 蓝条纹

    // 4. 连击指示器与击打反馈
    if (comboCount > 1) {
        canvas.setFont(&fonts::efontCN_12);
        canvas.setTextColor(canvas.color565(255, 180, 0));
        canvas.drawCenterString("COMBO x" + String(comboCount), SCREEN_W / 2, 45);
    }

    if (millis() - feedbackTime < 600 && hitFeedback.length() > 0) {
        canvas.setFont(&fonts::efontCN_12);
        if (hitFeedback == "PERFECT!") {
            canvas.setTextColor(canvas.color565(255, 230, 50));
        } else if (hitFeedback == "GOOD!") {
            canvas.setTextColor(canvas.color565(80, 255, 120));
        } else {
            canvas.setTextColor(canvas.color565(255, 80, 80));
        }
        canvas.drawCenterString(hitFeedback, SCREEN_W / 2, 70);
    }

    // 5. 底部操作指引
    int botY = SCREEN_H - 24;
    canvas.fillRoundRect(8, botY, SCREEN_W - 16, 20, 4, canvas.color565(16, 24, 36));
    canvas.setFont(&fonts::efontCN_12);
    canvas.setTextColor(canvas.color565(255, 220, 80));
    if (lives > 0) {
        canvas.drawCenterString("落入绿框 按A/向上颠球", SCREEN_W / 2, botY + 4);
    } else {
        canvas.drawCenterString("游戏结束 结算中...", SCREEN_W / 2, botY + 4);
    }
}

void GameBall::onBtnA() {
    if (lives <= 0 || finished) return;

    int targetTop = 175;
    int targetBottom = 205;
    int perfectLine = 190;

    // 判定皮球是否在拍击区间
    if (ballY >= targetTop && ballY <= targetBottom) {
        int dist = abs((int)ballY - perfectLine);
        if (dist <= 4) {
            // PERFECT 完美颠球
            comboCount++;
            if (comboCount > maxCombo) maxCombo = comboCount;
            score += 20 + comboCount * 5;
            hitFeedback = "PERFECT!";
            g_haptics.trigger(HAPTIC_SUCCESS);
            g_sound.playSound(SOUND_HAPPY);
        } else {
            // GOOD 良好颠球
            comboCount++;
            if (comboCount > maxCombo) maxCombo = comboCount;
            score += 10 + comboCount * 2;
            hitFeedback = "GOOD!";
            g_haptics.trigger(HAPTIC_CLICK);
            g_sound.playSound(SOUND_CLICK);
        }

        feedbackTime = millis();
        // 向上大力反弹 (连击越高弹得越灵动)
        ballSpeedY = - (5.2f + min(3.0f, comboCount * 0.2f));

    } else if (ballY < targetTop - 20) {
        // 拍空了 (太早)
        hitFeedback = "TOO EARLY!";
        feedbackTime = millis();
    }
}

void GameBall::onBtnB() {
    // 游戏中短按 BtnB 也可作为辅助拍球键
    onBtnA();
}
