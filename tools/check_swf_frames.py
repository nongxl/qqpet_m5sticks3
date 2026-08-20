import http.server
import socketserver
import threading
import time
from pathlib import Path
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

HTML_CODE = """<!DOCTYPE html><html><head><script src="/doc/qqpet_automation/qq-pet-macos/src/windows/js/ruffle/ruffle.js"></script></head>
<body><div id="pet"></div>
<script>
window.checkFrames = async function(swfUrl) {
    const r = window.RufflePlayer.newest();
    const p = r.createPlayer();
    document.getElementById('pet').innerHTML = '';
    document.getElementById('pet').appendChild(p);
    await p.load(swfUrl);
    await new Promise(res => setTimeout(res, 400));
    return {
        total: p.totalFrames || (p.TotalFrames ? p.TotalFrames() : 0),
        fps: p.metadata ? p.metadata.frameRate : 24
    };
};
</script></body></html>"""

(WORKSPACE / "tools/test_fc.html").write_text(HTML_CODE, encoding="utf-8")

test_swfs = [
    "MM/Adult/Eat1.swf",
    "MM/Adult/Eat2.swf",
    "MM/Adult/Clean1.swf",
    "MM/Adult/peaceful/Stand.swf",
    "MM/Adult/peaceful/play/P8.swf",
    "MM/Adult/peaceful/play/P2.swf",
    "MM/Adult/peaceful/play/P6.swf",
    "MM/Adult/upset/play/P1.swf",
    "MM/Adult/happy/play/P1.swf",
]

with sync_playwright() as p:
    browser = p.chromium.launch(channel="msedge", headless=True)
    page = browser.new_page()
    page.goto(f"http://127.0.0.1:{PORT}/tools/test_fc.html")
    for swf in test_swfs:
        url = f"/doc/qqpet_automation/qq-pet-macos/src/assets/Action/{swf}"
        info = page.evaluate(f'window.checkFrames("{url}")')
        print(f"{swf:35s} -> totalFrames={info.get('total')} fps={info.get('fps')}")
    browser.close()

server.shutdown()
