#include "pet_core.h"
#include <cstring>
#include <algorithm>

PetCore g_pet;

PetCore::PetCore() {
    transientAnim = ANIM_IDLE_STAND;
    transientAnimEndTime = 0;
    currentIdleSubAction = ANIM_IDLE_STAND;
    lastIdleSwitchTime = 0;
    initDefault();
}

void PetCore::initDefault() {
    memset(&state, 0, sizeof(state));
    strncpy(state.name, "Q宝", sizeof(state.name) - 1);
    strncpy(state.host, "主人", sizeof(state.host) - 1);
    state.gender = 0;
    state.is_adopted = false; // 初始未领养，触发首次并排选性别领养仪式
    state.growth = 120.0f;    // 初始 2 级雏鸟
    state.health = HEALTH_NORMAL;
    state.illness[0] = '\0';
    
    int maxVal = calculateMaxHungerClean(2);
    state.hunger = maxVal;
    state.clean = maxVal;
    state.mood = 900;

    state.coins = 500; // 初始赠送 500 元宝
    state.intellect = 100;
    state.charm = 100;
    state.bg_id = 1;   // 默认 1 号经典壁纸
    state.food_count = 50;     // 初始小鱼干 50
    state.food_salmon = 10;    // 初始三文鱼 10
    state.food_icecream = 5;   // 初始雪糕 5
    state.food_feast = 2;      // 初始大餐 2
    state.soap_count = 50;
    state.revival_count = 5;

    state.volume = 120;
    static const char* defaultMeds[] = {
        "10001", "板蓝根",
        "10002", "消食片",
        "10003", "枇杷糖浆",
        "20001", "银翘丸",
        "20002", "蓝色消炎药水",
        "20003", "甘草剂",
        "30001", "金色消炎药水",
        "30002", "龙胆草",
        "30003", "定喘丸",
        "30004", "退烧药",
        "40002", "仙人汤",
        "40003", "通风散",
        "60001", "还魂丹"
    };
    for (int i = 0; i < 13; ++i) {
        strncpy(state.medicines[i].id, defaultMeds[i * 2], sizeof(state.medicines[i].id) - 1);
        strncpy(state.medicines[i].name, defaultMeds[i * 2 + 1], sizeof(state.medicines[i].name) - 1);
        state.medicines[i].count = 5;
    }
}

void PetCore::adopt(uint8_t gender) {
    state.gender = (gender == 1) ? 1 : 0;
    state.is_adopted = true;
    transientAnim = ANIM_LEVELUP;
    transientAnimEndTime = millis() + 4000;
}

void PetCore::resetAdoption() {
    initDefault();
}


bool PetCore::work(String& outMsg) {
    if (isDead()) {
        outMsg = "宠物已死亡，无法打工！";
        return false;
    }
    if (state.health <= 2) {
        outMsg = "宠物病情严重，快去给它看病吧！";
        return false;
    }
    if (state.hunger < 100 || state.clean < 100) {
        outMsg = "太饿或太脏了，没有力气打工啦！";
        return false;
    }

    state.hunger = std::max(0, state.hunger - 60);
    state.clean = std::max(0, state.clean - 60);
    state.coins += 150;
    addGrowth(25.0f);
    triggerTransientAnim(ANIM_WORK, 4500);
    outMsg = "打工搬砖大获丰收！赚取 150 元宝，成长值 +25！";
    return true;
}

bool PetCore::study(String& outMsg) {
    if (isDead()) {
        outMsg = "宠物已死亡，无法学习！";
        return false;
    }
    if (state.health <= 2) {
        outMsg = "生病难受中，头昏脑胀学不进去！";
        return false;
    }
    if (state.hunger < 80 || state.clean < 80) {
        outMsg = "肚子咕咕叫或太脏了，静不下心学习！";
        return false;
    }

    state.hunger = std::max(0, state.hunger - 50);
    state.clean = std::max(0, state.clean - 50);
    state.intellect += 15;
    state.charm += 10;
    addGrowth(30.0f);
    triggerTransientAnim(ANIM_STUDY, 4500);
    outMsg = "专心上课自习！智力 +15，魅力 +10，经验 +30！";
    return true;
}

