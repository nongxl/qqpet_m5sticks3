#pragma once
#include "pet_models.h"
#include "game_data.h"

enum PetAnimState {
    ANIM_IDLE_STAND = 0, // 经典站立呼吸
    ANIM_IDLE_LOOK,      // 左右张望
    ANIM_IDLE_SCRATCH,   // 挠头抓痒
    ANIM_IDLE_STRETCH,   // 伸展双臂伸懒腰
    ANIM_IDLE_BOUNCE,    // 小碎步原地跳跳
    ANIM_IDLE_PAT_BELLY, // 摸圆滚滚肚皮
    ANIM_IDLE_DOZE,      // 打瞌睡冒 Zzz
    ANIM_HAPPY,          // 互动开心
    ANIM_PLAY,           // 专属逗玩抛球玩耍
    ANIM_WORK,           // 专属打工赚钱搬砖动画
    ANIM_STUDY,          // 专属认真研读学习动画
    ANIM_TRIP,           // 专属背包旅行动效
    ANIM_SAD,            // 心情低落
    ANIM_SICK,           // 生病难受顶冰袋
    ANIM_EAT,            // 捧小鱼大嚼
    ANIM_CLEAN,          // 搓澡全身泡泡
    ANIM_CURE,           // 喝药康复
    ANIM_LEVELUP,        // 升级跳跃星星
    ANIM_DYING,          // 虚弱濒死
    ANIM_DEAD,           // 死亡
    ANIM_DRAG            // 悬空扑腾挣扎
};


class PetCore {
public:
    PetCore();
    void initDefault();
    
    // 基础操作
    bool feed(int amount = 1000);
    bool bath(int amount = 1000);
    bool play(int amount = 150);
    bool cure(const char* medicineId);
    bool autoHeal(String& outMsg);
    bool revive();
    bool work(String& outMsg);
    bool study(String& outMsg);
    bool trip(String& outMsg);
    bool buyItem(const char* itemId, int count, String& outMsg);

    // 周期衰减与恶化处理
    void tickDecay(uint32_t deltaSeconds);
    
    // 状态查询 (按等级上限的 35% 比例动态判断饥饿与脏污，杜绝低等级误触发)
    int getLevel() const;
    int getMaxHunger() const;
    int getMaxClean() const;
    int getMaxMood() const { return 1000; }
    bool isHungry() const { return state.hunger < static_cast<int>(getMaxHunger() * 0.35f); }
    bool isDirty() const { return state.clean < static_cast<int>(getMaxClean() * 0.35f); }
    bool isSick() const { return state.health < HEALTH_NORMAL && state.health > 0; }
    bool isDead() const { return state.health == 0; }


    // 动画与自主行为管理
    PetAnimState getCurrentAnimState() const;
    void triggerTransientAnim(PetAnimState anim, uint32_t durationMs = 3500);
    void updateAnimState();

    PetState& getState() { return state; }
    const PetState& getState() const { return state; }

    void addGrowth(float val);

private:
    PetState state;
    PetAnimState transientAnim;
    uint32_t transientAnimEndTime;
    
    PetAnimState currentIdleSubAction;
    uint32_t lastIdleSwitchTime;

    void checkLevelUp(int oldLevel);
    void randomIllnessTrigger();
    void switchRandomIdleAction();
};

extern PetCore g_pet;
