#include "display_engine.h"
#include "config.h"
#include "asset_manager.h"
#include <M5Unified.h>

DisplayEngine g_display;


DisplayEngine::DisplayEngine() 
    : canvas(&M5.Display), menuVisible(false), menuSlideProgress(0.0f),
      wheelCurrentAngle(0.0f), wheelTargetAngle(0.0f),
      menuLastActiveTime(0), currentOption(MENU_FEED), bubbleEndTime(0),
      typewriterIndex(0), lastTypewriterTime(0), toastEndTime(0),
      cachedBattery(100), lastBatteryCheckTime(0), animFrame(0), isDragging(false) {}

void DisplayEngine::begin() {
    M5.Display.setRotation(0); // 竖屏 135 x 240
    M5.Display.setBrightness(180);
    canvas.setPsram(false); // 135x240 使用内部高速 SRAM (零延迟，彻底杜绝 PSRAM 刷新撕裂与抖动)
    canvas.createSprite(SCREEN_W, SCREEN_H);
    canvas.setFont(&fonts::efontCN_12); // 原生高质量中文字库
    canvas.setTextWrap(true);
}


void DisplayEngine::showBubble(const String& text, uint32_t durationMs) {
    bubbleText = text;
    bubbleEndTime = millis() + durationMs;
    typewriterIndex = 0;
    lastTypewriterTime = millis();
}

void DisplayEngine::showToast(const String& msg, uint32_t durationMs) {
    toastText = msg;
    toastEndTime = millis() + durationMs;
}

static const float MENU_ITEM_ANGLE = 26.0f;

void DisplayEngine::toggleMenu() {
    menuVisible = !menuVisible;
    menuLastActiveTime = millis();
    wheelTargetAngle = static_cast<float>(currentOption) * MENU_ITEM_ANGLE;
}

void DisplayEngine::nextMenuOption() {
    currentOption = static_cast<MenuOption>((static_cast<int>(currentOption) + 1) % MENU_COUNT);
    menuLastActiveTime = millis();
    wheelTargetAngle = static_cast<float>(currentOption) * MENU_ITEM_ANGLE;
}

void DisplayEngine::prevMenuOption() {
    currentOption = static_cast<MenuOption>((static_cast<int>(currentOption) + MENU_COUNT - 1) % MENU_COUNT);
    menuLastActiveTime = millis();
    wheelTargetAngle = static_cast<float>(currentOption) * MENU_ITEM_ANGLE;
}

void DisplayEngine::closeMenu() {
    menuVisible = false;
}



void DisplayEngine::update(int petOffsetX, int petOffsetY) {
    animFrame++;

    // 菜单超时自动隐藏
    if (menuVisible && (millis() - menuLastActiveTime > MENU_TIMEOUT_MS)) {
        menuVisible = false;
    }

    // 菜单弹出/收起平滑过渡动画
    float targetProgress = menuVisible ? 1.0f : 0.0f;
    menuSlideProgress += (targetProgress - menuSlideProgress) * 0.28f;
    if (std::abs(menuSlideProgress - targetProgress) < 0.02f) {
        menuSlideProgress = targetProgress;
    }

    // 1. 绘制清爽渐变背景
    drawBackground();

    // 2. 绘制顶部信息栏
    drawTopBar();

    // 3. 计算企鹅位置 (当右侧菜单展开时，企鹅平滑向左让位)
    int shiftLeft = static_cast<int>(menuSlideProgress * 38.0f);
    int petX = (SCREEN_W / 2) - shiftLeft + petOffsetX;
    int petY = 158 + petOffsetY;
    
    PetAnimState anim = g_pet.getCurrentAnimState();
    if (isDragging) {
        petY += (animFrame % 6 > 3) ? -3 : 3;
        drawPet(petX, petY, (anim == ANIM_IDLE_STAND) ? ANIM_DRAG : anim);
    } else {
        if (anim == ANIM_PLAY || anim == ANIM_IDLE_BOUNCE) {
            petY += (animFrame % 8 > 4) ? -4 : 0;
        }
        drawPet(petX, petY, anim);
    }




    // 4. 绘制对话气泡
    drawBubble();

    // 5. 绘制右侧仿原版弹出式气泡菜单
    if (menuSlideProgress > 0.05f) {
        drawRightBubbleMenu();
    }

    // 6. 绘制 Toast
    drawToast();

    // 推送画布
    canvas.pushSprite(0, 0);
}

