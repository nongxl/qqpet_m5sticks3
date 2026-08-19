#pragma once
#include "mini_game_base.h"

class GameRope : public MiniGameBase {
public:
    GameRope();
    void init() override;
    void update(float tiltX, float tiltY, float accelZ) override;
    void render(M5Canvas& canvas) override;
    void onBtnA() override;
    void onBtnB() override;
    bool isFinished() const override { return finished; }
    String getResultSummary() const override { return resultMsg; }

private:
    float ropeAngle;     // 跳绳旋转角度 (0 ~ 2*PI)
    float ropeSpeed;     // 摇绳旋转角速度
    float petJumpY;      // 企鹅跳跃高度 (0 ~ 30)
    float petJumpVy;     // 企鹅跳跃速度
    bool isJumping;
    int jumpCount;       // 成功连续跳绳次数
    int maxJumps;
    int lives;
    String feedback;
    uint32_t feedbackTime;
    bool finished;
    String resultMsg;
    uint32_t gameOverTime;
};
