import http.server
import socketserver
import threading
import time
from pathlib import Path
from PIL import Image
from playwright.sync_api import sync_playwright

WORKSPACE = Path("d:/workspace/QQpet_StickS3")
PORT = 8899

class QuietHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=str(WORKSPACE), **kwargs)
    def log_message(self, format, *args):
        pass

def main():
    server = socketserver.TCPServer(("127.0.0.1", PORT), QuietHandler)
    t = threading.Thread(target=server.serve_forever, daemon=True)
    t.start()

    html_tpl = """<!DOCTYPE html>
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

    # 检索更多 SWF
    swfs_to_check = [
        f"peaceful/play/P{i}.swf" for i in [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 30, 35, 40, 50, 60]
    ]

    out_base = WORKSPACE / "tools/inspect_actions"
    out_base.mkdir(parents=True, exist_ok=True)

    with sync_playwright() as p:
        browser = p.chromium.launch(channel="msedge", headless=True)
        page = browser.new_page()

        for swf in swfs_to_check:
            p_path = WORKSPACE / f"doc/qqpet_automation/qq-pet-macos/src/assets/Action/GG/Adult/{swf}"
            if not p_path.exists():
                continue
            rel_url = f"/doc/qqpet_automation/qq-pet-macos/src/assets/Action/GG/Adult/{swf}".replace("\\", "/")
            (WORKSPACE / "tools/temp_act.html").write_text(html_tpl.format(swf_url=rel_url), encoding="utf-8")
            page.goto(f"http://127.0.0.1:{PORT}/tools/temp_act.html")
            page.wait_for_selector("#pet ruffle-player", timeout=6000)
            time.sleep(0.3)

            # 抓取第 4 帧与第 10 帧
            time.sleep(0.3)
            b1 = page.locator("#pet").screenshot(omit_background=True)
            time.sleep(0.4)
            b2 = page.locator("#pet").screenshot(omit_background=True)
            name = swf.replace("/", "_").replace(".swf", "")
            (out_base / f"{name}_f1.png").write_bytes(b1)
            (out_base / f"{name}_f2.png").write_bytes(b2)
            print(f"Captured {swf}")

        browser.close()

    server.shutdown()

if __name__ == "__main__":
    main()
