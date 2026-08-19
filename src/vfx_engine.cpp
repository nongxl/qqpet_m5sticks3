#include "vfx_engine.h"
#include "pet_core.h"
#include "sound_manager.h"
#include "haptics.h"

extern PetCore g_pet;
extern HapticsEngine g_haptics;

VfxEngine::VfxEngine()
    : dailyChestActive(false), dailyChestOpened(false), chestOpenTime(0),
      reliveAuraEndTime(0), reliveAuraX(0), reliveAuraY(0) {
}

void VfxEngine::init() {
    particles.clear();
    dailyChestActive = false;
    dailyChestOpened = false;
    chestOpenTime = 0;
    reliveAuraEndTime = 0;
}

void VfxEngine::spawnHearts(float x, float y, int count) {
    for (int i = 0; i < count; ++i) {
        Particle p;
        p.x = x + random(-15, 16);
        p.y = y + random(-10, 10);
        p.vx = (random(-10, 11) / 10.0f) * 0.6f;
        p.vy = - (0.8f + random(5, 15) / 10.0f); // 向上漂浮
        p.life = 25 + random(0, 15);
        p.maxLife = p.life;
        p.type = 0; // 爱心
        p.color = (random(0, 2) == 0) ? 0xF9B4 : 0xFD14; // 粉红/粉紫
        p.size = 5.0f;
        particles.push_back(p);
    }
}

void VfxEngine::spawnStars(float x, float y, int count) {
    for (int i = 0; i < count; ++i) {
        Particle p;
        p.x = x + random(-20, 21);
        p.y = y + random(-15, 15);
        p.vx = (random(-15, 16) / 10.0f);
        p.vy = (random(-15, 16) / 10.0f) - 0.5f;
        p.life = 20 + random(0, 12);
        p.maxLife = p.life;
        p.type = 1; // 星星
        p.color = 0xFFE0; // 金黄闪耀
        p.size = 4.0f;
        particles.push_back(p);
    }
}

void VfxEngine::spawnCoinBurst(float x, float y, int count) {
    for (int i = 0; i < count; ++i) {
        Particle p;
        p.x = x;
        p.y = y;
        float angle = (i * 2.0f * PI / count) + (random(-2, 3) * 0.1f);
        float speed = 2.0f + random(10, 30) / 10.0f;
        p.vx = cos(angle) * speed;
        p.vy = sin(angle) * speed - 1.5f; // 向上抛出
        p.life = 35 + random(0, 15);
        p.maxLife = p.life;
        p.type = 2; // 金币
        p.color = 0xFEA0; // 亮金色
        p.size = 6.0f;
        particles.push_back(p);
    }
}

void VfxEngine::spawnBubbles(float x, float y, int count) {
    for (int i = 0; i < count; ++i) {
        Particle p;
        p.x = x + random(-35, 36);
        p.y = y + random(-15, 25);
        p.vx = (random(-10, 11) / 10.0f) * 0.5f;
        p.vy = - (0.6f + random(4, 12) / 10.0f); // 向上轻盈漂浮
        p.life = 35 + random(0, 20);
        p.maxLife = p.life;
        p.type = 4; // 五彩肥皂泡泡
        p.color = (random(0, 3) == 0) ? 0x867F : ((random(0, 2) == 0) ? 0xB57F : 0xDF7F); // 炫彩透明蓝粉紫
        p.size = 3.0f + random(0, 4); // 泡泡半径 3~6px
        particles.push_back(p);
    }
}

void VfxEngine::spawnRain(int count) {
    for (int i = 0; i < count; ++i) {
        Particle p;
        p.x = random(0, 135);
        p.y = random(0, 40);
        p.vx = -0.6f; // 斜向雨丝
        p.vy = 3.5f + random(10, 25) / 10.0f;
        p.life = 40;
        p.maxLife = p.life;
        p.type = 5; // 雨丝
        p.color = 0xAD7F; // 柔和蓝白色
        p.size = 4.0f;
        particles.push_back(p);
    }
}

void VfxEngine::spawnSnow(int count) {
    for (int i = 0; i < count; ++i) {
        Particle p;
        p.x = random(0, 135);
        p.y = random(-10, 20);
        p.vx = (random(-10, 11) / 10.0f) * 0.4f;
        p.vy = 0.8f + random(2, 8) / 10.0f;
        p.life = 60 + random(0, 20);
        p.maxLife = p.life;
        p.type = 6; // 雪花
        p.color = TFT_WHITE;
        p.size = 2.0f;
        particles.push_back(p);
    }
}

