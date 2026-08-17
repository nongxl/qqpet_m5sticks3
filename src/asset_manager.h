#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include <M5GFX.h>
#include <vector>
#include "pet_core.h"

#include <map>

struct InMemoryFrame {
    std::vector<uint8_t> buffer;
};

struct ActionClip {
    std::vector<InMemoryFrame> frames;
    uint8_t fps;
};

class AssetManager {
public:
    AssetManager();
    bool begin();

    // 绘制 135x240 精选原版背景壁纸 (bgId: 0 为默认极简天蓝, 1~16 为原版壁纸)
    void drawBackground(M5Canvas& canvas, uint8_t bgId);

    // 绘制企鹅当前动作帧 (支持 GG/MM 双性别、Egg/Kid/Adult 三段式生命演化、多套玩耍与打工动画)
    void drawPetFrame(M5Canvas& canvas, int x, int y, PetAnimState anim, uint8_t gender, int level, uint32_t currentMillis);

    // 绘制菜单图标
    void drawMenuIcon(M5Canvas& canvas, int x, int y, int optionIndex, bool active);

    // 预热加载当前阶段核心动作到 PSRAM
    void preloadCoreActions(uint8_t gender, int level);

private:
    bool isFsMounted;
    
    // PSRAM 多动作高速缓存池 (彻底消除切换动作时的 SPI Flash I/O 阻塞与卡顿)
    std::map<String, ActionClip> actionCache;

    // 当前缓存的背景壁纸
    uint8_t currentLoadedBgId;
    InMemoryFrame currentBgFrame;

    // 菜单图标内存缓存
    std::vector<InMemoryFrame> menuIconsNorm;
    std::vector<InMemoryFrame> menuIconsAct;
    bool iconsLoaded;

    ActionClip* getOrLoadActionClip(const String& actionName, uint8_t gender, const String& stage, uint8_t fps);
    void loadMenuIcons();
    String getActionNameByState(PetAnimState anim, const String& stage, uint8_t& outFps);
};



extern AssetManager g_assets;
