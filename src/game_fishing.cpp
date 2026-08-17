#include "game_fishing.h"
#include "haptics.h"

#include "pet_core.h"
#include "asset_manager.h"

extern PetCore g_pet;
extern HapticsEngine g_haptics;
extern AssetManager g_assets;

GameFishing::GameFishing() {
    init();
}

void GameFishing::init() {
    stage = FISH_READY;
    stateStartTime = millis();
    biteTimeout = 0;
    lineProgress = 0.0f;
    fishType = 0;
    score = 0;
    finished = false;
    resultMsg = "";
    waterWave = 0.0f;
}

void GameFishing::update(float tiltX, float tiltY, float accelZ) {
    if (finished) return;
    uint32_t now = millis();
    waterWave += 0.15f;

    if (stage == FISH_WAITING) {
        if (now >= biteTimeout) {
            stage = FISH_BITING;
            stateStartTime = now;
            g_haptics.trigger(HAPTIC_ALERT); // 咬钩震感警报！
            // 随机鱼类：60% 小鱼干, 25% 鲜嫩三文鱼, 10% 黄金元宝, 5% 破水鞋
            int r = random(0, 100);
            if (r < 60) fishType = 0;
            else if (r < 85) fishType = 1;
            else if (r < 95) fishType = 2;
            else fishType = 3;
        }
    } else if (stage == FISH_BITING) {
        // 持续急促微震提示
        if ((now / 150) % 2 == 0) {
            g_haptics.trigger(HAPTIC_CLICK);
        }
        // 体感上扬提竿 (或者按 BtnA)
        if (accelZ > 1.4f || tiltY < -0.6f) {
            stage = FISH_PULLING;
            lineProgress = 0.35f;
            stateStartTime = now;
            g_haptics.trigger(HAPTIC_SUCCESS);
        } else if (now - stateStartTime > 1800) {
            // 超时脱钩
            stage = FISH_ESCAPED;
            stateStartTime = now;
            g_haptics.trigger(HAPTIC_ALERT);
        }
    } else if (stage == FISH_PULLING) {
        // 阻尼回落
        lineProgress -= 0.008f;
        if (lineProgress <= 0.0f) {
            stage = FISH_ESCAPED;
            stateStartTime = now;
        } else if (lineProgress >= 1.0f) {
            stage = FISH_CAUGHT;
            stateStartTime = now;
            g_haptics.trigger(HAPTIC_LEVELUP);

            // 结算奖励入库
            PetState& st = const_cast<PetState&>(g_pet.getState());
            if (fishType == 0) {
                st.food_count += 3;
                resultMsg = "🎣 成功钓上 [美味小鱼干 x3]！已存入背包，成长+20！";
            } else if (fishType == 1) {
                st.food_salmon += 2;
                resultMsg = "🎉 极品收获！钓上 [鲜嫩三文鱼 x2]！成长+35！";
            } else if (fishType == 2) {
                st.coins += 100;
                resultMsg = "💎 宝箱大丰收！钓起 [沉船金元宝 +100]！成长+50！";
            } else {
                st.coins += 10;
                resultMsg = "👟 钓上了一只 [破旧的水鞋]，换取废铁元宝 +10！";
            }
            g_pet.addGrowth(25.0f);
        }
    } else if (stage == FISH_CAUGHT || stage == FISH_ESCAPED) {
        if (now - stateStartTime > 2800) {
            finished = true;
        }
    }
}

void GameFishing::onBtnA() {
    uint32_t now = millis();
    if (stage == FISH_READY) {
        stage = FISH_WAITING;
        stateStartTime = now;
        biteTimeout = now + random(1800, 4200); // 1.8 ~ 4.2 秒后咬钩
        g_haptics.trigger(HAPTIC_CLICK);
    } else if (stage == FISH_BITING) {
        // 及时按 A 键起竿
        stage = FISH_PULLING;
        lineProgress = 0.35f;
        stateStartTime = now;
        g_haptics.trigger(HAPTIC_SUCCESS);
    } else if (stage == FISH_PULLING) {
        // 连点收线填满进度条
        lineProgress += 0.12f;
        g_haptics.trigger(HAPTIC_CLICK);
    } else if (stage == FISH_CAUGHT || stage == FISH_ESCAPED) {
        finished = true;
    }
}

