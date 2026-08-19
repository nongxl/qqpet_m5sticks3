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

    # 检查 peaceful/play 下的不同 SWF
    test_swfs = [
        ("peaceful_P3", "peaceful/play/P3.swf"),
        ("peaceful_P8", "peaceful/play/P8.swf"),
        ("peaceful_P12", "peaceful/play/P12.swf"),
        ("peaceful_P20", "peaceful/play/P20.swf"),
        ("game_Game1", "game/Game1.swf"),
    ]

    with sync_playwright() as p:
        browser = p.chromium.launch(channel="msedge", headless=True)
        page = browser.new_page()

        for name, swf in test_swfs:
            p_path = WORKSPACE / f"doc/qqpet_automation/qq-pet-macos/src/assets/Action/GG/Adult/{swf}"
            if not p_path.exists():
                print(f"Not found: {swf}")
                continue
            rel_url = f"/doc/qqpet_automation/qq-pet-macos/src/assets/Action/GG/Adult/{swf}".replace("\\", "/")
            (WORKSPACE / "tools/inspect_temp.html").write_text(html_tpl.format(swf_url=rel_url), encoding="utf-8")
            page.goto(f"http://127.0.0.1:{PORT}/tools/inspect_temp.html")
            page.wait_for_selector("#pet ruffle-player", timeout=8000)
            time.sleep(0.4)

            out_dir = WORKSPACE / f"tools/inspect_swfs/{name}"
            out_dir.mkdir(parents=True, exist_ok=True)
            for f in range(12):
                bs = page.locator("#pet").screenshot(omit_background=True)
                (out_dir / f"f_{f:02d}.png").write_bytes(bs)
                time.sleep(0.1)
            print(f"Captured {name} ({swf})")

        browser.close()

    server.shutdown()

if __name__ == "__main__":
    main()
