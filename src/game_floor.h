#pragma once
#include "mini_game_base.h"
#include <vector>

struct CloudPlatform {
    float x, y;
    float w;
    int type; // 0: 普通云朵, 1: 弹簧云, 2: 尖刺云 (扣血)
};

class GameFloor : public MiniGameBase {
public:
    GameFloor();
    void init() override;
    void update(float tiltX, float tiltY, float accelZ) override;
    void render(M5Canvas& canvas) override;
    void onBtnA() override;
    void onBtnB() override;
    bool isFinished() const override { return finished; }
    String getResultSummary() const override { return resultMsg; }

private:
    float petX, petY;
    float vy;
    int currentFloor;
    int maxFloor;
    int lives;
    std::vector<CloudPlatform> platforms;
    uint32_t lastSpawnY;
    bool finished;
    String resultMsg;
    uint32_t gameOverTime;
};
