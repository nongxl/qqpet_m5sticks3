#include "display_engine.h"
#include "config.h"
#include "asset_manager.h"
#include "mini_game_manager.h"
#include "vfx_engine.h"
#include "weather_manager.h"
#include "network_manager.h"
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
    else if (subMode == SUB_SCREEN_CURE) maxItems = 1 + MEDICINE_COUNT + 1; // 1个医院打针 + 13种药 + 1个退出项
    else if (subMode == SUB_SCREEN_SHOP) maxItems = SHOP_PRODUCT_COUNT + 1; // 18种商品 + 1个退出项

    else if (subMode == SUB_SCREEN_WORK || subMode == SUB_SCREEN_STUDY || subMode == SUB_SCREEN_TRIP) maxItems = 4 + 1; // 4个时长档位 + 1个退出项
    else if (subMode == SUB_SCREEN_WARDROBE) maxItems = COSTUME_COUNT + 1; // 8款饰品 + 1个退出项
    
    subIndex = (subIndex + 1) % maxItems;
}

void DisplayEngine::prevSubScreenItem() {
    if (subMode == SUB_SCREEN_NONE) return;
    int maxItems = 1;
    if (subMode == SUB_SCREEN_FEED) maxItems = FOOD_COUNT + 1;
    else if (subMode == SUB_SCREEN_CURE) maxItems = 1 + MEDICINE_COUNT + 1;
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

    // 3. 绘制中央白色数字时钟与左侧实时天气胶囊
    drawClockAndWeather();

    // 4. 计算企鹅位置 (根据不同动作状态智能吸附边缘或居中踱步)
    PetAnimState anim = g_pet.getCurrentAnimState();
    int petX = (SCREEN_W / 2) + petOffsetX;
    int petY = 158 + petOffsetY;

    if (anim == ANIM_HIDE_LEFT) {
        // 智能识别屏幕左边缘：将企鹅身体大半隐藏在屏幕左侧边框外，只探出半张脸和小手张望躲猫猫
        petX = 20 + petOffsetX;
    } else if (anim == ANIM_HIDE_RIGHT) {
        // 智能识别屏幕右边缘：将企鹅身体隐藏在屏幕右侧边框外，从右侧边缘探头躲猫猫
        petX = SCREEN_W - 20 + petOffsetX;
    } else if (anim == ANIM_WALK_LEFT || anim == ANIM_WALK_RIGHT) {
        petX = constrain((SCREEN_W / 2) + (int)g_pet.getWalkOffsetX() + petOffsetX, 24, SCREEN_W - 24);
    } else {
        petX += (menuVisible ? 0 : (int)g_pet.getWalkOffsetX());
    }

    if (isDragging) {
        petY += (animFrame % 6 > 3) ? -3 : 3;
        drawPet(petX, petY, ANIM_DRAG);
    } else {
        if (anim == ANIM_PLAY || anim == ANIM_IDLE_BOUNCE || anim == ANIM_WORK || anim == ANIM_STUDY) {
            petY += (animFrame % 8 > 4) ? -3 : 0;
        }
        drawPet(petX, petY, anim);
    }


    // 绘制嘴叼体温计体温气泡
    if (anim == ANIM_TIWENJI) {
        float t = g_pet.getTemperature();
        char tBuf[24];
        snprintf(tBuf, sizeof(tBuf), "体温: %.1fC", t);
        canvas.fillRoundRect(petX - 38, petY - 56, 76, 16, 4, canvas.color565(255, 235, 235));
        canvas.drawRoundRect(petX - 38, petY - 56, 76, 16, 4, canvas.color565(255, 70, 70));
        canvas.setTextColor(canvas.color565(220, 20, 20));
        canvas.drawCenterString(tBuf, petX, petY - 54);
    }

    // 4. 绘制对话气泡
    drawBubble();

    // 5. 绘制右侧仿原版弹出式气泡菜单
    if (menuSlideProgress > 0.05f) {
        drawRightBubbleMenu();
    }

    // 6. 绘制 Toast
    drawToast();

    // 7. 渲染高光交互与粒子特效 (爱心、星星、金光法阵、每日宝箱)
    VfxEngine::getInstance().render(canvas);

    // 推送画布
    canvas.pushSprite(0, 0);
}