void GameFishing::onBtnB() {
    // 退出游戏
    finished = true;
    resultMsg = "退出钓鱼。";
}

void GameFishing::render(M5Canvas& canvas) {
    int SCREEN_W = 135;
    int SCREEN_H = 240;

    // 1. 绘制湖畔唯美水天背景 (上蓝天渐变，下碧波微澜)
    canvas.fillScreen(canvas.color565(160, 215, 255));
    // 湖面水域
    int waterY = 110;
    canvas.fillRect(0, waterY, SCREEN_W, SCREEN_H - waterY, canvas.color565(40, 140, 210));
    
    // 水面波光粼粼线条
    for (int y = waterY; y < SCREEN_H; y += 16) {
        int waveOff = static_cast<int>(sin(waterWave + y * 0.1f) * 6.0f);
        canvas.drawFastHLine(0, y, SCREEN_W, canvas.color565(80, 180, 240));
        canvas.fillCircle(40 + waveOff, y + 4, 3, canvas.color565(120, 210, 255));
        canvas.fillCircle(100 - waveOff, y + 10, 2, canvas.color565(120, 210, 255));
    }

    // 2. 绘制岸边栈桥与企鹅
    canvas.fillRect(0, 100, 52, 14, canvas.color565(150, 100, 60)); // 木栈桥
    canvas.drawRect(0, 100, 52, 14, canvas.color565(110, 70, 40));

    // 企鹅站在桥上
    const PetState& st = g_pet.getState();
    PetAnimState anim = (stage == FISH_PULLING) ? ANIM_WORK : ((stage == FISH_CAUGHT) ? ANIM_HAPPY : ANIM_IDLE_STAND);
    g_assets.drawPetFrame(canvas, -20, 56, anim, st.gender, g_pet.getLevel(), millis());

    // 3. 绘制鱼竿与鱼线
    int rodStartX = 28;
    int rodStartY = 96;
    int rodTipX = 64;
    int rodTipY = 68;
    canvas.drawLine(rodStartX, rodStartY, rodTipX, rodTipY, canvas.color565(80, 50, 20)); // 鱼竿

    // 浮标位置
    int floatX = 96;
    int floatY = waterY + 12;
    if (stage == FISH_BITING) {
        floatY += ((millis() / 80) % 2 == 0) ? 8 : -4; // 激烈抖动
    } else if (stage == FISH_WAITING) {
        floatY += static_cast<int>(sin(waterWave) * 3.0f);
    } else if (stage == FISH_READY) {
        floatX = rodTipX;
        floatY = rodTipY + 20;
    }

    // 鱼线
    canvas.drawLine(rodTipX, rodTipY, floatX, floatY, TFT_WHITE);

    // 浮标 (红白相间小浮漂)
    if (stage != FISH_READY) {
        canvas.fillCircle(floatX, floatY, 4, TFT_RED);
        canvas.fillCircle(floatX, floatY + 3, 3, TFT_WHITE);
    }

    // 4. 界面提示与状态反馈
    canvas.setFont(&fonts::efontCN_12);
    canvas.setTextSize(1);

    // 顶部标题横幅
    canvas.fillRoundRect(3, 3, SCREEN_W - 6, 22, 4, canvas.color565(20, 50, 90));
    canvas.setTextColor(canvas.color565(255, 220, 80));
    canvas.drawCenterString("🎣 湖畔悠闲钓鱼", SCREEN_W / 2, 7);

    if (stage == FISH_READY) {
        canvas.fillRoundRect(8, 170, SCREEN_W - 16, 44, 6, canvas.color565(248, 252, 255));
        canvas.drawRoundRect(8, 170, SCREEN_W - 16, 44, 6, canvas.color565(120, 180, 240));
        canvas.setTextColor(canvas.color565(30, 70, 130));
        canvas.drawCenterString("按【BtnA】抛竿入水", SCREEN_W / 2, 176);
        canvas.setTextColor(canvas.color565(120, 140, 160));
        canvas.drawCenterString("侧键BtnB退出", SCREEN_W / 2, 194);
    } else if (stage == FISH_WAITING) {
        canvas.setTextColor(TFT_WHITE);
        canvas.drawCenterString("水面静悄悄... 静待大鱼", SCREEN_W / 2, 185);
    } else if (stage == FISH_BITING) {
        canvas.fillRoundRect(8, 165, SCREEN_W - 16, 50, 6, canvas.color565(255, 240, 220));
        canvas.drawRoundRect(8, 165, SCREEN_W - 16, 50, 6, TFT_RED);
        canvas.setTextColor(TFT_RED);
        canvas.drawCenterString("💥 咬钩啦！有大鱼！", SCREEN_W / 2, 172);
        canvas.setTextColor(canvas.color565(200, 90, 0));
        canvas.drawCenterString("🔥 猛抬手腕 或 按BtnA起竿！", SCREEN_W / 2, 192);
    } else if (stage == FISH_PULLING) {
        // 溜鱼拉力条
        canvas.fillRoundRect(10, 165, SCREEN_W - 20, 52, 6, canvas.color565(245, 250, 255));
        canvas.drawRoundRect(10, 165, SCREEN_W - 20, 52, 6, canvas.color565(30, 140, 240));
        canvas.setTextColor(canvas.color565(20, 60, 120));
        canvas.drawCenterString("⚡ 连点【BtnA】收线！", SCREEN_W / 2, 170);

        int barW = static_cast<int>(lineProgress * 105.0f);
        if (barW > 105) barW = 105;
        if (barW < 0) barW = 0;
        canvas.fillRoundRect(15, 192, 105, 12, 4, canvas.color565(210, 225, 235));
        canvas.fillRoundRect(15, 192, barW, 12, 4, (lineProgress > 0.7f) ? canvas.color565(40, 180, 80) : canvas.color565(255, 150, 0));
    } else if (stage == FISH_CAUGHT) {
        canvas.fillRoundRect(6, 155, SCREEN_W - 12, 65, 6, canvas.color565(255, 250, 225));
        canvas.drawRoundRect(6, 155, SCREEN_W - 12, 65, 6, canvas.color565(255, 180, 0));
        canvas.setTextColor(canvas.color565(210, 120, 0));
        canvas.drawCenterString("🎉 钓到大收获啦！", SCREEN_W / 2, 162);
        canvas.setTextColor(canvas.color565(20, 120, 40));
        const char* fNames[] = {"[鲜美小鱼干 x3]", "[鲜嫩三文鱼 x2]", "[沉船金元宝 +100]", "[破旧的水鞋]"};
        canvas.drawCenterString(fNames[fishType], SCREEN_W / 2, 182);
        canvas.setTextColor(canvas.color565(120, 130, 140));
        canvas.drawCenterString("按BtnA继续钓鱼", SCREEN_W / 2, 202);
    } else if (stage == FISH_ESCAPED) {
        canvas.fillRoundRect(8, 170, SCREEN_W - 16, 44, 6, canvas.color565(255, 235, 235));
        canvas.drawRoundRect(8, 170, SCREEN_W - 16, 44, 6, TFT_RED);
        canvas.setTextColor(TFT_RED);
        canvas.drawCenterString("哎呀！大鱼脱钩溜走啦~", SCREEN_W / 2, 178);
        canvas.setTextColor(canvas.color565(120, 130, 140));
        canvas.drawCenterString("按BtnA重新抛竿", SCREEN_W / 2, 196);
    }
}
