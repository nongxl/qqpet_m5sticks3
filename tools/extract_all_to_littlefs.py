import os
import shutil
import time
import socketserver
import http.server
import threading
from pathlib import Path
from PIL import Image
from playwright.sync_api import sync_playwright

WORKSPACE_DIR = Path("d:/workspace/QQpet_StickS3")
DATA_DIR = WORKSPACE_DIR / "data/assets"
ICONS_SRC_DIR = WORKSPACE_DIR / "doc/qqpet_automation/qq-pet-macos/src/assets/control/icons"
BG_SRC_DIR = WORKSPACE_DIR / "doc/qqpet_automation/qq-pet-macos/src/assets/Background"
PORT = 8765

class QuietHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=str(WORKSPACE_DIR), **kwargs)
    def log_message(self, format, *args):
        pass

def start_server():
    server = socketserver.TCPServer(("127.0.0.1", PORT), QuietHandler)
    t = threading.Thread(target=server.serve_forever, daemon=True)
    t.start()
    return server

ADULT_ACTIONS = [
    {"name": "stand", "swf": "peaceful/Stand.swf", "frames": 8, "fps": 8},
    {"name": "happy", "swf": "happy/Stand.swf", "frames": 8, "fps": 8},
    {"name": "sad", "swf": "sad/Stand.swf", "frames": 8, "fps": 8},
    {"name": "play", "swf": "happy/play/P1.swf", "frames": 10, "fps": 10},
    {"name": "play_1", "swf": "happy/play/P2.swf", "frames": 10, "fps": 10},
    {"name": "play_2", "swf": "happy/play/P3.swf", "frames": 10, "fps": 10},
    {"name": "play_3", "swf": "happy/play/P4.swf", "frames": 10, "fps": 10},
    {"name": "play_4", "swf": "happy/play/P5.swf", "frames": 10, "fps": 10},
    {"name": "work", "swf": "game/Game1.swf", "frames": 10, "fps": 10},
    {"name": "study", "swf": "peaceful/play/P2.swf", "frames": 10, "fps": 10},
    {"name": "trip", "swf": "happy/play/P4.swf", "frames": 10, "fps": 10},
    {"name": "eat", "swf": "Eat1.swf", "frames": 10, "fps": 10},
    {"name": "clean", "swf": "Clean1.swf", "frames": 10, "fps": 10},
    {"name": "sick", "swf": "Sick1.swf", "frames": 8, "fps": 8},
    {"name": "dying", "swf": "Dying.swf", "frames": 8, "fps": 6},
    {"name": "cure", "swf": "Cure1.swf", "frames": 10, "fps": 10},
    {"name": "levelup", "swf": "LevUp.swf", "frames": 10, "fps": 10},
]

KID_ACTIONS = [
    {"name": "stand", "swf": "Stand.swf", "frames": 8, "fps": 8},
    {"name": "happy", "swf": "LevUp.swf", "frames": 8, "fps": 8},
    {"name": "sad", "swf": "Dirty.swf", "frames": 8, "fps": 8},
    {"name": "play", "swf": "play/P1.swf", "frames": 10, "fps": 10},
    {"name": "work", "swf": "play/P3.swf", "frames": 10, "fps": 10},
    {"name": "study", "swf": "play/P2.swf", "frames": 10, "fps": 10},
    {"name": "trip", "swf": "play/P4.swf", "frames": 10, "fps": 10},
    {"name": "eat", "swf": "Eat1.swf", "frames": 10, "fps": 10},
    {"name": "clean", "swf": "Clean.swf", "frames": 10, "fps": 10},
    {"name": "sick", "swf": "Sick.swf", "frames": 8, "fps": 8},
    {"name": "dying", "swf": "Dying.swf", "frames": 8, "fps": 6},
    {"name": "cure", "swf": "Cure.swf", "frames": 10, "fps": 10},
    {"name": "levelup", "swf": "LevUp.swf", "frames": 10, "fps": 10},
]

