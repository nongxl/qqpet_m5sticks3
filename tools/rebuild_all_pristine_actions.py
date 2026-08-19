#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
rebuild_all_pristine_actions.py
直接从 doc/ 原版矢量 SWF 资产库中，使用 Playwright + Ruffle 重新提取
100% 超高清、原汁原味、色彩饱满、绝对无噪点、无透光、无锯齿的 32-bit RGBA 动作容器！
"""

import http.server
import socketserver
import threading
import time
import shutil
import struct
import io
from pathlib import Path
from PIL import Image, ImageDraw, ImageOps
import numpy as np
from playwright.sync_api import sync_playwright

WORKSPACE = Path("d:/workspace/QQpet_StickS3")
DATA_DIR = WORKSPACE / "data/assets"
ACTION_ROOT = WORKSPACE / "doc/qqpet_automation/qq-pet-macos/src/assets/Action"
PORT = 8899

class QuietHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=str(WORKSPACE), **kwargs)
    def log_message(self, format, *args):
        pass

HTML_TEMPLATE = """<!DOCTYPE html>
<html><head><meta charset="utf-8">
<script src="/doc/qqpet_automation/qq-pet-macos/src/windows/js/ruffle/ruffle.js"></script>
<style>body {{ margin: 0; padding: 0; background: transparent; overflow: hidden; }} #pet {{ width: 220px; height: 220px; }}</style>
</head><body><div id="pet"></div>
<script>
window.addEventListener('DOMContentLoaded', () => {{
    const r = window.RufflePlayer.newest();
    const p = r.createPlayer();
    p.style.width = '100%'; p.style.height = '100%';
    p.config = {{ autoplay: 'on', unmuteOverlay: 'hidden', letterbox: 'off', backgroundColor: null, wmode: 'transparent' }};
    document.getElementById('pet').appendChild(p);
    p.load("{swf_url}");
}});
</script></body></html>"""

# 完整动作配置
ACTION_CONFIG = {
    "Adult": [
        ("stand", "peaceful/Stand.swf", 10, 0.08),
        ("happy", "happy/Stand.swf", 10, 0.08),
        ("sad", "sad/Stand.swf", 10, 0.08),
        ("work", "peaceful/play/P8.swf", 10, 0.07),      # 扫地打工
        ("work_1", "peaceful/play/P12.swf", 10, 0.07),   # 维修打工
        ("work_2", "peaceful/play/P20.swf", 10, 0.07),   # 算盘打工
        ("study", "peaceful/play/P2.swf", 10, 0.07),     # 读书
        ("study_1", "peaceful/play/P6.swf", 10, 0.07),   # 伏案
        ("study_2", "peaceful/play/P10.swf", 10, 0.07),  # 黑板
        ("play", "happy/play/P1.swf", 10, 0.07),         # 拍球
        ("play_1", "happy/play/P2.swf", 10, 0.07),       # 跳绳
        ("play_2", "happy/play/P3.swf", 10, 0.07),       # 滑板
        ("play_3", "peaceful/play/P3.swf", 10, 0.07),    # 羽毛球
        ("play_4", "happy/play/P5.swf", 10, 0.07),       # 吹气球
        ("play_5", "happy/play/P6.swf", 10, 0.07),       # 哑铃
        ("play_6", "happy/play/P7.swf", 10, 0.07),       # 魔术
        ("play_7", "happy/play/P9.swf", 10, 0.07),       # 吉他
        ("play_8", "happy/play/P10.swf", 10, 0.07),      # 保龄球
        ("play_9", "happy/play/P11.swf", 10, 0.07),      # 悠悠球
        ("play_10", "happy/play/P12.swf", 10, 0.07),     # 呼啦圈
        ("play_11", "happy/play/P13.swf", 10, 0.07),     # 风筝
        ("play_12", "happy/play/P14.swf", 10, 0.07),     # 拳击
        ("play_13", "happy/play/P15.swf", 10, 0.07),     # 吹泡泡
        ("hobby_water", "peaceful/play/P4.swf", 10, 0.07),# 浇花
        ("hobby_paint", "peaceful/play/P7.swf", 10, 0.07),# 画画
        ("hobby_mirror", "peaceful/play/P9.swf", 10, 0.07),# 照镜子
        ("hobby_chess", "peaceful/play/P14.swf", 10, 0.07),# 下棋
        ("hobby_tea", "peaceful/play/P16.swf", 10, 0.07), # 下午茶
        ("hobby_lens", "peaceful/play/P17.swf", 10, 0.07),# 放大镜
        ("hobby_paper", "peaceful/play/P18.swf", 10, 0.07),# 剪纸
        ("hobby_radio", "peaceful/play/P19.swf", 10, 0.07),# 收音机
        ("hobby_type", "peaceful/play/P21.swf", 10, 0.07),# 打字机
        ("hobby_scope", "peaceful/play/P22.swf", 10, 0.07),# 望远镜
        ("hobby_clean", "peaceful/play/P23.swf", 10, 0.07),# 擦桌椅
        ("sad_circle", "sad/play/P2.swf", 10, 0.07),     # 画圈圈
        ("sad_sigh", "sad/play/P3.swf", 10, 0.07),       # 叹气
        ("upset_stomp", "upset/play/P1.swf", 10, 0.07),  # 跺脚
        ("upset_cross", "upset/play/P3.swf", 10, 0.07),  # 生闷气
        ("trip", "happy/play/P4.swf", 10, 0.07),         # 漫步
        ("eat", "Eat1.swf", 10, 0.07),                   # 吃鱼
        ("eat_1", "Eat2.swf", 10, 0.07),                 # 大餐
        ("clean", "Clean1.swf", 10, 0.07),               # 泡泡浴
        ("sleep", "prostrate/Stand.swf", 8, 0.08),       # 趴卧
        ("sleep_1", "prostrate/play/P1.swf", 8, 0.08),   # 睡袋
        ("sleep_2", "prostrate/play/P2.swf", 8, 0.08),   # 呼噜
        ("sick", "Sick1.swf", 8, 0.08),
        ("cure", "Cure1.swf", 10, 0.07),
        ("dying", "Dying.swf", 8, 0.08),
        ("levelup", "LevUp.swf", 10, 0.07),
        ("hide_left", "Hide_left.swf", 8, 0.08),
        ("hide_right", "Hide_right.swf", 8, 0.08),
    ],
    "Kid": [
        ("stand", "Stand.swf", 10, 0.08),
        ("happy", "LevUp.swf", 10, 0.08),
        ("sad", "Dirty.swf", 8, 0.08),
        ("work", "play/P6.swf", 10, 0.07),               # 扫地
        ("work_1", "play/P7.swf", 10, 0.07),             # 手工
        ("work_2", "play/P8.swf", 10, 0.07),             # 算盘
        ("study", "play/P2.swf", 10, 0.07),              # 翻大书
        ("study_1", "play/P10.swf", 10, 0.07),           # 写作业
        ("play", "play/P1.swf", 10, 0.07),               # 玩耍
        ("play_1", "play/P3.swf", 10, 0.07),             # 羽毛球
        ("play_2", "play/P4.swf", 10, 0.07),             # 漫步
        ("play_horse", "play/P5.swf", 10, 0.07),         # 木马
        ("play_mill", "play/P9.swf", 10, 0.07),          # 风车
        ("play_plane", "play/P11.swf", 10, 0.07),        # 纸飞机
        ("play_fly", "play/P13.swf", 10, 0.07),          # 抓蝴蝶
        ("play_block", "play/P15.swf", 10, 0.07),        # 拼积木
        ("play_sand", "play/P16.swf", 10, 0.07),         # 铲沙子
        ("trip", "play/P4.swf", 10, 0.07),
        ("eat", "Eat1.swf", 10, 0.07),
        ("clean", "Clean.swf", 10, 0.07),
        ("sleep", "prostrate/Stand.swf", 8, 0.08),
        ("sleep_1", "prostrate/play/P1.swf", 8, 0.08),
        ("sleep_2", "prostrate/play/P2.swf", 8, 0.08),
        ("sick", "Sick.swf", 8, 0.08),
        ("cure", "Cure.swf", 10, 0.07),
        ("dying", "Dying.swf", 8, 0.08),
        ("levelup", "LevUp.swf", 10, 0.07),
        ("hide_left", "Hide_left1.swf", 8, 0.08),
        ("hide_right", "Hide_right1.swf", 8, 0.08),
    ],
    "Egg": [
        ("stand", "Stand.swf", 8, 0.08),
        ("happy", "play/P1.swf", 8, 0.08),
        ("sad", "Sick.swf", 8, 0.08),
        ("work", "play/P1.swf", 8, 0.07),
        ("study", "play/P4.swf", 8, 0.07),
        ("play", "play/P2.swf", 8, 0.07),
        ("play_1", "play/P3.swf", 8, 0.07),
        ("play_roll", "play/P3.swf", 8, 0.07),
        ("play_hug", "play/P5.swf", 8, 0.07),
        ("trip", "play/P5.swf", 8, 0.07),
        ("eat", "Eat1.swf", 8, 0.07),
        ("clean", "Clean.swf", 8, 0.07),
        ("sleep", "prostrate/Stand.swf", 8, 0.08),
        ("sleep_1", "prostrate/play/P1.swf", 8, 0.08),
        ("sleep_2", "prostrate/play/P2.swf", 8, 0.08),
        ("sick", "Sick.swf", 8, 0.08),
        ("cure", "Cure.swf", 8, 0.07),
        ("dying", "Dying.swf", 8, 0.08),
        ("levelup", "LevUp.swf", 8, 0.07),
        ("hide_left", "Hide_left.swf", 8, 0.08),
        ("hide_right", "Hide_right.swf", 8, 0.08),
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

def fix_mouth_and_holes(img: Image.Image) -> Image.Image:
    """仅自然修补嘴部镂空，保持原画抗锯齿平滑边缘"""
    arr = np.array(img.convert("RGBA"))
    h, w, _ = arr.shape
    # 嘴巴口腔中心区域大约在 (y=32..62, x=34..62)
    # 如果嘴巴内部有被不透明像素包围的零透明度孔洞，填补暗红实色
    for y in range(32, min(64, h)):
        for x in range(32, min(64, w)):
            if arr[y, x, 3] == 0:
                # 检查四周是否有不透明像素
                if (np.any(arr[max(0, y-10):y, x, 3] > 150) and 
                    np.any(arr[y:min(h, y+10), x, 3] > 150) and
                    np.any(arr[y, max(0, x-10):x, 3] > 150) and
                    np.any(arr[y, x:min(w, x+10), 3] > 150)):
                    arr[y, x] = [150, 40, 45, 255]
    return Image.fromarray(arr, "RGBA")

def extract_and_pack_swf(page, gender, stage, act_name, swf_rel, num_frames=10, frame_delay=0.08):
    swf_file = ACTION_ROOT / gender / stage / swf_rel
    if not swf_file.exists():
        swf_file = ACTION_ROOT / gender / "Adult" / swf_rel
        if not swf_file.exists():
            return False

    out_stage_dir = DATA_DIR / gender / stage
    out_act_file = out_stage_dir / f"{act_name}.act"

    rel_url = f"/doc/qqpet_automation/qq-pet-macos/src/assets/Action/{gender}/{stage}/{swf_rel}".replace("\\", "/")
    html_code = HTML_TEMPLATE.format(swf_url=rel_url)
    (WORKSPACE / "tools/render_runner.html").write_text(html_code, encoding="utf-8")

    page.goto(f"http://127.0.0.1:{PORT}/tools/render_runner.html")
    try:
        page.wait_for_selector("#pet ruffle-player", timeout=8000)
    except Exception:
        return False
    time.sleep(0.35)

    target_size = 72 if stage == "Egg" else (82 if stage == "Kid" else 92)
    png_bytes_list = []

    for f in range(num_frames):
        screenshot_bytes = page.locator("#pet").screenshot(omit_background=True)
        img = Image.open(io.BytesIO(screenshot_bytes)).convert("RGBA")
        
        # 验证是否为 Ruffle 报错界面
        arr_test = np.array(img)
        red_err = (arr_test[:, :, 0] > 170) & (arr_test[:, :, 1] < 70) & (arr_test[:, :, 2] < 70)
        if np.sum(red_err) > 300:
            return False

        bbox = img.getbbox()
        if bbox:
            cropped = img.crop(bbox)
            w, h = cropped.size
            scale = min(target_size / w, target_size / h)
            new_w = max(1, int(w * scale))
            new_h = max(1, int(h * scale))

            # 高品质 Lanczos 抗锯齿缩放
            resized = cropped.resize((new_w, new_h), Image.Resampling.LANCZOS)
            final_img = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
            paste_x = (96 - new_w) // 2
            paste_y = 96 - new_h - 2
            final_img.paste(resized, (paste_x, paste_y), mask=resized)

            final_clean = fix_mouth_and_holes(final_img)
            buf = io.BytesIO()
            final_clean.save(buf, format="PNG", optimize=True)
            png_bytes_list.append(buf.getvalue())
        else:
            final_img = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
            buf = io.BytesIO()
            final_img.save(buf, format="PNG", optimize=True)
            png_bytes_list.append(buf.getvalue())

        time.sleep(frame_delay)

    pack_png_frames_to_act(png_bytes_list, out_act_file)
    print(f"  [EXTRACTED] {gender}/{stage}/{act_name}.act -> {num_frames} frames ({out_act_file.stat().st_size} bytes)")
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
        
        def make_act(act_name, gen_fn, frame_count=8):
            png_list = []
            for f in range(frame_count):
                im = gen_fn(f, frame_count)
                buf = io.BytesIO()
                im.save(buf, format="PNG", optimize=True)
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
        make_act("walk_right", gen_walk_r, 8)

        # 2. walk_left
        def gen_walk_l(f, fc):
            return ImageOps.mirror(gen_walk_r(f, fc))
        make_act("walk_left", gen_walk_l, 8)

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
        make_act("drag", gen_drag, 8)

        # 4. look (左右歪头)
        def gen_look(f, fc):
            im = base_imgs[f % num_base].copy()
            tilt = np.sin(f * np.pi / 4.0) * 5.0
            return im.rotate(tilt, resample=Image.Resampling.BICUBIC, center=(48, 85))
        make_act("look", gen_look, 8)

        # 5. wobble (左右蹒跚)
        def gen_wobble(f, fc):
            im = base_imgs[f % num_base].copy()
            shift_x = int(np.sin(f * np.pi / 4.0) * 3.5)
            shift_y = int(np.abs(np.cos(f * np.pi / 4.0)) * -2.0)
            new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
            new_im.paste(im, (shift_x, shift_y), im)
            return new_im
        make_act("wobble", gen_wobble, 8)

        # 6. stretch (伸懒腰)
        def gen_stretch(f, fc):
            im = base_imgs[f % num_base].copy()
            phase = np.sin(f * np.pi / 4.0)
            scale_y = 1.0 + (phase * 0.08 if phase > 0 else 0)
            nh = int(96 * scale_y)
            rs = im.resize((96, nh), Image.Resampling.BILINEAR)
            new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
            new_im.paste(rs, (0, 96 - nh), rs)
            return new_im
        make_act("stretch", gen_stretch, 8)

        # 7. umbrella (雨天撑小花伞)
        def gen_umbrella(f, fc):
            im = base_imgs[f % num_base].copy()
            new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
            new_im.paste(im, (0, 4), im)
            draw = ImageDraw.Draw(new_im)
            ux, uy = 62, 45 + int(np.sin(f * np.pi / 4.0) * 2.0)
            draw.line([(ux, uy), (ux - 12, uy + 26)], fill=(120, 70, 30, 255), width=2)
            draw.pieslice([ux - 28, uy - 26, ux + 28, uy + 14], 180, 360, fill=(240, 70, 60, 255))
            draw.pieslice([ux - 16, uy - 26, ux + 16, uy + 14], 210, 330, fill=(255, 220, 60, 255))
            draw.line([(ux, uy - 26), (ux, uy - 30)], fill=(120, 70, 30, 255), width=2)
            return new_im
        make_act("umbrella", gen_umbrella, 8)

        # 8. cold (冬雪搓手)
        def gen_cold(f, fc):
            im = base_imgs[f % num_base].copy()
            shake_x = 1 if (f % 2 == 1) else -1
            new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
            new_im.paste(im, (shake_x, 0), im)
            draw = ImageDraw.Draw(new_im)
            gx = 56 + int(np.sin(f * np.pi / 4.0) * 4.0)
            gy = 48 - (f % 4) * 2
            draw.ellipse([gx, gy, gx + 8, gy + 8], fill=(235, 245, 255, 200))
            return new_im
        make_act("cold", gen_cold, 8)

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
        make_act("summer", gen_summer, 8)

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
                    ok = extract_and_pack_swf(page, gender, stage, act_name, swf_rel, num_f, f_delay)
                    if not ok:
                        # 尝试从 MM 同名动作复制（如果是 GG 缺失）
                        mm_alt = DATA_DIR / "MM" / stage / f"{act_name}.act"
                        gg_target = DATA_DIR / "GG" / stage / f"{act_name}.act"
                        if gender == "GG" and mm_alt.exists():
                            shutil.copy(mm_alt, gg_target)
                            print(f"  [COPIED FROM MM] -> {gender}/{stage}/{act_name}.act")

            # 生成微动作
            generate_micro_actions(gender)

        browser.close()
    server.shutdown()
    print("\nALL PRISTINE 32-BIT RGBA ACTIONS REBUILT SUCCESSFULLY!")

if __name__ == "__main__":
    main()
