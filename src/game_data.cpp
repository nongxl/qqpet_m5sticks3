#include "game_data.h"

// 经验成长表 (前 100 级标准成长值)
static const int LEVEL_TABLE[] = {
    0, 100, 300, 600, 1100, 1800, 2800, 4200, 5900, 8000,
    10600, 13700, 17400, 21700, 26700, 32500, 39000, 46300, 54500, 63600,
    73700, 84800, 97000, 110400, 124900, 140600, 157600, 175900, 195600, 216700,
    239300, 263500, 289200, 316500, 345500, 376200, 408700, 443000, 479200, 517400,
    557500, 599600, 643800, 690100, 738600, 789300, 842300, 897700, 955400, 1015500,
    1078100, 1143200, 1210900, 1281200, 1354200, 1430000, 1508500, 1589800, 1674000, 1761100,
    1851200, 1944300, 2040500, 2139900, 2242400, 2348100, 2457100, 2569400, 2685100, 2804200,
    2926800, 3053000, 3182700, 3316000, 3453000, 3593700, 3738200, 3886500, 4038700, 4194900,
    4355000, 4519100, 4687300, 4859600, 5036100, 5216800, 5401800, 5591200, 5784900, 5983000,
    6185600, 6392700, 6604400, 6820700, 7041700, 7267500, 7498000, 7733300, 7973500, 8218600
};
static const size_t LEVEL_TABLE_SIZE = sizeof(LEVEL_TABLE) / sizeof(LEVEL_TABLE[0]);

// 疾病链定义 (静态嵌套)
// 1. 感冒链
static const IllnessNode ILL_PNEUMONIA = {"肺炎", 1, "30001", "金色消炎药水", nullptr};
static const IllnessNode ILL_SEVERE_COLD = {"重感冒", 2, "20001", "银翘丸", &ILL_PNEUMONIA};
static const IllnessNode ILL_FEVER = {"发烧", 3, "30004", "退烧药", &ILL_SEVERE_COLD};
static const IllnessNode ILL_COLD = {"感冒", 4, "10001", "板蓝根", &ILL_FEVER};

// 2. 咳嗽链
static const IllnessNode ILL_TB = {"肺结核", 1, "40003", "通风散", nullptr};
static const IllnessNode ILL_ASTHMA = {"哮喘", 2, "30003", "定喘丸", &ILL_TB};
static const IllnessNode ILL_BRONCHITIS = {"支气管炎", 3, "20003", "甘草剂", &ILL_ASTHMA};
static const IllnessNode ILL_COUGH = {"咳嗽", 4, "10003", "枇杷糖浆", &ILL_BRONCHITIS};

// 3. 胃病链
static const IllnessNode ILL_STOMACH_CANCER = {"胃癌", 1, "40002", "仙人汤", nullptr};
static const IllnessNode ILL_GASTRIC_ULCER = {"胃溃疡", 2, "30002", "龙胆草", &ILL_STOMACH_CANCER};
static const IllnessNode ILL_GASTRITIS = {"胃炎", 3, "20002", "蓝色消炎药水", &ILL_GASTRIC_ULCER};
static const IllnessNode ILL_STOMACH_BLOAT = {"肚子胀", 4, "10002", "消食片", &ILL_GASTRITIS};

// 死亡节点
static const IllnessNode ILL_DEATH = {"死亡", 0, "60001", "还魂丹", nullptr};

static const IllnessNode* ROOT_CHAINS[] = {&ILL_COLD, &ILL_COUGH, &ILL_STOMACH_BLOAT};

// 药品映射表 (带售价)
const MedicineInfo MEDICINE_LIST[] = {
    {"10001", "板蓝根", "感冒", 50},
    {"10002", "消食片", "肚子胀", 50},
    {"10003", "枇杷糖浆", "咳嗽", 50},
    {"30004", "退烧药", "发烧", 80},
    {"20001", "银翘丸", "重感冒", 80},
    {"20002", "蓝色消炎药水", "胃炎", 80},
    {"20003", "甘草剂", "支气管炎", 80},
    {"30001", "金色消炎药水", "肺炎", 120},
    {"30002", "龙胆草", "胃溃疡", 120},
    {"30003", "定喘丸", "哮喘", 120},
    {"40002", "仙人汤", "胃癌", 200},
    {"40003", "通风散", "肺结核", 200},
    {"60001", "还魂丹", "死亡", 300}
};
const size_t MEDICINE_COUNT = sizeof(MEDICINE_LIST) / sizeof(MEDICINE_LIST[0]);

