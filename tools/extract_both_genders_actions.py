import http.server
import socketserver
import threading
import time
import shutil
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

ACTION_CONFIG = {
    "Adult": [
        ("stand", "peaceful/Stand.swf", 4, 0.18),
        ("happy", "happy/Stand.swf", 5, 0.14),
        ("sad", "sad/Stand.swf", 4, 0.18),
        ("play", "happy/play/P1.swf", 5, 0.14),
        ("work", "peaceful/play/P3.swf", 6, 0.18),  # 6 帧涵盖戴发带+连续挥汗打工
        ("study", "peaceful/play/P2.swf", 6, 0.18), # 6 帧涵盖翻书+专心阅读写字
        ("trip", "happy/play/P4.swf", 5, 0.14),
        ("eat", "Eat1.swf", 6, 0.15),
        ("clean", "Clean1.swf", 6, 0.15),
        ("sick", "Sick1.swf", 4, 0.18),
        ("cure", "Cure1.swf", 5, 0.15),
        ("dying", "Dying.swf", 4, 0.18),
        ("levelup", "LevUp.swf", 5, 0.14),
        ("hide_left", "Hide_left.swf", 4, 0.18),
        ("hide_right", "Hide_right.swf", 4, 0.18),
    ],
    "Kid": [
        ("stand", "Stand.swf", 4, 0.18),
        ("happy", "LevUp.swf", 5, 0.14),
        ("sad", "Dirty.swf", 4, 0.18),
        ("play", "play/P1.swf", 5, 0.14),
        ("work", "play/P3.swf", 6, 0.18),
        ("study", "play/P2.swf", 6, 0.18),
        ("trip", "play/P4.swf", 5, 0.14),
        ("eat", "Eat1.swf", 6, 0.15),
        ("clean", "Clean.swf", 6, 0.15),
        ("sick", "Sick.swf", 4, 0.18),
        ("cure", "Cure.swf", 5, 0.15),
        ("dying", "Dying.swf", 4, 0.18),
        ("levelup", "LevUp.swf", 5, 0.14),
        ("hide_left", "Hide_left1.swf", 4, 0.18),
        ("hide_right", "Hide_right1.swf", 4, 0.18),
    ],
    "Egg": [
        ("stand", "Stand.swf", 4, 0.18),
        ("happy", "play/P1.swf", 5, 0.14),
        ("sad", "Sick.swf", 4, 0.18),
        ("play", "play/P2.swf", 5, 0.14),
        ("work", "play/P3.swf", 6, 0.18),
        ("study", "play/P4.swf", 6, 0.18),
        ("trip", "play/P5.swf", 5, 0.14),
        ("eat", "Eat1.swf", 6, 0.15),
        ("clean", "Clean.swf", 6, 0.15),
        ("sick", "Sick.swf", 4, 0.18),
        ("cure", "Cure.swf", 5, 0.15),
        ("dying", "Dying.swf", 4, 0.18),
        ("levelup", "LevUp.swf", 5, 0.14),
        ("hide_left", "Hide_left1.swf", 4, 0.18),
        ("hide_right", "Hide_right1.swf", 4, 0.18),
    ]
}


