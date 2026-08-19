import http.server
import socketserver
import threading
import time
import shutil
import struct
from pathlib import Path
from PIL import Image, ImageDraw, ImageOps, ImageEnhance
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

# 官方原汁原味全量完整长序列帧配置
ACTION_CONFIG = {
    "Adult": [
        ("stand", "peaceful/Stand.swf", 12, 0.08),
        ("happy", "happy/Stand.swf", 14, 0.08),
        ("sad", "sad/Stand.swf", 12, 0.08),
        ("work", "peaceful/play/P8.swf", 18, 0.07),      # 扫地清洁工打工 (原版纯正劳作)
        ("work_1", "peaceful/play/P12.swf", 18, 0.07),   # 敲敲打打维修工打工
        ("work_2", "peaceful/play/P20.swf", 18, 0.07),   # 算盘会计记账打工
        ("study", "peaceful/play/P2.swf", 20, 0.07),     # 翻厚书写字阅读
        ("study_1", "peaceful/play/P6.swf", 18, 0.07),   # 戴眼镜伏案苦读
        ("study_2", "peaceful/play/P10.swf", 18, 0.07),  # 黑板前粉笔演算
        ("play", "happy/play/P1.swf", 16, 0.07),         # 拍皮球
        ("play_1", "happy/play/P2.swf", 16, 0.07),       # 跳绳
        ("play_2", "happy/play/P3.swf", 16, 0.07),       # 踩滑板
        ("play_3", "peaceful/play/P3.swf", 18, 0.07),    # 戴运动发带打羽毛球/网球
        ("play_4", "happy/play/P5.swf", 16, 0.07),       # 吹气球
        ("play_5", "happy/play/P6.swf", 16, 0.07),       # 举哑铃健身
        ("play_6", "happy/play/P7.swf", 16, 0.07),       # 变魔术
        ("play_7", "happy/play/P9.swf", 16, 0.07),       # 弹吉他
        ("play_8", "happy/play/P10.swf", 16, 0.07),      # 打保龄球
        ("play_9", "happy/play/P11.swf", 16, 0.07),      # 悠悠球
        ("play_10", "happy/play/P12.swf", 16, 0.07),     # 呼啦圈
        ("play_11", "happy/play/P13.swf", 16, 0.07),     # 放风筝
        ("play_12", "happy/play/P14.swf", 16, 0.07),     # 拳击手套
        ("play_13", "happy/play/P15.swf", 16, 0.07),     # 吹五彩大泡泡
        ("hobby_water", "peaceful/play/P4.swf", 16, 0.07),# 园艺浇花
        ("hobby_paint", "peaceful/play/P7.swf", 16, 0.07),# 画板画画
        ("hobby_mirror", "peaceful/play/P9.swf", 16, 0.07),# 照小镜子
        ("hobby_chess", "peaceful/play/P14.swf", 16, 0.07),# 专注下棋
        ("hobby_tea", "peaceful/play/P16.swf", 16, 0.07), # 喝下午茶
        ("hobby_lens", "peaceful/play/P17.swf", 16, 0.07),# 放大镜探险
        ("hobby_paper", "peaceful/play/P18.swf", 16, 0.07),# 剪纸手作
        ("hobby_radio", "peaceful/play/P19.swf", 16, 0.07),# 听收音机
        ("hobby_type", "peaceful/play/P21.swf", 16, 0.07),# 打字机
        ("hobby_scope", "peaceful/play/P22.swf", 16, 0.07),# 望远镜看星空
        ("hobby_clean", "peaceful/play/P23.swf", 16, 0.07),# 擦拭桌椅
        ("sad_circle", "sad/play/P2.swf", 16, 0.07),     # 蹲在角落画圈圈
        ("sad_sigh", "sad/play/P3.swf", 16, 0.07),       # 抱膝叹气发呆
        ("upset_stomp", "upset/play/P1.swf", 16, 0.07),  # 跺脚抓狂发脾气
        ("upset_cross", "upset/play/P3.swf", 16, 0.07),  # 背过身双手抱胸生闷气
        ("trip", "happy/play/P4.swf", 16, 0.07),         # 背包漫步
        ("eat", "Eat1.swf", 18, 0.07),                   # 美味吃鱼
        ("eat_1", "Eat2.swf", 18, 0.07),                 # 满汉全席大餐
        ("eat_2", "peaceful/play/P15.swf", 16, 0.07),    # 吃美味甜点蛋糕
        ("clean", "Clean1.swf", 18, 0.07),               # 满天泡泡浴
        ("sleep", "prostrate/Stand.swf", 14, 0.08),      # 官方原版趴卧呼吸
        ("sleep_1", "prostrate/play/P1.swf", 14, 0.08),  # 官方原版条纹睡袋
        ("sleep_2", "prostrate/play/P2.swf", 14, 0.08),  # 官方原版打呼噜
        ("sleep_3", "prostrate/play/P5.swf", 14, 0.08),  # 官方原版吊床
        ("sick", "Sick1.swf", 12, 0.08),
        ("cure", "Cure1.swf", 16, 0.07),
        ("dying", "Dying.swf", 12, 0.08),
        ("levelup", "LevUp.swf", 16, 0.07),
        ("hide_left", "Hide_left.swf", 12, 0.08),
        ("hide_right", "Hide_right.swf", 12, 0.08),
    ],
    "Kid": [
        ("stand", "Stand.swf", 12, 0.08),
        ("happy", "LevUp.swf", 14, 0.08),
        ("sad", "Dirty.swf", 12, 0.08),
        ("work", "play/P6.swf", 18, 0.07),               # 扫地清洁工
        ("work_1", "play/P7.swf", 18, 0.07),             # 敲敲打打手工
        ("work_2", "play/P8.swf", 18, 0.07),             # 算盘小账
        ("study", "play/P2.swf", 20, 0.07),              # 翻大书自习
        ("study_1", "play/P10.swf", 18, 0.07),           # 伏案写作业
        ("play", "play/P1.swf", 16, 0.07),               # 玩耍
        ("play_1", "play/P3.swf", 18, 0.07),             # 羽毛球拍
        ("play_2", "play/P4.swf", 16, 0.07),             # 漫步爬行
        ("play_horse", "play/P5.swf", 16, 0.07),         # 摇摇木马
        ("play_mill", "play/P9.swf", 16, 0.07),          # 彩色小风车
        ("play_plane", "play/P11.swf", 16, 0.07),        # 折纸飞机
        ("play_fly", "play/P13.swf", 16, 0.07),          # 抓蝴蝶
        ("play_block", "play/P15.swf", 16, 0.07),        # 拼积木
        ("play_sand", "play/P16.swf", 16, 0.07),         # 铲沙子
        ("trip", "play/P4.swf", 16, 0.07),
        ("eat", "Eat1.swf", 18, 0.07),
        ("clean", "Clean.swf", 18, 0.07),
        ("sleep", "prostrate/Stand.swf", 14, 0.08),      # 官方原版闭眼趴卧
        ("sleep_1", "prostrate/play/P1.swf", 14, 0.08),  # 官方原版睡袋
        ("sleep_2", "prostrate/play/P2.swf", 14, 0.08),  # 官方原版呼噜
        ("sick", "Sick.swf", 12, 0.08),
        ("cure", "Cure.swf", 16, 0.07),
        ("dying", "Dying.swf", 12, 0.08),
        ("levelup", "LevUp.swf", 16, 0.07),
        ("hide_left", "Hide_left1.swf", 12, 0.08),
        ("hide_right", "Hide_right1.swf", 12, 0.08),
    ],
    "Egg": [
        ("stand", "Stand.swf", 12, 0.08),
        ("happy", "play/P1.swf", 14, 0.08),
        ("sad", "Sick.swf", 12, 0.08),
        ("work", "play/P1.swf", 18, 0.07),               # 破壳期努力摇摆打工
        ("study", "play/P4.swf", 18, 0.07),              # 破壳期读书
        ("play", "play/P2.swf", 16, 0.07),               # 破壳期玩耍
        ("play_1", "play/P3.swf", 18, 0.07),             # 羽毛球拍
        ("play_roll", "play/P3.swf", 16, 0.07),          # 地上打滚
        ("play_hug", "play/P5.swf", 16, 0.07),           # 扑腾求抱抱
        ("trip", "play/P5.swf", 16, 0.07),
        ("eat", "Eat1.swf", 16, 0.07),
        ("clean", "Clean.swf", 16, 0.07),
        ("sleep", "prostrate/Stand.swf", 14, 0.08),      # 官方原版闭眼趴卧
        ("sleep_1", "prostrate/play/P1.swf", 14, 0.08),  # 官方原版睡袋
        ("sleep_2", "prostrate/play/P2.swf", 14, 0.08),  # 官方原版呼噜
        ("sick", "Sick.swf", 12, 0.08),
        ("cure", "Cure.swf", 16, 0.07),
        ("dying", "Dying.swf", 12, 0.08),
        ("levelup", "LevUp.swf", 16, 0.07),
        ("hide_left", "Hide_left1.swf", 12, 0.08),
        ("hide_right", "Hide_right1.swf", 12, 0.08),
    ]
}

