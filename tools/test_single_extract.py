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

HTML_SINGLE = """<!DOCTYPE html>
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


def test_extract(swf_rel, out_prefix):
    html = HTML_SINGLE.format(swf_url=f"/doc/qqpet_automation/qq-pet-macos/src/assets/Action/{swf_rel}")
    (WORKSPACE / "tools/test_single.html").write_text(html, encoding="utf-8")
    with sync_playwright() as p:
        browser = p.chromium.launch(channel="msedge", headless=True)
        page = browser.new_page(viewport={"width": 260, "height": 260})
        page.goto(f"http://127.0.0.1:{PORT}/tools/test_single.html")
        page.wait_for_selector("#pet ruffle-player", timeout=8000)
        time.sleep(0.4)
        for f in range(6):
            shot = page.locator("#pet").screenshot(omit_background=True)
            im = Image.open(io.BytesIO(shot)).convert("RGBA")
            bbox = im.getbbox()
            if bbox:
                cropped = im.crop(bbox)
                w, h = cropped.size
                scale = min(88 / w, 88 / h)
                nw, nh = max(1, int(w * scale)), max(1, int(h * scale))
                rs = cropped.resize((nw, nh), Image.Resampling.LANCZOS)
                final_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
                final_im.paste(rs, ((96 - nw) // 2, 96 - nh - 2), rs)
                
                # 仅对闭合空洞（如嘴巴内侧镂空）填补实色
                arr = np.array(final_im)
                has_px = arr[:, :, 3] > 20
                filled = binary_fill_holes(has_px)
                holes = filled & (~has_px)
                # 填补嘴腔/身体内部缝隙
                for y in range(96):
                    for x in range(96):
                        if holes[y, x]:
                            if 28 <= y <= 68 and 30 <= x <= 66:
                                arr[y, x] = [150, 40, 45, 255] # 口腔暗红
                            else:
                                arr[y, x] = [240, 240, 240, 255] # 身体/腹部白色
                final_clean = Image.fromarray(arr, "RGBA")
                q = final_clean.quantize(colors=64, method=Image.Quantize.FASTOCTREE)
                q.save(f"C:/Users/X1C/.gemini/antigravity-ide/brain/218e95fe-47ce-452c-b728-571b473288de/scratch/{out_prefix}_f{f}.png")
            time.sleep(0.08)
        browser.close()

if __name__ == "__main__":
    test_extract("MM/Adult/Eat1.swf", "single_eat")
    test_extract("MM/Adult/upset/play/P1.swf", "single_stomp")
    test_extract("MM/Adult/peaceful/play/P6.swf", "single_study")
    server.shutdown()
    print("Single layer extracted cleanly!")
