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

    html = """<!DOCTYPE html>
<html><head><meta charset="utf-8">
<script src="/doc/qqpet_automation/qq-pet-macos/src/windows/js/ruffle/ruffle.js"></script>
<style>body { margin: 0; padding: 0; background: transparent; overflow: hidden; } #pet { width: 220px; height: 220px; }</style>
</head><body><div id="pet"></div>
<script>
window.addEventListener('DOMContentLoaded', () => {
    const r = window.RufflePlayer.newest();
    const p = r.createPlayer();
    p.style.width = '100%'; p.style.height = '100%';
    p.config = { autoplay: 'on', unmuteOverlay: 'hidden', letterbox: 'off', backgroundColor: null, wmode: 'transparent' };
    document.getElementById('pet').appendChild(p);
    p.load('/doc/qqpet_automation/qq-pet-macos/src/assets/Action/MM/Adult/peaceful/play/P3.swf');
});
</script></body></html>"""

    (WORKSPACE / "tools/test_render.html").write_text(html, encoding="utf-8")

    with sync_playwright() as p:
        browser = p.chromium.launch(channel="msedge", headless=True)
        page = browser.new_page()
        page.goto(f"http://127.0.0.1:{PORT}/tools/test_render.html")
        page.wait_for_selector("#pet ruffle-player", timeout=10000)
        time.sleep(0.5)

        test_out = WORKSPACE / "tools/test_work_frames"
        test_out.mkdir(parents=True, exist_ok=True)

        print("Capturing 24 frames of MM/Adult work (peaceful/play/P3.swf)...")
        for f in range(24):
            bs = page.locator("#pet").screenshot(omit_background=True)
            fp = test_out / f"f_{f:02d}.png"
            fp.write_bytes(bs)
            time.sleep(0.08)

        browser.close()

    server.shutdown()
    print("Captured 24 frames!")
    for f in sorted(list(test_out.glob("*.png"))):
        im = Image.open(f)
        print(f.name, im.size, im.getbbox())

if __name__ == "__main__":
    main()