def pack_png_frames_to_act(frame_png_bytes_list, out_act_path):
    """将多帧 PNG 字节流紧凑打包为单一 .act 容器文件"""
    count = len(frame_png_bytes_list)
    header = struct.pack("BBBB", 0xAA, 1, count, 0)
    sizes = [len(b) for b in frame_png_bytes_list]
    sizes_bytes = struct.pack(f"<{count}H", *sizes)
    
    with open(out_act_path, "wb") as f:
        f.write(header)
        f.write(sizes_bytes)
        for b in frame_png_bytes_list:
            f.write(b)

def extract_and_pack_swf(page, gender, stage, act_name, swf_rel, num_frames, frame_delay):
    swf_file = ACTION_ROOT / gender / stage / swf_rel
    rel_url = f"/doc/qqpet_automation/qq-pet-macos/src/assets/Action/{gender}/{stage}/{swf_rel}".replace("\\", "/")
    if not swf_file.exists():
        swf_file = ACTION_ROOT / gender / "Adult" / swf_rel
        rel_url = f"/doc/qqpet_automation/qq-pet-macos/src/assets/Action/{gender}/Adult/{swf_rel}".replace("\\", "/")
        if not swf_file.exists():
            print(f"  [MISSING] {swf_file}")
            return False


    out_stage_dir = DATA_DIR / gender / stage
    out_stage_dir.mkdir(parents=True, exist_ok=True)
    out_act_file = out_stage_dir / f"{act_name}.act"

    rel_url = f"/doc/qqpet_automation/qq-pet-macos/src/assets/Action/{gender}/{stage}/{swf_rel}".replace("\\", "/")
    html_code = HTML_TEMPLATE.format(swf_url=rel_url)
    (WORKSPACE / "tools/render_runner.html").write_text(html_code, encoding="utf-8")

    page.goto(f"http://127.0.0.1:{PORT}/tools/render_runner.html")
    try:
        page.wait_for_selector("#pet ruffle-player", timeout=8000)
    except Exception as e:
        print(f"  [TIMEOUT] {act_name}")
        return False
    time.sleep(0.35)

    target_size = 72 if stage == "Egg" else (82 if stage == "Kid" else 92)
    png_bytes_list = []

    for f in range(num_frames):
        screenshot_bytes = page.locator("#pet").screenshot(omit_background=True)
        import io
        img = Image.open(io.BytesIO(screenshot_bytes)).convert("RGBA")
        bbox = img.getbbox()
        if bbox:
            cropped = img.crop(bbox)
            w, h = cropped.size
            scale = min(target_size / w, target_size / h)
            new_w = max(1, int(w * scale))
            new_h = max(1, int(h * scale))

            resized = cropped.resize((new_w, new_h), Image.Resampling.LANCZOS)
            final_img = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
            paste_x = (96 - new_w) // 2
            paste_y = 96 - new_h - 2
            final_img.paste(resized, (paste_x, paste_y))

            quantized = final_img.quantize(colors=32, method=Image.Quantize.FASTOCTREE)
            buf = io.BytesIO()
            quantized.save(buf, format="PNG", optimize=True)
            png_bytes_list.append(buf.getvalue())
        else:
            final_img = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
            buf = io.BytesIO()
            final_img.save(buf, format="PNG", optimize=True)
            png_bytes_list.append(buf.getvalue())

        time.sleep(frame_delay)

    pack_png_frames_to_act(png_bytes_list, out_act_file)
    print(f"  [PACKED .ACT] {gender}/{stage}/{act_name}.act -> {num_frames} frames ({out_act_file.stat().st_size} bytes)")
    return True

