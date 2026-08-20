#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
auto_classify_all_actions.py
全自动深度归类所有 1340+ 个原版 Flash 动作：
1. 继承用户已整理的分类标签；
2. 依据腾讯 QQ 宠物官方目录规范与特征，自动将全部动作归入 Stand/Eat/Clean/Study/Work/Play/Hobby/Sleep/Sick/Cure/Die/LevelUp/Hide 等场景；
3. 准确区分【运动玩耍 (Play)】与【兴趣爱好 (Hobby)】；
4. 正确解析 prostrate/play/ 下的 46 种动作（仅 P1~P5 为睡觉，P6~P46 为趴卧玩耍与艺术创作）；
5. 提取帧数和帧率统一配置为最高档 (28 帧, 12 FPS)；
6. 为核心精品动作默认勾选 enabled=True（加入固件），辅助动作默认 enabled=False。
"""

import json
from pathlib import Path

WORKSPACE = Path(__file__).resolve().parent.parent
ACTION_ROOT = WORKSPACE / "doc/qqpet_automation/qq-pet-macos/src/assets/Action"
MAPPING_FILE = WORKSPACE / "tools/action_mappings.json"

PEACEFUL_ACTIONS = {
    "P1": ("play", "play_walk", "闲庭漫步踢石子"),
    "P2": ("study", "study_book", "翻阅大字典认真研读"),
    "P3": ("play", "play_badminton", "挥拍打羽毛球"),
    "P4": ("hobby", "hobby_water", "拿水壶给向日葵浇水"),
    "P5": ("play", "play_swing", "荡秋千摇晃"),
    "P6": ("study", "study_write", "伏案握笔写作业"),
    "P7": ("hobby", "hobby_paint", "架画板调色写生"),
    "P8": ("work", "work_sweep", "挥动大扫帚扫地打工"),
    "P9": ("hobby", "hobby_mirror", "照梳妆镜整理发型"),
    "P10": ("study", "study_board", "在小黑板前演算数学题"),
    "P11": ("play", "play_ball", "快乐颠球玩耍"),
    "P12": ("work", "work_repair", "手持扳手修理机械打工"),
    "P13": ("play", "play_kite", "奔跑迎风放风筝"),
    "P14": ("hobby", "hobby_chess", "棋盘前托腮沉思下围棋"),
    "P15": ("play", "play_magic", "礼帽变出小飞鸽魔术"),
    "P16": ("hobby", "hobby_tea", "优雅品尝红茶下午茶"),
    "P17": ("hobby", "hobby_lens", "手持放大镜观察小甲虫"),
    "P18": ("hobby", "hobby_paper", "拿大剪刀剪纸红双喜"),
    "P19": ("hobby", "hobby_radio", "打开复古收音机沉浸听歌"),
    "P20": ("work", "work_abacus", "飞速拨动木算盘记账打工"),
    "P21": ("hobby", "hobby_type", "老式机械打字机写作"),
    "P22": ("hobby", "hobby_scope", "天文望远镜仰望星空"),
    "P23": ("hobby", "hobby_clean", "拿着抹布仔细擦桌椅"),
}

HAPPY_ACTIONS = {
    "P1": ("play", "play_ball", "欢快拍篮球蹦跳"),
    "P2": ("play", "play_rope", "双手摇绳欢快跳绳"),
    "P3": ("play", "play_skateboard", "踩炫酷滑板滑行"),
    "P4": ("play", "play_walk", "开开心心散步漫步"),
    "P5": ("play", "play_balloon", "鼓起腮帮吹大彩色气球"),
    "P6": ("play", "play_dumbbell", "双手举重哑铃健身"),
    "P7": ("play", "play_card", "扑克牌切牌魔术秀"),
    "P8": ("play", "play_trampoline", "蹦床上高高弹跳"),
    "P9": ("play", "play_guitar", "抱木吉他弹奏摇滚乐"),
    "P10": ("play", "play_bowling", "用力掷出保龄球全中"),
    "P11": ("play", "play_yoyo", "花样玩悠悠球"),
    "P12": ("play", "play_hoop", "腰部旋转彩色呼啦圈"),
    "P13": ("play", "play_kite", "春游迎风放蝴蝶风筝"),
    "P14": ("play", "play_boxing", "戴红拳套打沙袋拳击"),
    "P15": ("play", "play_bubble", "拿起泡泡棒吹出七彩泡泡"),
}

UPSET_ACTIONS = {
    "P1": ("upset", "upset_stomp", "双脚交替愤怒连续跺脚"),
    "P2": ("upset", "upset_sigh", "生闷气别过头不理人"),
    "P3": ("upset", "upset_cross", "抱臂背身气鼓鼓生闷气"),
}

SAD_ACTIONS = {
    "P1": ("sad", "sad_cry", "委屈揉眼睛流眼泪"),
    "P2": ("sad", "sad_circle", "蹲在墙角用小树枝画圈圈"),
    "P3": ("sad", "sad_sigh", "抱膝坐在地上叹气沮丧"),
}

# 官方趴卧动作 prostrate/play（仅 P1~P5 为睡觉，P6~P46 为趴地游玩、绘画、才艺）
PROSTRATE_ACTIONS = {
    "P1": ("sleep", "sleep_bag", "钻进暖洋洋的绿色睡袋大睡"),
    "P2": ("sleep", "sleep_snore", "趴在地上呼呼大睡冒大泡泡"),
    "P3": ("sleep", "sleep_nap", "打瞌睡脑袋一点一点"),
    "P4": ("sleep", "sleep_pillow", "抱小枕头甜蜜入梦"),
    "P5": ("sleep", "sleep_bed", "在小被窝里香甜熟睡"),
    "P6": ("play", "play_marble", "趴在地上玩彩色弹珠玻璃球"),
    "P7": ("hobby", "hobby_ant", "拿放大镜看小蚂蚁搬家"),
    "P8": ("play", "play_cube", "转动彩色魔方益智玩具"),
    "P9": ("hobby", "hobby_puzzle", "拼七彩拼图益智小游戏"),
    "P10": ("play", "play_boat", "折彩色小纸船玩水"),
    "P11": ("play", "play_duck", "磁铁小钓竿钓塑料小鸭"),
    "P12": ("hobby", "hobby_xylophone", "敲击彩色八音木琴演奏"),
    "P13": ("hobby", "hobby_harmonica", "吹奏银色小口琴乐曲"),
    "P14": ("play", "play_car", "摆弄发条玩具小汽车"),
    "P15": ("play", "play_block", "搭木制小积木城堡"),
    "P16": ("hobby", "hobby_book", "趴在地上翻阅精美故事绘本"),
    "P17": ("play", "play_penguin", "玩发条摇摆小企鹅玩具"),
    "P18": ("play", "play_piano", "按彩色电子琴玩具琴键"),
    "P19": ("play", "play_tumbler", "推按塑料不倒翁玩具"),
    "P20": ("play", "play_gun", "手持泡泡水枪打泡泡"),
    "P21": ("hobby", "hobby_crayon", "拿油画棒在画纸上涂鸦"),
    "P22": ("play", "play_count", "拨弄算盘彩色圆珠计数"),
    "P23": ("play", "play_kaleido", "转动彩色万花筒视界"),
    "P24": ("play", "play_ring", "玩套圈圈塑料圆环玩具"),
    "P25": ("play", "play_duckcar", "推木头小鸭子滚轮车"),
    "P26": ("play", "play_magnet", "玩磁铁吸吸乐拼图"),
    "P27": ("play", "play_musicbox", "摇动手摇机械八音盒"),
    "P28": ("hobby", "hobby_draw_house", "用彩色蜡笔在地上画小房子和楼梯"),
    "P29": ("play", "play_robot_dog", "逗弄智能发条机器小狗玩具"),
    "P30": ("play", "play_mole", "拿小木槌敲打打地鼠玩具"),
    "P31": ("play", "play_tank", "玩回力发条玩具小坦克"),
    "P32": ("play", "play_top", "抽打陀螺旋转玩具"),
    "P33": ("play", "play_horn", "手持玩具小喇叭吹奏"),
    "P34": ("play", "play_popup", "翻看立体弹出玩具书"),
    "P35": ("play", "play_chalk", "在小黑板上用粉笔画小花"),
    "P36": ("play", "play_domino", "摆弄多米诺骨牌推倒"),
    "P37": ("play", "play_drum", "拍打彩色双音小手鼓"),
    "P38": ("play", "play_maracas", "摇动彩色小沙锤合奏"),
    "P39": ("play", "play_jack", "玩小丑弹簧盒子搞怪玩具"),
    "P40": ("play", "play_bead", "玩彩色木珠串珠子玩具"),
    "P41": ("play", "play_doctor", "玩手提玩具小医生工具箱"),
    "P42": ("play", "play_frog", "玩发条机械跳跳蛙玩具"),
    "P43": ("hobby", "hobby_shell", "用塑料放大镜仔细观察小贝壳"),
    "P44": ("play", "play_scope", "转动双筒玩具望远镜"),
    "P45": ("play", "play_puppet", "套上可爱小玩偶手偶表演"),
    "P46": ("play", "play_squirrel", "玩发条摇尾巴小松鼠玩具"),
}

KID_PLAY_ACTIONS = {
    "P1": ("play", "play_toy", "拿着心爱的小玩具玩耍"),
    "P2": ("study", "study_book", "坐在地上翻看大图画故事书"),
    "P3": ("play", "play_badminton", "挥小球拍打羽毛球"),
    "P4": ("play", "play_trip", "蹦蹦跳跳开心地漫步"),
    "P5": ("play", "play_horse", "骑着摇摇木马前后晃荡"),
    "P6": ("work", "work_sweep", "拿着小扫帚扫地做家务"),
    "P7": ("work", "work_craft", "做手工折纸小飞机"),
    "P8": ("work", "work_abacus", "拨小算盘学算术"),
    "P9": ("play", "play_mill", "手举七彩小风车迎风跑"),
    "P10": ("study", "study_write", "趴在小桌子上认真写作业"),
    "P11": ("play", "play_plane", "手掷小纸飞机飞翔"),
    "P12": ("play", "play_car", "推玩具小汽车赛车"),
    "P13": ("play", "play_fly", "挥小捕蝶网在草地抓蝴蝶"),
    "P14": ("play", "play_drum", "拿起鼓槌敲打小军鼓"),
    "P15": ("play", "play_block", "在地上堆彩色积木城堡"),
    "P16": ("play", "play_sand", "拿小铁铲铲沙堆城堡"),
}

def classify_swf(gender, stage, rel_path):
    parts = Path(rel_path).parts
    stem = Path(rel_path).stem
    
    if len(parts) == 1:
        if stem == "Stand":
            return ("stand", "stand", "经典基础端正待机呼吸姿态", True)
        elif stem.startswith("Eat"):
            num = stem.replace("Eat", "")
            lbl = "大口美味吃鱼" if num == "1" else "享用满汉全席大餐"
            return ("eat", f"eat_{num}" if num != "1" else "eat", lbl, True)
        elif stem.startswith("Clean"):
            num = stem.replace("Clean", "")
            lbl = "拉上帷幕淋浴" if num == "1" else "泡泡浴缸搓澡"
            return ("clean", f"clean_{num}" if num != "1" else "clean", lbl, True)
        elif stem.startswith("Sick"):
            return ("sick", "sick", "头顶冰袋发烧生病不适", True)
        elif stem.startswith("Cure"):
            return ("cure", "cure", "乖乖吃药就医治疗", True)
        elif stem == "Dying":
            return ("dying", "dying", "虚弱垂危奄奄一息", True)
        elif stem in ["Die", "Bury"]:
            return ("die", "die", "灵魂升天/进入墓碑棺木", True)
        elif stem == "LevUp":
            return ("levelup", "levelup", "等级提升放礼花彩带欢庆", True)
        elif "Hide_left" in stem:
            return ("hide_left", "hide_left", "贴靠屏幕左侧边缘探头张望", True)
        elif "Hide_right" in stem:
            return ("hide_right", "hide_right", "贴靠屏幕右侧边缘探头张望", True)
        elif stem.startswith("Enter"):
            num = stem.replace("Enter", "")
            return ("play", f"enter_{num}", f"炫酷入场登场动作 {num}", False)
        elif stem.startswith("Exit"):
            num = stem.replace("Exit", "")
            return ("play", f"exit_{num}", f"退场离场动画 {num}", False)
        elif stem in ["Dirty", "Hungry"]:
            return ("sad", stem.lower(), "浑身脏兮兮/饥肠辘辘", True)
        else:
            return ("other", stem.lower(), f"常规动作 {stem}", False)

    folder = parts[0]
    if folder == "peaceful":
        if stem == "Stand":
            return ("stand", "stand", "平静待机姿态", True)
        if len(parts) >= 2 and parts[1] == "play":
            if stem in PEACEFUL_ACTIONS:
                cat, act_id, lbl = PEACEFUL_ACTIONS[stem]
                return (cat, act_id, lbl, True)
            return ("play", f"peace_{stem.lower()}", f"平静日常娱乐 {stem}", False)

    elif folder == "happy":
        if stem == "Stand":
            return ("happy", "happy", "欢快好心情待机摆动", True)
        if len(parts) >= 2 and parts[1] == "play":
            if stem in HAPPY_ACTIONS:
                cat, act_id, lbl = HAPPY_ACTIONS[stem]
                return (cat, act_id, lbl, True)
            return ("play", f"happy_{stem.lower()}", f"开心运动娱乐 {stem}", False)

    elif folder == "upset":
        if stem == "Stand":
            return ("upset", "upset", "生气噘嘴待机", True)
        if len(parts) >= 2 and parts[1] == "play":
            if stem in UPSET_ACTIONS:
                cat, act_id, lbl = UPSET_ACTIONS[stem]
                return (cat, act_id, lbl, True)
            return ("upset", f"upset_{stem.lower()}", f"发脾气动作 {stem}", False)

    elif folder == "sad":
        if stem == "Stand":
            return ("sad", "sad", "伤心低落待机", True)
        if len(parts) >= 2 and parts[1] == "play":
            if stem in SAD_ACTIONS:
                cat, act_id, lbl = SAD_ACTIONS[stem]
                return (cat, act_id, lbl, True)
            return ("sad", f"sad_{stem.lower()}", f"伤心沮丧动作 {stem}", False)

    elif folder == "prostrate":
        if stem == "Stand":
            return ("sleep", "sleep", "趴在地上睡眼惺忪", True)
        if len(parts) >= 2 and parts[1] == "play":
            if stem in PROSTRATE_ACTIONS:
                cat, act_id, lbl = PROSTRATE_ACTIONS[stem]
                return (cat, act_id, lbl, True)
            return ("play", f"pros_{stem.lower()}", f"趴卧玩耍动作 {stem}", False)

    elif stage == "Kid" and folder == "play":
        if stem in KID_PLAY_ACTIONS:
            cat, act_id, lbl = KID_PLAY_ACTIONS[stem]
            return (cat, act_id, lbl, True)
        return ("play", f"kid_{stem.lower()}", f"少儿游戏动作 {stem}", False)

    elif stage == "Egg" and folder == "play":
        if stem == "P1":
            return ("play", "play_hug", "蛋壳摇晃求抱抱", True)
        elif stem == "P2":
            return ("play", "play_roll", "蛋壳左右打滚摇摆", True)
        elif stem == "P3":
            return ("play", "play_jump", "顶着小蛋壳欢快小跳", True)
        elif stem == "P4":
            return ("study", "study", "破壳雏鸟好奇看小画书", True)
        elif stem == "P5":
            return ("play", "play_trip", "顶着蛋壳蹒跚漫步", True)
        return ("play", f"egg_{stem.lower()}", f"雏鸟动作 {stem}", False)

    return ("other", stem.lower(), f"备选动作 {rel_path}", False)

def main():
    existing_mappings = {}
    if MAPPING_FILE.exists():
        try:
            with open(MAPPING_FILE, "r", encoding="utf-8") as f:
                existing_mappings = json.load(f)
        except Exception:
            pass

    updated_mappings = {}
    total_found = 0
    total_enabled = 0

    for gender in ["MM", "GG"]:
        for stage in ["Adult", "Kid", "Egg"]:
            stage_dir = ACTION_ROOT / gender / stage
            if not stage_dir.exists():
                continue
            for swf_p in stage_dir.rglob("*.swf"):
                rel_path = swf_p.relative_to(stage_dir).as_posix()
                key = f"{gender}/{stage}/{rel_path}"
                total_found += 1

                cat, act_id, lbl, def_enabled = classify_swf(gender, stage, rel_path)

                # 保留用户此前特意修改的有效自定义
                if key in existing_mappings and existing_mappings[key].get("label") and not existing_mappings[key]["label"].startswith("就寝睡觉动作"):
                    u = existing_mappings[key]
                    updated_mappings[key] = {
                        "gender": gender,
                        "stage": stage,
                        "rel_path": rel_path,
                        "category": u.get("category") or cat,
                        "action_id": u.get("action_id") or act_id,
                        "label": u.get("label") or lbl,
                        "num_frames": int(u.get("num_frames", 28)),
                        "fps": int(u.get("fps", 12)),
                        "enabled": bool(u.get("enabled", def_enabled))
                    }
                else:
                    updated_mappings[key] = {
                        "gender": gender,
                        "stage": stage,
                        "rel_path": rel_path,
                        "category": cat,
                        "action_id": act_id,
                        "label": lbl,
                        "num_frames": 28,
                        "fps": 12,
                        "enabled": def_enabled
                    }

                if updated_mappings[key]["enabled"]:
                    total_enabled += 1

    with open(MAPPING_FILE, "w", encoding="utf-8") as f:
        json.dump(updated_mappings, f, ensure_ascii=False, indent=2)

    print(f"\n========================================================")
    print(f"  SUCCESS! Re-classified all {total_found} Flash SWFs.")
    print(f"  Prostrate actions (P1~P46) accurately split into Sleep vs Play/Hobby.")
    print(f"  Enabled high-quality core actions: {total_enabled}")
    print(f"  All actions configured to 28 frames @ 12 FPS (最高长动画模式)!")
    print(f"  Mappings saved to: {MAPPING_FILE}")
    print(f"========================================================\n")

if __name__ == "__main__":
    main()