void DisplayEngine::drawBackground() {
    const PetState& st = g_pet.getState();
    g_assets.drawBackground(canvas, st.bg_id);
}

void DisplayEngine::drawTopBar() {
    const PetState& st = g_pet.getState();
    
    // 0. 绘制顶部状态栏底座背景 (全宽圆角毛玻璃半透明底座 X=2, Y=2, 宽 131px, 高 19px)
    canvas.fillRoundRect(2, 2, SCREEN_W - 4, 19, 4, canvas.color565(238, 246, 255));
    canvas.drawRoundRect(2, 2, SCREEN_W - 4, 19, 4, canvas.color565(185, 215, 245));

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
        canvas.fillRoundRect(3, 3, 78, 17, 4, (st.current_task == TASK_WORK) ? canvas.color565(255, 240, 210) : canvas.color565(225, 240, 255));
        canvas.drawRoundRect(3, 3, 78, 17, 4, (st.current_task == TASK_WORK) ? canvas.color565(255, 170, 50) : canvas.color565(80, 160, 240));
        canvas.setTextColor((st.current_task == TASK_WORK) ? canvas.color565(200, 90, 0) : canvas.color565(20, 100, 200));
        canvas.drawString(buf, 6, 5);

    } else {
        // 1. 左侧：企鹅昵称与等级 (X=5, Y=5)
        canvas.drawString(String(st.name) + " Lv." + String(g_pet.getLevel()), 5, 5);
    }

    // 2. 右侧信号图标 (向右对齐至 X=98, Y=7 ~ 17)
    bool isWifiOk = g_net.isConnected();
    int sx = 98;
    if (isWifiOk) {
        // 三阶绿色/水蓝饱满信号条
        canvas.fillRect(sx,     13, 2, 3, canvas.color565(30, 180, 80));
        canvas.fillRect(sx + 3, 10, 2, 6, canvas.color565(30, 180, 80));
        canvas.fillRect(sx + 6, 7,  2, 9, canvas.color565(30, 180, 80));
    } else {
        // 灰色微弱信号条 + 红色斜杠
        canvas.fillRect(sx,     13, 2, 3, canvas.color565(190, 195, 200));
        canvas.fillRect(sx + 3, 10, 2, 6, canvas.color565(190, 195, 200));
        canvas.fillRect(sx + 6, 7,  2, 9, canvas.color565(190, 195, 200));
        canvas.drawLine(sx - 1, 7, sx + 8, 16, canvas.color565(230, 60, 60));
    }

    // 3. 右侧电池胶囊图标 (X=114, Y=7, 宽 16px 高 9px)
    int batX = 114;
    int batY = 7;
    // 电池外壳
    canvas.drawRoundRect(batX, batY, 15, 9, 2, canvas.color565(70, 90, 110));
    canvas.fillRect(batX + 15, batY + 2, 2, 5, canvas.color565(70, 90, 110));
    // 内部电量条
    int fillW = map(constrain(cachedBattery, 0, 100), 0, 100, 0, 11);
    uint16_t batCol = (cachedBattery <= 20) ? canvas.color565(230, 50, 50) : 
                      ((cachedBattery <= 50) ? canvas.color565(240, 160, 20) : canvas.color565(40, 180, 70));
    if (fillW > 0) {
        canvas.fillRect(batX + 2, batY + 2, fillW, 5, batCol);
    }

    // 顶部分隔线
    canvas.drawFastHLine(3, 22, SCREEN_W - 6, canvas.color565(190, 215, 240));
}

