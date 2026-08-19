#pragma once
#include "mini_game_base.h"

enum GuessStage {
    GUESS_SELECTING = 0, // 主人选择手势 (剪刀/石头/布)
    GUESS_COUNTDOWN,     // 3, 2, 1 倒计时出拳
    GUESS_REVEAL,        // 亮拳判定胜负
    GUESS_OVER           // 3局2胜对决结束结算
};

class GameGuess : public MiniGameBase {
public:
    GameGuess();
    void init() override;
    void update(float tiltX, float tiltY, float accelZ) override;
    void render(M5Canvas& canvas) override;
    void onBtnA() override;
    void onBtnB() override;
    bool isFinished() const override { return finished; }
    String getResultSummary() const override { return resultMsg; }

private:
    GuessStage stage;
    int playerChoice;    // 0: 剪刀, 1: 石头, 2: 布
    int petChoice;       // 0: 剪刀, 1: 石头, 2: 布
    int roundNum;        // 第几回合 (1, 2, 3...)
    int playerScore;     // 主人胜场
    int petScore;        // 企鹅胜场
    int lastRoundWinner; // 0: 平局, 1: 主人胜, 2: 企鹅胜
    uint32_t stageStartTime;
    bool finished;
    String resultMsg;
};
