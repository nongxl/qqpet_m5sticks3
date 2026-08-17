#pragma once
#include <Arduino.h>
#include <vector>

// 属性临界阈值
static constexpr int HUNGER_THRESHOLD = 720;   // 低于此值进入饥饿状态
static constexpr int CLEAN_THRESHOLD  = 1080;  // 低于此值进入脏污状态
static constexpr int MOOD_THRESHOLD   = 100;   // 低于此值心情低落
static constexpr int HEALTH_NORMAL    = 5;     // 正常健康值

// 疾病节点结构
struct IllnessNode {
    const char* name;
    int health;
    const char* cure_id;
    const char* cure_name;
    const IllnessNode* child; // 恶化后的下级疾病
};

// 药品定义
struct MedicineInfo {
    const char* id;
    const char* name;
    const char* target_illness;
};

// 经验等级与属性计算
int calculateLevel(float growth, int& currentLevelMinGrowth, int& nextLevelGrowth);
int calculateMaxHungerClean(int level);
const char* getIllnessNameByMedicine(const char* medicineId);
const char* getMedicineIdByIllness(const char* illnessName);
const IllnessNode* getIllnessNode(const char* illnessName);
const char* getRandomClassicQuote(const char* context);

// 商城价格查询
int getItemPrice(const char* itemId);
const char* getItemName(const char* itemId);


// 经典台词场景
extern const char* QUOTES_IDLE[];
extern const size_t QUOTES_IDLE_COUNT;
extern const char* QUOTES_HUNGRY[];
extern const size_t QUOTES_HUNGRY_COUNT;
extern const char* QUOTES_DIRTY[];
extern const size_t QUOTES_DIRTY_COUNT;
extern const char* QUOTES_SICK[];
extern const size_t QUOTES_SICK_COUNT;
extern const char* QUOTES_HAPPY[];
extern const size_t QUOTES_HAPPY_COUNT;
extern const char* QUOTES_LEVELUP[];
extern const size_t QUOTES_LEVELUP_COUNT;
