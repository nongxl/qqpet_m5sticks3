#pragma once
#include <Arduino.h>
#include <M5GFX.h>

enum MiniGameType {
    GAME_NONE = 0,
    GAME_FISHING,   // 1. 湖畔钓鱼
    GAME_SKIING,    // 2. 极速滑雪
    GAME_CATCHER,   // 3. 摘果接元宝
    GAME_JUMPER,    // 4. 步步高升跳一跳
    GAME_MINER,     // 5. 黄金矿工
    GAME_COUNT
};

class MiniGameBase {
public:
    virtual ~MiniGameBase() {}
    virtual void init() = 0;
    virtual void update(float tiltX, float tiltY, float accelZ) = 0;
    virtual void render(M5Canvas& canvas) = 0;
    virtual void onBtnA() = 0;
    virtual void onBtnB() = 0;
    virtual bool isFinished() const = 0;
    virtual String getResultSummary() const = 0;
};