bool PetCore::trip(String& outMsg) {
    if (isDead()) {
        outMsg = "宠物已死亡，无法旅游！";
        return false;
    }
    if (state.health <= 2) {
        outMsg = "宠物病情严重，无法外出旅行！";
        return false;
    }
    if (state.coins < 100) {
        outMsg = "元宝不足！背包旅行需要 100 元宝路费。";
        return false;
    }
    if (state.hunger < 80) {
        outMsg = "肚子太饿了，没力气出门旅行啦！";
        return false;
    }

    static const char* SCENE_NAMES[] = {
        "经典桌面", "阳光草地", "森林小道", "浪漫海滩", "夜幕星空",
        "企鹅客厅", "梦幻冰屋", "落叶枫林", "童话乐园", "蔚蓝深海",
        "飞舞樱花", "魔法城堡", "农场庄园", "太空星云", "暖冬雪景",
        "新春庭阁", "都市天际"
    };

    state.coins -= 100;
    state.hunger = std::max(0, state.hunger - 60);
    state.mood = 1000;
    addGrowth(50.0f);
    
    // 随机漫游到新风景 (1~16)
    uint8_t newBg = static_cast<uint8_t>(random(1, 17));
    if (newBg == state.bg_id) {
        newBg = (newBg % 16) + 1;
    }
    state.bg_id = newBg;

    triggerTransientAnim(ANIM_TRIP, 4500);
    const char* sceneName = (state.bg_id <= 16) ? SCENE_NAMES[state.bg_id] : "未知秘境";
    outMsg = String("背包旅行归来！来到了【") + sceneName + "】，带回明信片，经验+50！";
    return true;
}







int PetCore::getLevel() const {
    int minG = 0, nextG = 0;
    return calculateLevel(state.growth, minG, nextG);
}

int PetCore::getMaxHunger() const {
    return calculateMaxHungerClean(getLevel());
}

int PetCore::getMaxClean() const {
    return calculateMaxHungerClean(getLevel());
}

void PetCore::addGrowth(float val) {
    if (isDead()) return;
    int oldLv = getLevel();
    state.growth += val;
    checkLevelUp(oldLv);
}

void PetCore::checkLevelUp(int oldLevel) {
    int curLv = getLevel();
    if (curLv > oldLevel) {
        triggerTransientAnim(ANIM_LEVELUP, 4500);
    }
}

int PetCore::getFoodCount(int foodIndex) const {
    switch (foodIndex) {
        case 0: return state.food_count;
        case 1: return state.food_salmon;
        case 2: return state.food_icecream;
        case 3: return state.food_feast;
        default: return 0;
    }
}

int PetCore::getMedCount(int medIndex) const {
    if (medIndex >= 0 && medIndex < 13) {
        if (strcmp(MEDICINE_LIST[medIndex].id, "60001") == 0) {
            return state.revival_count;
        }
        return state.medicines[medIndex].count;
    }
    return 0;
}

bool PetCore::feedFood(int foodIndex, String& outMsg) {
    if (isDead()) {
        outMsg = "宠物已死亡，无法进食！";
        return false;
    }
    if (foodIndex < 0 || foodIndex >= (int)FOOD_COUNT) {
        outMsg = "未知的食物！";
        return false;
    }

    int count = getFoodCount(foodIndex);
    if (count <= 0) {
        outMsg = String(FOOD_LIST[foodIndex].name) + " 库存不足！请前往商城购买。";
        return false;
    }

    // 扣减库存
    switch (foodIndex) {
        case 0: state.food_count--; break;
        case 1: state.food_salmon--; break;
        case 2: state.food_icecream--; break;
        case 3: state.food_feast--; break;
    }

    int maxH = getMaxHunger();
    state.hunger = std::min(state.hunger + FOOD_LIST[foodIndex].hunger_gain, maxH);
    if (FOOD_LIST[foodIndex].mood_gain > 0) {
        state.mood = std::min(state.mood + FOOD_LIST[foodIndex].mood_gain, 1000);
    }
    addGrowth(15.0f);
    triggerTransientAnim(ANIM_EAT, 3500);

    outMsg = String("喂食了[") + FOOD_LIST[foodIndex].name + "]，饱食度 +" + String(FOOD_LIST[foodIndex].hunger_gain) + "！";
    return true;
}

