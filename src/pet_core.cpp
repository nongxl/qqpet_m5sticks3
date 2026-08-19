#include "pet_core.h"
#include "sound_manager.h"
#include "vfx_engine.h"
#include "weather_manager.h"
#include <cstring>
#include <algorithm>

PetCore g_pet;

float PetCore::getTemperature() const {
    if (state.health == 0) return 0.0f;
    if (isSick()) {
        return 38.6f + ((millis() / 5000) % 15) * 0.1f; // 38.6 ~ 40.0 ℃ 高烧
    }
    return 36.8f + ((millis() / 5000) % 5) * 0.1f;    // 36.8 ~ 37.2 ℃ 正常体温
}

bool PetCore::takeHospitalInjection(String& outMsg) {
    if (isDead()) {
        outMsg = "宠物已死亡，请使用还魂丹复活！";
        return false;
    }
    if (!isSick() && state.health >= HEALTH_NORMAL) {
        outMsg = "体魄健壮体温正常，无需去医院打针哦！";
        return false;
    }
    int cost = 80;
    if (state.coins < cost) {
        outMsg = "元宝不足！社区医院打针需要 " + String(cost) + " 元宝。";
        return false;
    }

    state.coins -= cost;
    state.health = HEALTH_NORMAL;
    state.illness[0] = '\0';
    state.mood = std::min(1000, state.mood + 400);
    addGrowth(30.0f);

    triggerTransientAnim(ANIM_INJECTION, 4500);
    VfxEngine::getInstance().spawnTears(48, 140, 15);
    g_sound.playSound(SOUND_SICK);

    outMsg = "嗷呜~ 扎了一大针！痛得飙泪，但病痛全消康复啦！";
    return true;
}


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
    state.growth = 0.0f;      // 初始 0 级破壳雏鸟 (Lv.0: 0/50 成长值)
    state.health = HEALTH_NORMAL;
    state.illness[0] = '\0';
    
    int maxVal = calculateMaxHungerClean(0);
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
    state.costume_owned_mask = (1 << 3) | (1 << 5); // 初始默认赠送红领巾(3)和粉红蝴蝶结(5)
    state.equipped_head = 0;
    state.equipped_neck = 0;
    state.equipped_hand = 0;


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


bool PetCore::startTask(PetTaskType type, uint32_t durationSec, String& outMsg) {
    int curLv = getLevel();
    const char* hName = (strlen(state.host) > 0) ? state.host : "主人";

    if (isDead()) {
        outMsg = "您的宠物已死亡，请使用还魂丹复活~";
        return false;
    }
    if (isSick() || state.health < HEALTH_NORMAL) {
        if (type == TASK_WORK) {
            outMsg = String(hName) + "~ 我生病了，等我治疗好了再赚元宝吧~~";
        } else if (type == TASK_STUDY) {
            outMsg = String(hName) + "~ 我生病了，等我治疗好了再上学吧~~";
        } else {
            outMsg = String(hName) + "~ 我生病身体难受，治好病再去旅行吧~~";
        }
        return false;
    }

    // 1. 打工等级门槛 (原版需 Lv.5 破壳成熟)
    if (type == TASK_WORK && curLv < 5) {
        outMsg = String(hName) + "~ 我的等级不够哦(需Lv.5)，陪我长大再试试吧~~";
        return false;
    }

    // 2. 学习等级门槛 (原版小学需 Lv.5 入学)
    if (type == TASK_STUDY && curLv < 5) {
        outMsg = String(hName) + "~ 我还太小啦(需Lv.5)，等我破壳长大了再去上学吧~~";
        return false;
    }

    // 3. 神州旅行等级门槛 (原版需 Lv.12 成年远行)
    if (type == TASK_TRIP && curLv < 12) {
        outMsg = String(hName) + "~ 外面世界好大(需Lv.12)，等我长成大企鹅再去环游神州吧~~";
        return false;
    }

    if (state.hunger < 300 || state.clean < 300) {
        outMsg = "太饿或太脏了，先吃饱洗干净再去吧！";
        return false;
    }
    if (type == TASK_TRIP && state.coins < 80) {
        outMsg = "哎呀，路费元宝不够了(需80Y)，先去打工或小游戏赚点路费吧~";
        return false;
    }


    if (isTaskActive()) {
        String oldMsg;
        stopTask(oldMsg, false);
    }

    if (type == TASK_TRIP) {
        state.coins -= 80;
    }

    state.current_task = static_cast<uint8_t>(type);
    state.task_start_time = millis() / 1000;
    state.task_duration = (durationSec < 60) ? 60 : durationSec;
    state.task_elapsed_sec = 0;

    int mins = state.task_duration / 60;
    if (type == TASK_WORK) {
        g_sound.playSound(SOUND_WORK);
        outMsg = String("开始搬砖打工！预计时长: ") + mins + " 分钟。随时按键可提前召回结算！";
    } else if (type == TASK_STUDY) {
        g_sound.playSound(SOUND_STUDY);
        outMsg = String("开始认真自习！预计时长: ") + mins + " 分钟。智力与学业成长UP！";
    } else if (type == TASK_TRIP) {
        g_sound.playSound(SOUND_HAPPY);
        outMsg = String("背起行囊旅行！预计时长: ") + mins + " 分钟。尽享神州美景！";
    }

    return true;
}

