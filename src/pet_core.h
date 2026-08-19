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
    ANIM_SLEEP,          // 💤 专属戴睡帽闭眼深睡打呼噜冒泡动画
    ANIM_DYING,          // 虚弱濒死
    ANIM_DEAD,           // 死亡
    ANIM_DRAG,           // 悬空扑腾挣扎
    ANIM_WALK_LEFT,      // 左右漫步巡逻
    ANIM_WALK_RIGHT,
    ANIM_HIDE_LEFT,      // 边缘探头躲猫猫 (左探头)
    ANIM_HIDE_RIGHT,     // 边缘探头躲猫猫 (右探头)
    ANIM_SNEEZE,         // 打喷嚏后仰
    ANIM_YAWN,           // 揉眼打哈欠
    ANIM_ANGRY,          // 生气跺脚
    ANIM_SHY,            // 害羞脸红
    ANIM_UMBRELLA,       // 雨天撑花伞
    ANIM_COLD,           // 冬雪搓手哈白气
    ANIM_SUMMER,         // 炎夏电风扇吹凉
    ANIM_TIWENJI,        // 嘴叼体温计测温
    ANIM_INJECTION,      // 鸭子医生大针筒打针
    ANIM_HOBBY_WATER,    // 🌸 园艺浇花
    ANIM_HOBBY_PAINT,    // 🎨 画板涂鸦画画
    ANIM_HOBBY_MIRROR,   // 🪞 照小镜子梳理打扮
    ANIM_HOBBY_CHESS,    // ♟️ 专注下棋思考
    ANIM_HOBBY_TEA,      // ☕ 优雅喝下午茶
    ANIM_HOBBY_LENS,     // 🔍 手持放大镜探险
    ANIM_HOBBY_PAPER,    // ✂️ 剪纸手作
    ANIM_HOBBY_RADIO,    // 📻 听小收音机音乐
    ANIM_HOBBY_TYPE,     // ⌨️ 打字机打字
    ANIM_HOBBY_SCOPE,    // 🔭 天文望远镜看星空
    ANIM_HOBBY_CLEAN,    // 🧽 认真擦拭桌椅
    ANIM_SAD_CIRCLE,     // 🌀 蹲在角落画圈圈
    ANIM_SAD_SIGH,       // 🧎 抱膝叹气发呆
    ANIM_UPSET_STOMP,    // 😤 跺脚抓狂发脾气
    ANIM_UPSET_CROSS     // 🙅 背过身双手抱胸生闷气
};


class PetCore {
public:
    PetCore();
    void initDefault();
    
    // 领养与重生系统
    bool isAdopted() const { return state.is_adopted; }
    void adopt(uint8_t gender);
    void setGender(uint8_t gender);
    void resetAdoption();

    float getWalkOffsetX() const { return walkOffsetX; }
    float getTemperature() const;
    bool takeHospitalInjection(String& outMsg);



    // 基础操作与细分道具
    bool feed(int amount = 1000);
    bool feedFood(int foodIndex, String& outMsg);
    bool bath(int amount = 1000);
    bool play(int amount = 150);
    bool cure(const char* medicineId);
    bool cureWithMed(int medIndex, String& outMsg);
    bool autoHeal(String& outMsg);
    bool revive();
    bool work(String& outMsg);
    bool study(String& outMsg);
    bool trip(String& outMsg);
    
    // 持续作业挂机系统 (打工/学习/旅游)
    bool isTaskActive() const { return state.current_task != TASK_NONE; }
    PetTaskType getCurrentTask() const { return static_cast<PetTaskType>(state.current_task); }
    bool startTask(PetTaskType type, uint32_t durationSec, String& outMsg);
    bool stopTask(String& outMsg, bool isNaturalFinish = false);
    uint32_t getTaskRemainingSec() const;
    float getTaskProgress() const;

    bool buyItem(const char* itemId, int count, String& outMsg);
    bool buyShopProduct(int productIndex, int count, String& outMsg);
    int getFoodCount(int foodIndex) const;
    int getMedCount(int medIndex) const;

    // 换装衣橱管理
    bool ownsCostume(int costumeId) const;
    bool buyCostume(int costumeId, String& outMsg);
    bool toggleEquipCostume(int costumeId, String& outMsg);
    int getEquippedCostume(int category) const;


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
    void setSleeping(bool sleeping) { sleepingState = sleeping; }
    bool isSleeping() const { return sleepingState; }

    PetState& getState() { return state; }
    const PetState& getState() const { return state; }

    void addGrowth(float val);

private:
    PetState state;
    PetAnimState transientAnim;
    uint32_t transientAnimEndTime;
    bool sleepingState;
    
    PetAnimState currentIdleSubAction;

    uint32_t lastIdleSwitchTime;
    float walkOffsetX;
    float walkTargetX;

    void checkLevelUp(int oldLevel);
    void randomIllnessTrigger();
    void switchRandomIdleAction();
};


extern PetCore g_pet;
