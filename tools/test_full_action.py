import http.server
import socketserver
import threading
import time
import io
from pathlib import Path
from PIL import Image
import numpy as np
from scipy.ndimage import binary_fill_holes
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

HTML_TEMPLATE = """<!DOCTYPE html>
<html><head><meta charset="utf-8">
<script src="/doc/qqpet_automation/qq-pet-macos/src/windows/js/ruffle/ruffle.js"></script>
<style>
body {{ margin: 0; padding: 0; background: transparent; overflow: hidden; }}
#pet {{ width: 220px; height: 220px; }}
</style>
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

def test_full_action(swf_rel, out_name, num_frames=24, fps=12):
    act_url = f"/doc/qqpet_automation/qq-pet-macos/src/assets/Action/{swf_rel}".replace("\\", "/")
    html_code = HTML_TEMPLATE.format(swf_url=act_url)
    (WORKSPACE / "tools/render_test_full.html").write_text(html_code, encoding="utf-8")

    interval = 1.0 / fps

    with sync_playwright() as p:
        browser = p.chromium.launch(channel="msedge", headless=True)
        page = browser.new_page(viewport={"width": 260, "height": 260})
        page.goto(f"http://127.0.0.1:{PORT}/tools/render_test_full.html")
        page.wait_for_selector("#pet ruffle-player", timeout=8000)
        time.sleep(0.35)

        total_bytes = 0
        for f in range(num_frames):
            screenshot_bytes = page.locator("#pet").screenshot(omit_background=True)
            img = Image.open(io.BytesIO(screenshot_bytes)).convert("RGBA")
            
            bbox = img.getbbox()
            if bbox:
                cropped = img.crop(bbox)
                w, h = cropped.size
                scale = min(90 / w, 90 / h)
                nw, nh = max(1, int(w * scale)), max(1, int(h * scale))
                rs = cropped.resize((nw, nh), Image.Resampling.LANCZOS)
                final_img = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
                final_img.paste(rs, ((96 - nw) // 2, 96 - nh - 2), rs)

                arr = np.array(final_img)
                has_px = arr[:, :, 3] > 20
                filled = binary_fill_holes(has_px)
                holes = filled & (~has_px)
                if np.any(holes):
                    for y in range(96):
                        for x in range(96):
                            if holes[y, x]:
                                if 28 <= y <= 68 and 30 <= x <= 66:
                                    arr[y, x] = [150, 40, 45, 255]
                                elif y > 68:
                                    arr[y, x] = [240, 240, 240, 255]
                                else:
                                    arr[y, x] = [30, 35, 45, 255]
                final_clean = Image.fromarray(arr, "RGBA")
                q = final_clean.quantize(colors=64, method=Image.Quantize.FASTOCTREE)
                buf = io.BytesIO()
                q.save(buf, format="PNG", optimize=True)
                total_bytes += len(buf.getvalue())
            time.sleep(interval)

        browser.close()
        print(f"Action '{out_name}' ({num_frames} frames, {num_frames/fps:.1f}s) total size: {total_bytes / 1024:.1f} KB (Avg: {total_bytes/num_frames:.0f} B/frame)")

if __name__ == "__main__":
    test_full_action("MM/Adult/Eat1.swf", "eat_full", num_frames=24, fps=10)
    test_full_action("MM/Adult/peaceful/play/P8.swf", "work_full", num_frames=28, fps=12)
    test_full_action("MM/Adult/upset/play/P1.swf", "stomp_full", num_frames=22, fps=10)
    server.shutdown()
