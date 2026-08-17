import os
import time
import threading
import http.server
import socketserver
from pathlib import Path
from playwright.sync_api import sync_playwright

PORT = 8766
WORKSPACE_DIR = Path("d:/workspace/QQpet_StickS3")

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
                transparent: true
            }};
            document.getElementById("pet-wrapper").appendChild(player);
            player.load("{swf_url}");
        }});
    </script>
</body>
</html>
"""

def main():
    server = start_server()
    preview_dir = WORKSPACE_DIR / "tools/action_previews"
    preview_dir.mkdir(exist_ok=True, parents=True)

    with sync_playwright() as p:
        browser = p.chromium.launch(headless=True, channel="msedge")
        page = browser.new_page(viewport={"width": 200, "height": 200})


        # 探测 Kid/play 下的 P1 ~ P30
        for i in range(1, 35):
            swf_rel = f"doc/qqpet_automation/qq-pet-macos/src/assets/Action/GG/Kid/play/P{i}.swf"
            html_content = HTML_TEMPLATE.format(swf_url=f"/{swf_rel}")
            page.set_content(html_content)
            time.sleep(0.4) # 等待加载
            out_png = preview_dir / f"Kid_P{i}.png"
            page.screenshot(path=str(out_png), omit_background=True)
            print(f"Captured Kid_P{i}.png")

        # 探测 Adult/happy/play 下的 P1 ~ P20
        for i in range(1, 25):
            swf_rel = f"doc/qqpet_automation/qq-pet-macos/src/assets/Action/GG/Adult/happy/play/P{i}.swf"
            html_content = HTML_TEMPLATE.format(swf_url=f"/{swf_rel}")
            page.set_content(html_content)
            time.sleep(0.4)
            out_png = preview_dir / f"Adult_P{i}.png"
            page.screenshot(path=str(out_png), omit_background=True)
            print(f"Captured Adult_P{i}.png")

        browser.close()
    server.shutdown()
    print("Done inspection!")

if __name__ == "__main__":
    main()