void VfxEngine::spawnTears(float x, float y, int count) {
    for (int i = 0; i < count; ++i) {
        Particle p;
        p.x = x + random(-8, 9);
        p.y = y + random(-4, 5);
        p.vx = - (1.2f + random(5, 15) / 10.0f); // 向左下飙泪
        p.vy = 1.0f + random(5, 15) / 10.0f;
        p.life = 25 + random(0, 10);
        p.maxLife = p.life;
        p.type = 7; // 泪花
        p.color = 0x5DFF; // 亮水蓝
        p.size = 3.0f;
        particles.push_back(p);
    }
}

void VfxEngine::triggerReliveAura(float x, float y) {
    reliveAuraEndTime = millis() + 2500;
    reliveAuraX = x;
    reliveAuraY = y;
}



#include "storage_manager.h"
#include <time.h>

void VfxEngine::showDailyChest() {
    const PetState& st = g_pet.getState();
    time_t nowSec = time(nullptr);
    uint32_t currentDay = (nowSec > 1700000000) ? (uint32_t)(nowSec / 86400) : ((nowSec > 100000) ? (uint32_t)(nowSec / 86400) : 1);
    
    // 如果今天已经签到开启过宝箱，则开机不重复弹出
    if (st.last_signin_day != 0 && st.last_signin_day == currentDay) {
        dailyChestActive = false;
        return;
    }

    dailyChestActive = true;
    dailyChestOpened = false;
    chestOpenTime = 0;
}

void VfxEngine::openDailyChest() {
    if (!dailyChestActive || dailyChestOpened) return;
    dailyChestOpened = true;
    chestOpenTime = millis();

    // 奖励元宝与心情 (修正为 1000 上限)
    PetState& st = const_cast<PetState&>(g_pet.getState());
    st.coins += 50;
    st.mood = std::min(1000, st.mood + 300);
    g_pet.addGrowth(30.0f);

    time_t nowSec = time(nullptr);
    st.last_signin_day = (nowSec > 1700000000) ? (uint32_t)(nowSec / 86400) : ((nowSec > 100000) ? (uint32_t)(nowSec / 86400) : 1);
    g_storage.savePetState(st);

    // 喷涌大量金币与星光粒子
    spawnCoinBurst(67, 120, 16);
    spawnStars(67, 120, 8);
    g_haptics.trigger(HAPTIC_LEVELUP);
    SoundManager::getInstance().playSound(SOUND_COIN);
}


void VfxEngine::closeDailyChest() {
    dailyChestActive = false;
    dailyChestOpened = false;
}

void VfxEngine::update() {
    // 更新粒子状态
    for (auto it = particles.begin(); it != particles.end(); ) {
        it->x += it->vx;
        it->y += it->vy;
        it->vy += 0.05f; // 轻微重力
        it->life--;

        if (it->life <= 0) {
            it = particles.erase(it);
        } else {
            ++it;
        }
    }

    if (dailyChestOpened && (millis() - chestOpenTime > 3000)) {
        closeDailyChest();
    }
}