def generate_and_pack_micro_actions(gender):
    print(f"\n--- Generating and packing 10~12 frame smooth micro actions for {gender} ---")
    import io
    for stage in ["Egg", "Kid", "Adult"]:
        stage_dir = DATA_DIR / gender / stage
        stand_act = stage_dir / "stand.act"
        if not stand_act.exists():
            continue

        # 解析 stand.act 获取基准待机帧
        with open(stand_act, "rb") as f:
            header = f.read(4)
            count = header[2]
            sizes = struct.unpack(f"<{count}H", f.read(count * 2))
            base_imgs = []
            for sz in sizes:
                base_imgs.append(Image.open(io.BytesIO(f.read(sz))).convert("RGBA"))

        num_base = len(base_imgs)

        def make_act(act_name, gen_fn, frame_count):
            png_list = []
            for f in range(frame_count):
                im = gen_fn(f, frame_count)
                q = im.quantize(colors=32, method=Image.Quantize.FASTOCTREE)
                buf = io.BytesIO()
                q.save(buf, format="PNG", optimize=True)
                png_list.append(buf.getvalue())
            pack_png_frames_to_act(png_list, stage_dir / f"{act_name}.act")

        # 1. walk_right (10 帧自然左右鸭子步)
        def gen_walk_r(f, fc):
            im = base_imgs[f % num_base].copy()
            angle = -3.0 + np.sin(f * np.pi / 2.5) * 4.0
            rot = im.rotate(angle, resample=Image.Resampling.BICUBIC, center=(48, 85))
            shift_y = int(np.abs(np.sin(f * np.pi / 2.5)) * -3.0)
            new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
            new_im.paste(rot, (0, shift_y), rot)
            return new_im
        make_act("walk_right", gen_walk_r, 10)

        # 2. walk_left (10 帧镜像)
        def gen_walk_l(f, fc):
            im = gen_walk_r(f, fc)
            return ImageOps.mirror(im)
        make_act("walk_left", gen_walk_l, 10)

        # 3. drag (10 帧悬空扑腾踢腿)
        def gen_drag(f, fc):
            im = base_imgs[f % num_base].copy()
            tilt = np.sin(f * np.pi / 2.0) * 5.0
            rot = im.rotate(tilt, resample=Image.Resampling.BICUBIC, center=(48, 48))
            squish = 1.0 + (0.06 if f % 2 == 0 else -0.06)
            nw, nh = int(96 * (2.0 - squish)), int(96 * squish)
            rs = rot.resize((nw, nh), Image.Resampling.BILINEAR)
            new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
            new_im.paste(rs, ((96 - nw) // 2, (96 - nh) // 2))
            return new_im
        make_act("drag", gen_drag, 10)

        # 4. look (10 帧左右歪头打量)
        def gen_look(f, fc):
            im = base_imgs[f % num_base].copy()
            tilt = np.sin(f * np.pi / 5.0) * 5.5
            return im.rotate(tilt, resample=Image.Resampling.BICUBIC, center=(48, 85))
        make_act("look", gen_look, 10)

        # 5. wobble (10 帧左右蹒跚摇摆)
        def gen_wobble(f, fc):
            im = base_imgs[f % num_base].copy()
            shift_x = int(np.sin(f * np.pi / 5.0) * 3.5)
            shift_y = int(np.abs(np.cos(f * np.pi / 5.0)) * -2.0)
            new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
            new_im.paste(im, (shift_x, shift_y), im)
            return new_im
        make_act("wobble", gen_wobble, 10)

        # 6. stretch (10 帧伸大懒腰)
        def gen_stretch(f, fc):
            im = base_imgs[f % num_base].copy()
            scale_y = 1.0 + np.sin(f * np.pi / 5.0) * 0.08
            scale_x = 1.0 - np.sin(f * np.pi / 5.0) * 0.04
            nw, nh = int(96 * scale_x), int(96 * scale_y)
            rs = im.resize((nw, nh), Image.Resampling.LANCZOS)
            new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
            new_im.paste(rs, ((96 - nw) // 2, 96 - nh - 2), rs)
            return new_im
        make_act("stretch", gen_stretch, 10)

        # 7. sneeze (10 帧打喷嚏)
        def gen_sneeze(f, fc):
            im = base_imgs[f % num_base].copy()
            new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
            if f in [0, 1, 2]:
                rot = im.rotate(-6.0, resample=Image.Resampling.BICUBIC, center=(48, 85))
                new_im.paste(rot, (0, -2), rot)
            elif f in [3, 4, 5]:
                rot = im.rotate(10.0, resample=Image.Resampling.BICUBIC, center=(48, 85))
                new_im.paste(rot, (0, 4), rot)
                draw = ImageDraw.Draw(new_im)
                draw.ellipse([64, 48, 70, 54], fill=(240, 245, 255, 200))
                draw.ellipse([72, 44, 76, 48], fill=(220, 235, 255, 180))
            else:
                rot = im.rotate(2.0, resample=Image.Resampling.BICUBIC, center=(48, 85))
                new_im.paste(rot, (0, 0), rot)
            return new_im
        make_act("sneeze", gen_sneeze, 10)

        # 8. yawn (10 帧打大哈欠)
        def gen_yawn(f, fc):
            im = base_imgs[f % num_base].copy()
            new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
            tilt = np.sin(f * np.pi / 5.0) * 3.0
            rot = im.rotate(tilt, resample=Image.Resampling.BICUBIC, center=(48, 85))
            new_im.paste(rot, (0, 0), rot)
            draw = ImageDraw.Draw(new_im)
            if f in [2, 3, 4, 5, 6]:
                draw.ellipse([46, 52, 54, 60], fill=(60, 40, 40, 240))
                draw.ellipse([48, 54, 52, 58], fill=(240, 100, 100, 240))
            return new_im
        make_act("yawn", gen_yawn, 10)

        # 9. angry (10 帧跺脚生气发抖)
        def gen_angry(f, fc):
            im = base_imgs[f % num_base].copy()
            enh = ImageEnhance.Color(im).enhance(1.4)
            shake_x = 2 if (f % 2 == 1) else -2
            shake_y = -3 if (f in [2, 3, 6, 7]) else 0
            new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
            new_im.paste(enh, (shake_x, shake_y), enh)
            draw = ImageDraw.Draw(new_im)
            ax, ay = 64, 28 + (f % 2) * 2
            draw.line([(ax-4, ay-4), (ax+4, ay+4)], fill=(230, 40, 40, 240), width=2)
            draw.line([(ax+4, ay-4), (ax-4, ay+4)], fill=(230, 40, 40, 240), width=2)
            return new_im
        make_act("angry", gen_angry, 10)

        # 10. shy (10 帧害羞脸红)
        def gen_shy(f, fc):
            im = base_imgs[f % num_base].copy()
            tilt = np.sin(f * np.pi / 5.0) * 3.5
            rot = im.rotate(tilt, resample=Image.Resampling.BICUBIC, center=(48, 85))
            new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
            new_im.paste(rot, (0, 2), rot)
            draw = ImageDraw.Draw(new_im)
            alpha = int(140 + np.sin(f * np.pi / 5.0) * 80)
            draw.ellipse([34, 48, 42, 54], fill=(255, 120, 140, alpha))
            draw.ellipse([58, 48, 66, 54], fill=(255, 120, 140, alpha))
            return new_im
        make_act("shy", gen_shy, 10)

        # 11. umbrella (10 帧雨天撑小花伞)
        def gen_umbrella(f, fc):
            im = base_imgs[f % num_base].copy()
            new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
            new_im.paste(im, (0, 4), im)
            draw = ImageDraw.Draw(new_im)
            ux, uy = 62, 45 + int(np.sin(f * np.pi / 5.0) * 2.0)
            draw.line([(ux, uy), (ux - 12, uy + 26)], fill=(120, 70, 30, 240), width=2)
            draw.pieslice([ux - 28, uy - 26, ux + 28, uy + 14], 180, 360, fill=(240, 70, 60, 240))
            draw.pieslice([ux - 16, uy - 26, ux + 16, uy + 14], 210, 330, fill=(255, 220, 60, 240))
            draw.line([(ux, uy - 26), (ux, uy - 30)], fill=(120, 70, 30, 240), width=2)
            return new_im
        make_act("umbrella", gen_umbrella, 10)

        # 12. cold (10 帧冬雪搓手哈气)
        def gen_cold(f, fc):
            im = base_imgs[f % num_base].copy()
            shake_x = 1 if (f % 2 == 1) else -1
            new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
            new_im.paste(im, (shake_x, 0), im)
            draw = ImageDraw.Draw(new_im)
            gx = 56 + int(np.sin(f * np.pi / 5.0) * 4.0)
            gy = 48 - (f % 5) * 2
            draw.ellipse([gx, gy, gx + 8, gy + 8], fill=(235, 245, 255, 180))
            draw.ellipse([gx + 4, gy - 4, gx + 10, gy + 2], fill=(235, 245, 255, 140))
            return new_im
        make_act("cold", gen_cold, 10)

        # 13. summer (10 帧小电扇吹风)
        def gen_summer(f, fc):
            im = base_imgs[f % num_base].copy()
            new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
            new_im.paste(im, (0, 0), im)
            draw = ImageDraw.Draw(new_im)
            fx, fy = 66, 56
            draw.line([(fx, fy), (fx - 8, fy + 16)], fill=(80, 80, 80, 240), width=2)
            draw.ellipse([fx - 8, fy - 8, fx + 8, fy + 8], outline=(40, 160, 240, 240), width=2)
            fan_ang = f * 36.0
            draw.arc([fx - 6, fy - 6, fx + 6, fy + 6], fan_ang, fan_ang + 60, fill=(30, 200, 255, 240), width=2)
            draw.arc([fx - 6, fy - 6, fx + 6, fy + 6], fan_ang + 180, fan_ang + 240, fill=(30, 200, 255, 240), width=2)
            return new_im
        make_act("summer", gen_summer, 10)

        # 14. tiwenji (10 帧体温计)
        def gen_tiwenji(f, fc):
            im = base_imgs[f % num_base].copy()
            new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
            new_im.paste(im, (0, 0), im)
            draw = ImageDraw.Draw(new_im)
            tx = 52
            ty = 56 + int(np.sin(f * np.pi / 5.0) * 1.5)
            draw.line([(tx, ty), (tx + 14, ty - 6)], fill=(220, 230, 240, 240), width=2)
            draw.line([(tx + 10, ty - 4), (tx + 14, ty - 6)], fill=(240, 50, 50, 240), width=2)
            return new_im
        make_act("tiwenji", gen_tiwenji, 10)

        # 15. injection (12 帧鸭子医生大针筒看病打针)
        def gen_injection(f, fc):
            im = base_imgs[f % num_base].copy()
            new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
            shift_x = -2 if (f in [4, 5, 6, 7]) else 0
            new_im.paste(im, (shift_x, 0), im)
            draw = ImageDraw.Draw(new_im)
            sx = 74 - min(f * 2, 8)
            sy = 48
            draw.rectangle([sx, sy - 6, sx + 14, sy + 6], fill=(220, 240, 255, 230), outline=(50, 120, 200, 240))
            draw.rectangle([sx + 1, sy - 4, sx + 8, sy + 4], fill=(255, 80, 80, 220))
            draw.line([(sx, sy), (sx - 6, sy)], fill=(160, 160, 160, 240), width=2)
            draw.line([(sx + 14, sy), (sx + 20, sy)], fill=(50, 120, 200, 240), width=2)
            draw.line([(sx + 20, sy - 4), (sx + 20, sy + 4)], fill=(50, 120, 200, 240), width=2)
            if f in [4, 5, 6, 7, 8]:
                draw.ellipse([34, 44, 38, 50], fill=(80, 180, 255, 230))
                draw.ellipse([58, 44, 62, 50], fill=(80, 180, 255, 230))
            return new_im
        make_act("injection", gen_injection, 12)




def clean_old_png_folders():
    print("\n--- Cleaning old loose png subdirectories in GG/ and MM/ ---")
    for gender in ["GG", "MM"]:
        for stage in ["Egg", "Kid", "Adult"]:
            s_dir = DATA_DIR / gender / stage
            if s_dir.exists():
                for sub in s_dir.iterdir():
                    if sub.is_dir():
                        shutil.rmtree(sub)

def main():
    print("==========================================================")
    print("Starting Official Full-Frame Action Packaging (.act format)")
    print("==========================================================")

    # 1. 先清理旧的散装目录
    clean_old_png_folders()

    server = socketserver.TCPServer(("127.0.0.1", PORT), QuietHandler)
    t = threading.Thread(target=server.serve_forever, daemon=True)
    t.start()

    with sync_playwright() as p:
        browser = p.chromium.launch(channel="msedge", headless=True)
        page = browser.new_page()

        for gender in ["GG", "MM"]:
            for stage, actions in ACTION_CONFIG.items():
                print(f"\n>>> Extracting and packing {gender}/{stage} ({len(actions)} actions)...")
                for act_name, swf_rel, num_frames, delay in actions:
                    extract_and_pack_swf(page, gender, stage, act_name, swf_rel, num_frames, delay)

        browser.close()

    server.shutdown()

    # 2. 为 GG 和 MM 两者生成 10~12 帧高质量微动作并打包为 .act
    for gender in ["GG", "MM"]:
        generate_and_pack_micro_actions(gender)

    # 3. 再次清理确保没有任何散装目录残留
    clean_old_png_folders()

    print("\n==========================================================")
    print("All actions packed into .act containers successfully!")
    print("==========================================================")

if __name__ == "__main__":
    main()