bool PetCore::stopTask(String& outMsg, bool isNaturalFinish) {
    if (!isTaskActive()) {
        outMsg = "当前没有正在进行的作业。";
        return false;
    }

    PetTaskType type = getCurrentTask();
    uint32_t elapsedSec = state.task_elapsed_sec;
    if (elapsedSec < 10) elapsedSec = 10;
    int elapsedMins = (elapsedSec + 59) / 60;

    state.current_task = TASK_NONE;
    state.task_start_time = 0;
    state.task_duration = 0;
    state.task_elapsed_sec = 0;

    if (type == TASK_WORK) {
        int earnedCoins = elapsedMins * 6; // 6元宝/分钟
        float earnedExp = elapsedMins * 1.5f; // 1.5经验/分钟
        state.coins += earnedCoins;
        addGrowth(earnedExp);
        triggerTransientAnim(ANIM_HAPPY, 3500);

        if (isNaturalFinish) {
            outMsg = String("🎉 打工圆满完成！工作 ") + elapsedMins + " 分钟，赚取 " + earnedCoins + " 元宝，经验 +" + static_cast<int>(earnedExp) + "！";
        } else {
            outMsg = String("⚒️ 召回打工企鹅！作业 ") + elapsedMins + " 分钟，按劳结算 " + earnedCoins + " 元宝，经验 +" + static_cast<int>(earnedExp) + "！";
        }
    } else if (type == TASK_STUDY) {
        int intellectGain = std::max(1, static_cast<int>(elapsedMins * 0.5f));
        int charmGain = std::max(1, static_cast<int>(elapsedMins * 0.4f));
        float earnedExp = elapsedMins * 1.2f;
        state.intellect += intellectGain;
        state.charm += charmGain;
        addGrowth(earnedExp);
        triggerTransientAnim(ANIM_HAPPY, 3500);

        if (isNaturalFinish) {
            outMsg = String("🎓 学业圆满完成！自习 ") + elapsedMins + " 分钟，智力 +" + intellectGain + "，魅力 +" + charmGain + "，经验 +" + static_cast<int>(earnedExp) + "！";
        } else {
            outMsg = String("📚 召回自习企鹅！学习 ") + elapsedMins + " 分钟，智力 +" + intellectGain + "，魅力 +" + charmGain + "！";
        }
    } else if (type == TASK_TRIP) {
        float earnedExp = elapsedMins * 1.0f;
        state.mood = 1000;
        addGrowth(earnedExp);
        triggerTransientAnim(ANIM_HAPPY, 3500);

        if (random(0, 100) < 60) {
            state.food_salmon += 2;
            outMsg = String("旅行归来！心情大好，带回特产 [鲜嫩三文鱼 x2]，经验 +") + static_cast<int>(earnedExp) + "！";
        } else {
            state.coins += 50;
            outMsg = String("旅行归来！沿途结交新朋友，带回元宝礼金 +50，经验 +") + static_cast<int>(earnedExp) + "！";
        }

    }
    return true;
}

