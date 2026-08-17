#include "asset_manager.h"

AssetManager g_assets;

AssetManager::AssetManager() 
    : isFsMounted(false), currentLoadedGender(255), currentLoadedStage(""), currentClipFps(8), currentLoadedBgId(255), iconsLoaded(false) {}

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

    // 顺序读取序列帧 f_00.png, f_01.png ...
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
    canvas.drawPng(frame.buffer.data(), frame.buffer.size(), x, y);
}


void AssetManager::loadMenuIcons() {
    if (!isFsMounted || iconsLoaded) return;

    static const char* iconNames[] = {
        "feed", "bath", "play", "cure", "status", "web"
    };

    menuIconsNorm.clear();
    menuIconsAct.clear();

    for (int i = 0; i < 6; ++i) {
        String normPath = String("/assets/icons/") + iconNames[i] + "_norm.png";
        String actPath = String("/assets/icons/") + iconNames[i] + "_act.png";

        InMemoryFrame normFrame;
        File f1 = LittleFS.open(normPath.c_str(), "r");
        if (f1) {
            normFrame.buffer.resize(f1.size());
            f1.read(normFrame.buffer.data(), f1.size());
            f1.close();
        }
        menuIconsNorm.push_back(std::move(normFrame));

        InMemoryFrame actFrame;
        File f2 = LittleFS.open(actPath.c_str(), "r");
        if (f2) {
            actFrame.buffer.resize(f2.size());
            f2.read(actFrame.buffer.data(), f2.size());
            f2.close();
        }
        menuIconsAct.push_back(std::move(actFrame));
    }
    iconsLoaded = true;
}

void AssetManager::drawMenuIcon(M5Canvas& canvas, int x, int y, int optionIndex, bool active) {
    if (optionIndex < 0 || optionIndex >= 9) return;

    // 将 9 项菜单映射到 6 个官方图标资源 (0:feed, 1:bath, 2:play, 3:work->play, 4:study->feed, 5:trip->bath, 6:cure, 7:status, 8:web)
    static const int iconMap[9] = {0, 1, 2, 2, 0, 1, 3, 4, 5};
    int iconIdx = iconMap[optionIndex];
    if (iconIdx < 0 || iconIdx >= 6) return;

    if (active) {
        if (iconIdx < (int)menuIconsAct.size() && !menuIconsAct[iconIdx].buffer.empty()) {
            canvas.drawPng(menuIconsAct[iconIdx].buffer.data(), menuIconsAct[iconIdx].buffer.size(), x, y);
        }
    } else {
        if (iconIdx < (int)menuIconsNorm.size() && !menuIconsNorm[iconIdx].buffer.empty()) {
            canvas.drawPng(menuIconsNorm[iconIdx].buffer.data(), menuIconsNorm[iconIdx].buffer.size(), x, y);
        }
    }
}


