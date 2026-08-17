#pragma once
#include "mini_game_base.h"

enum FishingStage {
    FISH_READY = 0,   // 等待抛竿
    FISH_WAITING,     // 浮标水面荡漾等待
    FISH_BITING,      // 咬钩！急促震动警报，等待扬竿起钩
    FISH_PULLING,     // 溜鱼收线博弈
    FISH_CAUGHT,      // 成功钓起战利品
    FISH_ESCAPED      // 脱钩逃跑
};

class GameFishing : public MiniGameBase {
public:
    GameFishing();
    void init() override;
    void update(float tiltX, float tiltY, float accelZ) override;
    void render(M5Canvas& canvas) override;
    void onBtnA() override;
    void onBtnB() override;
    bool isFinished() const override { return finished; }
    String getResultSummary() const override { return resultMsg; }

private:
    FishingStage stage;
    uint32_t stateStartTime;
    uint32_t biteTimeout;
    float lineProgress; // 溜鱼拉力进度 (0.0 ~ 1.0)
    int fishType;       // 0: 小鱼干, 1: 鲜嫩三文鱼, 2: 黄金元宝箱, 3: 破旧水鞋
    int score;
    bool finished;
    String resultMsg;
    float waterWave;
};