uint32_t PetCore::getTaskRemainingSec() const {
    if (!isTaskActive()) return 0;
    if (state.task_elapsed_sec >= state.task_duration) return 0;
    return state.task_duration - state.task_elapsed_sec;
}

float PetCore::getTaskProgress() const {
    if (!isTaskActive() || state.task_duration == 0) return 0.0f;
    float prog = static_cast<float>(state.task_elapsed_sec) / static_cast<float>(state.task_duration);
    return constrain(prog, 0.0f, 1.0f);
}

bool PetCore::work(String& outMsg) {
    return startTask(TASK_WORK, 900, outMsg); // 默认 15 分钟
}

bool PetCore::study(String& outMsg) {
    return startTask(TASK_STUDY, 900, outMsg); // 默认 15 分钟
}

bool PetCore::trip(String& outMsg) {
    return startTask(TASK_TRIP, 900, outMsg); // 默认 15 分钟
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
        g_sound.playSound(SOUND_LEVELUP);
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
    g_sound.playSound(SOUND_EAT);

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
        VfxEngine::getInstance().triggerReliveAura(67, 120);
        VfxEngine::getInstance().spawnStars(67, 120, 10);
        g_sound.playSound(SOUND_HAPPY);
        outMsg = "神圣金光降临！使用还魂丹成功复活！";
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
        outMsg = String("用药不对症！当前患有 [") + state.illness + "]，请选用对症药物。";
        return false;
    }

    // 扣减药品
    state.medicines[medIndex].count--;

    // 治愈疾病
    state.illness[0] = '\0';
    state.health = HEALTH_NORMAL;
    state.mood = std::min(state.mood + 300, 1000);
    addGrowth(20.0f);
    triggerTransientAnim(ANIM_CURE, 4000);
    g_sound.playSound(SOUND_HAPPY);

    outMsg = String("对症下药！成功治愈了[") + med.target_illness + "]，健康度恢复满值！";
    return true;
}

