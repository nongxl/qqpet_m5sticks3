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

HTML_STAGE = """<!DOCTYPE html>
<html><head><meta charset="utf-8">
<script src="/doc/qqpet_automation/qq-pet-macos/src/windows/js/ruffle/ruffle.js"></script>
<style>
body { margin: 0; padding: 0; background: transparent; overflow: hidden; }
#stage { position: relative; width: 144px; height: 144px; }
#base { position: absolute; top: 0; left: 0; width: 144px; height: 144px; z-index: 1; }
#act { position: absolute; top: 0; left: 0; width: 144px; height: 144px; z-index: 2; }
</style>
</head><body>
<div id="stage">
  <div id="base"></div>
  <div id="act"></div>
</div>
<script>
window.addEventListener('DOMContentLoaded', () => {
    const r = window.RufflePlayer.newest();
    
    // 1. 底层加载 Stand.swf (完整身躯底座)
    const pBase = r.createPlayer();
    pBase.style.width = '100%'; pBase.style.height = '100%';
    pBase.config = { autoplay: 'on', unmuteOverlay: 'hidden', letterbox: 'off', backgroundColor: null, wmode: 'transparent' };
    document.getElementById('base').appendChild(pBase);
    pBase.load("/doc/qqpet_automation/qq-pet-macos/src/assets/Action/MM/Adult/peaceful/Stand.swf");
    
    // 2. 顶层加载 Eat1.swf (用餐动作与道具)
    const pAct = r.createPlayer();
    pAct.style.width = '100%'; pAct.style.height = '100%';
    pAct.config = { autoplay: 'on', unmuteOverlay: 'hidden', letterbox: 'off', backgroundColor: null, wmode: 'transparent' };
    document.getElementById('act').appendChild(pAct);
    pAct.load("/doc/qqpet_automation/qq-pet-macos/src/assets/Action/MM/Adult/Eat1.swf");
});
</script></body></html>"""

(WORKSPACE / "tools/test_stage.html").write_text(HTML_STAGE, encoding="utf-8")

with sync_playwright() as p:
    browser = p.chromium.launch(channel="msedge", headless=True)
    page = browser.new_page(viewport={"width": 200, "height": 200})
    page.goto(f"http://127.0.0.1:{PORT}/tools/test_stage.html")
    page.wait_for_selector("#act ruffle-player", timeout=8000)
    time.sleep(0.5)
    
    for f in range(6):
        shot = page.locator("#stage").screenshot(omit_background=True)
        im = Image.open(io.BytesIO(shot)).convert("RGBA")
        im96 = im.resize((96, 96), Image.Resampling.LANCZOS)
        im96.save(f"C:/Users/X1C/.gemini/antigravity-ide/brain/218e95fe-47ce-452c-b728-571b473288de/scratch/test_dual_eat_f{f}.png")
        time.sleep(0.08)
    browser.close()

server.shutdown()
print("Extracted dual-layer 144x144 stage test frames!")
