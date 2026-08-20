import http.server
import socketserver
import threading
import time
import io
from pathlib import Path
from PIL import Image
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

HTML_CODE = """<!DOCTYPE html><html><head><script src="/doc/qqpet_automation/qq-pet-macos/src/windows/js/ruffle/ruffle.js"></script>
<style>body { margin: 0; background: #000; overflow: hidden; } #pet { width: 220px; height: 220px; }</style>
</head><body><div id="pet"></div>
<script>
window.loadSwf = function(url) {
    const r = window.RufflePlayer.newest();
    const p = r.createPlayer();
    p.style.width = '100%'; p.style.height = '100%';
    p.config = { autoplay: 'on', unmuteOverlay: 'hidden', letterbox: 'off', backgroundColor: '#000000', wmode: 'opaque' };
    document.getElementById('pet').innerHTML = '';
    document.getElementById('pet').appendChild(p);
    p.load(url);
};
</script></body></html>"""

(WORKSPACE / "tools/render_inspect_pros.html").write_text(HTML_CODE, encoding="utf-8")

out_dir = WORKSPACE / "tools/inspect_pros"
out_dir.mkdir(parents=True, exist_ok=True)

with sync_playwright() as p:
    browser = p.chromium.launch(channel="msedge", headless=True)
    page = browser.new_page(viewport={"width": 260, "height": 260})
    page.goto(f"http://127.0.0.1:{PORT}/tools/render_inspect_pros.html")

    for i in range(1, 47):
        swf_rel = f"/doc/qqpet_automation/qq-pet-macos/src/assets/Action/MM/Adult/prostrate/play/P{i}.swf"
        page.evaluate(f'window.loadSwf("{swf_rel}")')
        time.sleep(0.4)
        shot = page.locator("#pet").screenshot()
        im = Image.open(io.BytesIO(shot))
        im.save(out_dir / f"P{i}.png")
        print(f"P{i}.png captured")

    browser.close()

server.shutdown()
print("Done capturing all prostrate play actions!")
