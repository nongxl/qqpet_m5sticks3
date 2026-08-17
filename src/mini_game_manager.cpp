#include "mini_game_manager.h"
#include "game_fishing.h"
#include "game_skiing.h"
#include "game_catcher.h"
#include "game_jumper.h"
#include "game_miner.h"
#include "display_engine.h"
#include "haptics.h"


extern DisplayEngine g_display;
extern HapticsEngine g_haptics;

MiniGameManager::MiniGameManager()
    : currentType(GAME_NONE), currentGame(nullptr), selectedMenuIndex(0) {
}

void MiniGameManager::init() {
    currentType = GAME_NONE;
    currentGame = nullptr;
    selectedMenuIndex = 0;
}

void MiniGameManager::selectGame(MiniGameType type) {
    currentType = type;
    if (type == GAME_FISHING) {
        currentGame = std::unique_ptr<MiniGameBase>(new GameFishing());
    } else if (type == GAME_SKIING) {
        currentGame = std::unique_ptr<MiniGameBase>(new GameSkiing());
    } else if (type == GAME_CATCHER) {
        currentGame = std::unique_ptr<MiniGameBase>(new GameCatcher());
    } else if (type == GAME_JUMPER) {
        currentGame = std::unique_ptr<MiniGameBase>(new GameJumper());
    } else if (type == GAME_MINER) {
        currentGame = std::unique_ptr<MiniGameBase>(new GameMiner());
    } else {
        currentGame = nullptr;
    }
}

void MiniGameManager::stopGame() {
    if (currentGame) {
        String summary = currentGame->getResultSummary();
        if (summary.length() > 0) {
            g_display.showToast(summary.c_str());
        }
    }
    currentGame = nullptr;
    currentType = GAME_NONE;
}

void MiniGameManager::update(float tiltX, float tiltY, float accelZ) {
    if (currentGame) {
        currentGame->update(tiltX, tiltY, accelZ);
        if (currentGame->isFinished()) {
            stopGame();
        }
    }
}

void MiniGameManager::render(M5Canvas& canvas) {
    if (currentGame) {
        currentGame->render(canvas);
        return;
    }

    // 渲染游戏厅选择大厅 (135x240 竖屏卡片)
    int SCREEN_W = 135;
    int SCREEN_H = 240;

    canvas.fillScreen(canvas.color565(18, 24, 38));

    // 顶部标题栏
    canvas.fillRoundRect(3, 3, SCREEN_W - 6, 26, 4, canvas.color565(28, 42, 68));
    canvas.setFont(&fonts::efontCN_12);
    canvas.setTextSize(1);
    canvas.setTextColor(canvas.color565(255, 215, 60));
    canvas.drawCenterString("🎮 Q宠体感游戏厅", SCREEN_W / 2, 9);

    const char* gameTitles[] = {
        "🎣 湖畔钓鱼",
        "⛷️ 极速滑雪",
        "🪙 摘果接宝",
        "☁️ 步步高升",
        "⛏️ 黄金矿工",
        "🔙 退出大厅"
    };

    const char* gameDescs[] = {
        "咬钩急震 + 抬手扬竿起鱼",
        "倾斜变道 + BtnA跳跃避障",
        "倾斜滑行接元宝 + 防炸弹",
        "倾斜微调落点 + 翅膀滑翔",
        "瞄准摆角 + 射爪抓大金块",
        "返回待机主界面"
    };

    int cardY = 35;
    int cardH = 30;

    for (int i = 0; i < 6; ++i) {
        bool isSel = (i == selectedMenuIndex);
        uint16_t bgCol = isSel ? canvas.color565(255, 235, 120) : canvas.color565(28, 38, 56);
        uint16_t borderCol = isSel ? canvas.color565(255, 180, 0) : canvas.color565(45, 60, 85);
        uint16_t titleCol = isSel ? canvas.color565(30, 20, 0) : TFT_WHITE;
        uint16_t descCol = isSel ? canvas.color565(120, 70, 0) : canvas.color565(140, 160, 180);

        int curY = cardY + i * (cardH + 4);
        canvas.fillRoundRect(6, curY, SCREEN_W - 12, cardH, 4, bgCol);
        canvas.drawRoundRect(6, curY, SCREEN_W - 12, cardH, 4, borderCol);

        canvas.setFont(&fonts::efontCN_12);
        canvas.setTextColor(titleCol);
        canvas.drawString(gameTitles[i], 12, curY + 4);

        canvas.setFont(&fonts::efontCN_10);
        canvas.setTextColor(descCol);
        canvas.drawString(gameDescs[i], 12, curY + 18);
    }
}

void MiniGameManager::onBtnA() {
    if (currentGame) {
        currentGame->onBtnA();
        return;
    }
    // 在大厅按 BtnA 确认启动游戏
    g_haptics.trigger(HAPTIC_SUCCESS);
    if (selectedMenuIndex >= 0 && selectedMenuIndex < 5) {
        selectGame(static_cast<MiniGameType>(selectedMenuIndex + 1));
    } else {
        // 退出大厅
        g_display.closeSubScreen();
    }
}

void MiniGameManager::onBtnB() {
    if (currentGame) {
        currentGame->onBtnB();
        return;
    }
    // 在大厅按 BtnB 切换下一项
    nextMenuIndex();
    g_haptics.trigger(HAPTIC_CLICK);
}

void MiniGameManager::nextMenuIndex() {
    selectedMenuIndex = (selectedMenuIndex + 1) % 6;
}

void MiniGameManager::prevMenuIndex() {
    selectedMenuIndex = (selectedMenuIndex + 5) % 6;
}
