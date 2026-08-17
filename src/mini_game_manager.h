#pragma once
#include "mini_game_base.h"
#include <memory>

class MiniGameManager {
public:
    static MiniGameManager& getInstance() {
        static MiniGameManager instance;
        return instance;
    }

    void init();
    void selectGame(MiniGameType type);
    void stopGame();
    bool isGameRunning() const { return currentGame != nullptr; }
    MiniGameType getCurrentGameType() const { return currentType; }

    void update(float tiltX, float tiltY, float accelZ);
    void render(M5Canvas& canvas);
    void onBtnA();
    void onBtnB();

    int getSelectedMenuIndex() const { return selectedMenuIndex; }
    void nextMenuIndex();
    void prevMenuIndex();

private:
    MiniGameManager();
    MiniGameType currentType;
    std::unique_ptr<MiniGameBase> currentGame;
    int selectedMenuIndex; // 0 ~ 4 对应 5 款游戏
};