// 食物列表 (4 款经典原版食物)
const FoodItemInfo FOOD_LIST[] = {
    {"food_fish", "小鱼干", "基础小鱼干，饱食+400", 400, 0, 10},
    {"food_salmon", "鲜嫩三文鱼", "高级鲜鱼，饱食+1000", 1000, 0, 25},
    {"food_icecream", "企鹅雪糕", "夏日甜品，饱食+1600/心情+100", 1600, 100, 40},
    {"food_feast", "海鲜大餐", "满汉全席，饱食+3000/心情+300", 3000, 300, 80}
};
const size_t FOOD_COUNT = sizeof(FOOD_LIST) / sizeof(FOOD_LIST[0]);

// 商城商品全量列表 (衣橱、食物、清洁与药品)
const ShopProductInfo SHOP_PRODUCTS[] = {
    {"wardrobe", "企鹅衣橱", "试穿/穿戴/购买饰品", 0, "wardrobe"},

    {"food_fish", "小鱼干", "饱食+400", 10, "food"},

    {"food_salmon", "鲜嫩三文鱼", "饱食+1000", 25, "food"},
    {"food_icecream", "企鹅雪糕", "饱食+1600/心+100", 40, "food"},
    {"food_feast", "海鲜大餐", "饱食+3000/心+300", 80, "food"},
    {"soap", "泡泡香皂", "洗香香，清洁+1000", 20, "clean"},
    {"10001", "板蓝根", "主治: 感冒", 50, "med"},
    {"10002", "消食片", "主治: 肚子胀", 50, "med"},
    {"10003", "枇杷糖浆", "主治: 咳嗽", 50, "med"},
    {"30004", "退烧药", "主治: 发烧", 80, "med"},
    {"20001", "银翘丸", "主治: 重感冒", 80, "med"},
    {"20002", "蓝色消炎水", "主治: 胃炎", 80, "med"},
    {"20003", "甘草剂", "主治: 支气管炎", 80, "med"},
    {"30001", "金色消炎水", "主治: 肺炎", 120, "med"},
    {"30002", "龙胆草", "主治: 胃溃疡", 120, "med"},
    {"30003", "定喘丸", "主治: 哮喘", 120, "med"},
    {"40002", "仙人汤", "主治: 胃癌", 200, "med"},
    {"40003", "通风散", "主治: 肺结核", 200, "med"},
    {"60001", "还魂丹", "主治: 起死回生", 300, "med"}
};
const size_t SHOP_PRODUCT_COUNT = sizeof(SHOP_PRODUCTS) / sizeof(SHOP_PRODUCTS[0]);


// 经典台词库
const char* QUOTES_IDLE[] = {
    "主人，今天过得开心吗？",
    "咕噜噜~ 企鹅在想心事...",
    "摇摇晃晃，我是最棒的桌宠！",
    "好想吃小鱼干呀~",
    "主人工作辛苦啦，休息一下吧~",
    "今天的天气真好呢！",
    "拍拍翅膀，活力满满！"
};
const size_t QUOTES_IDLE_COUNT = sizeof(QUOTES_IDLE) / sizeof(QUOTES_IDLE[0]);

const char* QUOTES_HUNGRY[] = {
    "好饿好饿，肚皮贴后背啦！",
    "主人快喂我，不然要饿扁了~",
    "咕噜咕噜...我要吃东西！"
};
const size_t QUOTES_HUNGRY_COUNT = sizeof(QUOTES_HUNGRY) / sizeof(QUOTES_HUNGRY[0]);

const char* QUOTES_DIRTY[] = {
    "身上好痒呀，想洗香香~",
    "呜呜，小企鹅变脏企鹅了...",
    "主人帮我擦擦身上的灰吧！"
};
const size_t QUOTES_DIRTY_COUNT = sizeof(QUOTES_DIRTY) / sizeof(QUOTES_DIRTY[0]);

const char* QUOTES_SICK[] = {
    "头好晕呀，我好像生病了...",
    "难受想睡觉...需要喝药药吗？",
    "阿嚏！主人救救我..."
};
const size_t QUOTES_SICK_COUNT = sizeof(QUOTES_SICK) / sizeof(QUOTES_SICK[0]);

const char* QUOTES_HAPPY[] = {
    "哇！太舒服啦~ 嘻嘻！",
    "最喜欢主人摸摸啦！",
    "摇一摇，跳支企鹅舞~",
    "心情大好，耶！"
};
const size_t QUOTES_HAPPY_COUNT = sizeof(QUOTES_HAPPY) / sizeof(QUOTES_HAPPY[0]);

const char* QUOTES_LEVELUP[] = {
    "叮！我升级变强壮啦！",
    "哇塞！成长值满满，升级了！",
    "主人快看，我又长大了一岁！"
};
const size_t QUOTES_LEVELUP_COUNT = sizeof(QUOTES_LEVELUP) / sizeof(QUOTES_LEVELUP[0]);

