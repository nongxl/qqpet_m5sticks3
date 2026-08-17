#include "asset_manager.h"

AssetManager g_assets;

static constexpr uint16_t CHROMA_KEY = 0x0001;

AssetManager::AssetManager() 
    : isFsMounted(false),
      currentLoadedGender(255), currentLoadedStage(""), currentClipFps(8),
      adoptFramesLoaded(false), currentLoadedBgId(255), iconsLoaded(false) {}




bool AssetManager::begin() {
    isFsMounted = LittleFS.begin(false);
    if (!isFsMounted) {
        Serial.println("[LittleFS] Mount failed! Formatting...");
        isFsMounted = LittleFS.begin(true);
    }
    if (isFsMounted) {
        Serial.println("[LittleFS] Mounted successfully!");
        loadMenuIcons();
    }
    return isFsMounted;
}


void AssetManager::drawBackground(M5Canvas& canvas, uint8_t bgId) {
    if (bgId == 0 || !isFsMounted) {
        // 0 号默认壁纸：经典桌面柔和淡蓝渐变
        canvas.fillScreen(canvas.color565(225, 242, 255));
        canvas.fillRect(0, 168, 135, 72, canvas.color565(210, 235, 250));
        canvas.drawFastHLine(0, 168, 135, canvas.color565(185, 215, 240));
        return;
    }

    if (bgId > 16) bgId = 16;

    if (currentLoadedBgId != bgId || currentBgFrame.buffer.empty()) {
        char bgPath[64];
        snprintf(bgPath, sizeof(bgPath), "/assets/bg/bg_%02d.png", bgId);
        File f = LittleFS.open(bgPath, "r");
        if (f) {
            currentBgFrame.buffer.resize(f.size());
            f.read(currentBgFrame.buffer.data(), f.size());
            f.close();
            currentLoadedBgId = bgId;
        }
    }

    if (!currentBgFrame.buffer.empty()) {
        canvas.drawPng(currentBgFrame.buffer.data(), currentBgFrame.buffer.size(), 0, 0);
    } else {
        canvas.fillScreen(canvas.color565(225, 242, 255));
    }
}

String AssetManager::getActionNameByState(PetAnimState anim, const String& stage, uint8_t& outFps) {
    outFps = 10;
    switch (anim) {
        case ANIM_IDLE_STAND:
            outFps = 8;
            return "stand";
        case ANIM_IDLE_LOOK:
        case ANIM_IDLE_BOUNCE:
        case ANIM_HAPPY:
            outFps = 10;
            return "happy";
        case ANIM_IDLE_SCRATCH:
        case ANIM_IDLE_STRETCH:
        case ANIM_IDLE_DOZE:
        case ANIM_IDLE_PAT_BELLY:
            outFps = 8;
            return "stand";

        case ANIM_PLAY:
            outFps = 10;
            // 逗玩玩耍动作库：仅在 Adult 成年期轮播 play ~ play_4，幼年期与破壳期使用专属 play
            if (stage == "Adult") {
                int r = (millis() / 4000) % 5;
                if (r == 1) return "play_1";
                if (r == 2) return "play_2";
                if (r == 3) return "play_3";
                if (r == 4) return "play_4";
            }
            return "play";
        case ANIM_WORK:
            outFps = 10;
            return "work";
        case ANIM_STUDY:
            outFps = 10;
            return "study";
        case ANIM_TRIP:
            outFps = 10;
            return "trip";


        case ANIM_EAT:
            outFps = 10;
            return "eat";
        case ANIM_CLEAN:
            outFps = 10;
            return "clean";
        case ANIM_SAD:
            outFps = 8;
            return "sad";
        case ANIM_SICK:
            outFps = 8;
            return "sick";
        case ANIM_DEAD:
        case ANIM_DYING:
            outFps = 6;
            return "dying";

        case ANIM_CURE:
            outFps = 10;
            return "cure";
        case ANIM_LEVELUP:
        case ANIM_DRAG:
            outFps = 10;
            return "levelup";
        default:
            outFps = 8;
            return "stand";
    }
}

