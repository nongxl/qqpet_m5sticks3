#pragma once
#include "mini_game_base.h"
#include <vector>

struct SkiObstacle {
    float x;
    float y;
    int type; // 0: 松树, 1: 雪人, 2: 金币
};

class GameSkiing : public MiniGameBase {
public:
    GameSkiing();
    void init() override;
    void update(float tiltX, float tiltY, float accelZ) override;
    void render(M5Canvas& canvas) override;
    void onBtnA() override;
    void onBtnB() override;
    bool isFinished() const override { return finished; }
    String getResultSummary() const override { return resultMsg; }

private:
    float playerX;
    float playerY;
    float speed;
    float distance;
    int coinsCollected;
    bool isJumping;
    float jumpProgress; // 0.0 ~ 1.0
    bool isCrashed;
    uint32_t crashTime;
    bool finished;
    String resultMsg;
    std::vector<SkiObstacle> obstacles;
    uint32_t lastSpawnTime;
};
