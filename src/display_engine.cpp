#include "display_engine.h"
#include "config.h"
#include "asset_manager.h"
#include "mini_game_manager.h"
#include "haptics.h"
#include <M5Unified.h>



DisplayEngine g_display;


DisplayEngine::DisplayEngine() 
    : canvas(&M5.Display), menuVisible(false), menuSlideProgress(0.0f),
      wheelCurrentAngle(0.0f), wheelTargetAngle(0.0f),
      menuLastActiveTime(0), currentOption(MENU_FEED),
      subMode(SUB_SCREEN_NONE), subIndex(0),
      bubbleEndTime(0), typewriterIndex(0), lastTypewriterTime(0), toastEndTime(0),
      statusCardVisible(false), cachedBattery(100), lastBatteryCheckTime(0), animFrame(0), isDragging(false) {}



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

void DisplayEngine::openMenu() {
    menuVisible = true;
    menuLastActiveTime = millis();
}

void DisplayEngine::closeMenu() {
    menuVisible = false;
}

void DisplayEngine::toggleMenu() {
    menuVisible = !menuVisible;
    menuLastActiveTime = millis();
}


void DisplayEngine::nextMenuOption() {
    currentOption = static_cast<MenuOption>((static_cast<int>(currentOption) + 1) % MENU_COUNT);
    menuLastActiveTime = millis();
}

void DisplayEngine::prevMenuOption() {
    currentOption = static_cast<MenuOption>((static_cast<int>(currentOption) + MENU_COUNT - 1) % MENU_COUNT);
    menuLastActiveTime = millis();
}



void DisplayEngine::openSubScreen(SubScreenMode mode) {
    subMode = mode;
    subIndex = 0;
    menuVisible = false; // 打开全屏子界面时关闭轮盘菜单
}

void DisplayEngine::closeSubScreen() {
    subMode = SUB_SCREEN_NONE;
    subIndex = 0;
}

void DisplayEngine::nextSubScreenItem() {
    if (subMode == SUB_SCREEN_NONE) return;
    int maxItems = 1;
    if (subMode == SUB_SCREEN_FEED) maxItems = FOOD_COUNT + 1; // 4种食物 + 1个退出项
    else if (subMode == SUB_SCREEN_CURE) maxItems = MEDICINE_COUNT + 1; // 13种药 + 1个退出项
    else if (subMode == SUB_SCREEN_SHOP) maxItems = SHOP_PRODUCT_COUNT + 1; // 18种商品 + 1个退出项
    else if (subMode == SUB_SCREEN_WORK || subMode == SUB_SCREEN_STUDY || subMode == SUB_SCREEN_TRIP) maxItems = 4 + 1; // 4个时长档位 + 1个退出项
    else if (subMode == SUB_SCREEN_WARDROBE) maxItems = COSTUME_COUNT + 1; // 8款饰品 + 1个退出项
    
    subIndex = (subIndex + 1) % maxItems;
}

void DisplayEngine::prevSubScreenItem() {
    if (subMode == SUB_SCREEN_NONE) return;
    int maxItems = 1;
    if (subMode == SUB_SCREEN_FEED) maxItems = FOOD_COUNT + 1;
    else if (subMode == SUB_SCREEN_CURE) maxItems = MEDICINE_COUNT + 1;
    else if (subMode == SUB_SCREEN_SHOP) maxItems = SHOP_PRODUCT_COUNT + 1;
    else if (subMode == SUB_SCREEN_WORK || subMode == SUB_SCREEN_STUDY || subMode == SUB_SCREEN_TRIP) maxItems = 4 + 1;
    else if (subMode == SUB_SCREEN_WARDROBE) maxItems = COSTUME_COUNT + 1;
    
    subIndex = (subIndex + maxItems - 1) % maxItems;
}