void DisplayEngine::drawClockAndWeather() {
    // 1. 获取网络对时时间
    struct tm timeinfo;
    bool hasTime = getLocalTime(&timeinfo, 5);
    char timeStr[16];
    if (hasTime && timeinfo.tm_year > (2020 - 1900)) {
        snprintf(timeStr, sizeof(timeStr), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
    } else {
        uint32_t sec = millis() / 1000;
        snprintf(timeStr, sizeof(timeStr), "%02d:%02d", (sec / 3600) % 24, (sec / 60) % 60);
    }

    bool showWeather = g_net.isConnected() && WeatherManager::getInstance().isSyncedWithNetwork();
    WeatherType wt = WeatherManager::getInstance().getCurrentWeather();
    int temp = WeatherManager::getInstance().getCurrentTemp();

    // 2. 计算时钟与天气布局 (Font4 大号粗体时钟宽度约 60px)
    int clockW = 60;
    int weatherW = showWeather ? 38 : 0;
    int totalW = showWeather ? (weatherW + 6 + clockW) : clockW;

    int startX = (SCREEN_W - totalW) / 2;
    int baseY = 25;

    // 3. 仅绘制精致透明边框，绝对不填充任何背景底色 (完全透出底层精美壁纸)
    canvas.drawRoundRect(startX - 5, baseY - 2, totalW + 10, 28, 5, canvas.color565(200, 225, 255));

    int curX = startX;

    // 4. 左侧：天气微标 + 气温 (仅在联网且同步成功时展示)
    if (showWeather) {
        int wx = curX + 6;
        int wy = baseY + 12;
        if (wt == WEATHER_SUNNY) {
            canvas.fillCircle(wx, wy, 4, canvas.color565(255, 200, 0));
            canvas.drawCircle(wx, wy, 5, canvas.color565(255, 140, 0));
        } else if (wt == WEATHER_RAINY) {
            canvas.fillCircle(wx - 2, wy - 1, 4, canvas.color565(120, 180, 230));
            canvas.fillCircle(wx + 3, wy - 1, 4, canvas.color565(120, 180, 230));
            canvas.drawLine(wx - 2, wy + 4, wx - 3, wy + 7, canvas.color565(80, 150, 255));
            canvas.drawLine(wx + 3, wy + 4, wx + 2, wy + 7, canvas.color565(80, 150, 255));
        } else if (wt == WEATHER_SNOWY) {
            canvas.drawLine(wx - 4, wy, wx + 4, wy, canvas.color565(160, 220, 255));
            canvas.drawLine(wx - 4, wy, wx + 4, wy, canvas.color565(160, 220, 255));
            canvas.fillCircle(wx, wy, 2, TFT_WHITE);
        } else {
            canvas.fillCircle(wx - 2, wy, 4, canvas.color565(180, 200, 220));
            canvas.fillCircle(wx + 3, wy - 1, 5, canvas.color565(200, 220, 240));
        }

        // 气温数值 (加粗描边立体纯白文字)
        String tempStr = String(temp) + "C";
        canvas.setFont(&fonts::Font2);
        canvas.setTextColor(TFT_BLACK);
        canvas.drawString(tempStr, curX + 14, baseY + 5);
        canvas.drawString(tempStr, curX + 16, baseY + 5);
        canvas.drawString(tempStr, curX + 15, baseY + 4);
        canvas.drawString(tempStr, curX + 15, baseY + 6);
        canvas.setTextColor(TFT_WHITE);
        canvas.drawString(tempStr, curX + 15, baseY + 5);

        curX += weatherW + 6;
    }

    // 5. 大号加粗白色数字时钟 (Font4 加 4 向黑色描边，立体醒目且绝不遮挡壁纸)
    canvas.setFont(&fonts::Font4);
    canvas.setTextColor(TFT_BLACK);
    canvas.drawString(timeStr, curX - 1, baseY);
    canvas.drawString(timeStr, curX + 1, baseY);
    canvas.drawString(timeStr, curX, baseY - 1);
    canvas.drawString(timeStr, curX, baseY + 1);
    canvas.setTextColor(TFT_WHITE);
    canvas.drawString(timeStr, curX, baseY);
}

#include "asset_manager.h"









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
        int boxW = SCREEN_W - 8; // 127px
        int boxY = 24;           // 紧贴顶部状态栏下方
        int boxH = 86;

        const PetState& st = g_pet.getState();
        int minG = 0, nextG = 0;
        int level = calculateLevel(st.growth, minG, nextG);

        // 高级浅蓝毛玻璃卡片底座 + 柔和边框
        canvas.fillRoundRect(boxX, boxY, boxW, boxH, 6, canvas.color565(245, 250, 255));
        canvas.drawRoundRect(boxX, boxY, boxW, boxH, 6, canvas.color565(130, 180, 235));

        canvas.setFont(&fonts::efontCN_12);

        // 1. 第一行 (Y=28): 等级性别 (左) + 经验进度条 (右)
        canvas.setTextColor(canvas.color565(20, 60, 130));
        String lvStr = "Lv." + String(level) + ((st.gender == 1) ? " (MM)" : " (GG)");
        canvas.drawString(lvStr, boxX + 5, boxY + 4);

        int expRange = (nextG > minG) ? (nextG - minG) : 100;
        int curExp = static_cast<int>(st.growth) - minG;
        if (curExp < 0) curExp = 0;
        int expBarW = (curExp * 32) / expRange;
        if (expBarW > 32) expBarW = 32;

        canvas.setTextColor(canvas.color565(90, 110, 130));
        canvas.drawString("EXP", boxX + 60, boxY + 4);
        canvas.fillRoundRect(boxX + 85, boxY + 6, 34, 6, 3, canvas.color565(215, 225, 235));
        canvas.fillRoundRect(boxX + 85, boxY + 6, expBarW, 6, 3, canvas.color565(60, 160, 255));

        // 2. 第二行 (Y=48): 饱食度条 (左) + 清洁度条 (右)
        int hMax = g_pet.getMaxHunger();
        int cMax = g_pet.getMaxClean();
        int hungerW = (hMax > 0) ? (st.hunger * 26 / hMax) : 0;
        int cleanW = (cMax > 0) ? (st.clean * 26 / cMax) : 0;
        if (hungerW > 26) hungerW = 26;
        if (cleanW > 26) cleanW = 26;

        canvas.setTextColor(canvas.color565(220, 90, 10));
        canvas.drawString("饱", boxX + 5, boxY + 24);
        canvas.fillRoundRect(boxX + 20, boxY + 26, 26, 6, 3, canvas.color565(215, 220, 225));
        canvas.fillRoundRect(boxX + 20, boxY + 26, hungerW, 6, 3, g_pet.isHungry() ? TFT_RED : canvas.color565(255, 140, 0));

        canvas.setTextColor(canvas.color565(20, 120, 220));
        canvas.drawString("洁", boxX + 60, boxY + 24);
        canvas.fillRoundRect(boxX + 75, boxY + 26, 26, 6, 3, canvas.color565(215, 220, 225));
        canvas.fillRoundRect(boxX + 75, boxY + 26, cleanW, 6, 3, g_pet.isDirty() ? TFT_RED : canvas.color565(40, 160, 255));

        // 3. 第三行 (Y=68): 心情条 (左) + 智力与元宝 (右)
        int moodW = st.mood * 26 / 1000;
        if (moodW > 26) moodW = 26;

        canvas.setTextColor(canvas.color565(220, 50, 110));
        canvas.drawString("心", boxX + 5, boxY + 44);
        canvas.fillRoundRect(boxX + 20, boxY + 46, 26, 6, 3, canvas.color565(215, 220, 225));
        canvas.fillRoundRect(boxX + 20, boxY + 46, moodW, 6, 3, (st.mood < MOOD_THRESHOLD) ? TFT_RED : canvas.color565(255, 80, 140));

        canvas.setTextColor(canvas.color565(50, 70, 90));
        canvas.drawString("智" + String(st.intellect), boxX + 54, boxY + 44);

        canvas.setTextColor(canvas.color565(210, 130, 0));
        canvas.drawString(String(st.coins) + "Y", boxX + 90, boxY + 44);

        // 4. 第四行 (Y=88): 健康状态/疾病
        if (g_pet.isDead()) {
            canvas.setTextColor(TFT_RED);
            canvas.drawString("● 状态: 已死亡(需还魂丹)", boxX + 5, boxY + 64);
        } else if (g_pet.isSick()) {
            canvas.setTextColor(canvas.color565(220, 80, 0));
            canvas.drawString(String("● 生病: ") + st.illness + " (需吃药)", boxX + 5, boxY + 64);
        } else {
            canvas.setTextColor(canvas.color565(20, 150, 50));
            canvas.drawString("● 状态: 健康活泼", boxX + 5, boxY + 64);
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
        titleStr = "食物背包";
        rightInfoStr = "饱食:" + String(st.hunger);
        totalItems = FOOD_COUNT + 1;
    } else if (subMode == SUB_SCREEN_CURE) {
        titleStr = "对症药箱";
        rightInfoStr = g_pet.isSick() ? String(st.illness) : "健康";
        totalItems = MEDICINE_COUNT + 1;
    } else if (subMode == SUB_SCREEN_SHOP) {
        titleStr = "元宝商城";
        rightInfoStr = String(st.coins) + " Y";
        totalItems = SHOP_PRODUCT_COUNT + 1;
    } else if (subMode == SUB_SCREEN_WORK) {
        titleStr = "打工搬砖";
        rightInfoStr = "+6Y/分";
        totalItems = 4 + 1; // 4个档位 + 退出项
    } else if (subMode == SUB_SCREEN_STUDY) {
        titleStr = "认真自习";
        rightInfoStr = "+智力";
        totalItems = 4 + 1;
    } else if (subMode == SUB_SCREEN_TRIP) {
        titleStr = "背包旅行";
        rightInfoStr = "80Y/次";
        totalItems = 4 + 1;
    } else if (subMode == SUB_SCREEN_WARDROBE) {
        titleStr = "企鹅衣橱";
        rightInfoStr = "魅力:" + String(st.charm);
        totalItems = COSTUME_COUNT + 1;
    }

    canvas.setTextColor(canvas.color565(255, 220, 80));
    canvas.drawString(titleStr, 8, 6);
    canvas.setTextColor(TFT_WHITE);
    canvas.drawRightString(rightInfoStr, SCREEN_W - 8, 6);

    // 3. 中部滚动卡片列表 (Y=28 ~ 202, 每张卡片高 42px, 左侧带 20x20 素材图标)
    int cardH = 42;
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
        {5, "5分钟 体验", "+30元宝 / +7经验", "+2智力 / +6经验", "近郊漫游/心情满"},
        {15, "15分钟 标准", "+90元宝 / +22经验", "+7智力 / +18经验", "名胜名产/心情满"},
        {30, "30分钟 进阶", "+180元宝 / +45经验", "+15智力 / +36经验", "跨省漫步/特产UP"},
        {60, "60分钟 挂机", "+360元宝 / +90经验", "+30智力 / +72经验", "神州漫步/特产丰"}
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
            canvas.setTextColor(isSelected ? canvas.color565(220, 40, 40) : canvas.color565(110, 125, 140));
            canvas.drawCenterString("[ 退出返回桌面 ]", SCREEN_W / 2, curY + 14);
            continue;
        }

        // 绘制具体数据项 (全宽清爽排版)
        int textX = boxX + 6;

        if (subMode == SUB_SCREEN_FEED) {
            const auto& food = FOOD_LIST[idx];
            int count = g_pet.getFoodCount(idx);

            // 第一行 (Y+4): 食物名称 (左) + 拥有数量 (右)
            canvas.setTextColor(isSelected ? canvas.color565(20, 40, 80) : canvas.color565(50, 70, 95));
            canvas.drawString(food.name, textX, curY + 4);

            if (count > 0) {
                canvas.setTextColor(canvas.color565(15, 140, 40));
                canvas.drawRightString("余:" + String(count), boxX + boxW - 6, curY + 4);
            } else {
                canvas.setTextColor(canvas.color565(220, 50, 50));
                canvas.drawRightString("缺货", boxX + boxW - 6, curY + 4);
            }

            // 第二行 (Y+22): 属性加成
            canvas.setTextColor(isSelected ? canvas.color565(200, 90, 0) : canvas.color565(130, 140, 155));
            String effectStr = "+" + String(food.hunger_gain) + "饱食";
            if (food.mood_gain > 0) effectStr += " / +" + String(food.mood_gain) + "心";
            canvas.drawString(effectStr, textX, curY + 22);

        } else if (subMode == SUB_SCREEN_CURE) {
            if (idx == 0) {
                // 第一项：🏥 社区医院打针看诊 (80元宝)
                canvas.setTextColor(isSelected ? canvas.color565(210, 30, 30) : canvas.color565(190, 40, 40));
                canvas.drawString("🏥 医院急诊打针", textX, curY + 4);

                canvas.setTextColor(canvas.color565(210, 120, 0));
                canvas.drawRightString("80Y", boxX + boxW - 6, curY + 4);

                canvas.setTextColor(isSelected ? canvas.color565(220, 80, 0) : canvas.color565(140, 120, 110));
                canvas.drawString("大针筒扎针 瞬间治愈所有疾病", textX, curY + 22);
            } else {
                int medIdx = idx - 1;
                const auto& med = MEDICINE_LIST[medIdx];
                int count = g_pet.getMedCount(medIdx);
                bool isMatching = (g_pet.isSick() && strcmp(st.illness, med.target_illness) == 0) ||
                                  (g_pet.isDead() && strcmp(med.id, "60001") == 0);

                // 第一行 (Y+4): 药品名称 (左) + 拥有数量 (右)
                canvas.setTextColor(isSelected ? canvas.color565(20, 40, 80) : canvas.color565(50, 70, 95));
                canvas.drawString(med.name, textX, curY + 4);

                if (count > 0) {
                    canvas.setTextColor(canvas.color565(15, 140, 40));
                    canvas.drawRightString("余:" + String(count), boxX + boxW - 6, curY + 4);
                } else {
                    canvas.setTextColor(canvas.color565(220, 50, 50));
                    canvas.drawRightString("无药", boxX + boxW - 6, curY + 4);
                }

                // 第二行 (Y+22): 主治病症
                if (isMatching) {
                    canvas.setTextColor(canvas.color565(0, 150, 40));
                    canvas.drawString("[对症] 治" + String(med.target_illness), textX, curY + 22);
                } else {
                    canvas.setTextColor(isSelected ? canvas.color565(180, 80, 0) : canvas.color565(130, 140, 155));
                    canvas.drawString("主治: " + String(med.target_illness), textX, curY + 22);
                }
            }


        } else if (subMode == SUB_SCREEN_SHOP) {
            const auto& prod = SHOP_PRODUCTS[idx];

            // 第一行 (Y+4): 商品名称 (左) + 价格 (右)
            canvas.setTextColor(isSelected ? canvas.color565(20, 40, 80) : canvas.color565(50, 70, 95));
            canvas.drawString(prod.name, textX, curY + 4);

            canvas.setTextColor(canvas.color565(210, 120, 0));
            canvas.drawRightString(String(prod.price) + "Y", boxX + boxW - 6, curY + 4);

            // 第二行 (Y+22): 效果描述
            canvas.setTextColor(isSelected ? canvas.color565(0, 110, 180) : canvas.color565(130, 140, 155));
            canvas.drawString(prod.desc, textX, curY + 22);

        } else if (subMode == SUB_SCREEN_WORK || subMode == SUB_SCREEN_STUDY || subMode == SUB_SCREEN_TRIP) {
            const auto& opt = TASK_OPTS[idx];
            int curLv = g_pet.getLevel();
            bool isLocked = (subMode == SUB_SCREEN_WORK && curLv < 5) || 
                            (subMode == SUB_SCREEN_STUDY && curLv < 5) || 
                            (subMode == SUB_SCREEN_TRIP && curLv < 12);

            // 第一行 (Y+4): 选项名称 (左) + 锁定/可用状态 (右)
            canvas.setTextColor(isSelected ? canvas.color565(20, 40, 80) : canvas.color565(50, 70, 95));
            canvas.drawString(opt.name, textX, curY + 4);

            if (isLocked) {
                canvas.setTextColor(canvas.color565(230, 40, 40));
                canvas.drawRightString((subMode == SUB_SCREEN_TRIP) ? "[需Lv.12]" : "[需Lv.5]", boxX + boxW - 6, curY + 4);
            } else {
                canvas.setTextColor(canvas.color565(20, 140, 40));
                canvas.drawRightString("可进行", boxX + boxW - 6, curY + 4);
            }

            // 第二行 (Y+22): 任务收益描述
            if (isLocked) {
                canvas.setTextColor(canvas.color565(210, 80, 80));
                canvas.drawString("等级不足 暂未解锁", textX, curY + 22);
            } else {
                canvas.setTextColor(isSelected ? canvas.color565(200, 90, 0) : canvas.color565(120, 135, 150));
                if (subMode == SUB_SCREEN_WORK) {
                    canvas.drawString(opt.workDesc, textX, curY + 22);
                } else if (subMode == SUB_SCREEN_STUDY) {
                    canvas.drawString(opt.studyDesc, textX, curY + 22);
                } else {
                    canvas.drawString(opt.tripDesc, textX, curY + 22);
                }
            }

        } else if (subMode == SUB_SCREEN_WARDROBE) {
            const auto& c = COSTUME_LIST[idx];
            bool owned = g_pet.ownsCostume(c.id);
            bool isEquipped = (g_pet.getEquippedCostume(c.category) == c.id);

            // 第一行 (Y+4): 饰品名称 (左) + 穿戴/拥有/价格 (右)
            canvas.setTextColor(isSelected ? canvas.color565(20, 40, 80) : canvas.color565(50, 70, 95));
            canvas.drawString(c.name, textX, curY + 4);

            if (isEquipped) {
                canvas.setTextColor(canvas.color565(0, 150, 50));
                canvas.drawRightString("已戴上", boxX + boxW - 6, curY + 4);
            } else if (owned) {
                canvas.setTextColor(canvas.color565(20, 110, 210));
                canvas.drawRightString("已拥有", boxX + boxW - 6, curY + 4);
            } else {
                canvas.setTextColor(canvas.color565(210, 120, 0));
                canvas.drawRightString(String(c.price) + "Y", boxX + boxW - 6, curY + 4);
            }

            // 第二行 (Y+22): 穿戴与购买提示
            if (isEquipped) {
                canvas.setTextColor(canvas.color565(0, 140, 50));
                canvas.drawString("[已穿戴] 按A脱下", textX, curY + 22);
            } else if (owned) {
                canvas.setTextColor(isSelected ? canvas.color565(0, 100, 200) : canvas.color565(110, 130, 150));
                canvas.drawString("魅力+" + String(c.charm_gain) + " (按A戴上)", textX, curY + 22);
            } else {
                canvas.setTextColor(isSelected ? canvas.color565(190, 80, 0) : canvas.color565(130, 140, 155));
                canvas.drawString(c.desc, textX, curY + 22);
            }
        }
    }


    // 4. 底部操作指引栏 (Y=214 ~ 236，精炼居中排版，舒适不越界)
    int botY = SCREEN_H - 24;
    canvas.fillRoundRect(8, botY, SCREEN_W - 16, 22, 5, canvas.color565(30, 45, 65));
    canvas.drawRoundRect(8, botY, SCREEN_W - 16, 22, 5, canvas.color565(100, 140, 190));

    canvas.setTextColor(canvas.color565(255, 220, 80));
    if (subMode == SUB_SCREEN_FEED) {
        canvas.drawCenterString("B切换 | A喂食", SCREEN_W / 2, botY + 5);
    } else if (subMode == SUB_SCREEN_CURE) {
        canvas.drawCenterString("B切换 | A服药", SCREEN_W / 2, botY + 5);
    } else if (subMode == SUB_SCREEN_SHOP) {
        canvas.drawCenterString("B切换 | A购买", SCREEN_W / 2, botY + 5);
    } else if (subMode == SUB_SCREEN_WORK) {
        canvas.drawCenterString("B切换 | A打工", SCREEN_W / 2, botY + 5);
    } else if (subMode == SUB_SCREEN_STUDY) {
        canvas.drawCenterString("B切换 | A自习", SCREEN_W / 2, botY + 5);
    } else if (subMode == SUB_SCREEN_TRIP) {
        canvas.drawCenterString("B切换 | A漫游", SCREEN_W / 2, botY + 5);
    } else if (subMode == SUB_SCREEN_WARDROBE) {
        canvas.drawCenterString("B切换 | A穿戴", SCREEN_W / 2, botY + 5);
    }



}