bool PetCore::cureWithMed(int medIndex, String& outMsg) {
    if (medIndex < 0 || medIndex >= (int)MEDICINE_COUNT) {
        outMsg = "未知的药品！";
        return false;
    }

    const auto& med = MEDICINE_LIST[medIndex];
    int count = getMedCount(medIndex);
    if (count <= 0) {
        outMsg = String(med.name) + " 库存不足！请前往商城购买。";
        return false;
    }

    if (strcmp(med.id, "60001") == 0) { // 还魂丹
        if (!isDead()) {
            outMsg = "宠物生命体征正常，无需使用还魂丹！";
            return false;
        }
        revive();
        outMsg = "使用还魂丹成功复活！";
        return true;
    }

    if (isDead()) {
        outMsg = "宠物已死亡，请使用还魂丹！";
        return false;
    }

    if (!isSick()) {
        outMsg = "身体极健康，无需吃药！";
        return false;
    }

    if (strcmp(state.illness, med.target_illness) != 0) {
        outMsg = String("药不对症！") + med.name + " 主治 " + med.target_illness + "，无法治疗 " + state.illness + "。";
        return false;
    }

    // 扣减药品
    state.medicines[medIndex].count--;
    state.health = HEALTH_NORMAL;
    state.illness[0] = '\0';
    addGrowth(25.0f);
    triggerTransientAnim(ANIM_CURE, 3500);
    outMsg = String("服用[") + med.name + "]，成功治愈了 " + med.target_illness + "！";
    return true;
}

bool PetCore::buyShopProduct(int productIndex, int count, String& outMsg) {
    if (productIndex < 0 || productIndex >= (int)SHOP_PRODUCT_COUNT) {
        outMsg = "商品不存在！";
        return false;
    }
    if (count <= 0) count = 1;

    const auto& prod = SHOP_PRODUCTS[productIndex];
    int totalCost = prod.price * count;

    if (state.coins < totalCost) {
        outMsg = String("元宝不足！需要 ") + String(totalCost) + " 元宝，当前拥有 " + String(state.coins) + " 元宝。";
        return false;
    }

    state.coins -= totalCost;

    // 分配到对应库存
    if (strcmp(prod.id, "food_fish") == 0) state.food_count += count;
    else if (strcmp(prod.id, "food_salmon") == 0) state.food_salmon += count;
    else if (strcmp(prod.id, "food_icecream") == 0) state.food_icecream += count;
    else if (strcmp(prod.id, "food_feast") == 0) state.food_feast += count;
    else if (strcmp(prod.id, "soap") == 0) state.soap_count += count;
    else if (strcmp(prod.id, "60001") == 0) state.revival_count += count;
    else {
        for (int i = 0; i < 13; ++i) {
            if (strcmp(state.medicines[i].id, prod.id) == 0) {
                state.medicines[i].count += count;
                break;
            }
        }
    }

    outMsg = String("购买成功！获得了 ") + String(count) + " 个[" + prod.name + "]！";
    return true;
}

bool PetCore::feed(int amount) {

    String msg;
    return feedFood(0, msg);
}

bool PetCore::bath(int amount) {
    if (isDead()) return false;
    if (state.soap_count <= 0) return false;
    state.soap_count--;
    int maxC = getMaxClean();
    state.clean = std::min(state.clean + amount, maxC);
    addGrowth(10.0f);
    triggerTransientAnim(ANIM_CLEAN, 3500);
    return true;
}

bool PetCore::play(int amount) {

    if (isDead()) return false;
    state.mood = std::min(state.mood + amount, 1000);
    addGrowth(5.0f);
    triggerTransientAnim(ANIM_HAPPY, 3000);
    return true;
}

