#pragma once
#include <Arduino.h>

// 药品库存单元
struct MedicineInventoryItem {
    char id[8];
    char name[16];
    int count;
};

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

    int food_count;         // 食物库存 (次)
    int soap_count;         // 清洁用品库存 (次)
    int revival_count;      // 还魂丹库存
    MedicineInventoryItem medicines[13]; // 常用药品库存

    // 系统配置
    char wifi_ssid[64];
    char wifi_pwd[64];
    char deepseek_key[128];
    uint8_t volume;
    uint8_t brightness;
    
    uint32_t last_active_time; // 上次运行时间戳 (秒)
};

