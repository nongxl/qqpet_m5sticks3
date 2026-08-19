import http.server
import socketserver
import threading
import time
from pathlib import Path
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

    out_dir = WORKSPACE / "tools/inspect_kid_sleep"
    out_dir.mkdir(parents=True, exist_ok=True)

    with sync_playwright() as p:
        browser = p.chromium.launch(channel="msedge", headless=True)
        page = browser.new_page()

        for i in [5, 9, 11, 12, 13, 15, 16, 21, 22, 25, 26, 28]:
            swf_rel = f"/doc/qqpet_automation/qq-pet-macos/src/assets/Action/MM/Kid/play/P{i}.swf"
            (WORKSPACE / "tools/temp_ks.html").write_text(html_tpl.format(swf_url=swf_rel), encoding="utf-8")
            page.goto(f"http://127.0.0.1:{PORT}/tools/temp_ks.html")
            page.wait_for_selector("#pet ruffle-player", timeout=5000)
            time.sleep(0.4)
            time.sleep(0.4)
            b = page.locator("#pet").screenshot(omit_background=True)
            (out_dir / f"kid_P{i}.png").write_bytes(b)
            print(f"Kid P{i} captured")

        browser.close()

    server.shutdown()

if __name__ == "__main__":
    main()