void VfxEngine::render(M5Canvas& canvas) {
    int SCREEN_W = 135;
    int SCREEN_H = 240;

    // 1. 渲染神圣起死回生金光光柱法阵
    uint32_t now = millis();
    if (now < reliveAuraEndTime) {
        int auraProgress = (reliveAuraEndTime - now);
        float alphaWave = sin((now % 400) * PI / 200.0f);

        // 旋转金光法阵光环 (围绕企鹅底部)
        int auraY = (int)reliveAuraY + 40;
        int auraW = 42 + (int)(alphaWave * 6.0f);
        canvas.drawEllipse((int)reliveAuraX, auraY, auraW, 10, canvas.color565(255, 230, 80));
        canvas.drawEllipse((int)reliveAuraX, auraY, auraW - 2, 8, canvas.color565(255, 180, 0));

        // 升腾金光光柱线
        for (int i = -3; i <= 3; ++i) {
            int lineX = (int)reliveAuraX + i * 8;
            int lineH = 60 + (int)(sin(now * 0.01f + i) * 20.0f);
            canvas.drawFastVLine(lineX, auraY - lineH, lineH, canvas.color565(255, 240, 150));
        }

        // 飘散金色仙气粒子
        if (random(0, 3) == 0) {
            spawnStars(reliveAuraX, reliveAuraY + 20, 2);
        }
    }

    // 2. 渲染飘浮粒子 (爱心、星星、金币)
    for (const auto& p : particles) {
        int px = (int)p.x;
        int py = (int)p.y;
        if (px < -10 || px > SCREEN_W + 10 || py < -10 || py > SCREEN_H + 10) continue;

        if (p.type == 0) {
            // 粉红爱心
            canvas.fillCircle(px - 2, py - 1, 2, p.color);
            canvas.fillCircle(px + 2, py - 1, 2, p.color);
            canvas.fillTriangle(px - 4, py - 1, px + 4, py - 1, px, py + 4, p.color);
        } else if (p.type == 1) {
            // 金色四芒星
            canvas.drawPixel(px, py, TFT_WHITE);
            canvas.drawPixel(px - 1, py, p.color);
            canvas.drawPixel(px + 1, py, p.color);
            canvas.drawPixel(px, py - 1, p.color);
            canvas.drawPixel(px, py + 1, p.color);
        } else if (p.type == 2) {
            // 金元宝/金币
            canvas.fillCircle(px, py, 4, p.color);
            canvas.drawCircle(px, py, 4, canvas.color565(200, 120, 0));
            canvas.drawPixel(px, py, canvas.color565(255, 255, 100)); // 中心高光
        } else if (p.type == 4) {
            // 五彩肥皂泡泡 (七彩透明光环 + 晶莹高光)
            int r = (int)p.size;
            canvas.drawCircle(px, py, r, p.color);
            canvas.drawPixel(px - r / 2, py - r / 2, TFT_WHITE);
            canvas.drawPixel(px - r / 2 + 1, py - r / 2, TFT_WHITE);
        } else if (p.type == 5) {
            // 雨丝 (斜向细线)
            canvas.drawLine(px, py, px - 2, py + 5, p.color);
        } else if (p.type == 6) {
            // 雪花 (白色柔美十字雪花点)
            canvas.drawPixel(px, py, TFT_WHITE);
            canvas.drawPixel(px - 1, py, canvas.color565(210, 235, 255));
            canvas.drawPixel(px + 1, py, canvas.color565(210, 235, 255));
            canvas.drawPixel(px, py - 1, canvas.color565(210, 235, 255));
            canvas.drawPixel(px, py + 1, canvas.color565(210, 235, 255));
        } else if (p.type == 7) {
            // 泪水 (蓝色泪滴)
            canvas.fillCircle(px, py, 2, p.color);
            canvas.drawPixel(px, py - 2, TFT_WHITE);
        }
    }



    // 3. 渲染每日签到开宝箱仪式弹窗
    if (dailyChestActive) {
        // 半透明暗色背景幕
        canvas.fillRect(8, 50, SCREEN_W - 16, 140, canvas.color565(20, 30, 48));
        canvas.drawRoundRect(8, 50, SCREEN_W - 16, 140, 6, canvas.color565(255, 215, 60));
        canvas.drawRoundRect(9, 51, SCREEN_W - 18, 138, 5, canvas.color565(200, 160, 40));

        canvas.setFont(&fonts::efontCN_12);
        canvas.setTextSize(1);
        canvas.setTextColor(canvas.color565(255, 220, 80));
        canvas.drawCenterString("🎁 每日签到礼包", SCREEN_W / 2, 58);

        if (!dailyChestOpened) {
            // 待开启的金宝箱
            int bx = SCREEN_W / 2;
            int by = 110;
            canvas.fillRoundRect(bx - 24, by - 16, 48, 36, 4, canvas.color565(210, 140, 0));
            canvas.drawRoundRect(bx - 24, by - 16, 48, 36, 4, canvas.color565(255, 220, 50));
            // 宝箱锁扣
            canvas.fillCircle(bx, by + 2, 4, canvas.color565(255, 240, 120));

            canvas.setTextColor(TFT_WHITE);
            canvas.drawCenterString("按 BtnA 开启宝箱", SCREEN_W / 2, 145);
            canvas.setTextColor(canvas.color565(80, 220, 140));
            canvas.drawCenterString("领 50 元宝 + 心情", SCREEN_W / 2, 165);
        } else {
            // 已开启金光四射
            canvas.setTextColor(canvas.color565(255, 215, 0));
            canvas.drawCenterString("🎉 签到成功！", SCREEN_W / 2, 100);
            canvas.setTextColor(canvas.color565(0, 255, 120));
            canvas.drawCenterString("+50 元宝 / +40 心情", SCREEN_W / 2, 125);
            canvas.setTextColor(canvas.color565(255, 180, 200));
            canvas.drawCenterString("企鹅今天元气满满~", SCREEN_W / 2, 150);
        }
    }
}