def extract_swf_action(page, gender, stage, act_name, swf_rel, num_frames, frame_delay):
    swf_file = ACTION_ROOT / gender / stage / swf_rel
    if not swf_file.exists():
        print(f"  [MISSING] {swf_file}")
        return False

    out_dir = DATA_DIR / gender / stage / act_name
    if out_dir.exists():
        shutil.rmtree(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

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

    for f in range(num_frames):
        screenshot_bytes = page.locator("#pet").screenshot(omit_background=True)
        frame_path = out_dir / f"f_{f:02d}.png"
        frame_path.write_bytes(screenshot_bytes)

        img = Image.open(frame_path).convert("RGBA")
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
            quantized.save(frame_path, optimize=True)
        else:
            final_img = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
            final_img.save(frame_path, optimize=True)

        time.sleep(frame_delay)

    print(f"  [OK] {gender}/{stage}/{act_name} -> {num_frames} frames")
    return True

def generate_micro_and_weather_actions(gender):
    print(f"\n--- Generating smooth micro, weather and hospital actions for {gender} ---")
    for stage in ["Egg", "Kid", "Adult"]:
        base_dir = DATA_DIR / gender / stage
        stand_dir = base_dir / "stand"
        if not stand_dir.exists():
            continue

        stand_frames = sorted(list(stand_dir.glob("*.png")))
        if not stand_frames:
            continue

        base_imgs = [Image.open(f).convert("RGBA") for f in stand_frames]
        num_base = len(base_imgs)

        # 1. walk_right (4 帧自然左右鸭子步)
        walk_r_dir = base_dir / "walk_right"
        walk_r_dir.mkdir(parents=True, exist_ok=True)
        for f in range(4):
            im = base_imgs[f % num_base].copy()
            angle = -3.0 + np.sin(f * np.pi) * 4.0
            rot = im.rotate(angle, resample=Image.Resampling.BICUBIC, center=(48, 85))
            shift_y = int(np.abs(np.sin(f * np.pi)) * -3.0)
            new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
            new_im.paste(rot, (0, shift_y), rot)
            q = new_im.quantize(colors=32, method=Image.Quantize.FASTOCTREE)
            q.save(walk_r_dir / f"f_{f:02d}.png", optimize=True)

        # 2. walk_left (4 帧镜像)
        walk_l_dir = base_dir / "walk_left"
        walk_l_dir.mkdir(parents=True, exist_ok=True)
        for f in range(4):
            rf = walk_r_dir / f"f_{f:02d}.png"
            im = Image.open(rf).convert("RGBA")
            flipped = ImageOps.mirror(im)
            q = flipped.quantize(colors=32, method=Image.Quantize.FASTOCTREE)
            q.save(walk_l_dir / f"f_{f:02d}.png", optimize=True)

        # 3. drag (4 帧悬空扑腾踢腿)
        drag_dir = base_dir / "drag"
        drag_dir.mkdir(parents=True, exist_ok=True)
        for f in range(4):
            im = base_imgs[f % num_base].copy()
            tilt = np.sin(f * np.pi) * 5.0
            rot = im.rotate(tilt, resample=Image.Resampling.BICUBIC, center=(48, 48))
            squish = 1.0 + (0.06 if f % 2 == 0 else -0.06)
            nw, nh = int(96 * (2.0 - squish)), int(96 * squish)
            rs = rot.resize((nw, nh), Image.Resampling.BILINEAR)
            new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
            new_im.paste(rs, ((96 - nw) // 2, (96 - nh) // 2))
            q = new_im.quantize(colors=32, method=Image.Quantize.FASTOCTREE)
            q.save(drag_dir / f"f_{f:02d}.png", optimize=True)

        # 4. look (4 帧左右歪头张望)
        look_dir = base_dir / "look"
        look_dir.mkdir(parents=True, exist_ok=True)
        for f in range(4):
            im = base_imgs[f % num_base].copy()
            tilt = np.sin(f * np.pi / 2.0) * 5.5
            rot = im.rotate(tilt, resample=Image.Resampling.BICUBIC, center=(48, 85))
            q = rot.quantize(colors=32, method=Image.Quantize.FASTOCTREE)
            q.save(look_dir / f"f_{f:02d}.png", optimize=True)

        # 5. wobble (4 帧左右蹒跚摇摆)
        wobble_dir = base_dir / "wobble"
        wobble_dir.mkdir(parents=True, exist_ok=True)
        for f in range(4):
            im = base_imgs[f % num_base].copy()
            shift_x = int(np.sin(f * np.pi / 2.0) * 3.5)
            shift_y = int(np.abs(np.cos(f * np.pi / 2.0)) * -2.0)
            new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
            new_im.paste(im, (shift_x, shift_y), im)
            q = new_im.quantize(colors=32, method=Image.Quantize.FASTOCTREE)
            q.save(wobble_dir / f"f_{f:02d}.png", optimize=True)

        # 6. stretch (4 帧伸大懒腰)
        stretch_dir = base_dir / "stretch"
        stretch_dir.mkdir(parents=True, exist_ok=True)
        for f in range(4):
            im = base_imgs[f % num_base].copy()
            scale_y = 1.0 + np.sin(f * np.pi / 2.0) * 0.08
            scale_x = 1.0 - np.sin(f * np.pi / 2.0) * 0.04
            nw, nh = int(96 * scale_x), int(96 * scale_y)
            rs = im.resize((nw, nh), Image.Resampling.LANCZOS)
            new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
            new_im.paste(rs, ((96 - nw) // 2, 96 - nh - 2), rs)
            q = new_im.quantize(colors=32, method=Image.Quantize.FASTOCTREE)
            q.save(stretch_dir / f"f_{f:02d}.png", optimize=True)

        # 7. sneeze (5 帧打喷嚏全流程)
        sneeze_dir = base_dir / "sneeze"
        sneeze_dir.mkdir(parents=True, exist_ok=True)
        for f in range(5):
            im = base_imgs[f % num_base].copy()
            new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
            if f in [0, 1]:
                rot = im.rotate(-6.0, resample=Image.Resampling.BICUBIC, center=(48, 85))
                new_im.paste(rot, (0, -2), rot)
            elif f in [2, 3]:
                rot = im.rotate(10.0, resample=Image.Resampling.BICUBIC, center=(48, 85))
                new_im.paste(rot, (0, 4), rot)
                draw = ImageDraw.Draw(new_im)
                draw.ellipse([64, 48, 70, 54], fill=(240, 245, 255, 200))
                draw.ellipse([72, 44, 76, 48], fill=(220, 235, 255, 180))
            else:
                rot = im.rotate(2.0, resample=Image.Resampling.BICUBIC, center=(48, 85))
                new_im.paste(rot, (0, 0), rot)
            q = new_im.quantize(colors=32, method=Image.Quantize.FASTOCTREE)
            q.save(sneeze_dir / f"f_{f:02d}.png", optimize=True)

        # 8. yawn (5 帧揉眼打大哈欠)
        yawn_dir = base_dir / "yawn"
        yawn_dir.mkdir(parents=True, exist_ok=True)
        for f in range(5):
            im = base_imgs[f % num_base].copy()
            new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
            tilt = np.sin(f * np.pi / 2.0) * 3.0
            rot = im.rotate(tilt, resample=Image.Resampling.BICUBIC, center=(48, 85))
            new_im.paste(rot, (0, 0), rot)
            draw = ImageDraw.Draw(new_im)
            if f in [1, 2, 3]:
                draw.ellipse([46, 52, 54, 60], fill=(60, 40, 40, 240))
                draw.ellipse([48, 54, 52, 58], fill=(240, 100, 100, 240))
            q = new_im.quantize(colors=32, method=Image.Quantize.FASTOCTREE)
            q.save(yawn_dir / f"f_{f:02d}.png", optimize=True)

        # 9. angry (5 帧跺脚生气发抖)
        angry_dir = base_dir / "angry"
        angry_dir.mkdir(parents=True, exist_ok=True)
        for f in range(5):
            im = base_imgs[f % num_base].copy()
            enh = ImageEnhance.Color(im).enhance(1.4)
            shake_x = 2 if (f % 2 == 1) else -2
            shake_y = -3 if (f in [1, 2]) else 0
            new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
            new_im.paste(enh, (shake_x, shake_y), enh)
            draw = ImageDraw.Draw(new_im)
            ax, ay = 64, 28 + (f % 2) * 2
            draw.line([(ax-4, ay-4), (ax+4, ay+4)], fill=(230, 40, 40, 240), width=2)
            draw.line([(ax+4, ay-4), (ax-4, ay+4)], fill=(230, 40, 40, 240), width=2)
            q = new_im.quantize(colors=32, method=Image.Quantize.FASTOCTREE)
            q.save(angry_dir / f"f_{f:02d}.png", optimize=True)

        # 10. shy (4 帧害羞双颊泛红)
        shy_dir = base_dir / "shy"
        shy_dir.mkdir(parents=True, exist_ok=True)
        for f in range(4):
            im = base_imgs[f % num_base].copy()
            tilt = np.sin(f * np.pi / 2.0) * 3.5
            rot = im.rotate(tilt, resample=Image.Resampling.BICUBIC, center=(48, 85))
            new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
            new_im.paste(rot, (0, 2), rot)
            draw = ImageDraw.Draw(new_im)
            alpha = int(140 + np.sin(f * np.pi / 2.0) * 80)
            draw.ellipse([34, 48, 42, 54], fill=(255, 120, 140, alpha))
            draw.ellipse([58, 48, 66, 54], fill=(255, 120, 140, alpha))
            q = new_im.quantize(colors=32, method=Image.Quantize.FASTOCTREE)
            q.save(shy_dir / f"f_{f:02d}.png", optimize=True)

        # 11. umbrella (4 帧雨天小花伞)
        umb_dir = base_dir / "umbrella"
        umb_dir.mkdir(parents=True, exist_ok=True)
        for f in range(4):
            im = base_imgs[f % num_base].copy()
            new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
            new_im.paste(im, (0, 4), im)
            draw = ImageDraw.Draw(new_im)
            ux, uy = 62, 45 + int(np.sin(f * np.pi / 2.0) * 2.0)
            draw.line([(ux, uy), (ux - 12, uy + 26)], fill=(120, 70, 30, 240), width=2)
            draw.pieslice([ux - 28, uy - 26, ux + 28, uy + 14], 180, 360, fill=(240, 70, 60, 240))
            draw.pieslice([ux - 16, uy - 26, ux + 16, uy + 14], 210, 330, fill=(255, 220, 60, 240))
            draw.line([(ux, uy - 26), (ux, uy - 30)], fill=(120, 70, 30, 240), width=2)
            q = new_im.quantize(colors=32, method=Image.Quantize.FASTOCTREE)
            q.save(umb_dir / f"f_{f:02d}.png", optimize=True)

        # 12. cold (4 帧冬雪搓手哈气)
        cold_dir = base_dir / "cold"
        cold_dir.mkdir(parents=True, exist_ok=True)
        for f in range(4):
            im = base_imgs[f % num_base].copy()
            shake_x = 1 if (f % 2 == 1) else -1
            new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
            new_im.paste(im, (shake_x, 0), im)
            draw = ImageDraw.Draw(new_im)
            gx = 56 + int(np.sin(f * np.pi / 2.0) * 4.0)
            gy = 48 - (f % 2) * 2
            draw.ellipse([gx, gy, gx + 8, gy + 8], fill=(235, 245, 255, 180))
            draw.ellipse([gx + 4, gy - 4, gx + 10, gy + 2], fill=(235, 245, 255, 140))
            q = new_im.quantize(colors=32, method=Image.Quantize.FASTOCTREE)
            q.save(cold_dir / f"f_{f:02d}.png", optimize=True)

        # 13. summer (4 帧小电风扇清凉吹风)
        summer_dir = base_dir / "summer"
        summer_dir.mkdir(parents=True, exist_ok=True)
        for f in range(4):
            im = base_imgs[f % num_base].copy()
            new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
            new_im.paste(im, (0, 0), im)
            draw = ImageDraw.Draw(new_im)
            fx, fy = 66, 56
            draw.line([(fx, fy), (fx - 8, fy + 16)], fill=(80, 80, 80, 240), width=2)
            draw.ellipse([fx - 8, fy - 8, fx + 8, fy + 8], outline=(40, 160, 240, 240), width=2)
            fan_ang = f * 90.0
            draw.arc([fx - 6, fy - 6, fx + 6, fy + 6], fan_ang, fan_ang + 60, fill=(30, 200, 255, 240), width=2)
            draw.arc([fx - 6, fy - 6, fx + 6, fy + 6], fan_ang + 180, fan_ang + 240, fill=(30, 200, 255, 240), width=2)
            q = new_im.quantize(colors=32, method=Image.Quantize.FASTOCTREE)
            q.save(summer_dir / f"f_{f:02d}.png", optimize=True)

        # 14. tiwenji (4 帧含水银体温计)
        tiwenji_dir = base_dir / "tiwenji"
        tiwenji_dir.mkdir(parents=True, exist_ok=True)
        for f in range(4):
            im = base_imgs[f % num_base].copy()
            new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
            new_im.paste(im, (0, 0), im)
            draw = ImageDraw.Draw(new_im)
            tx = 52
            ty = 56 + int(np.sin(f * np.pi / 2.0) * 1.5)
            draw.line([(tx, ty), (tx + 14, ty - 6)], fill=(220, 230, 240, 240), width=2)
            draw.line([(tx + 10, ty - 4), (tx + 14, ty - 6)], fill=(240, 50, 50, 240), width=2)
            q = new_im.quantize(colors=32, method=Image.Quantize.FASTOCTREE)
            q.save(tiwenji_dir / f"f_{f:02d}.png", optimize=True)

        # 15. injection (5 帧鸭子医生大针筒看病打针)
        inj_dir = base_dir / "injection"
        inj_dir.mkdir(parents=True, exist_ok=True)
        for f in range(5):
            im = base_imgs[f % num_base].copy()
            new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
            shift_x = -2 if (f in [1, 2, 3]) else 0
            new_im.paste(im, (shift_x, 0), im)
            draw = ImageDraw.Draw(new_im)
            sx = 74 - min(f * 2, 8)
            sy = 48
            draw.rectangle([sx, sy - 6, sx + 14, sy + 6], fill=(220, 240, 255, 230), outline=(50, 120, 200, 240))
            draw.rectangle([sx + 1, sy - 4, sx + 8, sy + 4], fill=(255, 80, 80, 220))
            draw.line([(sx, sy), (sx - 6, sy)], fill=(160, 160, 160, 240), width=2)
            draw.line([(sx + 14, sy), (sx + 20, sy)], fill=(50, 120, 200, 240), width=2)
            draw.line([(sx + 20, sy - 4), (sx + 20, sy + 4)], fill=(50, 120, 200, 240), width=2)
            if f in [1, 2, 3]:
                draw.ellipse([34, 44, 38, 50], fill=(80, 180, 255, 230))
                draw.ellipse([58, 44, 62, 50], fill=(80, 180, 255, 230))
            q = new_im.quantize(colors=32, method=Image.Quantize.FASTOCTREE)
            q.save(inj_dir / f"f_{f:02d}.png", optimize=True)


def main():
    print("==========================================================")
    print("Extracting full GG & MM actions (Egg, Kid, Adult)")
    print("==========================================================")

    server = socketserver.TCPServer(("127.0.0.1", PORT), QuietHandler)
    t = threading.Thread(target=server.serve_forever, daemon=True)
    t.start()

    with sync_playwright() as p:
        browser = p.chromium.launch(channel="msedge", headless=True)
        page = browser.new_page()

        for gender in ["GG", "MM"]:
            for stage, actions in ACTION_CONFIG.items():
                print(f"\n>>> Extracting {gender}/{stage} ({len(actions)} actions)...")
                for act_name, swf_rel, num_frames, delay in actions:
                    extract_swf_action(page, gender, stage, act_name, swf_rel, num_frames, delay)

        browser.close()

    server.shutdown()

    # 为 GG 和 MM 两者均生成完整的高清生活微动作
    for gender in ["GG", "MM"]:
        generate_micro_and_weather_actions(gender)

    print("\n==========================================================")
    print("GG and MM full actions generated successfully!")
    print("==========================================================")

if __name__ == "__main__":
    main()
