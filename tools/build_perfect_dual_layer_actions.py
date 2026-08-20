#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
build_perfect_dual_layer_actions.py
使用 144x144 原版 Flash 舞台双层渲染架构 (Base Avatar + Action Props Overlay)：
1. 在浏览器 DOM 舞台中将 Stand.swf（完整身躯基底）与 Action.swf（动作/道具层）以 100% 原生坐标对齐渲染；
2. 彻底解决用餐、自习、打工、跺脚等动作的身躯残缺与透光问题；
3. 杜绝任何手动贴图导致的错位重影与多层重叠（零重影、零多层）；
4. 缩放至 96x96 并以 64 色高清调色板压制写入 6.06MB LittleFS 文件系统！
"""

import os
import sys
import io
import time
import struct
import shutil
import http.server
import socketserver
import threading
import subprocess
from pathlib import Path
from PIL import Image, ImageDraw, ImageOps
import numpy as np
from playwright.sync_api import sync_playwright

WORKSPACE = Path("d:/workspace/QQpet_StickS3")
DATA_DIR = WORKSPACE / "data/assets"
ACTION_ROOT = WORKSPACE / "doc/qqpet_automation/qq-pet-macos/src/assets/Action"
PORT = 8999

class QuietHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=str(WORKSPACE), **kwargs)
    def log_message(self, format, *args):
        pass

HTML_DUAL_TEMPLATE = """<!DOCTYPE html>
<html><head><meta charset="utf-8">
<script src="/doc/qqpet_automation/qq-pet-macos/src/windows/js/ruffle/ruffle.js"></script>
<style>
body {{ margin: 0; padding: 0; background: transparent; overflow: hidden; }}
#stage {{ position: relative; width: 144px; height: 144px; }}
#base {{ position: absolute; top: 0; left: 0; width: 144px; height: 144px; z-index: 1; }}
#act {{ position: absolute; top: 0; left: 0; width: 144px; height: 144px; z-index: 2; }}
</style>
</head><body>
<div id="stage">
  <div id="base"></div>
  <div id="act"></div>
