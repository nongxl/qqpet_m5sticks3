import http.server
import socketserver
import threading
import time
import io
from pathlib import Path
from PIL import Image
import numpy as np
from playwright.sync_api import sync_playwright

WORKSPACE = Path("d:/workspace/QQpet_StickS3")
PORT = 8999

class QuietHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=str(WORKSPACE), **kwargs)
    def log_message(self, format, *args):
        pass

server = socketserver.TCPServer(("", PORT), QuietHandler)
threading.Thread(target=server.serve_forever, daemon=True).start()

HTML_TEST = """<!DOCTYPE html>
<html><head><meta charset="utf-8">
<script src="/doc/qqpet_automation/qq-pet-macos/src/windows/js/ruffle/ruffle.js"></script>
<style>body { margin: 0; padding: 0; background: #00FF00; overflow: hidden; } #pet { width: 220px; height: 220px; }</style>
</head><body><div id="pet"></div>
<script>
window.addEventListener('DOMContentLoaded', () => {
    const r = window.RufflePlayer.newest();
    const p = r.createPlayer();
    p.style.width = '100%'; p.style.height = '100%';
    p.config = { autoplay: 'on', unmuteOverlay: 'hidden', letterbox: 'off', backgroundColor: '#00FF00', wmode: 'opaque' };
    document.getElementById('pet').appendChild(p);
    p.load("/doc/qqpet_automation/qq-pet-macos/src/assets/Action/MM/Adult/Eat1.swf");
});
</script></body></html>"""

(WORKSPACE / "tools/test_green.html").write_text(HTML_TEST, encoding="utf-8")

with sync_playwright() as p:
    browser = p.chromium.launch(channel="msedge", headless=True)
    page = browser.new_page(viewport={"width": 260, "height": 260})
    page.goto(f"http://127.0.0.1:{PORT}/tools/test_green.html")
    page.wait_for_selector("#pet ruffle-player", timeout=8000)
    time.sleep(0.5)
    
    for f in range(6):
        shot = page.locator("#pet").screenshot()
        im = Image.open(io.BytesIO(shot))
        im.save(f"C:/Users/X1C/.gemini/antigravity-ide/brain/218e95fe-47ce-452c-b728-571b473288de/scratch/test_green_eat_f{f}.png")
        time.sleep(0.08)
    browser.close()

server.shutdown()
print("Extracted green background test frames!")
