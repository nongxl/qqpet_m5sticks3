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

    if (currentLoadedBgId != bgId || currentBgFrame.buffer == nullptr) {
        char bgPath[64];
        snprintf(bgPath, sizeof(bgPath), "/assets/bg/bg_%02d.png", bgId);
        File f = LittleFS.open(bgPath, "r");
        if (f) {
            currentBgFrame = InMemoryFrame(f.size());
            if (currentBgFrame.buffer) {
                f.read(currentBgFrame.buffer, f.size());
                currentLoadedBgId = bgId;
            }
            f.close();
        }
    }

    if (currentBgFrame.buffer && currentBgFrame.size > 0) {
        canvas.drawPng(currentBgFrame.buffer, currentBgFrame.size, 0, 0);
    } else {
        canvas.fillScreen(canvas.color565(225, 242, 255));
    }
}


String AssetManager::getActionNameByState(PetAnimState anim, const String& stage, uint8_t& outFps) {
    outFps = 8;
    switch (anim) {
        case ANIM_IDLE_STAND:
            outFps = 6; // 12 帧完整呼吸站立以 6 fps 播放，2秒1次从容平稳呼吸
            return "stand";
        case ANIM_IDLE_LOOK:
            outFps = 6;
            return "look"; // 左右歪头打量主人
        case ANIM_IDLE_SCRATCH:
            outFps = 6;
            return "wobble"; // 憨态蹒跚左右摇晃
        case ANIM_IDLE_STRETCH:
            outFps = 6;
            return "stretch"; // 伸个舒服小懒腰
        case ANIM_IDLE_BOUNCE:
        case ANIM_HAPPY:
            outFps = 8;
            return "happy"; // 欢快蹦跳
        case ANIM_IDLE_DOZE:
        case ANIM_IDLE_PAT_BELLY:
            outFps = 8;
            return "play"; // 玩耍拍球/转圈
        case ANIM_PLAY:
            outFps = 8;
            if (stage == "Adult") {
                int r = (millis() / 5000) % 14;
                if (r == 1) return "play_1";
                if (r == 2) return "play_2";
                if (r == 3) return "play_3";
                if (r == 4) return "play_4";
                if (r == 5) return "play_5";
                if (r == 6) return "play_6";
                if (r == 7) return "play_7";
                if (r == 8) return "play_8";
                if (r == 9) return "play_9";
                if (r == 10) return "play_10";
                if (r == 11) return "play_11";
                if (r == 12) return "play_12";
                if (r == 13) return "play_13";
            } else if (stage == "Kid") {
                int r = (millis() / 5000) % 9;
                if (r == 1) return "play_1";
                if (r == 2) return "play_2";
                if (r == 3) return "play_horse";
                if (r == 4) return "play_mill";
                if (r == 5) return "play_plane";
                if (r == 6) return "play_fly";
                if (r == 7) return "play_block";
                if (r == 8) return "play_sand";
            } else if (stage == "Egg") {
                int r = (millis() / 5000) % 4;
                if (r == 1) return "play_1";
                if (r == 2) return "play_roll";
                if (r == 3) return "play_hug";
            }
            return "play";
        case ANIM_WORK:
            outFps = 9;
            if (stage == "Adult") {
                int r = (millis() / 6000) % 3;
                if (r == 1) return "work_1";
                if (r == 2) return "work_2";
            } else if (stage == "Kid") {
                int r = (millis() / 6000) % 3;
                if (r == 1) return "work_1";
                if (r == 2) return "work_2";
            }
            return "work";
        case ANIM_STUDY:
            outFps = 9;
            if (stage == "Adult") {
                int r = (millis() / 6000) % 3;
                if (r == 1) return "study_1";
                if (r == 2) return "study_2";
            } else if (stage == "Kid") {
                int r = (millis() / 6000) % 2;
                if (r == 1) return "study_1";
            }
            return "study";

        case ANIM_TRIP:
            outFps = 8;
            return "trip";
        case ANIM_EAT:
            outFps = 8;
            if (stage == "Adult") {
                int r = (millis() / 5000) % 3;
                if (r == 1) return "eat_1";
                if (r == 2) return "eat_2";
            }
            return "eat";
        case ANIM_CLEAN:
            outFps = 8;
            return "clean";
        case ANIM_SAD:
            outFps = 6;
            if (stage == "Adult") {
                int r = (millis() / 6000) % 3;
                if (r == 1) return "sad_circle";
                if (r == 2) return "sad_sigh";
            }
            return "sad";
        case ANIM_SICK:
            outFps = 6;
            return "sick";
        case ANIM_DEAD:
        case ANIM_DYING:
            outFps = 6;
            return "dying";
        case ANIM_CURE:
            outFps = 8;
            return "cure";
        case ANIM_SLEEP:
            outFps = 6;
            if (stage == "Adult") {
                int r = (millis() / 7000) % 4;
                if (r == 1) return "sleep_1";
                if (r == 2) return "sleep_2";
                if (r == 3) return "sleep_3";
            } else {
                int r = (millis() / 7000) % 3;
                if (r == 1) return "sleep_1";
                if (r == 2) return "sleep_2";
            }
            return "sleep";

        case ANIM_LEVELUP:
            outFps = 8;
            return "levelup";
        case ANIM_DRAG:
            outFps = 8;
            return "drag";
        case ANIM_WALK_LEFT:
            outFps = 6;
            return "walk_left";
        case ANIM_WALK_RIGHT:
            outFps = 6;
            return "walk_right";
        case ANIM_HIDE_LEFT:
            outFps = 6;
            return "hide_left";
        case ANIM_HIDE_RIGHT:
            outFps = 6;
            return "hide_right";
        case ANIM_SNEEZE:
            outFps = 6;
            return "sneeze";
        case ANIM_YAWN:
            outFps = 6;
            return "yawn";
        case ANIM_ANGRY:
            outFps = 8;
            return "angry";
        case ANIM_SHY:
            outFps = 6;
            return "shy";
        case ANIM_UMBRELLA:
            outFps = 6;
            return "umbrella";
        case ANIM_COLD:
            outFps = 6;
            return "cold";
        case ANIM_SUMMER:
            outFps = 6;
            return "summer";
        case ANIM_TIWENJI:
            outFps = 6;
            return "tiwenji";
        case ANIM_INJECTION:
            outFps = 8;
            return "injection";
        case ANIM_HOBBY_WATER:
            outFps = 7;
            return "hobby_water";
        case ANIM_HOBBY_PAINT:
            outFps = 7;
            return "hobby_paint";
        case ANIM_HOBBY_MIRROR:
            outFps = 7;
            return "hobby_mirror";
        case ANIM_HOBBY_CHESS:
            outFps = 7;
            return "hobby_chess";
        case ANIM_HOBBY_TEA:
            outFps = 7;
            return "hobby_tea";
        case ANIM_HOBBY_LENS:
            outFps = 7;
            return "hobby_lens";
        case ANIM_HOBBY_PAPER:
            outFps = 7;
            return "hobby_paper";
        case ANIM_HOBBY_RADIO:
            outFps = 7;
            return "hobby_radio";
        case ANIM_HOBBY_TYPE:
            outFps = 7;
            return "hobby_type";
        case ANIM_HOBBY_SCOPE:
            outFps = 7;
            return "hobby_scope";
        case ANIM_HOBBY_CLEAN:
            outFps = 7;
            return "hobby_clean";
        case ANIM_SAD_CIRCLE:
            outFps = 6;
            return "sad_circle";
        case ANIM_SAD_SIGH:
            outFps = 6;
            return "sad_sigh";
        case ANIM_UPSET_STOMP:
            outFps = 7;
            return "upset_stomp";
        case ANIM_UPSET_CROSS:
            outFps = 6;
            return "upset_cross";
        default:
            outFps = 6;
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

    // 1. 优先尝试加载本性别本阶段的 .act 二进制容器
    String actPath = "/assets/" + genderStr + "/" + stage + "/" + actionName + ".act";
    if (!LittleFS.exists(actPath)) {
        // 先尝试回退到本性别的 Adult 动作 (严格保证 MM / GG 形象纯正统一)
        actPath = "/assets/" + genderStr + "/Adult/" + actionName + ".act";
        if (!LittleFS.exists(actPath)) {
            // 再尝试回退到本性别当前阶段的 stand.act
            actPath = "/assets/" + genderStr + "/" + stage + "/stand.act";
            if (!LittleFS.exists(actPath)) {
                // 终极保底
                actPath = "/assets/GG/" + stage + "/stand.act";
            }
        }
    }


    if (LittleFS.exists(actPath)) {
        File f = LittleFS.open(actPath, "r");
        if (f) {
            uint8_t header[4];
            if (f.read(header, 4) == 4 && header[0] == 0xAA) {
                uint8_t count = header[2];
                std::vector<uint16_t> sizes(count);
                f.read(reinterpret_cast<uint8_t*>(sizes.data()), count * sizeof(uint16_t));
                for (uint8_t i = 0; i < count; ++i) {
                    InMemoryFrame frame(sizes[i]);
                    if (frame.buffer) {
                        f.read(frame.buffer, sizes[i]);
                        currentClipFrames.push_back(std::move(frame));
                    }
                }
                f.close();
                return;
            }
            f.close();
        }
    }


    // 2. 回退兼容老旧散装 PNG 目录
    String dirPath = "/assets/" + genderStr + "/" + stage + "/" + actionName;
    char testFile[64];
    snprintf(testFile, sizeof(testFile), "%s/f_00.png", dirPath.c_str());
    if (!LittleFS.exists(testFile)) {
        dirPath = String("/assets/GG/") + stage + "/" + actionName;
    }

    for (int i = 0; i < 30; ++i) {
        char filename[64];
        snprintf(filename, sizeof(filename), "%s/f_%02d.png", dirPath.c_str(), i);
        if (!LittleFS.exists(filename)) break;

        File f = LittleFS.open(filename, "r");
        if (f) {
            size_t sz = f.size();
            InMemoryFrame frame(sz);
            if (frame.buffer) {
                f.read(frame.buffer, sz);
                currentClipFrames.push_back(std::move(frame));
            }
            f.close();
        }

    }
}


void AssetManager::drawCostume(M5Canvas& canvas, int costumeId, int petCenterX, int petCenterY, int level, uint32_t currentMillis) {
    if (costumeId < 1 || costumeId > COSTUME_COUNT || !isFsMounted) return;
    const auto& c = COSTUME_LIST[costumeId - 1];

    String path = String("/assets/costumes/") + c.file;
    if (!LittleFS.exists(path)) return;

    // 锚点偏移计算 (Egg 较小，Kid 中等，Adult 较高大)
    int headOffsetY = (level < 5) ? -28 : ((level < 12) ? -33 : -38);
    int neckOffsetY = (level < 5) ? -12 : ((level < 12) ? -15 : -18);
    int handOffsetX = (level < 5) ? 22 : ((level < 12) ? 26 : 30);
    int handOffsetY = (level < 5) ? -6 : ((level < 12) ? -8 : -10);

    // 身体上下自然呼吸微位移
    int breath = ((currentMillis / 250) % 2 == 0) ? -1 : 0;

    int drawX = petCenterX;
    int drawY = petCenterY + breath;

    if (c.category == 0) { // 头部
        drawX -= 16;
        drawY += headOffsetY;
        if (costumeId == 4) drawY -= 8; // 天使光环悬浮
    } else if (c.category == 1) { // 颈部
        drawX -= 13;
        drawY += neckOffsetY;
    } else { // 随身挂件/小背包/魔杖
        if (costumeId == 7) {
            drawX -= 32;
            drawY += neckOffsetY + 2;
        } else {
            drawX += handOffsetX - 8;
            drawY += handOffsetY;
        }
    }

    canvas.drawPngFile(LittleFS, path.c_str(), drawX, drawY);
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
    
    // 1. 直接内存解压进行原生逐像素 32 位 Alpha 物理混合
    if (frame.buffer && frame.size > 0) {
        canvas.drawPng(frame.buffer, frame.size, x, y);
    }

    // 2. 动态叠加当前佩戴的头/颈/手持饰品 (仅幼年期与成年期支持，雏鸟蛋壳期不叠加)
    if (level >= 5) {
        const PetState& st = g_pet.getState();
        int centerX = x + 48;
        int centerY = y + 48;
        if (st.equipped_head > 0) drawCostume(canvas, st.equipped_head, centerX, centerY, level, currentMillis);
        if (st.equipped_neck > 0) drawCostume(canvas, st.equipped_neck, centerX, centerY, level, currentMillis);
        if (st.equipped_hand > 0) drawCostume(canvas, st.equipped_hand, centerX, centerY, level, currentMillis);
    }

}

void AssetManager::loadMenuIcons() {
    if (!isFsMounted || iconsLoaded) return;

    static const char* iconNames[11] = {
        "feed", "bath", "play", "wardrobe", "work", "study", "trip", "cure", "shop", "status", "web"
    };

    for (int i = 0; i < 11; ++i) {
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

        auto loadAct = [](const char* path, std::vector<InMemoryFrame>& outFrames) {
            if (!LittleFS.exists(path)) return;
            File f = LittleFS.open(path, "r");
            if (!f) return;
            uint8_t header[4];
            if (f.read(header, 4) == 4 && header[0] == 0xAA) {
                uint8_t count = header[2];
                std::vector<uint16_t> sizes(count);
                f.read(reinterpret_cast<uint8_t*>(sizes.data()), count * sizeof(uint16_t));
                for (uint8_t i = 0; i < count; ++i) {
                    InMemoryFrame fr(sizes[i]);
                    if (fr.buffer) {
                        f.read(fr.buffer, sizes[i]);
                        outFrames.push_back(std::move(fr));
                    }
                }
            }
            f.close();
        };

        loadAct("/assets/GG/Egg/stand.act", adoptGgFrames);
        loadAct("/assets/MM/Egg/stand.act", adoptMmFrames);
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

    if (frames[fIdx].buffer && frames[fIdx].size > 0) {
        canvas.drawPng(frames[fIdx].buffer, frames[fIdx].size, x, drawY);
    }
}


void AssetManager::drawMenuIcon(M5Canvas& canvas, int x, int y, int optionIndex, bool active) {
    if (optionIndex < 0 || optionIndex >= 11) return;

    if (active) {
        sprIconsAct[optionIndex].pushSprite(&canvas, x, y, CHROMA_KEY);
    } else {
        sprIconsNorm[optionIndex].pushSprite(&canvas, x, y, CHROMA_KEY);
    }
}