EGG_ACTIONS = [
    {"name": "stand", "swf": "Stand.swf", "frames": 8, "fps": 8},
    {"name": "happy", "swf": "play/P1.swf", "frames": 8, "fps": 8},
    {"name": "sad", "swf": "Sick.swf", "frames": 8, "fps": 8},
    {"name": "play", "swf": "play/P2.swf", "frames": 10, "fps": 10},
    {"name": "work", "swf": "play/P3.swf", "frames": 10, "fps": 10},
    {"name": "study", "swf": "play/P4.swf", "frames": 10, "fps": 10},
    {"name": "trip", "swf": "play/P5.swf", "frames": 10, "fps": 10},
    {"name": "eat", "swf": "Eat1.swf", "frames": 10, "fps": 10},
    {"name": "clean", "swf": "Clean.swf", "frames": 10, "fps": 10},
    {"name": "sick", "swf": "Sick.swf", "frames": 8, "fps": 8},
    {"name": "dying", "swf": "Dying.swf", "frames": 8, "fps": 6},
    {"name": "cure", "swf": "Cure.swf", "frames": 10, "fps": 10},
    {"name": "levelup", "swf": "LevUp.swf", "frames": 10, "fps": 10},
]







HTML_TEMPLATE = """
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <script src="/doc/qqpet_automation/qq-pet-macos/src/windows/js/ruffle/ruffle.js"></script>
    <style>
        body {{ margin: 0; padding: 0; background: transparent; overflow: hidden; }}
        #pet-wrapper {{ width: 200px; height: 200px; }}
    </style>
</head>
<body>
    <div id="pet-wrapper"></div>
    <script>
        window.addEventListener("DOMContentLoaded", () => {{
            const ruffle = window.RufflePlayer.newest();
            const player = ruffle.createPlayer();
            player.style.width = "100%";
            player.style.height = "100%";
            player.config = {{
                autoplay: "on",
                unmuteOverlay: "hidden",
                letterbox: "off",
                backgroundColor: null,
                wmode: "transparent"
            }};
            document.getElementById("pet-wrapper").appendChild(player);
            player.load("{swf_url}");
        }});
    </script>
</body>
</html>
"""

def extract_actions_list(page, gender_name, stage_name, action_list):
    print(f"\n==================== Extracting {gender_name}/{stage_name} ====================")
    for act in action_list:
        act_name = act["name"]
        swf_rel = f"doc/qqpet_automation/qq-pet-macos/src/assets/Action/{gender_name}/{stage_name}/{act['swf']}"
        num_frames = act["frames"]
        frame_delay = 1.0 / act["fps"]

        out_act_dir = DATA_DIR / f"{gender_name}/{stage_name}/{act_name}"
        out_act_dir.mkdir(parents=True, exist_ok=True)

        swf_full_path = WORKSPACE_DIR / swf_rel
        if not swf_full_path.exists():
            print(f"  [Warning] SWF not found: {swf_full_path}")
            continue

        print(f"[{gender_name}/{stage_name}] Extracting {act_name} ({num_frames} frames)...")
        html_content = HTML_TEMPLATE.format(swf_url=f"/{swf_rel.replace(chr(92), '/')}")
        temp_html = WORKSPACE_DIR / "tools/temp_player.html"
        temp_html.write_text(html_content, encoding="utf-8")

        page.goto(f"http://127.0.0.1:{PORT}/tools/temp_player.html")
        page.wait_for_selector("#pet-wrapper ruffle-player", timeout=10000)
        time.sleep(0.4)

        for f in range(num_frames):
            screenshot_bytes = page.locator("#pet-wrapper").screenshot(omit_background=True)
            frame_path = out_act_dir / f"f_{f:02d}.png"
            frame_path.write_bytes(screenshot_bytes)

            img = Image.open(frame_path).convert("RGBA")
            bbox = img.getbbox()
            if bbox:
                cropped = img.crop(bbox)
                # Egg 破壳雏鸟 (72px), Kid 幼年 (82px), Adult 成年 (92px)
                if stage_name == "Egg":
                    target_size = 72
                elif stage_name == "Kid":
                    target_size = 82
                else:
                    target_size = 92

                w, h = cropped.size
                scale = min(target_size / w, target_size / h)
                new_w = int(w * scale)
                new_h = int(h * scale)

                resized = cropped.resize((new_w, new_h), Image.Resampling.LANCZOS)
                final_img = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
                paste_x = (96 - new_w) // 2
                paste_y = 96 - new_h - 2
                final_img.paste(resized, (paste_x, paste_y))
                
                quantized = final_img.quantize(colors=32, method=Image.Quantize.FASTOCTREE)
                quantized.save(frame_path, optimize=True)


            time.sleep(frame_delay)

