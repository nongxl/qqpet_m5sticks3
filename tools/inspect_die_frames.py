import os
import io
import asyncio
from pathlib import Path
from PIL import Image
from playwright.async_api import async_playwright
import http.server, socketserver, threading

WORKSPACE = Path(__file__).resolve().parent.parent

class H(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *a, **k): super().__init__(*a, directory=str(WORKSPACE), **k)
    def log_message(self, *a): pass

server = socketserver.TCPServer(('', 8993), H)
threading.Thread(target=server.serve_forever, daemon=True).start()

HTML = """<!DOCTYPE html>
<html>
<head><script src="/doc/qqpet_automation/qq-pet-macos/src/windows/js/ruffle/ruffle.js"></script></head>
<body style="margin:0; background:transparent;">
  <div id="pet" style="width:200px; height:200px;"></div>
  <script>
    window.load = function(u) {
      const r = window.RufflePlayer.newest();
      const p = r.createPlayer();
      p.style.width = '100%'; p.style.height = '100%';
      p.config = { autoplay: 'on', letterbox: 'off', wmode: 'transparent' };
      const el = document.getElementById('pet');
      el.innerHTML = ''; el.appendChild(p);
      return p.load(u);
    };
  </script>
</body>
</html>"""

async def main():
    scratch = WORKSPACE / "scratch"
    scratch.mkdir(exist_ok=True)
    (scratch / "test_die.html").write_text(HTML, encoding="utf-8")
    async with async_playwright() as p:
        b = await p.chromium.launch(channel="msedge", headless=True)
        page = await b.new_page(viewport={"width": 200, "height": 200})
        await page.goto("http://127.0.0.1:8993/scratch/test_die.html")
        
        for g in ["MM", "GG"]:
            for s in ["Adult", "Kid"]:
                for swf in ["Die.swf", "Dying.swf", "Bury.swf"]:
                    url = f"/doc/qqpet_automation/qq-pet-macos/src/assets/Action/{g}/{s}/{swf}"
                    await page.evaluate(f'window.load("{url}")')
                    await asyncio.sleep(0.8)
                    shot = await page.locator("#pet").screenshot(omit_background=True)
                    img = Image.open(io.BytesIO(shot))
                    img.save(str(scratch / f"{g}_{s}_{swf}.png"))
                    print(f"Saved scratch/{g}_{s}_{swf}.png")
        await b.close()
    server.shutdown()

if __name__ == "__main__":
    asyncio.run(main())
