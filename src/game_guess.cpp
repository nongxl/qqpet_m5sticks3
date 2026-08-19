#include "game_guess.h"
#include "pet_core.h"
#include "haptics.h"
#include "sound_manager.h"

extern PetCore g_pet;
extern HapticsEngine g_haptics;

static const char* CHOICE_NAMES[] = {"✌️ 剪刀", "✊ 石头", "🖐️ 步/布"};

GameGuess::GameGuess() {
    init();
}

void GameGuess::init() {
    stage = GUESS_SELECTING;
    playerChoice = 0;
    petChoice = 0;
    roundNum = 1;
    playerScore = 0;
    petScore = 0;
    lastRoundWinner = 0;
    stageStartTime = millis();
    finished = false;
    resultMsg = "";
}

void GameGuess::update(float tiltX, float tiltY, float accelZ) {
    uint32_t now = millis();

    if (stage == GUESS_COUNTDOWN) {
        if (now - stageStartTime > 1200) {
            // 倒计时结束，企鹅随机出拳
            petChoice = random(0, 3);
            
            // 判定胜负: (0:剪刀, 1:石头, 2:布)
            if (playerChoice == petChoice) {
                lastRoundWinner = 0; // 平局
            } else if ((playerChoice == 0 && petChoice == 2) ||
                       (playerChoice == 1 && petChoice == 0) ||
                       (playerChoice == 2 && petChoice == 1)) {
                lastRoundWinner = 1; // 主人胜
                playerScore++;
                g_haptics.trigger(HAPTIC_SUCCESS);
                g_sound.playSound(SOUND_COIN);
            } else {
                lastRoundWinner = 2; // 企鹅胜
                petScore++;
                g_haptics.trigger(HAPTIC_CLICK);
            }

            stage = GUESS_REVEAL;
            stageStartTime = now;
        }
    } else if (stage == GUESS_REVEAL) {
        if (now - stageStartTime > 2000) {
            // 检查是否决出 3 局 2 胜
            if (playerScore >= 2 || petScore >= 2 || roundNum >= 5) {
                stage = GUESS_OVER;
                stageStartTime = now;
                PetState& st = const_cast<PetState&>(g_pet.getState());
                if (playerScore > petScore) {
                    st.coins += 35;
                    st.mood = std::min(1000, st.mood + 150);
                    g_pet.addGrowth(15.0f);
                    resultMsg = "🎉 猜拳大胜！+35元宝 / +150心情";
                } else if (playerScore < petScore) {
                    st.coins += 10;
                    st.mood = std::min(1000, st.mood + 60);
                    g_pet.addGrowth(5.0f);
                    resultMsg = "企鹅赢啦！+10元宝 / +60心情";
                } else {
                    st.coins += 20;
                    st.mood = std::min(1000, st.mood + 100);
                    g_pet.addGrowth(10.0f);
                    resultMsg = "握手言和！+20元宝 / +100心情";
                }

            } else {
                // 下一轮
                roundNum++;
                stage = GUESS_SELECTING;
                stageStartTime = now;
            }
        }
    } else if (stage == GUESS_OVER) {
        if (now - stageStartTime > 2200) {
            finished = true;
        }
    }
}