void DisplayEngine::drawBackground() {
    const PetState& st = g_pet.getState();
    g_assets.drawBackground(canvas, st.bg_id);
}


void DisplayEngine::drawTopBar() {
    const PetState& st = g_pet.getState();
    canvas.setFont(&fonts::efontCN_12);
    canvas.setTextColor(canvas.color565(30, 45, 60));
    
    // 企鹅昵称与等级
    canvas.drawString(String(st.name) + " Lv." + String(g_pet.getLevel()), 6, 6);

    // 金币元宝展示
    canvas.setTextColor(canvas.color565(210, 130, 0));
    canvas.drawString("Y" + String(st.coins), 62, 6);

    // 电池电量 (使用静态缓存，彻底切断 I2C 轮询以杜绝屏幕闪烁)
    canvas.setTextColor(canvas.color565(50, 75, 110));
    canvas.drawRightString(String(cachedBattery) + "%", SCREEN_W - 6, 6);
    canvas.fillRoundRect(SCREEN_W - 36, 8, 4, 8, 1, canvas.color565(60, 190, 80));

    // 顶部分隔线
    canvas.drawFastHLine(4, 22, SCREEN_W - 8, canvas.color565(190, 215, 240));
}


#include "asset_manager.h"

void DisplayEngine::drawPet(int centerX, int centerY, PetAnimState anim) {

    // 经典地面柔和阴影
    canvas.fillEllipse(centerX, centerY + 42, 28, 7, canvas.color565(165, 190, 195));

    // 从 LittleFS 按需加载/RAM缓存渲染 (支持 GG/MM 双性别与 Kid/Adult 成长进化)
    const PetState& st = g_pet.getState();
    int drawX = centerX - 48;
    int drawY = centerY - 48;
    g_assets.drawPetFrame(canvas, drawX, drawY, anim, st.gender, g_pet.getLevel(), millis());
}




void DisplayEngine::drawBubble() {
    if (millis() > bubbleEndTime || bubbleText.length() == 0) return;

    // 按完整 UTF-8 字符平滑推进打字机
    if (millis() - lastTypewriterTime > 40 && typewriterIndex < bubbleText.length()) {
        unsigned char c = (unsigned char)bubbleText[typewriterIndex];
        if ((c & 0x80) == 0) typewriterIndex += 1;          // 1 字节 ASCII
        else if ((c & 0xE0) == 0xC0) typewriterIndex += 2;  // 2 字节
        else if ((c & 0xF0) == 0xE0) typewriterIndex += 3;  // 3 字节 (常用汉字)
        else if ((c & 0xF8) == 0xF0) typewriterIndex += 4;  // 4 字节 Emoji
        else typewriterIndex += 1;
        if (typewriterIndex > bubbleText.length()) typewriterIndex = bubbleText.length();
        lastTypewriterTime = millis();
    }

    String currentSubText = bubbleText.substring(0, typewriterIndex);

    // 精确中文字符换行排版 (每行最大 100 像素，约 8 个汉字)
    canvas.setFont(&fonts::efontCN_12);
    canvas.setTextSize(1);
    
    std::vector<String> lines;
    String curLine = "";
    int curLineWidth = 0;
    int maxLineWidth = 102; // 气泡内文字最大允许宽度

    for (size_t i = 0; i < currentSubText.length(); ) {
        unsigned char c = (unsigned char)currentSubText[i];
        int charLen = 1;
        if ((c & 0x80) == 0) charLen = 1;
        else if ((c & 0xE0) == 0xC0) charLen = 2;
        else if ((c & 0xF0) == 0xE0) charLen = 3;
        else if ((c & 0xF8) == 0xF0) charLen = 4;

        String ch = currentSubText.substring(i, i + charLen);
        int chW = canvas.textWidth(ch);

        if (curLineWidth + chW > maxLineWidth) {
            lines.push_back(curLine);
            curLine = ch;
            curLineWidth = chW;
        } else {
            curLine += ch;
            curLineWidth += chW;
        }
        i += charLen;
    }
    if (curLine.length() > 0) {
        lines.push_back(curLine);
    }

    // 限制最多显示 3 行
    if (lines.size() > 3) {
        lines.resize(3);
    }

    size_t lineCount = lines.empty() ? 1 : lines.size();
    int bubbleH = 22 + (lineCount * 14); // 1行: 36, 2行: 50, 3行: 64
    int bubbleY = 82 - bubbleH;          // 底部固定在 y = 82
    int bubbleX = 6;
    int bubbleW = SCREEN_W - 12;

    // 绘制圆润气泡卡片
    canvas.fillRoundRect(bubbleX, bubbleY, bubbleW, bubbleH, 8, TFT_WHITE);
    canvas.drawRoundRect(bubbleX, bubbleY, bubbleW, bubbleH, 8, canvas.color565(160, 195, 230));

    // 小三角指向企鹅头顶
    canvas.fillTriangle(SCREEN_W / 2 - 4, 82, SCREEN_W / 2 + 4, 82, SCREEN_W / 2, 88, TFT_WHITE);
    canvas.drawLine(SCREEN_W / 2 - 4, 82, SCREEN_W / 2, 88, canvas.color565(160, 195, 230));
    canvas.drawLine(SCREEN_W / 2 + 4, 82, SCREEN_W / 2, 88, canvas.color565(160, 195, 230));

    // 文本逐行精确居左打印 (带内边距与垂直居中)
    canvas.setTextColor(canvas.color565(30, 45, 70));
    int textStartY = bubbleY + 8;
    for (size_t l = 0; l < lines.size(); ++l) {
        canvas.drawString(lines[l], bubbleX + 8, textStartY + l * 14);
    }
}

