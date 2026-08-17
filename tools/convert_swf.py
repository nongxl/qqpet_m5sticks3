import os
import sys
import time
import json
import http.server
import socketserver
import threading
from pathlib import Path
from PIL import Image
from playwright.sync_api import sync_playwright

WORKSPACE_DIR = Path("d:/workspace/QQpet_StickS3")
RUFFLE_DIR = WORKSPACE_DIR / "doc/qqpet_automation/qq-pet-macos/src/windows/js/ruffle"
ASSETS_DIR = WORKSPACE_DIR / "doc/qqpet_automation/qq-pet-macos/src/assets/Action/GG/Adult"
OUTPUT_DIR = WORKSPACE_DIR / "src/pet_frames"

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

# 需要提取的核心动作清单
ACTIONS_TO_EXTRACT = [
    {"name": "stand", "swf": "peaceful/Stand.swf", "frames": 12, "fps": 8},
    {"name": "stand1", "swf": "peaceful/Stand1.swf", "frames": 12, "fps": 8},
    {"name": "happy", "swf": "happy/Stand.swf", "frames": 14, "fps": 10},
    {"name": "play", "swf": "happy/play/P1.swf", "frames": 16, "fps": 10},
    {"name": "eat", "swf": "Eat1.swf", "frames": 16, "fps": 10},
    {"name": "clean", "swf": "Clean1.swf", "frames": 16, "fps": 10},
    {"name": "sick", "swf": "Sick1.swf", "frames": 12, "fps": 8},
    {"name": "dying", "swf": "Dying.swf", "frames": 10, "fps": 6},
    {"name": "cure", "swf": "Cure1.swf", "frames": 14, "fps": 10},
    {"name": "levelup", "swf": "LevUp.swf", "frames": 16, "fps": 10},
]


HTML_TEMPLATE = """
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <script src="/doc/qqpet_automation/qq-pet-macos/src/windows/js/ruffle/ruffle.js"></script>
    <style>
        body, html {{ margin: 0; padding: 0; background: transparent; overflow: hidden; }}
        #pet-wrapper {{ width: 140px; height: 140px; background: transparent; }}
    </style>
</head>
<body>
    <div id="pet-wrapper"></div>
    <script>
        window.isRuffleReady = false;
        window.RufflePlayer = window.RufflePlayer || {{}};
        const ruffle = window.RufflePlayer.newest();
        const player = ruffle.createPlayer();
        player.id = "player";
        player.style.width = "140px";
        player.style.height = "140px";
        document.getElementById("pet-wrapper").appendChild(player);

        player.load({{
            url: "{swf_url}",
            autoplay: "on",
            backgroundColor: null,
            letterbox: "off",
            unmuteOverlay: "hidden",
            wmode: "transparent"
        }}).then(() => {{
            window.isRuffleReady = true;
        }}).catch(e => {{
            console.error("Ruffle error:", e);
        }});
    </script>
</body>
</html>
"""

def extract_all():
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    server = start_server()
    print("HTTP Server started on 127.0.0.1:8765")

    with sync_playwright() as p:
        browser = p.chromium.launch(headless=True, channel="msedge")
        context = browser.new_context(viewport={"width": 160, "height": 160}, device_scale_factor=1)
        page = context.new_page()

        for act in ACTIONS_TO_EXTRACT:
            act_name = act["name"]
            swf_rel = act["swf"]
            num_frames = act["frames"]
            frame_delay = 1.0 / act["fps"]

            print(f"\n[Extracting] {act_name} from {swf_rel} ({num_frames} frames)...")
            swf_url = f"/doc/qqpet_automation/qq-pet-macos/src/assets/Action/GG/Adult/{swf_rel}"
            
            # 临时生成播放 HTML
            temp_html = WORKSPACE_DIR / "tools/temp_player.html"
            temp_html.parent.mkdir(exist_ok=True)
            temp_html.write_text(HTML_TEMPLATE.format(swf_url=swf_url), encoding="utf-8")

            page.goto(f"http://127.0.0.1:{PORT}/tools/temp_player.html")
            page.wait_for_function("window.isRuffleReady === true", timeout=15000)
            time.sleep(0.5) # 等待渲染就绪

            act_dir = OUTPUT_DIR / act_name
            act_dir.mkdir(parents=True, exist_ok=True)

            for f in range(num_frames):
                screenshot_bytes = page.locator("#pet-wrapper").screenshot(omit_background=True)
                frame_path = act_dir / f"frame_{f:02d}.png"
                frame_path.write_bytes(screenshot_bytes)
                
                # 自动裁剪透明白边并等比缩放到 96x96 (以高度 90px 为基准保持所有动作体型一致)
                img = Image.open(frame_path).convert("RGBA")
                bbox = img.getbbox()
                if bbox:
                    cropped = img.crop(bbox)
                    # 统一将企鹅身体等比放大至 92px 高度 (或宽度不超过 92px)
                    target_size = 92
                    w, h = cropped.size
                    scale = min(target_size / w, target_size / h)
                    new_w = int(w * scale)
                    new_h = int(h * scale)
                    
                    resized = cropped.resize((new_w, new_h), Image.Resampling.LANCZOS)
                    
                    # 贴在 96x96 透明画布正中靠下 (企鹅脚丫贴底)
                    final_img = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
                    paste_x = (96 - new_w) // 2
                    paste_y = 96 - new_h - 2 # 底部留 2px 阴影边距
                    final_img.paste(resized, (paste_x, paste_y))
                    final_img.save(frame_path)
                
                time.sleep(frame_delay)


            print(f"  -> Saved {num_frames} frames to {act_dir}")

        browser.close()
    print("\nAll frames successfully extracted!")

if __name__ == "__main__":
    extract_all()
