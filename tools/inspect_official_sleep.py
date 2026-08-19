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

    out_dir = WORKSPACE / "tools/inspect_official_sleep"
    out_dir.mkdir(parents=True, exist_ok=True)

    with sync_playwright() as p:
        browser = p.chromium.launch(channel="msedge", headless=True)
        page = browser.new_page()

        for swf in ["prostrate/Stand.swf", "prostrate/play/P1.swf", "prostrate/play/P2.swf", "prostrate/play/P5.swf"]:
            for gender in ["GG", "MM"]:
                swf_rel = f"/doc/qqpet_automation/qq-pet-macos/src/assets/Action/{gender}/Adult/{swf}"
                (WORKSPACE / "tools/temp_off_sleep.html").write_text(html_tpl.format(swf_url=swf_rel), encoding="utf-8")
                page.goto(f"http://127.0.0.1:{PORT}/tools/temp_off_sleep.html")
                page.wait_for_selector("#pet ruffle-player", timeout=5000)
                time.sleep(0.4)
                time.sleep(0.3)
                b = page.locator("#pet").screenshot(omit_background=True)
                name = f"{gender}_{swf.replace('/', '_').replace('.swf', '')}.png"
                (out_dir / name).write_bytes(b)
                print(f"Captured official {name}")

        browser.close()

    server.shutdown()

if __name__ == "__main__":
    main()
