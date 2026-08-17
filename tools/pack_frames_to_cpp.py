import os
from pathlib import Path

FRAMES_DIR = Path("d:/workspace/QQpet_StickS3/src/pet_frames")
OUT_HEADER = Path("d:/workspace/QQpet_StickS3/src/pet_anim_assets.h")
OUT_CPP = Path("d:/workspace/QQpet_StickS3/src/pet_anim_assets.cpp")

ACTION_DIRS = ["stand", "stand1", "happy", "play", "eat", "clean", "sick", "dying", "cure", "levelup"]

def generate_assets():
    header_content = """#pragma once
#include <Arduino.h>

struct RawFrame {
    const uint8_t* data;
    size_t length;
};

struct OfficialAnimClip {
    const RawFrame* frames;
    size_t frameCount;
    uint8_t fps;
};

extern const OfficialAnimClip CLIP_STAND;
extern const OfficialAnimClip CLIP_STAND1;
extern const OfficialAnimClip CLIP_HAPPY;
extern const OfficialAnimClip CLIP_PLAY;
extern const OfficialAnimClip CLIP_EAT;
extern const OfficialAnimClip CLIP_CLEAN;
extern const OfficialAnimClip CLIP_SICK;
extern const OfficialAnimClip CLIP_DYING;
extern const OfficialAnimClip CLIP_CURE;
extern const OfficialAnimClip CLIP_LEVELUP;

const OfficialAnimClip* getClipByAnimState(int animState);
"""

    cpp_content = ['#include "pet_anim_assets.h"', '#include "pet_core.h"\n']

    clip_names = []

    for act in ACTION_DIRS:
        act_path = FRAMES_DIR / act
        if not act_path.exists():
            continue
        
        frame_files = sorted(list(act_path.glob("*.png")))
        if not frame_files:
            continue
        
        clip_name = f"CLIP_{act.upper()}"
        clip_names.append((act, clip_name, len(frame_files)))
        
        frame_var_names = []
        for i, f_path in enumerate(frame_files):
            var_name = f"FRAME_{act.upper()}_{i:02d}_PNG"
            frame_var_names.append(var_name)
            
            raw_bytes = f_path.read_bytes()
            hex_data = ", ".join(f"0x{b:02x}" for b in raw_bytes)
            
            cpp_content.append(f"static const uint8_t {var_name}[] PROGMEM = {{{hex_data}}};")

        # 构造 RawFrame 数组
        raw_frames_arr = f"RAW_FRAMES_{act.upper()}"
        frames_init = ", ".join(f"{{{v}, sizeof({v})}}" for v in frame_var_names)
        cpp_content.append(f"static const RawFrame {raw_frames_arr}[] = {{{frames_init}}};\n")
        
        # 构造 OfficialAnimClip
        fps = 10 if act in ["eat", "clean", "happy", "play", "cure", "levelup"] else (6 if act == "dying" else 8)
        cpp_content.append(f"const OfficialAnimClip {clip_name} = {{{raw_frames_arr}, {len(frame_files)}, {fps}}};\n")

    # 生成 getClipByAnimState 分发函数
    dispatch_func = """
const OfficialAnimClip* getClipByAnimState(int animState) {
    switch (animState) {
        case ANIM_IDLE_STAND:
            return &CLIP_STAND;
        case ANIM_IDLE_LOOK:
        case ANIM_IDLE_SCRATCH:
        case ANIM_IDLE_STRETCH:
        case ANIM_IDLE_BOUNCE:
        case ANIM_IDLE_DOZE:
        case ANIM_IDLE_PAT_BELLY:
            return &CLIP_STAND1;
        case ANIM_HAPPY:
            return &CLIP_HAPPY;
        case ANIM_PLAY:
            return &CLIP_PLAY;
        case ANIM_EAT:
            return &CLIP_EAT;
        case ANIM_CLEAN:
            return &CLIP_CLEAN;
        case ANIM_SICK:
        case ANIM_SAD:
            return &CLIP_SICK;
        case ANIM_DEAD:
            return &CLIP_DYING;
        case ANIM_CURE:
            return &CLIP_CURE;
        case ANIM_LEVELUP:
        case ANIM_DRAG:
            return &CLIP_LEVELUP;
        default:
            return &CLIP_STAND;
    }
}
"""
    cpp_content.append(dispatch_func)

    OUT_HEADER.write_text(header_content, encoding="utf-8")
    OUT_CPP.write_text("\n".join(cpp_content), encoding="utf-8")
    print("pet_anim_assets.h & .cpp generated successfully!")

if __name__ == "__main__":
    generate_assets()
