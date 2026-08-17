#pragma once
#include "mini_game_base.h"
#include <vector>

struct MineItem {
    float x;
    float y;
    float radius;
    int value;
    float weight; // 重量，影响拉回速度 (0.4 ~ 2.0)
    int type;     // 0: 大金块, 1: 小金块, 2: 璀璨钻石, 3: 大石头, 4: 神秘宝箱
};

enum ClawState {
    CLAW_SWINGING = 0, // 左右钟摆瞄准
    CLAW_EXTENDING,    // 射出延伸
    CLAW_RETRACTING    // 抓取拉回
};

class GameMiner : public MiniGameBase {
public:
    GameMiner();
    void init() override;
    void update(float tiltX, float tiltY, float accelZ) override;
    void render(M5Canvas& canvas) override;
    void onBtnA() override;
    void onBtnB() override;
    bool isFinished() const override { return finished; }
    String getResultSummary() const override { return resultMsg; }

private:
    float clawAngle;     // 弧度
    float swingSpeed;
    float clawLength;
    ClawState clawState;
    int grabbedItemIdx;  // -1 为空抓
    int score;
    int totalGold;
    uint32_t gameStartTime;
    uint32_t durationMs;
    bool isGameOver;
    uint32_t gameOverTime;
    bool finished;
    String resultMsg;
    std::vector<MineItem> items;
};