#include <cmath>

void DisplayEngine::drawRightBubbleMenu() {
    // 官方原版右侧旋转半椭圆轮盘 (紧贴右侧边缘，横向 rx=44px，纵向 ry=86px 上下拉开)
    int cx = SCREEN_W + 6; // 141
    int cy = 128;
    float rx = menuSlideProgress * 44.0f; // 横向半轴 (探出深度)
    float ry = menuSlideProgress * 86.0f; // 纵向半轴 (上下大幅度拉开)

    // 轮盘平滑旋转阻尼插值 (物理阻尼惯性)
    wheelCurrentAngle += (wheelTargetAngle - wheelCurrentAngle) * 0.24f;

    static const char* MENU_NAMES[] = {
        "喂食", "洗澡", "逗玩", "打工", "学习", "旅游", "看病", "状态", "后台"
    };

    const PetState& st = g_pet.getState();

    // 1. 绘制向左拱起的半椭圆轮盘轨道微弱淡蓝虚线弧
    if (menuSlideProgress > 0.4f) {
        for (int a = -78; a <= 78; a += 6) {
            float rad = a * 0.0174532925f;
            int dotX = cx - static_cast<int>(rx * std::cos(rad));
            int dotY = cy + static_cast<int>(ry * std::sin(rad));
            if (dotY >= 26 && dotY <= SCREEN_H - 12) {
                canvas.drawPixel(dotX, dotY, canvas.color565(165, 205, 245));
            }
        }
    }

    // 2. 先绘制非选中项 (按半椭圆参数方程计算位置)
    for (int i = 0; i < MENU_COUNT; ++i) {
        if (i == currentOption) continue; // 选中项稍后在顶层突出绘制

        float itemAngle = (static_cast<float>(i) * MENU_ITEM_ANGLE) - wheelCurrentAngle;
        if (std::abs(itemAngle) > 80.0f) continue; // 超出上下可视范围的不绘制

        float rad = itemAngle * 0.0174532925f;
        int bx = cx - static_cast<int>(rx * std::cos(rad));
        int by = cy + static_cast<int>(ry * std::sin(rad));

        if (by < 26 || by > SCREEN_H - 12) continue;

        // 半透明淡蓝小圆底座
        canvas.fillCircle(bx, by, 11, canvas.color565(232, 245, 255));
        canvas.drawCircle(bx, by, 11, canvas.color565(140, 190, 240));

        // 渲染 20x20 原版小图标
        g_assets.drawMenuIcon(canvas, bx - 10, by - 10, i, false);
    }

    // 3. 顶层突出绘制当前选中项 (紧贴右侧最前沿，放大 1.4 倍，恒定完美居中)
    int selIdx = currentOption;
    float selAngle = (static_cast<float>(selIdx) * MENU_ITEM_ANGLE) - wheelCurrentAngle;
    float selRad = selAngle * 0.0174532925f;
    int selX = cx - static_cast<int>(rx * std::cos(selRad));
    int selY = cy + static_cast<int>(ry * std::sin(selRad));


    if (selY < 32) selY = 32;
    if (selY > SCREEN_H - 24) selY = SCREEN_H - 24;

    // 亮金黄 + 亮白双层高光放大圆盘
    canvas.fillCircle(selX, selY, 17, canvas.color565(255, 210, 50));
    canvas.drawCircle(selX, selY, 17, canvas.color565(255, 245, 160));
    canvas.drawCircle(selX, selY, 18, canvas.color565(255, 170, 0));

    // 渲染 28x28 放大图标
    g_assets.drawMenuIcon(canvas, selX - 14, selY - 14, selIdx, true);

    // 4. 在选中图标正下方悬浮展示选项标签
    String label = MENU_NAMES[selIdx];
    if (selIdx == MENU_FEED) label += "(" + String(st.food_count) + ")";
    else if (selIdx == MENU_BATH) label += "(" + String(st.soap_count) + ")";
    else if (selIdx == MENU_WORK) label += "(+150Y)";
    else if (selIdx == MENU_STUDY) label += "(+智力)";
    else if (selIdx == MENU_TRIP) label += "(-100Y)";
    else if (selIdx == MENU_CURE && strlen(st.illness) > 0) label += "·" + String(st.illness);


    canvas.setFont(&fonts::efontCN_12);
    int tagW = canvas.textWidth(label) + 12;
    int tagH = 20;
    int tagX = selX - (tagW / 2); // 居中对齐在图标正下方
    if (tagX + tagW > SCREEN_W - 3) tagX = SCREEN_W - 3 - tagW;
    if (tagX < 4) tagX = 4;
    int tagY = selY + 22; // 位于 18px 放大底座的正下方
    if (tagY + tagH > SCREEN_H - 10) tagY = selY - 26; // 如果靠底部则自动浮动至图标上方

    canvas.fillRoundRect(tagX, tagY, tagW, tagH, 5, canvas.color565(35, 45, 60));
    canvas.drawRoundRect(tagX, tagY, tagW, tagH, 5, canvas.color565(180, 215, 250));
    canvas.setTextColor(TFT_WHITE);
    canvas.drawCenterString(label, tagX + (tagW / 2), tagY + 4);
}