def extract_backgrounds():
    print("\n==================== Extracting & Cropping Backgrounds ====================")
    bg_out = DATA_DIR / "bg"
    bg_out.mkdir(parents=True, exist_ok=True)

    for i in range(1, 17):
        src_name = f"b{i:07d}.png"
        src_file = BG_SRC_DIR / src_name
        if not src_file.exists():
            continue
        
        img = Image.open(src_file).convert("RGB")
        w, h = img.size
        # 裁切为 135:240 竖屏长宽比
        target_ratio = 135.0 / 240.0
        current_ratio = w / h

        if current_ratio > target_ratio:
            # 原图较宽，裁切左右两边
            new_w = int(h * target_ratio)
            left = (w - new_w) // 2
            cropped = img.crop((left, 0, left + new_w, h))
        else:
            # 原图较长，裁切上下
            new_h = int(w / target_ratio)
            top = (h - new_h) // 2
            cropped = img.crop((0, top, w, top + new_h))

        resized = cropped.resize((135, 240), Image.Resampling.LANCZOS)
        # 64色调色板压缩，每张仅 ~7KB
        quantized = resized.quantize(colors=64, method=Image.Quantize.FASTOCTREE)
        out_file = bg_out / f"bg_{i:02d}.png"
        quantized.save(out_file, optimize=True)
        print(f"  -> Processed bg_{i:02d}.png")

def extract_icons():
    print("\n==================== Extracting Menu Icons ====================")
    icons_out = DATA_DIR / "icons"
    icons_out.mkdir(parents=True, exist_ok=True)

    icon_mapping = [
        ("feed", "weishi.png"),
        ("bath", "qingjie.png"),
        ("play", "wanshua.png"),
        ("work", "dagong.png"),
        ("study", "xuexi.png"),
        ("trip", "lvyou.png"),
        ("cure", "zhibing.png"),
        ("shop", "richang.png"),
        ("status", "chongwu.png"),
        ("web", "guanli.png"),
    ]

    for name, f_name in icon_mapping:
        src = ICONS_SRC_DIR / f_name
        if not src.exists(): continue
        img = Image.open(src).convert("RGBA")
        
        norm_img = img.resize((20, 20), Image.Resampling.LANCZOS)
        norm_q = norm_img.quantize(colors=32, method=Image.Quantize.FASTOCTREE)
        norm_q.save(icons_out / f"{name}_norm.png", optimize=True)

        act_img = img.resize((28, 28), Image.Resampling.LANCZOS)
        act_q = act_img.quantize(colors=32, method=Image.Quantize.FASTOCTREE)
        act_q.save(icons_out / f"{name}_act.png", optimize=True)
    print("  -> All 10 Icons extracted successfully!")


def main():
    if (WORKSPACE_DIR / "data").exists():
        shutil.rmtree(WORKSPACE_DIR / "data")
    DATA_DIR.mkdir(parents=True, exist_ok=True)

    server = start_server()
    print("Local HTTP Server started.")

    with sync_playwright() as p:
        browser = p.chromium.launch(channel="msedge", headless=True)
        page = browser.new_page(viewport={"width": 300, "height": 300})

        # 1. 提取 GG (Egg + Kid + Adult)
        extract_actions_list(page, "GG", "Egg", EGG_ACTIONS)
        extract_actions_list(page, "GG", "Kid", KID_ACTIONS)
        extract_actions_list(page, "GG", "Adult", ADULT_ACTIONS)

        # 2. 提取 MM (Egg + Kid + Adult)
        extract_actions_list(page, "MM", "Egg", EGG_ACTIONS)
        extract_actions_list(page, "MM", "Kid", KID_ACTIONS)
        extract_actions_list(page, "MM", "Adult", ADULT_ACTIONS)

        browser.close()

    # 3. 提取背景壁纸 (16 款原版)
    extract_backgrounds()

    # 4. 提取菜单图标
    extract_icons()

    print("\nAll full-stage assets (Egg/Kid/Adult + 16 BGs + Icons) ready for LittleFS!")

if __name__ == "__main__":
    main()
