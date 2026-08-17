#pragma once
#include "mini_game_base.h"
#include <vector>

struct JumperPlatform {
    float x;
    float y;
    float w;
    int type; // 0: 普通云朵, 1: 移动云朵, 2: 弹簧超弹云朵
    float vx;
};

class GameJumper : public MiniGameBase {
public:
    GameJumper();
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
    float playerVy;
    float cameraY;
    float maxHeight;
    bool canGlide; // 是否还有一次翅膀滑翔救援
    bool isGameOver;
    uint32_t gameOverTime;
    bool finished;
    String resultMsg;
    std::vector<JumperPlatform> platforms;
    float highestSpawnY;
};
