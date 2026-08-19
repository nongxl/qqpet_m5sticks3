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

class AssetManager {
public:
    AssetManager();
    bool begin();

    // 绘制 135x240 精选原版背景壁纸 (bgId: 0 为默认极简天蓝, 1~16 为原版壁纸)
    void drawBackground(M5Canvas& canvas, uint8_t bgId);

    // 绘制企鹅当前动作帧 (支持 GG/MM 双性别、Egg/Kid/Adult 三段式生命演化、多套玩耍与打工动画)
    void drawPetFrame(M5Canvas& canvas, int x, int y, PetAnimState anim, uint8_t gender, int level, uint32_t currentMillis);

    // 绘制菜单图标 (使用预解码 Sprite 纯内存搬运，0ms 零延迟)
    void drawMenuIcon(M5Canvas& canvas, int x, int y, int optionIndex, bool active);

    // 绘制领养仪式专用双星企鹅 (GG 与 MM 独立内存缓存，零 Flash I/O 冲突，满速 30 FPS 丝滑响应)
    void drawAdoptionPet(M5Canvas& canvas, int x, int y, uint8_t gender, bool active, uint32_t currentMillis);

    // 绘制穿戴饰品 (逐帧锚点贴合律动)
    void drawCostume(M5Canvas& canvas, int costumeId, int petCenterX, int petCenterY, int level, uint32_t currentMillis);


private:
    bool isFsMounted;
    
    // 当前动作序列帧的轻量内存池 (直接内存解码，进行真正 32-bit Alpha 原生混合，彻底消除蛋壳黑边)
    String currentLoadedAction;
    uint8_t currentLoadedGender;
    String currentLoadedStage;
    uint8_t currentClipFps;
    std::vector<InMemoryFrame> currentClipFrames;

    // 领养仪式专属双星缓存
    std::vector<InMemoryFrame> adoptGgFrames;
    std::vector<InMemoryFrame> adoptMmFrames;
    bool adoptFramesLoaded;

    // 当前缓存的背景壁纸
    uint8_t currentLoadedBgId;
    InMemoryFrame currentBgFrame;

    // 预加载 11 款官方原版图标原生 PNG 内存缓存与 Sprite 零拷贝直接渲染
    M5Canvas sprIconsNorm[11];
    M5Canvas sprIconsAct[11];
    bool iconsLoaded;







    void loadActionClip(const String& actionName, uint8_t gender, const String& stage, uint8_t fps);
    void loadMenuIcons();
    String getActionNameByState(PetAnimState anim, const String& stage, uint8_t& outFps);
};




extern AssetManager g_assets;