bool PetCore::cure(const char* medicineId) {
    if (!medicineId) return false;
    if (isDead()) {
        if (strcmp(medicineId, "60001") == 0) {
            return revive();
        }
        return false;
    }
    if (!isSick()) return false;

    const char* targetIllness = getIllnessNameByMedicine(medicineId);
    if (strlen(targetIllness) > 0 && strcmp(state.illness, targetIllness) == 0) {
        state.health = HEALTH_NORMAL;
        state.illness[0] = '\0';
        addGrowth(20.0f);
        triggerTransientAnim(ANIM_CURE, 3500);
        return true;
    }
    return false;
}


bool PetCore::autoHeal(String& outMsg) {
    if (isDead()) {
        if (revive()) {
            outMsg = "使用还魂丹成功复活！";
            return true;
        } else {
            outMsg = "还魂丹不足，无法复活！";
            return false;
        }
    }

    if (!isSick()) {
        outMsg = "身体健康，无需吃药。";
        return true;
    }

    const char* medId = getMedicineIdByIllness(state.illness);
    if (cure(medId)) {
        outMsg = String("成功服药，治愈了[") + state.illness + "]！";
        return true;
    }

    outMsg = String("缺少治疗[") + state.illness + "]的药品！";
    return false;
}

bool PetCore::revive() {
    if (!isDead()) return false;
    if (state.revival_count > 0) {
        state.revival_count--;
        state.health = 3;
        strncpy(state.illness, "发烧", sizeof(state.illness) - 1);
        state.hunger = getMaxHunger() / 2;
        state.clean = getMaxClean() / 2;
        state.mood = 500;
        triggerTransientAnim(ANIM_HAPPY, 3500);
        return true;
    }
    return false;
}

void PetCore::tickDecay(uint32_t deltaSeconds) {
    if (isDead() || deltaSeconds == 0) return;

    float cycles = static_cast<float>(deltaSeconds) / 60.0f;
    int baseDecay = static_cast<int>(cycles * 6.0f);
    if (baseDecay < 1 && deltaSeconds >= 30) baseDecay = 1;

    int extraDecay = (state.mood < 600) ? static_cast<int>(cycles * 2.0f) : 0;
    int totalDecay = baseDecay + extraDecay;

    state.hunger = std::max(0, state.hunger - totalDecay);
    state.clean = std::max(0, state.clean - totalDecay);
    state.mood = std::max(0, state.mood - static_cast<int>(cycles * 3.0f));

    if (!isSick()) {
        if (isHungry() || isDirty()) {
            randomIllnessTrigger();
        }
    } else {
        if (random(0, 100) < 5) {
            state.health = std::max(0, state.health - 1);
            if (state.health == 0) {
                triggerTransientAnim(ANIM_DEAD, 10000);
            }
        }
    }
}

void PetCore::randomIllnessTrigger() {
    if (random(0, 100) < 15) {
        int r = random(0, 3);
        if (r == 0) strncpy(state.illness, "感冒", sizeof(state.illness) - 1);
        else if (r == 1) strncpy(state.illness, "咳嗽", sizeof(state.illness) - 1);
        else strncpy(state.illness, "肚子胀", sizeof(state.illness) - 1);
        state.health = 4;
        triggerTransientAnim(ANIM_SICK, 3500);
    }
}

