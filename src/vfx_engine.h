#pragma once
#include <Arduino.h>
#include <M5GFX.h>
#include <vector>

struct Particle {
    float x, y;
    float vx, vy;
    int life;
    int maxLife;
    int type; // 0: 爱心, 1: 星星, 2: 金币, 3: 仙气金光
    uint16_t color;
    float size;
};

class VfxEngine {
public:
    static VfxEngine& getInstance() {
        static VfxEngine instance;
        return instance;
    }

    void init();
    void update();
    void render(M5Canvas& canvas);

    // 触发特效
    void spawnHearts(float x, float y, int count = 3);
    void spawnStars(float x, float y, int count = 4);
    void spawnCoinBurst(float x, float y, int count = 12);
    void spawnBubbles(float x, float y, int count = 6);
    void spawnRain(int count = 4);
    void spawnSnow(int count = 3);
    void spawnTears(float x, float y, int count = 6);
    void triggerReliveAura(float x, float y);



    // 每日签到
    bool isDailyChestActive() const { return dailyChestActive; }
    void showDailyChest();
    void openDailyChest();
    void closeDailyChest();

private:
    VfxEngine();
    std::vector<Particle> particles;
    bool dailyChestActive;
    bool dailyChestOpened;
    uint32_t chestOpenTime;
    uint32_t reliveAuraEndTime;
    float reliveAuraX, reliveAuraY;
};
