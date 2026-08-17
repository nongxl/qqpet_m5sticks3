#pragma once
#include "mini_game_base.h"
#include <vector>

struct FallingItem {
    float x;
    float y;
    float vy;
    int type; // 0: 元宝, 1: 小鱼干, 2: 幸运彩星, 3: 黑色炸弹
};

class GameCatcher : public MiniGameBase {
public:
    GameCatcher();
    void init() override;
    void update(float tiltX, float tiltY, float accelZ) override;
    void render(M5Canvas& canvas) override;
    void onBtnA() override;
    void onBtnB() override;
    bool isFinished() const override { return finished; }
    String getResultSummary() const override { return resultMsg; }

private:
    float playerX;
    int lives;
    int score;
    int coinsGained;
    uint32_t gameStartTime;
    uint32_t totalDurationMs;
    bool isShieldActive;
    uint32_t shieldEndTime;
    bool isGameOver;
    uint32_t gameOverTime;
    bool finished;
    String resultMsg;
    std::vector<FallingItem> items;
    uint32_t lastDropTime;
};