void PetCore::triggerTransientAnim(PetAnimState anim, uint32_t durationMs) {
    uint32_t now = millis();
    // 优先级保护：进食/洗澡/逗玩/打工/学习/旅游/康复/升级/死亡属于核心不可打断动作
    bool isHighPriorityCurrent = (transientAnim == ANIM_EAT || transientAnim == ANIM_CLEAN || 
                                  transientAnim == ANIM_PLAY || transientAnim == ANIM_WORK || 
                                  transientAnim == ANIM_STUDY || transientAnim == ANIM_TRIP || 
                                  transientAnim == ANIM_CURE || transientAnim == ANIM_LEVELUP || 
                                  transientAnim == ANIM_DEAD);
    bool isHighPriorityNew = (anim == ANIM_EAT || anim == ANIM_CLEAN || 
                              anim == ANIM_PLAY || anim == ANIM_WORK || 
                              anim == ANIM_STUDY || anim == ANIM_TRIP || 
                              anim == ANIM_CURE || anim == ANIM_LEVELUP || 
                              anim == ANIM_DEAD);

    if (isHighPriorityCurrent && now < transientAnimEndTime && !isHighPriorityNew) {
        return; // 保护核心动作播完
    }

    transientAnim = anim;
    transientAnimEndTime = now + durationMs;
}



void PetCore::switchRandomIdleAction() {
    // 根据宠物当前心情和状态自主抉择日常动作
    if (isHungry()) {
        currentIdleSubAction = (random(0, 2) == 0) ? ANIM_IDLE_PAT_BELLY : ANIM_IDLE_STAND;
        return;
    }

    int r = random(0, 100);
    if (state.mood >= 800) {
        // 心情极佳：高概率欢快摇摆(happy)、蹦跳(bounce)或四处张望(look)
        if (r < 45) {
            currentIdleSubAction = ANIM_HAPPY;
        } else if (r < 65) {
            currentIdleSubAction = ANIM_IDLE_BOUNCE;
        } else if (r < 80) {
            currentIdleSubAction = ANIM_IDLE_LOOK;
        } else {
            currentIdleSubAction = ANIM_IDLE_STAND;
        }
    } else {
        // 平静日常：站立呼吸、张望、抓痒、伸懒腰、打瞌睡
        if (r < 35) {
            currentIdleSubAction = ANIM_IDLE_STAND;
        } else if (r < 55) {
            currentIdleSubAction = ANIM_IDLE_LOOK;
        } else if (r < 75) {
            currentIdleSubAction = ANIM_IDLE_SCRATCH;
        } else if (r < 90) {
            currentIdleSubAction = ANIM_IDLE_STRETCH;
        } else {
            currentIdleSubAction = ANIM_IDLE_DOZE;
        }
    }
}

void PetCore::updateAnimState() {
    uint32_t now = millis();
    if (transientAnim != ANIM_IDLE_STAND && now > transientAnimEndTime) {
        transientAnim = ANIM_IDLE_STAND;
    }

    // 自主日常动作轮换 (每 4~6 秒随机切换一次)
    if (now - lastIdleSwitchTime > (4500 + random(0, 2500))) {
        lastIdleSwitchTime = now;
        switchRandomIdleAction();
    }
}

PetAnimState PetCore::getCurrentAnimState() const {
    // 1. 死亡状态
    if (state.health == 0) return ANIM_DEAD;

    // 2. 临时核心动作 (进食、洗澡、逗玩、打工、学习、旅游、康复、升级等)
    if (transientAnim != ANIM_IDLE_STAND && millis() <= transientAnimEndTime) {
        return transientAnim;
    }

    // 3. 濒死状态 (health == 1)
    if (state.health == 1) return ANIM_DYING;

    // 4. 生病状态 (health < 5 或 illness 有病症)
    if (isSick() || state.health < 5) return ANIM_SICK;

    // 5. 心情低落 (mood < 500)
    if (state.mood < 500) return ANIM_SAD;

    // 6. 正常健康态下的自主日常杂耍动作流转
    return currentIdleSubAction;
}

bool PetCore::buyItem(const char* itemId, int count, String& outMsg) {
    if (!itemId) {
        outMsg = "物品不存在！";
        return false;
    }
    for (size_t i = 0; i < SHOP_PRODUCT_COUNT; ++i) {
        if (strcmp(SHOP_PRODUCTS[i].id, itemId) == 0) {
            return buyShopProduct(i, count, outMsg);
        }
    }
    if (strcmp(itemId, "food") == 0) {
        return buyShopProduct(0, count, outMsg);
    }
    outMsg = "未找到对应商品！";
    return false;
}