void DisplayEngine::showStatusCard(uint32_t durationMs) {
    statusCardEndTime = millis() + durationMs;
}

void DisplayEngine::showWebPortalCard(uint32_t durationMs) {
    webPortalCardEndTime = millis() + durationMs;
}

#include "network_manager.h"

void DisplayEngine::drawToast() {
    int boxW = SCREEN_W - 12; // 123 像素宽
    int boxX = 6;

    // 1. 如果用户打开了【网页后台管理】，弹出专属大卡片面板 (展示 15 秒或按 A 键退出)
    if (millis() <= webPortalCardEndTime) {
        int cardH = 176;
        int cardY = 32;
        canvas.fillRoundRect(boxX, cardY, boxW, cardH, 8, canvas.color565(248, 252, 255));
        canvas.drawRoundRect(boxX, cardY, boxW, cardH, 8, canvas.color565(120, 175, 235));

        canvas.setFont(&fonts::efontCN_12);
        canvas.setTextSize(1);

        // 标题
        canvas.fillRoundRect(boxX + 2, cardY + 2, boxW - 4, 22, 6, canvas.color565(160, 200, 245));
        canvas.setTextColor(canvas.color565(20, 35, 60));
        canvas.drawCenterString("网络管理后台", SCREEN_W / 2, cardY + 6);

        int textY = cardY + 30;

        if (g_net.isConnected()) {
            const PetState& st = g_pet.getState();
            canvas.setTextColor(canvas.color565(60, 80, 100));
            canvas.drawString("已连WiFi:", boxX + 6, textY);
            canvas.setTextColor(canvas.color565(20, 120, 40));
            canvas.drawString(String(st.wifi_ssid).substring(0, 12), boxX + 6, textY + 16);

            canvas.setTextColor(canvas.color565(60, 80, 100));
            canvas.drawString("局域网后台网址:", boxX + 6, textY + 38);
            canvas.setTextColor(canvas.color565(10, 100, 210));
            canvas.drawString("http://", boxX + 6, textY + 54);
            canvas.drawString(g_net.getIPAddress(), boxX + 6, textY + 70);
        } else {
            canvas.setTextColor(canvas.color565(60, 80, 100));
            canvas.drawString("手机连接WiFi:", boxX + 6, textY);
            canvas.setTextColor(canvas.color565(220, 100, 20));
            canvas.drawString("QQPet-StickS3", boxX + 6, textY + 16);

            canvas.setTextColor(canvas.color565(60, 80, 100));
            canvas.drawString("手机浏览器输入:", boxX + 6, textY + 38);
            canvas.setTextColor(canvas.color565(10, 100, 210));
            canvas.drawString("http://", boxX + 6, textY + 54);
            canvas.drawString("192.168.4.1", boxX + 6, textY + 70);
        }

        // 底部关闭提示
        canvas.setTextColor(canvas.color565(140, 150, 160));
        canvas.drawCenterString("按前面板A键退出", SCREEN_W / 2, cardY + cardH - 20);
        return;
    }

    // 2. 如果有操作提示 (Toast)，优先弹出黑色提示条
    if (millis() <= toastEndTime && toastText.length() > 0) {
        int boxH = 26;
        int boxY = SCREEN_H - 34;
        canvas.fillRoundRect(boxX, boxY, boxW, boxH, 6, canvas.color565(35, 35, 45));
        canvas.setFont(&fonts::efontCN_12);
        canvas.setTextColor(TFT_WHITE);
        canvas.drawCenterString(toastText, SCREEN_W / 2, boxY + 6);
        return;
    }

    // 3. 如果用户选择了【宠物状态】，弹出双行精致五维状态卡片
    if (millis() <= statusCardEndTime) {
        int boxH = 50;
        int boxY = SCREEN_H - 58;

        const PetState& st = g_pet.getState();
        canvas.fillRoundRect(boxX, boxY, boxW, boxH, 8, canvas.color565(245, 250, 255));
        canvas.drawRoundRect(boxX, boxY, boxW, boxH, 8, canvas.color565(140, 185, 235));

        canvas.setFont(&fonts::efontCN_12);
        canvas.setTextSize(1);

        int hMax = g_pet.getMaxHunger();
        int cMax = g_pet.getMaxClean();
        int hungerW = (hMax > 0) ? (st.hunger * 28 / hMax) : 0;
        int cleanW = (cMax > 0) ? (st.clean * 28 / cMax) : 0;
        int moodW = st.mood * 28 / 1000;
        if (hungerW > 28) hungerW = 28;
        if (cleanW > 28) cleanW = 28;
        if (moodW > 28) moodW = 28;

        // 第一行：【饱】进度条 (左) + 【洁】进度条 (右)
        // 饱
        canvas.setTextColor(canvas.color565(220, 100, 30));
        canvas.drawString("饱", boxX + 6, boxY + 7);
        canvas.fillRoundRect(boxX + 20, boxY + 9, 28, 8, 3, canvas.color565(215, 220, 225));
        canvas.fillRoundRect(boxX + 20, boxY + 9, hungerW, 8, 3, (st.hunger < HUNGER_THRESHOLD) ? TFT_RED : canvas.color565(255, 140, 0));

        // 洁
        canvas.setTextColor(canvas.color565(30, 120, 220));
        canvas.drawString("洁", boxX + 58, boxY + 7);
        canvas.fillRoundRect(boxX + 72, boxY + 9, 28, 8, 3, canvas.color565(215, 220, 225));
        canvas.fillRoundRect(boxX + 72, boxY + 9, cleanW, 8, 3, (st.clean < CLEAN_THRESHOLD) ? TFT_RED : canvas.color565(40, 160, 255));

        // 第二行：【心】进度条 (左) + 健康/生病状态 (右)
        // 心
        canvas.setTextColor(canvas.color565(220, 60, 120));
        canvas.drawString("心", boxX + 6, boxY + 28);
        canvas.fillRoundRect(boxX + 20, boxY + 30, 28, 8, 3, canvas.color565(215, 220, 225));
        canvas.fillRoundRect(boxX + 20, boxY + 30, moodW, 8, 3, (st.mood < MOOD_THRESHOLD) ? TFT_RED : canvas.color565(255, 90, 150));

        // 状态说明 (健康/生病)
        if (g_pet.isDead()) {
            canvas.setTextColor(TFT_RED);
            canvas.drawString("状态:已死亡", boxX + 56, boxY + 28);
        } else if (g_pet.isSick()) {
            canvas.setTextColor(canvas.color565(220, 100, 0));
            canvas.drawString(String("病:") + st.illness, boxX + 56, boxY + 28);
        } else {
            canvas.setTextColor(canvas.color565(40, 160, 60));
            canvas.drawString("状态:极健康", boxX + 56, boxY + 28);
        }
        return;
    }

    // 4. 默认状态：完全不绘制，保持主屏纯净
}