bool PetCore::buyShopProduct(int productIndex, int count, String& outMsg) {
    if (productIndex < 0 || productIndex >= (int)SHOP_PRODUCT_COUNT) {
        outMsg = "未知的商品！";
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
    g_sound.playSound(SOUND_COIN);

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
    VfxEngine::getInstance().spawnBubbles(67, 140, 20);
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


    // 1. 如果正在进行作业 (打工/学习/旅游)，推进秒数并执行加速消耗
    if (isTaskActive()) {
        state.task_elapsed_sec += deltaSeconds;

        float taskCycles = static_cast<float>(deltaSeconds) / 60.0f;
        if (state.current_task == TASK_WORK) {
            // 打工搬砖加速消耗：饱食 -10/分, 清洁 -10/分
            state.hunger = std::max(0, state.hunger - static_cast<int>(taskCycles * 10.0f));
            state.clean = std::max(0, state.clean - static_cast<int>(taskCycles * 10.0f));
        } else if (state.current_task == TASK_STUDY) {
            // 学习自习消耗：饱食 -8/分, 清洁 -6/分, 心情 -4/分
            state.hunger = std::max(0, state.hunger - static_cast<int>(taskCycles * 8.0f));
            state.clean = std::max(0, state.clean - static_cast<int>(taskCycles * 6.0f));
            state.mood = std::max(0, state.mood - static_cast<int>(taskCycles * 4.0f));
        } else if (state.current_task == TASK_TRIP) {
            // 旅游消耗：饱食 -6/分, 心情持续保持满格
            state.hunger = std::max(0, state.hunger - static_cast<int>(taskCycles * 6.0f));
            state.mood = 1000;
        }

        // 异常保护：饱食度归零或生病严重，自动中断作业回家
        if (state.hunger <= 0 || state.clean <= 0 || state.health <= 2) {
            String endMsg;
            stopTask(endMsg, false);
            return;
        }

        // 自然达成目标时长，自动完成结算
        if (state.task_elapsed_sec >= state.task_duration) {
            String endMsg;
            stopTask(endMsg, true);
            return;
        }
    }

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
    // 0. 生病状态下：叼体温计测温或顶冰袋
    if (isSick() || state.health < 5) {
        currentIdleSubAction = (random(0, 2) == 0) ? ANIM_TIWENJI : ANIM_SICK;
        return;
    }

    // 1. 如果处于饥饿或脏污状态：生气跺脚、抓痒或站立
    if (isHungry() || isDirty()) {
        int r = random(0, 100);
        if (r < 40) currentIdleSubAction = ANIM_ANGRY;   // 生气跺脚
        else if (r < 70) currentIdleSubAction = ANIM_IDLE_SCRATCH;
        else currentIdleSubAction = ANIM_IDLE_STAND;
        return;
    }

    // 2. 天气联动专属动作优先感知
    WeatherType wt = WeatherManager::getInstance().getCurrentWeather();
    int temp = WeatherManager::getInstance().getCurrentTemp();
    if (wt == WEATHER_RAINY && random(0, 100) < 35) {
        currentIdleSubAction = ANIM_UMBRELLA; // 雨天撑花伞
        return;
    }
    if ((wt == WEATHER_SNOWY || temp < 5) && random(0, 100) < 35) {
        currentIdleSubAction = ANIM_COLD; // 冬雪搓手哈白气
        return;
    }
    if ((wt == WEATHER_SUNNY && temp > 28) && random(0, 100) < 30) {
        currentIdleSubAction = ANIM_SUMMER; // 夏日吹电风扇
        return;
    }

    int r = random(0, 100);
    // 丰富生动的日常动作大合集：
    if (r < 22) {
        currentIdleSubAction = ANIM_IDLE_STAND;
    } else if (r < 35) {
        currentIdleSubAction = ANIM_IDLE_LOOK;
    } else if (r < 46) {
        currentIdleSubAction = ANIM_IDLE_SCRATCH;
    } else if (r < 56) {
        currentIdleSubAction = ANIM_IDLE_STRETCH;
    } else if (r < 66) {
        currentIdleSubAction = ANIM_IDLE_BOUNCE;
    } else if (r < 74) {
        currentIdleSubAction = ANIM_SNEEZE;
    } else if (r < 82) {
        currentIdleSubAction = ANIM_YAWN;
    } else if (r < 88) {
        currentIdleSubAction = ANIM_SHY;
    } else if (r < 94) {
        currentIdleSubAction = ANIM_HIDE_LEFT;
    } else {
        currentIdleSubAction = ANIM_HIDE_RIGHT;
    }
}

void PetCore::updateAnimState() {
    uint32_t now = millis();
    if (transientAnim != ANIM_IDLE_STAND && now > transientAnimEndTime) {
        transientAnim = ANIM_IDLE_STAND;
    }

    // 持续洗澡时生成漫天飞扬的肥皂泡泡
    if (getCurrentAnimState() == ANIM_CLEAN && (now % 160 < 35)) {
        VfxEngine::getInstance().spawnBubbles(67, 145, 2);
    }

    // 医院打针特写时持续飙泪
    if (getCurrentAnimState() == ANIM_INJECTION && (now % 120 < 35)) {
        VfxEngine::getInstance().spawnTears(48, 140, 3);
    }

    // 真实天气全屏粒子飘落
    WeatherType wt = WeatherManager::getInstance().getCurrentWeather();
    if (wt == WEATHER_RAINY && (now % 180 < 35)) {
        VfxEngine::getInstance().spawnRain(2);
    } else if (wt == WEATHER_SNOWY && (now % 250 < 35)) {
        VfxEngine::getInstance().spawnSnow(2);
    }


    // 桌面自主踱步平滑插值位移
    if (fabs(walkTargetX - walkOffsetX) > 0.5f) {
        if (walkTargetX > walkOffsetX) {
            walkOffsetX += 0.5f;
            if (transientAnim == ANIM_IDLE_STAND && !isTaskActive() && !isSick() && state.health > 1) {
                transientAnim = ANIM_WALK_RIGHT;
                transientAnimEndTime = now + 350;
            }
        } else {
            walkOffsetX -= 0.5f;
            if (transientAnim == ANIM_IDLE_STAND && !isTaskActive() && !isSick() && state.health > 1) {
                transientAnim = ANIM_WALK_LEFT;
                transientAnimEndTime = now + 350;
            }
        }
    } else {
        walkOffsetX = walkTargetX;
    }

    // 自主日常动作与偶发踱步决策 (每 6~10 秒切换一次)
    if (now - lastIdleSwitchTime > (6000 + random(0, 4000))) {
        lastIdleSwitchTime = now;
        switchRandomIdleAction();

        // 35% 概率在桌面上自主走动一段小距离 (在 -18px ~ +18px 之间踱步)
        if (!isTaskActive() && !isSick() && state.health > 1 && random(0, 100) < 35) {
            walkTargetX = (float)random(-18, 19);
        }
    }
}






PetAnimState PetCore::getCurrentAnimState() const {
    // 1. 死亡状态
    if (state.health == 0) return ANIM_DEAD;

    // 2. 临时核心动作 (进食、洗澡、逗玩、康复、升级等)
    if (transientAnim != ANIM_IDLE_STAND && millis() <= transientAnimEndTime) {
        return transientAnim;
    }

    // 3. 如果当前处于持续作业状态，持续播放对应作业动画
    if (isTaskActive()) {
        if (state.current_task == TASK_WORK) return ANIM_WORK;
        if (state.current_task == TASK_STUDY) return ANIM_STUDY;
        if (state.current_task == TASK_TRIP) return ANIM_TRIP;
    }

    // 4. 濒死状态 (health == 1)
    if (state.health == 1) return ANIM_DYING;

    // 5. 生病状态 (health < 5 或 illness 有病症)
    if (isSick() || state.health < 5) return ANIM_SICK;

    // 6. 心情低落 (mood < 500)
    if (state.mood < 500) return ANIM_SAD;

    // 7. 正常健康态下的自主日常杂耍动作流转
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

bool PetCore::ownsCostume(int costumeId) const {

    if (costumeId < 1 || costumeId > COSTUME_COUNT) return false;
    return (state.costume_owned_mask & (1 << costumeId)) != 0;
}

bool PetCore::buyCostume(int costumeId, String& outMsg) {
    if (costumeId < 1 || costumeId > COSTUME_COUNT) {
        outMsg = "饰品不存在！";
        return false;
    }
    const auto& c = COSTUME_LIST[costumeId - 1];
    if (ownsCostume(costumeId)) {
        outMsg = String("已拥有【") + c.name + "】，无需重复购买！";
        return false;
    }
    if (state.coins < c.price) {
        outMsg = String("元宝不足！【") + c.name + "】售价 " + c.price + "Y，当前仅有 " + state.coins + "Y";
        return false;
    }
    state.coins -= c.price;
    state.costume_owned_mask |= (1 << costumeId);
    state.charm += c.charm_gain;
    outMsg = String("🎉 购买成功！获得【") + c.name + "】魅力+" + c.charm_gain + "！";
    return true;
}

bool PetCore::toggleEquipCostume(int costumeId, String& outMsg) {
    if (costumeId < 1 || costumeId > COSTUME_COUNT) {
        outMsg = "饰品不存在！";
        return false;
    }
    const auto& c = COSTUME_LIST[costumeId - 1];
    if (!ownsCostume(costumeId)) {
        outMsg = String("未拥有【") + c.name + "】，请先前往商城购买！";
        return false;
    }

    if (c.category == 0) { // 头部
        if (state.equipped_head == costumeId) {
            state.equipped_head = 0;
            outMsg = String("已脱下【") + c.name + "】";
        } else {
            state.equipped_head = costumeId;
            outMsg = String("✨ 已戴上【") + c.name + "】";
        }
    } else if (c.category == 1) { // 颈部
        if (state.equipped_neck == costumeId) {
            state.equipped_neck = 0;
            outMsg = String("已脱下【") + c.name + "】";
        } else {
            state.equipped_neck = costumeId;
            outMsg = String("✨ 已戴上【") + c.name + "】";
        }
    } else { // 随身
        if (state.equipped_hand == costumeId) {
            state.equipped_hand = 0;
            outMsg = String("已收起【") + c.name + "】";
        } else {
            state.equipped_hand = costumeId;
            outMsg = String("✨ 已装备【") + c.name + "】";
        }
    }
    return true;
}

int PetCore::getEquippedCostume(int category) const {
    if (category == 0) return state.equipped_head;
    if (category == 1) return state.equipped_neck;
    return state.equipped_hand;
}