void DisplayEngine::update(int petOffsetX, int petOffsetY) {
    // 0. 如果处于全屏子界面模式 (喂食选择/对症看病/元宝商城)，优先渲染全屏沉浸界面
    if (isSubScreenOpen()) {
        renderSubScreen();
        drawToast();
        canvas.pushSprite(0, 0);
        return;
    }

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

    // 3. 计算企鹅位置 (圆圈菜单模式下企鹅端坐在圆心正中央)
    int petX = (SCREEN_W / 2) + petOffsetX;
    int petY = 158 + petOffsetY;

    
    PetAnimState anim = g_pet.getCurrentAnimState();
    if (isDragging) {
        petY += (animFrame % 6 > 3) ? -3 : 3;
        drawPet(petX, petY, (anim == ANIM_IDLE_STAND) ? ANIM_DRAG : anim);
    } else {
        if (anim == ANIM_PLAY || anim == ANIM_IDLE_BOUNCE || anim == ANIM_WORK || anim == ANIM_STUDY) {
            petY += (animFrame % 8 > 4) ? -3 : 0;
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
    
    // 如果正在持续作业中，展示精美作业状态倒计时徽章
    if (g_pet.isTaskActive()) {
        uint32_t rem = g_pet.getTaskRemainingSec();
        int remM = rem / 60;
        int remS = rem % 60;
        char buf[32];
        const char* taskPrefix = (st.current_task == TASK_WORK) ? "打工" : ((st.current_task == TASK_STUDY) ? "自习" : "漫游");
        snprintf(buf, sizeof(buf), "[%s %02d:%02d]", taskPrefix, remM, remS);

        // 浅橙/浅蓝倒计时胶囊底座
        canvas.fillRoundRect(4, 3, 82, 17, 4, (st.current_task == TASK_WORK) ? canvas.color565(255, 240, 210) : canvas.color565(225, 240, 255));
        canvas.drawRoundRect(4, 3, 82, 17, 4, (st.current_task == TASK_WORK) ? canvas.color565(255, 170, 50) : canvas.color565(80, 160, 240));
        canvas.setTextColor((st.current_task == TASK_WORK) ? canvas.color565(200, 90, 0) : canvas.color565(20, 100, 200));
        canvas.drawString(buf, 7, 5);

        // 电池电量
        canvas.setTextColor(canvas.color565(50, 75, 110));
        canvas.drawRightString(String(cachedBattery) + "%", SCREEN_W - 6, 6);
        canvas.drawFastHLine(4, 22, SCREEN_W - 8, canvas.color565(190, 215, 240));
        return;
    }

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
    int cx = SCREEN_W / 2; // 67 px (企鹅水平中心)
    int cy = 130;          // 企鹅竖直中心
    float radius = menuSlideProgress * 49.0f; // 环绕半径 49px

    static const char* MENU_NAMES[11] = {
        "喂食", "洗澡", "游戏", "衣橱", "打工", "学习", "旅游", "看病", "商城", "状态", "设置"
    };

    const PetState& st = g_pet.getState();

    // 1. 绘制围绕企鹅的半透明发光导轨圆环
    if (menuSlideProgress > 0.3f) {
        int rInt = static_cast<int>(radius);
        canvas.drawCircle(cx, cy, rInt, canvas.color565(180, 215, 255));
        canvas.drawCircle(cx, cy, rInt + 1, canvas.color565(210, 235, 255));
    }

    // 2. 绘制 11 个围绕企鹅的菜单图标
    float sector = (2.0f * 3.14159265f) / static_cast<float>(MENU_COUNT);

    for (int i = 0; i < MENU_COUNT; ++i) {
        if (i == currentOption) continue; // 选中项稍后在顶层突出绘制

        float itemRad = - (3.14159265f / 2.0f) + (static_cast<float>(i) * sector);
        int bx = cx + static_cast<int>(radius * std::cos(itemRad));
        int by = cy + static_cast<int>(radius * std::sin(itemRad));

        // 半透明白蓝圆形底座 (22x22)
        canvas.fillCircle(bx, by, 11, canvas.color565(240, 248, 255));
        canvas.drawCircle(bx, by, 11, canvas.color565(140, 190, 240));

        // 渲染 20x20 精致图标
        g_assets.drawMenuIcon(canvas, bx - 10, by - 10, i, false);
    }

    // 3. 顶层突出绘制当前【重力感应高亮选中项】(放大到 34x34，金色双层发光外环)
    int selIdx = currentOption;
    float selRad = - (3.14159265f / 2.0f) + (static_cast<float>(selIdx) * sector);
    int selX = cx + static_cast<int>(radius * std::cos(selRad));
    int selY = cy + static_cast<int>(radius * std::sin(selRad));

    canvas.fillCircle(selX, selY, 17, canvas.color565(255, 215, 40));
    canvas.drawCircle(selX, selY, 17, canvas.color565(255, 250, 180));
    canvas.drawCircle(selX, selY, 18, canvas.color565(255, 160, 0));

    // 渲染 28x28 放大高光图标
    g_assets.drawMenuIcon(canvas, selX - 14, selY - 14, selIdx, true);

    // 4. 底部中央悬浮展示当前选中的功能胶囊气泡 + 提示
    String label = MENU_NAMES[selIdx];
    if (selIdx == MENU_FEED) label += "(" + String(st.food_count) + ")";
    else if (selIdx == MENU_BATH) label += "(" + String(st.soap_count) + ")";
    else if (selIdx == MENU_WARDROBE) label += "(换装)";
    else if (selIdx == MENU_WORK) label += "(+150Y)";
    else if (selIdx == MENU_STUDY) label += "(+智力)";
    else if (selIdx == MENU_TRIP) label += "(旅行)";
    else if (selIdx == MENU_CURE && strlen(st.illness) > 0) label += "·" + String(st.illness);

    canvas.setFont(&fonts::efontCN_12);
    int tagW = canvas.textWidth(label) + 16;
    int tagH = 20;
    int tagX = (SCREEN_W - tagW) / 2;
    int tagY = SCREEN_H - 32;


    canvas.fillRoundRect(tagX, tagY, tagW, tagH, 6, canvas.color565(30, 45, 65));
    canvas.drawRoundRect(tagX, tagY, tagW, tagH, 6, canvas.color565(255, 215, 80));
    canvas.setTextColor(canvas.color565(255, 240, 120));
    canvas.drawCenterString(label, SCREEN_W / 2, tagY + 4);
}









void DisplayEngine::showStatusCard() {
    statusCardVisible = true;
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

    // 2. 如果有操作提示 (Toast)，弹出自适应多行深色提示条 (彻底杜绝文字超出边框)
    if (millis() <= toastEndTime && toastText.length() > 0) {
        canvas.setFont(&fonts::efontCN_12);
        
        // 自动按最大 9 个汉字折行，自适应计算行高
        std::vector<String> lines;
        String curLine = "";
        int curCharCount = 0;
        int len = toastText.length();
        
        for (int i = 0; i < len;) {
            uint8_t c = (uint8_t)toastText[i];
            int charLen = 1;
            if (c >= 0xE0) charLen = 3;      // 3 字节 UTF-8 汉字
            else if (c >= 0xC0) charLen = 2;
            
            String ch = toastText.substring(i, i + charLen);
            i += charLen;
            
            curLine += ch;
            curCharCount += (charLen > 1) ? 2 : 1;
            
            if (curCharCount >= 18) { // 满 9 个汉字自动折行
                lines.push_back(curLine);
                curLine = "";
                curCharCount = 0;
                if (lines.size() >= 3) break;
            }
        }
        if (curLine.length() > 0 && lines.size() < 3) {
            lines.push_back(curLine);
        }
        if (lines.empty()) lines.push_back(toastText);

        int lineCount = lines.size();
        int boxH = 14 + lineCount * 14;
        int boxY = SCREEN_H - boxH - 6;
        int tX = 4;
        int tW = SCREEN_W - 8; // 127px

        // 深黑蓝高级圆角底座 + 柔和光晕边框
        canvas.fillRoundRect(tX, boxY, tW, boxH, 6, canvas.color565(30, 35, 45));
        canvas.drawRoundRect(tX, boxY, tW, boxH, 6, canvas.color565(120, 155, 195));

        canvas.setTextColor(TFT_WHITE);
        int textStartY = boxY + 6;
        for (size_t l = 0; l < lines.size(); ++l) {
            canvas.drawCenterString(lines[l], SCREEN_W / 2, textStartY + l * 14);
        }
        return;
    }


    // 3. 如果用户选择了【宠物状态】，在上方状态栏下方(Y=26)弹出精致全维属性状态卡片 (常驻显示，按任意键关闭)
    if (statusCardVisible) {
        int boxX = 4;
        int boxW = SCREEN_W - 8; // 127
        int boxY = 26;           // 位于顶部状态栏(0~24)正下方，上方视野开阔不遮挡底部企鹅
        int boxH = 76;

        const PetState& st = g_pet.getState();
        int minG = 0, nextG = 0;
        int level = calculateLevel(st.growth, minG, nextG);

        // 高级毛玻璃浅蓝卡片底座
        canvas.fillRoundRect(boxX, boxY, boxW, boxH, 6, canvas.color565(242, 248, 255));
        canvas.drawRoundRect(boxX, boxY, boxW, boxH, 6, canvas.color565(130, 180, 235));

        canvas.setFont(&fonts::efontCN_12);
        canvas.setTextSize(1);

        // 1. 第一行 (Y=30): 等级、性别与经验进度条
        canvas.setTextColor(canvas.color565(20, 60, 120));
        String lvStr = "Lv." + String(level) + ((st.gender == 1) ? "(MM)" : "(GG)");
        canvas.drawString(lvStr, boxX + 5, boxY + 4);

        int expRange = (nextG > minG) ? (nextG - minG) : 100;
        int curExp = static_cast<int>(st.growth) - minG;
        if (curExp < 0) curExp = 0;
        int expBarW = (curExp * 36) / expRange;
        if (expBarW > 36) expBarW = 36;

        canvas.setTextColor(canvas.color565(100, 110, 130));
        canvas.drawString("EXP", boxX + 60, boxY + 4);
        canvas.fillRoundRect(boxX + 83, boxY + 6, 36, 7, 3, canvas.color565(215, 225, 235));
        canvas.fillRoundRect(boxX + 83, boxY + 6, expBarW, 7, 3, canvas.color565(80, 175, 255));

        // 2. 第二行 (Y=46): 饱食度条 (左) + 清洁度条 (右)
        int hMax = g_pet.getMaxHunger();
        int cMax = g_pet.getMaxClean();
        int hungerW = (hMax > 0) ? (st.hunger * 28 / hMax) : 0;
        int cleanW = (cMax > 0) ? (st.clean * 28 / cMax) : 0;
        if (hungerW > 28) hungerW = 28;
        if (cleanW > 28) cleanW = 28;

        canvas.setTextColor(canvas.color565(220, 100, 30));
        canvas.drawString("饱", boxX + 5, boxY + 22);
        canvas.fillRoundRect(boxX + 20, boxY + 24, 28, 7, 3, canvas.color565(215, 220, 225));
        canvas.fillRoundRect(boxX + 20, boxY + 24, hungerW, 7, 3, g_pet.isHungry() ? TFT_RED : canvas.color565(255, 140, 0));

        canvas.setTextColor(canvas.color565(30, 120, 220));
        canvas.drawString("洁", boxX + 60, boxY + 22);
        canvas.fillRoundRect(boxX + 75, boxY + 24, 28, 7, 3, canvas.color565(215, 220, 225));
        canvas.fillRoundRect(boxX + 75, boxY + 24, cleanW, 7, 3, g_pet.isDirty() ? TFT_RED : canvas.color565(40, 160, 255));

        // 3. 第三行 (Y=62): 心情条 (左) + 智力与元宝 (右)
        int moodW = st.mood * 28 / 1000;
        if (moodW > 28) moodW = 28;

        canvas.setTextColor(canvas.color565(220, 60, 120));
        canvas.drawString("心", boxX + 5, boxY + 40);
        canvas.fillRoundRect(boxX + 20, boxY + 42, 28, 7, 3, canvas.color565(215, 220, 225));
        canvas.fillRoundRect(boxX + 20, boxY + 42, moodW, 7, 3, (st.mood < MOOD_THRESHOLD) ? TFT_RED : canvas.color565(255, 90, 150));

        canvas.setTextColor(canvas.color565(60, 70, 90));
        String intelStr = "智" + String(st.intellect);
        canvas.drawString(intelStr, boxX + 54, boxY + 40);

        // 绘制 10x8 精致金元宝矢量小图标 (金黄元宝底座 + 椭圆金顶高光)
        int coinX = boxX + 88;
        int coinY = boxY + 42;
        canvas.fillRoundRect(coinX, coinY + 2, 10, 5, 2, canvas.color565(255, 185, 0));
        canvas.fillCircle(coinX + 5, coinY + 2, 2, canvas.color565(255, 235, 90));
        canvas.drawRoundRect(coinX, coinY + 2, 10, 5, 2, canvas.color565(210, 140, 0));

        String coinStr = String(st.coins);
        canvas.drawString(coinStr, coinX + 13, boxY + 40);

        // 4. 第四行 (Y=78): 健康状态/疾病
        if (g_pet.isDead()) {
            canvas.setTextColor(TFT_RED);
            canvas.drawString("● 状态: 已死亡(需还魂丹)", boxX + 5, boxY + 58);
        } else if (g_pet.isSick()) {
            canvas.setTextColor(canvas.color565(220, 90, 0));
            canvas.drawString(String("● 生病: ") + st.illness + " (请吃药)", boxX + 5, boxY + 58);
        } else {
            canvas.setTextColor(canvas.color565(30, 150, 60));
            canvas.drawString("● 状态: 极健康活泼", boxX + 5, boxY + 58);
        }
        return;
    }


    // 4. 默认状态：完全不绘制，保持主屏纯净
}

void DisplayEngine::drawAdoptionScreen(uint8_t selectedGender) {
    // 1. 背景：经典晴空草地
    g_assets.drawBackground(canvas, 1);

    // 2. 顶部领养仪式横幅 (全宽舒展，留有舒适呼吸边距)
    canvas.fillRoundRect(2, 4, SCREEN_W - 4, 38, 6, canvas.color565(30, 55, 95));
    canvas.drawRoundRect(2, 4, SCREEN_W - 4, 38, 6, canvas.color565(140, 185, 235));
    canvas.setFont(&fonts::efontCN_12);
    canvas.setTextColor(canvas.color565(255, 220, 80));
    canvas.drawCenterString("【QQ宠物领养仪式】", SCREEN_W / 2, 9);
    canvas.setTextColor(TFT_WHITE);
    canvas.drawCenterString("选择陪伴一生的专属萌宠", SCREEN_W / 2, 23);



    // 3. 中间并排展示两只活泼企鹅 (左GG 帅哥 vs 右MM 妹子)
    uint32_t now = millis();

    // --- 左侧 GG 企鹅 ---
    bool isGg = (selectedGender == 0);
    int ggX = 35;
    int ggY = 126;
    if (isGg) {
        // 金色高亮光环
        canvas.fillEllipse(ggX, ggY, 24, 10, canvas.color565(255, 220, 50));
        canvas.drawEllipse(ggX, ggY, 24, 10, canvas.color565(255, 160, 0));
    } else {
        canvas.fillEllipse(ggX, ggY, 20, 7, canvas.color565(180, 200, 220));
    }
    // 极速绘制 GG 企鹅帧 (零 Flash I/O 冲突，满速 30 FPS)
    g_assets.drawAdoptionPet(canvas, ggX - 48, ggY - 78, 0, isGg, now);
    
    // GG 标签
    if (isGg) {
        canvas.fillRoundRect(ggX - 24, ggY + 8, 48, 16, 4, canvas.color565(255, 180, 0));
        canvas.setTextColor(TFT_WHITE);
        canvas.drawCenterString("GG 帅哥", ggX, ggY + 10);
    } else {
        canvas.fillRoundRect(ggX - 22, ggY + 8, 44, 16, 4, canvas.color565(210, 225, 240));
        canvas.setTextColor(canvas.color565(80, 95, 115));
        canvas.drawCenterString("GG 帅哥", ggX, ggY + 10);
    }

    // --- 右侧 MM 企鹅 ---
    bool isMm = (selectedGender == 1);
    int mmX = 99;
    int mmY = 126;
    if (isMm) {
        // 粉红高亮光环
        canvas.fillEllipse(mmX, mmY, 24, 10, canvas.color565(255, 140, 180));
        canvas.drawEllipse(mmX, mmY, 24, 10, canvas.color565(230, 60, 120));
    } else {
        canvas.fillEllipse(mmX, mmY, 20, 7, canvas.color565(180, 200, 220));
    }
    // 极速绘制 MM 企鹅帧 (零 Flash I/O 冲突，满速 30 FPS)
    g_assets.drawAdoptionPet(canvas, mmX - 48, mmY - 78, 1, isMm, now);


    // MM 标签
    if (isMm) {
        canvas.fillRoundRect(mmX - 24, mmY + 8, 48, 16, 4, canvas.color565(255, 90, 140));
        canvas.setTextColor(TFT_WHITE);
        canvas.drawCenterString("MM 妹子", mmX, mmY + 10);
    } else {
        canvas.fillRoundRect(mmX - 22, mmY + 8, 44, 16, 4, canvas.color565(210, 225, 240));
        canvas.setTextColor(canvas.color565(80, 95, 115));
        canvas.drawCenterString("MM 妹子", mmX, mmY + 10);
    }

    // 4. 底部操作指引卡片 (全宽舒适排版)
    int guideY = 158;
    canvas.fillRoundRect(3, guideY, SCREEN_W - 6, 76, 6, canvas.color565(248, 252, 255));
    canvas.drawRoundRect(3, guideY, SCREEN_W - 6, 76, 6, canvas.color565(140, 185, 235));

    canvas.setTextColor(canvas.color565(40, 80, 140));
    canvas.drawString("> 按【侧键BtnB】切换", 8, guideY + 6);
    canvas.setTextColor(canvas.color565(220, 100, 0));
    canvas.drawString("* 按【面板BtnA】领养", 8, guideY + 24);
    canvas.setTextColor(canvas.color565(120, 130, 150));
    canvas.drawString("注: 领养后将终生绑定性别", 8, guideY + 44);
    canvas.drawString("专属陪伴，不可随意更换", 8, guideY + 58);

    // 推送画布
    canvas.pushSprite(0, 0);
}

void DisplayEngine::renderSubScreen() {
    if (subMode == SUB_SCREEN_GAMES) {
        MiniGameManager::getInstance().render(canvas);
        return;
    }

    // 1. 全屏淡蓝毛玻璃背景
    canvas.fillScreen(canvas.color565(235, 244, 255));
    canvas.setFont(&fonts::efontCN_12);


    const PetState& st = g_pet.getState();

    // 2. 顶部高级标题栏 (Y=2 ~ 26)
    canvas.fillRoundRect(2, 2, SCREEN_W - 4, 24, 4, canvas.color565(30, 60, 105));
    canvas.drawRoundRect(2, 2, SCREEN_W - 4, 24, 4, canvas.color565(120, 175, 235));

    String titleStr = "";
    String rightInfoStr = "";

    int totalItems = 0;
    if (subMode == SUB_SCREEN_FEED) {
        titleStr = "【食物背包】";
        rightInfoStr = "饱食:" + String(st.hunger);
        totalItems = FOOD_COUNT + 1;
    } else if (subMode == SUB_SCREEN_CURE) {
        titleStr = "【对症药箱】";
        rightInfoStr = g_pet.isSick() ? String("病:") + st.illness : (g_pet.isDead() ? "已死亡" : "健康");
        totalItems = MEDICINE_COUNT + 1;
    } else if (subMode == SUB_SCREEN_SHOP) {
        titleStr = "【元宝商城】";
        rightInfoStr = String(st.coins) + " 元宝";
        totalItems = SHOP_PRODUCT_COUNT + 1;
    } else if (subMode == SUB_SCREEN_WORK) {
        titleStr = "【打工搬砖】";
        rightInfoStr = "收益:6Y/分";
        totalItems = 4 + 1; // 4个档位 + 退出项
    } else if (subMode == SUB_SCREEN_STUDY) {
        titleStr = "【认真自习】";
        rightInfoStr = "智力+经验";
        totalItems = 4 + 1;
    } else if (subMode == SUB_SCREEN_TRIP) {
        titleStr = "【背包旅行】";
        rightInfoStr = "路费:80Y";
        totalItems = 4 + 1;
    } else if (subMode == SUB_SCREEN_WARDROBE) {
        titleStr = "【👗 企鹅衣橱】";
        rightInfoStr = "魅力:" + String(st.charm);
        totalItems = COSTUME_COUNT + 1;
    }


    canvas.setTextColor(canvas.color565(255, 220, 80));
    canvas.drawString(titleStr, 6, 6);
    canvas.setTextColor(TFT_WHITE);
    canvas.drawRightString(rightInfoStr, SCREEN_W - 6, 6);

    // 3. 中部滚动卡片列表 (Y=28 ~ 212, 每张卡片高 36px, 同时显示 4 项)
    int cardH = 36;
    int visibleCount = 4;
    int startIdx = 0;
    if (subIndex >= visibleCount) {
        startIdx = subIndex - visibleCount + 1;
    }
    if (startIdx + visibleCount > totalItems) {
        startIdx = std::max(0, totalItems - visibleCount);
    }

    static const struct {
        int mins;
        const char* name;
        const char* workDesc;
        const char* studyDesc;
        const char* tripDesc;
    } TASK_OPTS[] = {
        {5, "5 分钟 (快速体验)", "+30 元宝 / +7 经验", "+2 智力 / +6 经验", "漫游周边，心情满格"},
        {15, "15 分钟 (标准作业)", "+90 元宝 / +22 经验", "+7 智力 / +18 经验", "风景名胜，带回特产"},
        {30, "30 分钟 (深度沉浸)", "+180 元宝 / +45 经验", "+15 智力 / +36 经验", "跨省漫游，结交好友"},
        {60, "60 分钟 (长效挂机)", "+360 元宝 / +90 经验", "+30 智力 / +72 经验", "神州漫步，豪华特产"}
    };

    int startY = 28;
    for (int i = 0; i < visibleCount && (startIdx + i) < totalItems; ++i) {
        int idx = startIdx + i;
        int curY = startY + i * (cardH + 2);
        bool isSelected = (idx == subIndex);

        int boxX = 3;
        int boxW = SCREEN_W - 6;

        if (isSelected) {
            // 金色高光放大卡片
            canvas.fillRoundRect(boxX, curY, boxW, cardH, 5, canvas.color565(255, 250, 225));
            canvas.drawRoundRect(boxX, curY, boxW, cardH, 5, canvas.color565(255, 170, 0));
            canvas.drawRoundRect(boxX + 1, curY + 1, boxW - 2, cardH - 2, 4, canvas.color565(255, 215, 80));
        } else {
            canvas.fillRoundRect(boxX, curY, boxW, cardH, 5, canvas.color565(248, 252, 255));
            canvas.drawRoundRect(boxX, curY, boxW, cardH, 5, canvas.color565(200, 220, 240));
        }

        // 最后一项是退出项
        if (idx == totalItems - 1) {
            canvas.setTextColor(isSelected ? canvas.color565(220, 50, 50) : canvas.color565(120, 130, 140));
            canvas.drawCenterString("[ 退出返回桌面 ]", SCREEN_W / 2, curY + 11);
            continue;
        }

        // 绘制具体数据项
        if (subMode == SUB_SCREEN_FEED) {
            const auto& food = FOOD_LIST[idx];
            int count = g_pet.getFoodCount(idx);

            // 第一行：食物名称 + 拥有数量
            canvas.setTextColor(isSelected ? canvas.color565(20, 40, 80) : canvas.color565(60, 80, 100));
            canvas.drawString(food.name, boxX + 6, curY + 4);

            if (count > 0) {
                canvas.setTextColor(canvas.color565(20, 140, 40));
                canvas.drawRightString("拥有:" + String(count), boxX + boxW - 6, curY + 4);
            } else {
                canvas.setTextColor(canvas.color565(220, 60, 60));
                canvas.drawRightString("缺货", boxX + boxW - 6, curY + 4);
            }

            // 第二行：属性加成
            canvas.setTextColor(isSelected ? canvas.color565(200, 100, 0) : canvas.color565(140, 150, 160));
            String effectStr = "+" + String(food.hunger_gain) + "饱食";
            if (food.mood_gain > 0) effectStr += "/+" + String(food.mood_gain) + "心";
            canvas.drawString(effectStr, boxX + 6, curY + 19);

        } else if (subMode == SUB_SCREEN_CURE) {
            const auto& med = MEDICINE_LIST[idx];
            int count = g_pet.getMedCount(idx);
            bool isMatching = (g_pet.isSick() && strcmp(st.illness, med.target_illness) == 0) ||
                              (g_pet.isDead() && strcmp(med.id, "60001") == 0);

            // 第一行：药品名称 + 拥有数量
            canvas.setTextColor(isSelected ? canvas.color565(20, 40, 80) : canvas.color565(60, 80, 100));
            canvas.drawString(med.name, boxX + 6, curY + 4);

            if (count > 0) {
                canvas.setTextColor(canvas.color565(20, 140, 40));
                canvas.drawRightString("拥有:" + String(count), boxX + boxW - 6, curY + 4);
            } else {
                canvas.setTextColor(canvas.color565(220, 60, 60));
                canvas.drawRightString("无药", boxX + boxW - 6, curY + 4);
            }

            // 第二行：主治病症 (对症时高亮绿标)
            if (isMatching) {
                canvas.setTextColor(canvas.color565(0, 160, 50));
                canvas.drawString("[对症] 主治: " + String(med.target_illness), boxX + 6, curY + 19);
            } else {
                canvas.setTextColor(isSelected ? canvas.color565(180, 90, 0) : canvas.color565(140, 150, 160));
                canvas.drawString("主治: " + String(med.target_illness), boxX + 6, curY + 19);
            }

        } else if (subMode == SUB_SCREEN_SHOP) {
            const auto& prod = SHOP_PRODUCTS[idx];
            
            // 第一行：商品名称 + 价格
            canvas.setTextColor(isSelected ? canvas.color565(20, 40, 80) : canvas.color565(60, 80, 100));
            canvas.drawString(prod.name, boxX + 6, curY + 4);

            canvas.setTextColor(canvas.color565(220, 120, 0));
            canvas.drawRightString(String(prod.price) + "Y", boxX + boxW - 6, curY + 4);

            // 第二行：效果描述
            canvas.setTextColor(isSelected ? canvas.color565(0, 120, 180) : canvas.color565(140, 150, 160));
            canvas.drawString(prod.desc, boxX + 6, curY + 19);
        } else if (subMode == SUB_SCREEN_WORK || subMode == SUB_SCREEN_STUDY || subMode == SUB_SCREEN_TRIP) {
            const auto& opt = TASK_OPTS[idx];
            int curLv = g_pet.getLevel();
            bool isLocked = (subMode == SUB_SCREEN_WORK && curLv < 5) || 
                            (subMode == SUB_SCREEN_STUDY && curLv < 5) || 
                            (subMode == SUB_SCREEN_TRIP && curLv < 12);

            canvas.setTextColor(isSelected ? canvas.color565(20, 40, 80) : canvas.color565(60, 80, 100));
            canvas.drawString(opt.name, boxX + 6, curY + 4);

            if (isLocked) {
                canvas.setTextColor(canvas.color565(230, 70, 70));
                canvas.drawRightString((subMode == SUB_SCREEN_TRIP) ? "🔒需Lv.12" : "🔒需Lv.5", boxX + boxW - 6, curY + 4);
            }

            canvas.setTextColor(isSelected ? canvas.color565(220, 100, 0) : canvas.color565(140, 150, 160));
            if (isLocked) {
                canvas.drawString((subMode == SUB_SCREEN_TRIP) ? "成年长成大企鹅后方可远行" : "破壳长大后方可解锁", boxX + 6, curY + 19);
            } else if (subMode == SUB_SCREEN_WORK) {
                canvas.drawString(opt.workDesc, boxX + 6, curY + 19);
            } else if (subMode == SUB_SCREEN_STUDY) {
                canvas.drawString(opt.studyDesc, boxX + 6, curY + 19);
            } else {
                canvas.drawString(opt.tripDesc, boxX + 6, curY + 19);
            }
        } else if (subMode == SUB_SCREEN_WARDROBE) {


            const auto& c = COSTUME_LIST[idx];
            bool owned = g_pet.ownsCostume(c.id);
            bool isEquipped = (g_pet.getEquippedCostume(c.category) == c.id);

            // 第一行：饰品名称 + 状态/售价
            canvas.setTextColor(isSelected ? canvas.color565(20, 40, 80) : canvas.color565(60, 80, 100));
            canvas.drawString(c.name, boxX + 6, curY + 4);

            if (isEquipped) {
                canvas.setTextColor(canvas.color565(0, 160, 60));
                canvas.drawRightString("✨已戴上", boxX + boxW - 6, curY + 4);
            } else if (owned) {
                canvas.setTextColor(canvas.color565(20, 120, 220));
                canvas.drawRightString("已拥有", boxX + boxW - 6, curY + 4);
            } else {
                canvas.setTextColor(canvas.color565(220, 120, 0));
                canvas.drawRightString(String(c.price) + "Y", boxX + boxW - 6, curY + 4);
            }

            // 第二行：效果与穿戴提示
            if (isEquipped) {
                canvas.setTextColor(canvas.color565(0, 140, 50));
                canvas.drawString("[已穿戴] 魅力+" + String(c.charm_gain) + " (按A脱下)", boxX + 6, curY + 19);
            } else if (owned) {
                canvas.setTextColor(isSelected ? canvas.color565(0, 100, 200) : canvas.color565(120, 140, 160));
                canvas.drawString("魅力+" + String(c.charm_gain) + " (按A戴上)", boxX + 6, curY + 19);
            } else {
                canvas.setTextColor(isSelected ? canvas.color565(200, 90, 0) : canvas.color565(140, 150, 160));
                canvas.drawString(c.desc, boxX + 6, curY + 19);
            }
        }
    }

    // 4. 底部操作指引栏 (Y=216 ~ 238)
    int botY = SCREEN_H - 22;
    canvas.fillRoundRect(2, botY, SCREEN_W - 4, 20, 4, canvas.color565(30, 45, 65));
    canvas.drawRoundRect(2, botY, SCREEN_W - 4, 20, 4, canvas.color565(100, 140, 190));

    canvas.setTextColor(canvas.color565(255, 220, 80));
    if (subMode == SUB_SCREEN_FEED) {
        canvas.drawCenterString("BtnB切换 | BtnA喂食", SCREEN_W / 2, botY + 4);
    } else if (subMode == SUB_SCREEN_CURE) {
        canvas.drawCenterString("BtnB切换 | BtnA服药", SCREEN_W / 2, botY + 4);
    } else if (subMode == SUB_SCREEN_SHOP) {
        canvas.drawCenterString("BtnB切换 | BtnA购买", SCREEN_W / 2, botY + 4);
    } else if (subMode == SUB_SCREEN_WORK) {
        canvas.drawCenterString("BtnB切换 | BtnA开始打工", SCREEN_W / 2, botY + 4);
    } else if (subMode == SUB_SCREEN_STUDY) {
        canvas.drawCenterString("BtnB切换 | BtnA开始自习", SCREEN_W / 2, botY + 4);
    } else if (subMode == SUB_SCREEN_TRIP) {
        canvas.drawCenterString("BtnB切换 | BtnA出发漫游", SCREEN_W / 2, botY + 4);
    } else if (subMode == SUB_SCREEN_WARDROBE) {
        canvas.drawCenterString("BtnB切换 | BtnA穿脱/购买", SCREEN_W / 2, botY + 4);
    }

}