void AssetManager::loadActionClip(const String& actionName, uint8_t gender, const String& stage, uint8_t fps) {

    if (!isFsMounted) return;
    if (currentLoadedAction == actionName && currentLoadedGender == gender && currentLoadedStage == stage && !currentClipFrames.empty()) {
        return; // 命中当前动作内存缓存，零延迟
    }

    currentClipFrames.clear();
    currentClipFrames.shrink_to_fit();
    currentLoadedAction = actionName;
    currentLoadedGender = gender;
    currentLoadedStage = stage;
    currentClipFps = fps;

    String genderStr = (gender == 1) ? "MM" : "GG";
    String dirPath = "/assets/" + genderStr + "/" + stage + "/" + actionName;

    // 如果目标动作不存在，智能回退
    char testFile[64];
    snprintf(testFile, sizeof(testFile), "%s/f_00.png", dirPath.c_str());
    if (!LittleFS.exists(testFile)) {
        dirPath = String("/assets/GG/") + stage + "/" + actionName;
        snprintf(testFile, sizeof(testFile), "%s/f_00.png", dirPath.c_str());
        if (!LittleFS.exists(testFile)) {
            if (actionName == "study" || actionName == "work") {
                dirPath = "/assets/" + genderStr + "/" + stage + "/happy";
                snprintf(testFile, sizeof(testFile), "%s/f_00.png", dirPath.c_str());
                if (!LittleFS.exists(testFile)) dirPath = String("/assets/GG/") + stage + "/happy";
            } else if (actionName.startsWith("play") || actionName == "trip") {
                dirPath = "/assets/" + genderStr + "/" + stage + "/play";
                snprintf(testFile, sizeof(testFile), "%s/f_00.png", dirPath.c_str());
                if (!LittleFS.exists(testFile)) dirPath = String("/assets/GG/") + stage + "/play";
            } else {
                dirPath = "/assets/" + genderStr + "/" + stage + "/stand";
            }
        }
    }

    // 将序列帧 PNG 一次性载入内存 (仅 ~20KB，零 Flash I/O 阻塞)
    for (int i = 0; i < 30; ++i) {
        char filename[64];
        snprintf(filename, sizeof(filename), "%s/f_%02d.png", dirPath.c_str(), i);
        if (!LittleFS.exists(filename)) break;

        File f = LittleFS.open(filename, "r");
        if (f) {
            size_t sz = f.size();
            InMemoryFrame frame;
            frame.buffer.resize(sz);
            f.read(frame.buffer.data(), sz);
            f.close();
            currentClipFrames.push_back(std::move(frame));
        }
    }
}

void AssetManager::drawPetFrame(M5Canvas& canvas, int x, int y, PetAnimState anim, uint8_t gender, int level, uint32_t currentMillis) {
    uint8_t fps = 8;
    String stage = (level < 5) ? "Egg" : ((level < 12) ? "Kid" : "Adult");

    String act = getActionNameByState(anim, stage, fps);
    loadActionClip(act, gender, stage, fps);

    if (currentClipFrames.empty()) {
        canvas.fillCircle(x + 48, y + 48, 26, canvas.color565(255, 180, 0));
        return;
    }

    size_t frameIdx = (currentMillis * currentClipFps / 1000) % currentClipFrames.size();
    const InMemoryFrame& frame = currentClipFrames[frameIdx];
    
    // 直接内存解压进行原生逐像素 32 位 Alpha 物理混合 (彻底消灭蛋壳黑边与杂色)
    canvas.drawPng(frame.buffer.data(), frame.buffer.size(), x, y);
}

