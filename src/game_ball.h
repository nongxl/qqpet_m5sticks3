#pragma once
#include "mini_game_base.h"

class GameBall : public MiniGameBase {
public:
    GameBall();
    void init() override;
    void update(float tiltX, float tiltY, float accelZ) override;
    void render(M5Canvas& canvas) override;
    void onBtnA() override;
    void onBtnB() override;
    bool isFinished() const override { return finished; }
    String getResultSummary() const override { return resultMsg; }

private:
    float ballY;         // 皮球 Y 坐标 (30 ~ 210)
    float ballSpeedY;    // 皮球下落/弹起速度
    float ballX;         // 皮球 X 坐标
    int comboCount;      // 连击次数
    int maxCombo;        // 最高连击
    int score;           // 累计得分
    int lives;           // 剩余生命 (3颗心)
    String hitFeedback;  // "PERFECT!" / "GOOD!" / "MISS!"
    uint32_t feedbackTime;
    bool finished;
    String resultMsg;
    uint32_t gameOverTime;
};
