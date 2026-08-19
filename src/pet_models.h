#pragma once
#include <Arduino.h>

// 药品库存单元
struct MedicineInventoryItem {
    char id[8];
    char name[16];
    int count;
};

enum PetTaskType : uint8_t {
    TASK_NONE = 0,
    TASK_WORK = 1,  // 铁镐打工搬砖
    TASK_STUDY = 2, // 研读自习上课
    TASK_TRIP = 3   // 背包神州漫游
};

// 换装饰品定义
struct CostumeItem {
    int id;               // 1 ~ 8
    const char* name;     // 饰品名称
    const char* file;     // 文件名
    int category;         // 0: 头部帽子, 1: 颈部配饰, 2: 随身挂饰
    int price;            // 元宝商城售价
    int charm_gain;       // 魅力加成
    const char* desc;     // 描述
};

static const CostumeItem COSTUME_LIST[] = {
    {1, "学霸博士帽", "hat_grad.png", 0, 120, 15, "智力与学霸象征"},
    {2, "酷炫墨镜", "glass_cool.png", 0, 150, 20, "帅气遮阳拉风"},
    {3, "鲜艳红领巾", "scarf_red.png", 1, 80, 10, "朝气蓬勃红领巾"},
    {4, "天使金色光环", "hat_halo.png", 0, 200, 30, "神圣纯洁发光光环"},
    {5, "萌粉蝴蝶结", "bow_pink.png", 1, 90, 12, "甜美可爱MM最爱"},
    {6, "尊贵黄金皇冠", "hat_crown.png", 0, 300, 50, "王者荣耀闪闪发光"},
    {7, "探险家小背包", "pack_explorer.png", 2, 100, 15, "环游世界必备行囊"},
    {8, "魔法星月魔杖", "wand_magic.png", 2, 180, 25, "神秘梦幻星月魔力"},
    {9, "圣诞狂欢红帽", "hat_xmas.png", 0, 160, 22, "欢庆圣诞毛绒红帽"},
    {10, "神秘巫师高帽", "hat_wizard.png", 0, 220, 35, "星月暗夜魔法巫师"},
    {11, "七彩飘空气球", "prop_balloon.png", 2, 110, 18, "梦幻童年随身气球"},
    {12, "晴雨折叠遮阳伞", "prop_umbrella.png", 2, 130, 20, "遮阳挡雨户外神器"}
};
static const int COSTUME_COUNT = sizeof(COSTUME_LIST) / sizeof(COSTUME_LIST[0]);



// 宠物完整状态模型
struct PetState {
    char name[32];          // 宠物昵称
    char host[32];          // 主人称呼
    uint8_t gender;         // 0: GG, 1: MM
    bool is_adopted;        // 是否已完成领养仪式 (false: 进入首次并排选性别领养仪式)
    float growth;           // 成长经验值

    int hunger;             // 当前饥饿度
    int clean;              // 当前清洁度
    int mood;               // 当前心情值
    int health;             // 健康值 (5: 正常, 4-1: 生病, 0: 死亡)
    char illness[32];       // 当前疾病名称 ("" 表示健康)
    
    // 属性与资产
    int coins;              // 元宝/金币资产
    int intellect;          // 智力值 (通过学习提升)
    int charm;              // 魅力值 (通过学习与打扮提升)
    uint8_t bg_id;          // 当前壁纸编号 (0: 纯色默认, 1~16: 原版精选壁纸)

    int food_count;         // 基础小鱼干库存 (次)
    int food_salmon;        // 鲜嫩三文鱼库存 (次)
    int food_icecream;      // 企鹅雪糕库存 (次)
    int food_feast;         // 满汉海鲜大餐库存 (次)
    int soap_count;         // 清洁用品香皂库存 (次)
    int revival_count;      // 还魂丹库存
    MedicineInventoryItem medicines[13]; // 常用药品库存

    // 持续作业挂机状态 (打工/学习/旅游)
    uint8_t current_task;       // 0: 空闲, 1: 打工, 2: 学习, 3: 旅游
    uint32_t task_start_time;   // 开始绝对时间戳 (秒)
    uint32_t task_duration;     // 目标设定时长 (秒，例如 300, 900, 1800, 3600)
    uint32_t task_elapsed_sec;  // 已实际进行有效秒数

    // 换装衣橱系统
    uint16_t costume_owned_mask; // 位图，bit 1~8 对应 8 款饰品
    int8_t equipped_head;        // 头部饰品 ID (0 为无)
    int8_t equipped_neck;        // 颈部饰品 ID (0 为无)
    int8_t equipped_hand;        // 随身饰品 ID (0 为无)

    // 系统配置
    char wifi_ssid[64];
    char wifi_pwd[64];
    char deepseek_key[128];
    uint8_t volume;
    uint8_t brightness;

    
    uint32_t last_active_time; // 上次运行时间戳 (秒)
    uint32_t last_signin_day;  // 上次每日签到日期戳 (天数)
};