void GameGuess::render(M5Canvas& canvas) {
    int SCREEN_W = 135;
    int SCREEN_H = 240;

    canvas.fillScreen(canvas.color565(20, 26, 42));

    // 1. 顶部标题栏
    canvas.fillRoundRect(4, 4, SCREEN_W - 8, 24, 4, canvas.color565(32, 48, 76));
    canvas.setFont(&fonts::efontCN_12);
    canvas.setTextSize(1);
    canvas.setTextColor(canvas.color565(255, 215, 60));
    canvas.drawString("✌️ 企鹅猜拳对决", 8, 8);
    canvas.setTextColor(TFT_WHITE);
    canvas.drawRightString("第" + String(roundNum) + "局", SCREEN_W - 8, 8);

    // 2. 比分指示板 (Y=32 ~ 62)
    canvas.fillRoundRect(8, 32, SCREEN_W - 16, 26, 4, canvas.color565(12, 18, 30));
    canvas.setTextColor(canvas.color565(80, 200, 255));
    canvas.drawString("主人 " + String(playerScore), 16, 38);
    canvas.setTextColor(canvas.color565(200, 200, 200));
    canvas.drawCenterString("VS", SCREEN_W / 2, 38);
    canvas.setTextColor(canvas.color565(255, 130, 180));
    canvas.drawRightString(String(petScore) + " 企鹅", SCREEN_W - 16, 38);

    // 3. 中部对决展示区 (Y=66 ~ 170)
    int petY = 72;
    canvas.fillRoundRect(8, petY, SCREEN_W - 16, 96, 6, canvas.color565(28, 38, 58));
    canvas.drawRoundRect(8, petY, SCREEN_W - 16, 96, 6, canvas.color565(55, 75, 110));

    if (stage == GUESS_SELECTING) {
        canvas.setTextColor(canvas.color565(255, 230, 100));
        canvas.drawCenterString("请主人准备出拳...", SCREEN_W / 2, petY + 20);
        canvas.setTextColor(canvas.color565(140, 160, 185));
        canvas.drawCenterString("企鹅正在背后藏手势~", SCREEN_W / 2, petY + 45);
        canvas.setTextColor(canvas.color565(0, 220, 140));
        canvas.drawCenterString("三局两胜 赢取元宝", SCREEN_W / 2, petY + 68);

    } else if (stage == GUESS_COUNTDOWN) {
        uint32_t elapsed = millis() - stageStartTime;
        int cd = 3 - (elapsed / 400);
        if (cd < 1) cd = 1;
        canvas.setTextColor(canvas.color565(255, 180, 0));
        canvas.setTextSize(2);
        canvas.drawCenterString(String(cd) + " !", SCREEN_W / 2, petY + 32);
        canvas.setTextSize(1);

    } else if (stage == GUESS_REVEAL || stage == GUESS_OVER) {
        // 亮拳大 PK
        canvas.setTextColor(canvas.color565(80, 200, 255));
        canvas.drawString("主人: " + String(CHOICE_NAMES[playerChoice]), 16, petY + 16);

        canvas.setTextColor(canvas.color565(255, 130, 180));
        canvas.drawString("企鹅: " + String(CHOICE_NAMES[petChoice]), 16, petY + 40);

        // 胜负横幅
        if (lastRoundWinner == 1) {
            canvas.setTextColor(canvas.color565(0, 255, 100));
            canvas.drawCenterString("🎉 本局主人获胜！", SCREEN_W / 2, petY + 68);
        } else if (lastRoundWinner == 2) {
            canvas.setTextColor(canvas.color565(255, 90, 90));
            canvas.drawCenterString("😆 企鹅技高一筹~", SCREEN_W / 2, petY + 68);
        } else {
            canvas.setTextColor(canvas.color565(255, 215, 60));
            canvas.drawCenterString("🤝 心有灵犀 平局！", SCREEN_W / 2, petY + 68);
        }
    }

    // 4. 底部手势选择轮盘卡片 (Y=174 ~ 210)
    int selY = 174;
    int optW = 38;
    for (int i = 0; i < 3; ++i) {
        int optX = 8 + i * 41;
        bool isSel = (i == playerChoice);
        uint16_t bg = isSel ? canvas.color565(255, 230, 100) : canvas.color565(32, 42, 60);
        uint16_t border = isSel ? canvas.color565(255, 160, 0) : canvas.color565(60, 80, 110);
        uint16_t textCol = isSel ? canvas.color565(20, 30, 10) : TFT_WHITE;

        canvas.fillRoundRect(optX, selY, optW, 34, 4, bg);
        canvas.drawRoundRect(optX, selY, optW, 34, 4, border);

        canvas.setTextColor(textCol);
        if (i == 0) canvas.drawCenterString("剪刀", optX + optW / 2, selY + 10);
        else if (i == 1) canvas.drawCenterString("石头", optX + optW / 2, selY + 10);
        else canvas.drawCenterString("布", optX + optW / 2, selY + 10);
    }

    // 5. 底部按键提示
    int botY = SCREEN_H - 24;
    canvas.fillRoundRect(8, botY, SCREEN_W - 16, 20, 4, canvas.color565(14, 20, 32));
    canvas.setTextColor(canvas.color565(255, 220, 80));
    if (stage == GUESS_SELECTING) {
        canvas.drawCenterString("B切换手势 | A确认出拳", SCREEN_W / 2, botY + 4);
    } else {
        canvas.drawCenterString("对决中...", SCREEN_W / 2, botY + 4);
    }
}

void GameGuess::onBtnA() {
    if (stage == GUESS_SELECTING) {
        stage = GUESS_COUNTDOWN;
        stageStartTime = millis();
        g_haptics.trigger(HAPTIC_CLICK);
    }
}

void GameGuess::onBtnB() {
    if (stage == GUESS_SELECTING) {
        playerChoice = (playerChoice + 1) % 3;
        g_haptics.trigger(HAPTIC_CLICK);
    }
}