void AssetManager::loadMenuIcons() {
    if (!isFsMounted || iconsLoaded) return;

    static const char* iconNames[10] = {
        "feed", "bath", "play", "work", "study", "trip", "cure", "shop", "status", "web"
    };

    for (int i = 0; i < 10; ++i) {
        String normPath = String("/assets/icons/") + iconNames[i] + "_norm.png";
        String actPath = String("/assets/icons/") + iconNames[i] + "_act.png";

        sprIconsNorm[i].setColorDepth(16);
        sprIconsNorm[i].createSprite(20, 20);
        sprIconsNorm[i].fillScreen(CHROMA_KEY);
        sprIconsNorm[i].drawPngFile(LittleFS, normPath.c_str(), 0, 0);

        sprIconsAct[i].setColorDepth(16);
        sprIconsAct[i].createSprite(28, 28);
        sprIconsAct[i].fillScreen(CHROMA_KEY);
        sprIconsAct[i].drawPngFile(LittleFS, actPath.c_str(), 0, 0);
    }
    iconsLoaded = true;
}

void AssetManager::drawAdoptionPet(M5Canvas& canvas, int x, int y, uint8_t gender, bool active, uint32_t currentMillis) {
    if (!isFsMounted) {
        canvas.fillCircle(x + 48, y + 48, 24, (gender == 1) ? canvas.color565(255, 120, 180) : canvas.color565(50, 150, 255));
        return;
    }

    if (!adoptFramesLoaded) {
        adoptGgFrames.clear();
        adoptMmFrames.clear();

        // 1. 载入 GG 雏鸟动作 (stand)
        for (int i = 0; i < 20; ++i) {
            char fn[64];
            snprintf(fn, sizeof(fn), "/assets/GG/Egg/stand/f_%02d.png", i);
            if (!LittleFS.exists(fn)) break;
            File f = LittleFS.open(fn, "r");
            if (f) {
                InMemoryFrame fr;
                fr.buffer.resize(f.size());
                f.read(fr.buffer.data(), f.size());
                f.close();
                adoptGgFrames.push_back(std::move(fr));
            }
        }

        // 2. 载入 MM 雏鸟动作 (stand)
        for (int i = 0; i < 20; ++i) {
            char fn[64];
            snprintf(fn, sizeof(fn), "/assets/MM/Egg/stand/f_%02d.png", i);
            if (!LittleFS.exists(fn)) break;
            File f = LittleFS.open(fn, "r");
            if (f) {
                InMemoryFrame fr;
                fr.buffer.resize(f.size());
                f.read(fr.buffer.data(), f.size());
                f.close();
                adoptMmFrames.push_back(std::move(fr));
            }
        }
        adoptFramesLoaded = true;
    }

    const auto& frames = (gender == 1) ? adoptMmFrames : adoptGgFrames;
    if (frames.empty()) {
        canvas.fillCircle(x + 48, y + 48, 24, (gender == 1) ? canvas.color565(255, 120, 180) : canvas.color565(50, 150, 255));
        return;
    }

    // 选中时欢快高频蹦跳 (10 fps)，未选中时温和呼吸 (6 fps)
    uint8_t fps = active ? 10 : 6;
    size_t fIdx = (currentMillis * fps / 1000) % frames.size();
    
    // 如果被选中，叠加微弱上下蹦跳位移
    int drawY = y;
    if (active) {
        drawY += (fIdx % 4 > 2) ? -4 : 0;
    }

    canvas.drawPng(frames[fIdx].buffer.data(), frames[fIdx].buffer.size(), x, drawY);
}

void AssetManager::drawMenuIcon(M5Canvas& canvas, int x, int y, int optionIndex, bool active) {
    if (optionIndex < 0 || optionIndex >= 10) return;

    if (active) {
        sprIconsAct[optionIndex].pushSprite(&canvas, x, y, CHROMA_KEY);
    } else {
        sprIconsNorm[optionIndex].pushSprite(&canvas, x, y, CHROMA_KEY);
    }
}







