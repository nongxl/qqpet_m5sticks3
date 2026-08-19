#include "mini_game_manager.h"
#include "game_fishing.h"
#include "game_skiing.h"
#include "game_catcher.h"
#include "game_jumper.h"
#include "game_miner.h"
#include "game_guess.h"
#include "game_ball.h"
#include "game_floor.h"
#include "game_rope.h"
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

bool MiniGameManager::isGameEnabled(MiniGameType type) const {
    return (type == GAME_FISHING || type == GAME_MINER);
}

void MiniGameManager::selectGame(MiniGameType type) {
    if (!isGameEnabled(type)) {
        g_display.showToast("该游戏正在开发中~");
        g_haptics.trigger(HAPTIC_ALERT);
        return;
    }

    currentType = type;
    if (type == GAME_FISHING) {
        currentGame = std::unique_ptr<MiniGameBase>(new GameFishing());
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

    // 渲染游戏厅选择大厅 (135x240 竖屏 4 卡片视口滚动)
    int SCREEN_W = 135;
    int SCREEN_H = 240;

    canvas.fillScreen(canvas.color565(18, 24, 38));

    // 1. 顶部标题栏
    canvas.fillRoundRect(4, 4, SCREEN_W - 8, 22, 4, canvas.color565(28, 42, 68));
    canvas.setFont(&fonts::efontCN_12);
    canvas.setTextSize(1);
    canvas.setTextColor(canvas.color565(255, 215, 60));
    canvas.drawString("Q宠互动游戏厅", 8, 7);
    canvas.setTextColor(TFT_WHITE);
    canvas.drawRightString(String(selectedMenuIndex + 1) + "/10", SCREEN_W - 8, 7);

    const char* gameTitles[] = {
        "湖畔钓鱼",
        "极速滑雪",
        "摘果接宝",
        "步步高升",
        "黄金矿工",
        "企鹅猜拳",
        "拍皮球乐",
        "下100层",
        "节奏跳绳",
        "[ 退出大厅 ]"
    };

    const char* gameDescs[] = {
        "咬钩急震 + 抬手扬竿起鱼",
        "倾斜变道 + BtnA跳跃避障",
        "倾斜滑行接元宝 + 防炸弹",
        "倾斜微调落点 + 翅膀滑翔",
        "瞄准摆角 + 射爪抓大金块",
        "经典剪刀石头布 赢取元宝",
        "节奏抓准时机 向上颠皮球",
        "重力感应倾斜踩云 躲避尖刺",
        "绳子扫过脚底时 抓准起跳",
        "返回待机主界面"
    };

    int totalGames = 10;
    int cardH = 40;
    int visibleCount = 4;
    int startIdx = 0;
    if (selectedMenuIndex >= visibleCount) {
        startIdx = selectedMenuIndex - visibleCount + 1;
    }
    if (startIdx + visibleCount > totalGames) {
        startIdx = std::max(0, totalGames - visibleCount);
    }

    int startY = 30;
    for (int i = 0; i < visibleCount && (startIdx + i) < totalGames; ++i) {
        int idx = startIdx + i;
        int curY = startY + i * (cardH + 3);
        bool isSel = (idx == selectedMenuIndex);

        int boxX = 4;
        int boxW = SCREEN_W - 8;

        if (isSel) {
            canvas.fillRoundRect(boxX, curY, boxW, cardH, 5, canvas.color565(255, 250, 225));
            canvas.drawRoundRect(boxX, curY, boxW, cardH, 5, canvas.color565(255, 170, 0));
            canvas.drawRoundRect(boxX + 1, curY + 1, boxW - 2, cardH - 2, 4, canvas.color565(255, 215, 80));
        } else {
            canvas.fillRoundRect(boxX, curY, boxW, cardH, 5, canvas.color565(28, 38, 56));
            canvas.drawRoundRect(boxX, curY, boxW, cardH, 5, canvas.color565(45, 60, 85));
        }

        if (idx == totalGames - 1) {
            canvas.setTextColor(isSel ? canvas.color565(220, 40, 40) : canvas.color565(140, 160, 180));
            canvas.drawCenterString(gameTitles[idx], SCREEN_W / 2, curY + 13);
            continue;
        }

        bool enabled = isGameEnabled(static_cast<MiniGameType>(idx + 1));

        // 第一行 (Y+4): 游戏名称 (左) + 状态 (右)
        if (enabled) {
            canvas.setTextColor(isSel ? canvas.color565(20, 40, 80) : TFT_WHITE);
            canvas.drawString(gameTitles[idx], boxX + 6, curY + 4);

            canvas.setTextColor(isSel ? canvas.color565(210, 120, 0) : canvas.color565(80, 180, 255));
            canvas.drawRightString("开放中", boxX + boxW - 6, curY + 4);
        } else {
            canvas.setTextColor(isSel ? canvas.color565(120, 120, 120) : canvas.color565(90, 100, 115));
            canvas.drawString(gameTitles[idx], boxX + 6, curY + 4);

            canvas.setTextColor(isSel ? canvas.color565(160, 160, 160) : canvas.color565(80, 90, 105));
            canvas.drawRightString("暂未开放", boxX + boxW - 6, curY + 4);
        }

        // 第二行 (Y+22): 玩法简述
        if (enabled) {
            canvas.setTextColor(isSel ? canvas.color565(180, 80, 0) : canvas.color565(140, 160, 180));
        } else {
            canvas.setTextColor(isSel ? canvas.color565(150, 150, 150) : canvas.color565(75, 85, 100));
        }
        canvas.drawString(gameDescs[idx], boxX + 6, curY + 22);
    }


    // 底部操作指引
    int botY = SCREEN_H - 24;
    canvas.fillRoundRect(8, botY, SCREEN_W - 16, 20, 4, canvas.color565(14, 20, 32));
    canvas.drawRoundRect(8, botY, SCREEN_W - 16, 20, 4, canvas.color565(60, 85, 125));
    canvas.setTextColor(canvas.color565(255, 220, 80));
    canvas.drawCenterString("B切换 | A进入游戏", SCREEN_W / 2, botY + 4);
}

void MiniGameManager::onBtnA() {
    if (currentGame) {
        currentGame->onBtnA();
        return;
    }
    // 在大厅按 BtnA 确认启动游戏
    g_haptics.trigger(HAPTIC_SUCCESS);
    if (selectedMenuIndex >= 0 && selectedMenuIndex < 9) {
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
    selectedMenuIndex = (selectedMenuIndex + 1) % 10;
}

void MiniGameManager::prevMenuIndex() {
    selectedMenuIndex = (selectedMenuIndex + 9) % 10;
}