int calculateLevel(float growth, int& currentLevelMinGrowth, int& nextLevelGrowth) {
    if (growth < 50.0f) {
        currentLevelMinGrowth = 0;
        nextLevelGrowth = 50;
        return 0; // 原版正统 0 级起步 (破壳雏鸟阶段)
    }
    for (size_t i = 1; i < LEVEL_TABLE_SIZE; ++i) {
        if (growth < LEVEL_TABLE[i]) {
            currentLevelMinGrowth = (i == 1) ? 50 : LEVEL_TABLE[i - 1];
            nextLevelGrowth = LEVEL_TABLE[i];
            return static_cast<int>(i);
        }
    }
    currentLevelMinGrowth = LEVEL_TABLE[LEVEL_TABLE_SIZE - 1];
    nextLevelGrowth = LEVEL_TABLE[LEVEL_TABLE_SIZE - 1];
    return static_cast<int>(LEVEL_TABLE_SIZE);
}


int calculateMaxHungerClean(int level) {
    int cap = level > 30 ? 30 : level;
    return 3000 + 100 * cap;
}

const char* getIllnessNameByMedicine(const char* medicineId) {
    if (!medicineId) return "";
    for (size_t i = 0; i < MEDICINE_COUNT; ++i) {
        if (strcmp(MEDICINE_LIST[i].id, medicineId) == 0) {
            return MEDICINE_LIST[i].target_illness;
        }
    }
    return "";
}

const char* getMedicineIdByIllness(const char* illnessName) {
    if (!illnessName) return "";
    for (size_t i = 0; i < MEDICINE_COUNT; ++i) {
        if (strcmp(MEDICINE_LIST[i].target_illness, illnessName) == 0) {
            return MEDICINE_LIST[i].id;
        }
    }
    return "";
}

const IllnessNode* getIllnessNode(const char* illnessName) {
    if (!illnessName || strlen(illnessName) == 0) return nullptr;
    if (strcmp(illnessName, "死亡") == 0) return &ILL_DEATH;
    
    for (int i = 0; i < 3; ++i) {
        const IllnessNode* node = ROOT_CHAINS[i];
        while (node) {
            if (strcmp(node->name, illnessName) == 0) {
                return node;
            }
            node = node->child;
        }
    }
    return nullptr;
}

const char* getRandomClassicQuote(const char* context) {
    if (strcmp(context, "hungry") == 0) {
        return QUOTES_HUNGRY[random(0, QUOTES_HUNGRY_COUNT)];
    } else if (strcmp(context, "dirty") == 0) {
        return QUOTES_DIRTY[random(0, QUOTES_DIRTY_COUNT)];
    } else if (strcmp(context, "sick") == 0) {
        return QUOTES_SICK[random(0, QUOTES_SICK_COUNT)];
    } else if (strcmp(context, "happy") == 0) {
        return QUOTES_HAPPY[random(0, QUOTES_HAPPY_COUNT)];
    } else if (strcmp(context, "levelup") == 0) {
        return QUOTES_LEVELUP[random(0, QUOTES_LEVELUP_COUNT)];
    }
    return QUOTES_IDLE[random(0, QUOTES_IDLE_COUNT)];
}

int getItemPrice(const char* itemId) {
    if (!itemId) return 0;
    if (strcmp(itemId, "food") == 0) return 20;
    if (strcmp(itemId, "soap") == 0) return 20;
    if (strcmp(itemId, "60001") == 0) return 300; // 还魂丹
    if (strcmp(itemId, "10001") == 0 || strcmp(itemId, "10002") == 0 || strcmp(itemId, "10003") == 0) return 50; // 初级药
    if (strcmp(itemId, "20001") == 0 || strcmp(itemId, "20002") == 0 || strcmp(itemId, "20003") == 0 || strcmp(itemId, "30004") == 0) return 80; // 中级药
    if (strcmp(itemId, "30001") == 0 || strcmp(itemId, "30002") == 0 || strcmp(itemId, "30003") == 0) return 120; // 高级药
    if (strcmp(itemId, "40002") == 0 || strcmp(itemId, "40003") == 0) return 200; // 特级药
    return 50;
}

const char* getItemName(const char* itemId) {
    if (!itemId) return "未知物品";
    if (strcmp(itemId, "food") == 0) return "美味小鱼";
    if (strcmp(itemId, "soap") == 0) return "泡泡香皂";
    for (size_t i = 0; i < MEDICINE_COUNT; ++i) {
        if (strcmp(MEDICINE_LIST[i].id, itemId) == 0) {
            return MEDICINE_LIST[i].name;
        }
    }
    return "神秘道具";
}