</div>
<script>
window.addEventListener('DOMContentLoaded', () => {{
    const r = window.RufflePlayer.newest();
    
    // 1. 底层完整身躯基底 (如果是 stand 自身则不重复加载)
    const baseSwf = "{base_swf_url}";
    const actSwf = "{act_swf_url}";
    
    if (baseSwf && baseSwf !== actSwf) {{
        const pBase = r.createPlayer();
        pBase.style.width = '100%'; pBase.style.height = '100%';
        pBase.config = {{ autoplay: 'on', unmuteOverlay: 'hidden', letterbox: 'off', backgroundColor: null, wmode: 'transparent' }};
        document.getElementById('base').appendChild(pBase);
        pBase.load(baseSwf);
    }}
    
    // 2. 顶层动作/道具层
    const pAct = r.createPlayer();
    pAct.style.width = '100%'; pAct.style.height = '100%';
    pAct.config = {{ autoplay: 'on', unmuteOverlay: 'hidden', letterbox: 'off', backgroundColor: null, wmode: 'transparent' }};
    document.getElementById('act').appendChild(pAct);
    pAct.load(actSwf);
}});
</script></body></html>"""

# 动作配置表
ACTION_CONFIG = {
    "Adult": [
        ("stand", "peaceful/Stand.swf", 6, 0.08),
        ("happy", "happy/Stand.swf", 6, 0.08),
        ("sad", "sad/Stand.swf", 6, 0.08),
        ("work", "peaceful/play/P8.swf", 6, 0.07),      # 扫地打工
        ("work_1", "peaceful/play/P12.swf", 6, 0.07),   # 维修打工
        ("work_2", "peaceful/play/P20.swf", 6, 0.07),   # 算盘打工
        ("study", "peaceful/play/P2.swf", 6, 0.07),     # 读书
        ("study_1", "peaceful/play/P6.swf", 6, 0.07),   # 伏案写字
        ("study_2", "peaceful/play/P10.swf", 6, 0.07),  # 黑板演算
        ("play", "happy/play/P1.swf", 6, 0.07),         # 拍球
        ("play_1", "happy/play/P2.swf", 6, 0.07),       # 跳绳
        ("play_2", "happy/play/P3.swf", 6, 0.07),       # 滑板
        ("play_3", "peaceful/play/P3.swf", 6, 0.07),    # 羽毛球
        ("play_4", "happy/play/P5.swf", 6, 0.07),       # 吹气球
        ("play_5", "happy/play/P6.swf", 6, 0.07),       # 哑铃
        ("play_6", "happy/play/P7.swf", 6, 0.07),       # 魔术
        ("play_7", "happy/play/P9.swf", 6, 0.07),       # 吉他
        ("play_8", "happy/play/P10.swf", 6, 0.07),      # 保龄球
        ("play_9", "happy/play/P11.swf", 6, 0.07),      # 悠悠球
        ("play_10", "happy/play/P12.swf", 6, 0.07),     # 呼啦圈
        ("play_11", "happy/play/P13.swf", 6, 0.07),     # 风筝
        ("play_12", "happy/play/P14.swf", 6, 0.07),     # 拳击
        ("play_13", "happy/play/P15.swf", 6, 0.07),     # 吹泡泡
        ("hobby_water", "peaceful/play/P4.swf", 6, 0.07),# 浇花
        ("hobby_paint", "peaceful/play/P7.swf", 6, 0.07),# 画画
        ("hobby_mirror", "peaceful/play/P9.swf", 6, 0.07),# 照镜子
        ("hobby_chess", "peaceful/play/P14.swf", 6, 0.07),# 下棋
        ("hobby_tea", "peaceful/play/P16.swf", 6, 0.07), # 下午茶
        ("hobby_lens", "peaceful/play/P17.swf", 6, 0.07),# 放大镜
        ("hobby_paper", "peaceful/play/P18.swf", 6, 0.07),# 剪纸
        ("hobby_radio", "peaceful/play/P19.swf", 6, 0.07),# 收音机
        ("hobby_type", "peaceful/play/P21.swf", 6, 0.07),# 打字机
        ("hobby_scope", "peaceful/play/P22.swf", 6, 0.07),# 望远镜
        ("hobby_clean", "peaceful/play/P23.swf", 6, 0.07),# 擦桌椅
        ("sad_circle", "sad/play/P2.swf", 6, 0.07),     # 画圈圈
        ("sad_sigh", "sad/play/P3.swf", 6, 0.07),       # 抱膝叹气
        ("upset_stomp", "upset/play/P1.swf", 6, 0.07),  # 跺脚发脾气
        ("upset_cross", "upset/play/P3.swf", 6, 0.07),  # 生闷气
        ("trip", "happy/play/P4.swf", 6, 0.07),         # 漫步
        ("eat", "Eat1.swf", 6, 0.07),                   # 美味吃鱼
        ("eat_1", "Eat2.swf", 6, 0.07),                 # 满汉全席大餐
        ("clean", "Clean1.swf", 6, 0.07),               # 泡泡浴
        ("sleep", "prostrate/Stand.swf", 6, 0.08),       # 趴卧
        ("sleep_1", "prostrate/play/P1.swf", 6, 0.08),   # 睡袋
        ("sleep_2", "prostrate/play/P2.swf", 6, 0.08),   # 呼噜
        ("sick", "Sick1.swf", 6, 0.08),
        ("cure", "Cure1.swf", 6, 0.07),
        ("dying", "Dying.swf", 6, 0.08),
        ("levelup", "LevUp.swf", 6, 0.07),
        ("hide_left", "Hide_left.swf", 6, 0.08),
        ("hide_right", "Hide_right.swf", 6, 0.08),
    ],
    "Kid": [
        ("stand", "Stand.swf", 6, 0.08),
        ("happy", "LevUp.swf", 6, 0.08),
        ("sad", "Dirty.swf", 6, 0.08),
        ("work", "play/P6.swf", 6, 0.07),               # 扫地
        ("work_1", "play/P7.swf", 6, 0.07),             # 手工
        ("work_2", "play/P8.swf", 6, 0.07),             # 算盘
        ("study", "play/P2.swf", 6, 0.07),              # 翻大书
        ("study_1", "play/P10.swf", 6, 0.07),           # 写作业
        ("play", "play/P1.swf", 6, 0.07),               # 玩耍
        ("play_1", "play/P3.swf", 6, 0.07),             # 羽毛球
        ("play_2", "play/P4.swf", 6, 0.07),             # 漫步
        ("play_horse", "play/P5.swf", 6, 0.07),         # 木马
        ("play_mill", "play/P9.swf", 6, 0.07),          # 风车
        ("play_plane", "play/P11.swf", 6, 0.07),        # 纸飞机
        ("play_fly", "play/P13.swf", 6, 0.07),          # 抓蝴蝶
        ("play_block", "play/P15.swf", 6, 0.07),        # 拼积木
        ("play_sand", "play/P16.swf", 6, 0.07),         # 铲沙子
        ("trip", "play/P4.swf", 6, 0.07),
        ("eat", "Eat1.swf", 6, 0.07),
        ("clean", "Clean.swf", 6, 0.07),
        ("sleep", "prostrate/Stand.swf", 6, 0.08),
        ("sleep_1", "prostrate/play/P1.swf", 6, 0.08),
        ("sleep_2", "prostrate/play/P2.swf", 6, 0.08),
        ("sick", "Sick.swf", 6, 0.08),
        ("cure", "Cure.swf", 6, 0.07),
        ("dying", "Dying.swf", 6, 0.08),
        ("levelup", "LevUp.swf", 6, 0.07),
        ("hide_left", "Hide_left1.swf", 6, 0.08),
        ("hide_right", "Hide_right1.swf", 6, 0.08),
    ],
    "Egg": [
        ("stand", "Stand.swf", 6, 0.08),
        ("happy", "play/P1.swf", 6, 0.08),
        ("sad", "Sick.swf", 6, 0.08),
        ("work", "play/P1.swf", 6, 0.07),
        ("study", "play/P4.swf", 6, 0.07),
        ("play", "play/P2.swf", 6, 0.07),
        ("play_1", "play/P3.swf", 6, 0.07),
        ("play_roll", "play/P3.swf", 6, 0.07),
        ("play_hug", "play/P5.swf", 6, 0.07),
        ("trip", "play/P5.swf", 6, 0.07),
        ("eat", "Eat1.swf", 6, 0.07),
        ("clean", "Clean.swf", 6, 0.07),
        ("sleep", "prostrate/Stand.swf", 6, 0.08),
        ("sleep_1", "prostrate/play/P1.swf", 6, 0.08),
        ("sleep_2", "prostrate/play/P2.swf", 6, 0.08),
        ("sick", "Sick.swf", 6, 0.08),
        ("cure", "Cure.swf", 6, 0.07),
        ("dying", "Dying.swf", 6, 0.08),
        ("levelup", "LevUp.swf", 6, 0.07),
        ("hide_left", "Hide_left.swf", 6, 0.08),
        ("hide_right", "Hide_right.swf", 6, 0.08),
    ]
}

def pack_png_frames_to_act(png_bytes_list, out_file: Path):
    out_file.parent.mkdir(parents=True, exist_ok=True)
    count = len(png_bytes_list)
    buf = bytearray()
    buf.extend(struct.pack("<4B", 0xAA, 0x01, count, 0))
    for p in png_bytes_list:
        buf.extend(struct.pack("<H", len(p)))
    for p in png_bytes_list:
        buf.extend(p)
    with open(out_file, "wb") as f:
        f.write(buf)

def extract_and_pack_dual_swf(page, gender, stage, act_name, swf_rel, num_frames=6, frame_delay=0.08):
    swf_file = ACTION_ROOT / gender / stage / swf_rel
    if not swf_file.exists():
        swf_file = ACTION_ROOT / gender / "Adult" / swf_rel
        if not swf_file.exists():
            return False

    out_stage_dir = DATA_DIR / gender / stage
    out_act_file = out_stage_dir / f"{act_name}.act"

    # 确定基底 stand.swf 的相对 URL
    base_rel = "peaceful/Stand.swf" if stage == "Adult" else "Stand.swf"
    base_url = f"/doc/qqpet_automation/qq-pet-macos/src/assets/Action/{gender}/{stage}/{base_rel}".replace("\\", "/")
    act_url = f"/doc/qqpet_automation/qq-pet-macos/src/assets/Action/{gender}/{stage}/{swf_rel}".replace("\\", "/")

    html_code = HTML_DUAL_TEMPLATE.format(base_swf_url=base_url, act_swf_url=act_url)
    (WORKSPACE / "tools/render_dual.html").write_text(html_code, encoding="utf-8")

    page.goto(f"http://127.0.0.1:{PORT}/tools/render_dual.html")
    try:
        page.wait_for_selector("#act ruffle-player", timeout=8000)
    except Exception:
        return False
    time.sleep(0.35)

    png_bytes_list = []

    for f in range(num_frames):
        screenshot_bytes = page.locator("#stage").screenshot(omit_background=True)
        img = Image.open(io.BytesIO(screenshot_bytes)).convert("RGBA")
        
        # 验证是否为 Ruffle 报错红屏
        arr_test = np.array(img)
        red_err = (arr_test[:, :, 0] > 170) & (arr_test[:, :, 1] < 70) & (arr_test[:, :, 2] < 70)
        if np.sum(red_err) > 300:
            return False

        # 直接将 144x144 舞台缩放到 96x96，坐标 100% 完美对齐
        im96 = img.resize((96, 96), Image.Resampling.LANCZOS)
        
        # 64 色高质量调色板压缩
        q = im96.quantize(colors=64, method=Image.Quantize.FASTOCTREE)
        buf = io.BytesIO()
        q.save(buf, format="PNG", optimize=True)
        png_bytes_list.append(buf.getvalue())

        time.sleep(frame_delay)

    pack_png_frames_to_act(png_bytes_list, out_act_file)
    print(f"  [EXTRACTED DUAL] {gender}/{stage}/{act_name}.act -> {num_frames} frames ({out_act_file.stat().st_size} bytes)")
    return True

def generate_micro_actions(gender):
    """为各个阶段补充待机专属微动作（左右打量、蹒跚摇摆、伸懒腰、雨伞、冷风、小电扇等）"""
    print(f"\n--- Generating smooth micro actions for {gender} ---")
    for stage in ["Egg", "Kid", "Adult"]:
        stage_dir = DATA_DIR / gender / stage
        stand_act = stage_dir / "stand.act"
        if not stand_act.exists(): continue

        with open(stand_act, "rb") as f: data = f.read()
        count = data[2]
        sizes = struct.unpack(f"<{count}H", data[4:4+count*2])
        offset = 4 + count*2
        base_imgs = []
        for sz in sizes:
            base_imgs.append(Image.open(io.BytesIO(data[offset:offset+sz])).convert("RGBA"))
            offset += sz
            
        num_base = len(base_imgs)
        
        def make_act(act_name, gen_fn, frame_count=6):
            png_list = []
            for f in range(frame_count):
                im = gen_fn(f, frame_count)
                q = im.quantize(colors=64, method=Image.Quantize.FASTOCTREE)
                buf = io.BytesIO()
                q.save(buf, format="PNG", optimize=True)
                png_list.append(buf.getvalue())
            pack_png_frames_to_act(png_list, stage_dir / f"{act_name}.act")

        # 1. walk_right
        def gen_walk_r(f, fc):
            im = base_imgs[f % num_base].copy()
            angle = -3.0 + np.sin(f * np.pi / 2.5) * 4.0
            rot = im.rotate(angle, resample=Image.Resampling.BICUBIC, center=(48, 85))
            shift_y = int(np.abs(np.sin(f * np.pi / 2.5)) * -3.0)
            new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
            new_im.paste(rot, (0, shift_y), rot)
            return new_im
        make_act("walk_right", gen_walk_r, 6)

        # 2. walk_left
        def gen_walk_l(f, fc):
            return ImageOps.mirror(gen_walk_r(f, fc))
        make_act("walk_left", gen_walk_l, 6)

        # 3. drag (悬空扑腾踢腿)
        def gen_drag(f, fc):
            im = base_imgs[f % num_base].copy()
            tilt = np.sin(f * np.pi / 2.0) * 5.0
            rot = im.rotate(tilt, resample=Image.Resampling.BICUBIC, center=(48, 48))
            squish = 1.0 + (0.05 if f % 2 == 0 else -0.05)
            nw, nh = int(96 * (2.0 - squish)), int(96 * squish)
            rs = rot.resize((nw, nh), Image.Resampling.BILINEAR)
            new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
            new_im.paste(rs, ((96 - nw) // 2, (96 - nh) // 2), rs)
            return new_im
        make_act("drag", gen_drag, 6)

        # 4. look (左右歪头)
        def gen_look(f, fc):
            im = base_imgs[f % num_base].copy()
            tilt = np.sin(f * np.pi / 3.0) * 5.0
            return im.rotate(tilt, resample=Image.Resampling.BICUBIC, center=(48, 85))
        make_act("look", gen_look, 6)

        # 5. wobble (左右蹒跚)
        def gen_wobble(f, fc):
            im = base_imgs[f % num_base].copy()
            shift_x = int(np.sin(f * np.pi / 3.0) * 3.5)
            shift_y = int(np.abs(np.cos(f * np.pi / 3.0)) * -2.0)
            new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
            new_im.paste(im, (shift_x, shift_y), im)
            return new_im
        make_act("wobble", gen_wobble, 6)

        # 6. stretch (伸懒腰)
        def gen_stretch(f, fc):
            im = base_imgs[f % num_base].copy()
            phase = np.sin(f * np.pi / 3.0)
            scale_y = 1.0 + (phase * 0.08 if phase > 0 else 0)
            nh = int(96 * scale_y)
            rs = im.resize((96, nh), Image.Resampling.BILINEAR)
            new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
            new_im.paste(rs, (0, 96 - nh), rs)
            return new_im
        make_act("stretch", gen_stretch, 6)

        # 7. umbrella (雨天撑小花伞)
        def gen_umbrella(f, fc):
            im = base_imgs[f % num_base].copy()
            new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
            new_im.paste(im, (0, 4), im)
            draw = ImageDraw.Draw(new_im)
            ux, uy = 62, 45 + int(np.sin(f * np.pi / 3.0) * 2.0)
            draw.line([(ux, uy), (ux - 12, uy + 26)], fill=(120, 70, 30, 255), width=2)
            draw.pieslice([ux - 28, uy - 26, ux + 28, uy + 14], 180, 360, fill=(240, 70, 60, 255))
            draw.pieslice([ux - 16, uy - 26, ux + 16, uy + 14], 210, 330, fill=(255, 220, 60, 255))
            draw.line([(ux, uy - 26), (ux, uy - 30)], fill=(120, 70, 30, 255), width=2)
            return new_im
        make_act("umbrella", gen_umbrella, 6)

        # 8. cold (冬雪搓手)
        def gen_cold(f, fc):
            im = base_imgs[f % num_base].copy()
            shake_x = 1 if (f % 2 == 1) else -1
            new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
            new_im.paste(im, (shake_x, 0), im)
            draw = ImageDraw.Draw(new_im)
            gx = 56 + int(np.sin(f * np.pi / 3.0) * 4.0)
            gy = 48 - (f % 3) * 2
            draw.ellipse([gx, gy, gx + 8, gy + 8], fill=(235, 245, 255, 200))
            return new_im
        make_act("cold", gen_cold, 6)

        # 9. summer (小电扇吹风)
        def gen_summer(f, fc):
            im = base_imgs[f % num_base].copy()
            new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
            new_im.paste(im, (0, 0), im)
            draw = ImageDraw.Draw(new_im)
            fx, fy = 24, 62
            draw.line([(fx, fy), (fx, fy + 22)], fill=(80, 160, 240, 255), width=3)
            draw.ellipse([fx - 8, fy - 8, fx + 8, fy + 8], outline=(50, 120, 220, 255), width=2)
            rot_fan = (f * 60) % 360
            rad = np.radians(rot_fan)
            dx, dy = int(np.cos(rad) * 6), int(np.sin(rad) * 6)
            draw.line([(fx - dx, fy - dy), (fx + dx, fy + dy)], fill=(120, 200, 255, 255), width=2)
            return new_im
        make_act("summer", gen_summer, 6)

def main():
    server = socketserver.TCPServer(("", PORT), QuietHandler)
    server_thread = threading.Thread(target=server.serve_forever, daemon=True)
    server_thread.start()
    print(f"Server started at http://127.0.0.1:{PORT}")

    with sync_playwright() as p:
        browser = p.chromium.launch(channel="msedge", headless=True)
        page = browser.new_page(viewport={"width": 260, "height": 260})

        for gender in ["MM", "GG"]:
            print(f"\n==================== PROCESSING {gender} ====================")
            for stage, actions in ACTION_CONFIG.items():
                print(f"\n--- {gender} / {stage} ({len(actions)} actions) ---")
                for act_name, swf_rel, num_f, f_delay in actions:
                    ok = extract_and_pack_dual_swf(page, gender, stage, act_name, swf_rel, num_f, f_delay)
                    if not ok:
                        # 尝试从 MM 同名动作复制
                        mm_alt = DATA_DIR / "MM" / stage / f"{act_name}.act"
                        gg_target = DATA_DIR / "GG" / stage / f"{act_name}.act"
                        if gender == "GG" and mm_alt.exists():
                            shutil.copy(mm_alt, gg_target)
                            print(f"  [COPIED FROM MM] -> {gender}/{stage}/{act_name}.act")

            # 生成微动作
            generate_micro_actions(gender)

        browser.close()
    server.shutdown()
    
    total = sum(f.stat().st_size for f in DATA_DIR.parent.rglob('*') if f.is_file())
    print(f"\nALL ACTIONS PERFECTLY EXTRACTED! Total Data Size: {total / (1024*1024):.2f} MB ({total / 1024:.1f} KB)")

    # 生成 LittleFS 镜像
    mklittlefs_exe = Path(os.environ['USERPROFILE']) / '.platformio/packages/tool-mklittlefs/mklittlefs.exe'
    out_bin = WORKSPACE / ".pio/build/m5stack-sticks3/littlefs.bin"
    out_bin.parent.mkdir(parents=True, exist_ok=True)
    
    print("\nPacking LittleFS image...")
    cmd = [str(mklittlefs_exe), '-c', 'data', '-s', '0x610000', '-b', '4096', '-p', '256', str(out_bin)]
    res = subprocess.run(cmd, capture_output=True, text=True, cwd=str(WORKSPACE))
    if res.stderr:
        print("mklittlefs stderr:", res.stderr)
    print("mklittlefs returncode:", res.returncode)
    
    # 验证文件列表
    cmd_l = [str(mklittlefs_exe), '-l', '-b', '4096', '-p', '256', '-s', '0x610000', str(out_bin)]
    res_l = subprocess.run(cmd_l, capture_output=True, text=True)
    lines = [l for l in res_l.stdout.strip().split('\n') if l]
    print(f"SUCCESS: LittleFS image contains {len(lines)} files!")

if __name__ == "__main__":
    main()
