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

server = socketserver.TCPServer(('', 8991), H)
threading.Thread(target=server.serve_forever, daemon=True).start()

HTML = """<!DOCTYPE html>
<html>
<head>
  <script src="/doc/qqpet_automation/qq-pet-macos/src/windows/js/ruffle/ruffle.js"></script>
</head>
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
    (scratch / "test_kid.html").write_text(HTML, encoding="utf-8")
    async with async_playwright() as p:
        b = await p.chromium.launch(channel="msedge", headless=True)
        page = await b.new_page(viewport={"width": 200, "height": 200})
        await page.goto("http://127.0.0.1:8991/scratch/test_kid.html")
        
        for name, url in [
            ("GG_Kid_Stand", "/doc/qqpet_automation/qq-pet-macos/src/assets/Action/GG/Kid/Stand.swf"),
            ("MM_Kid_Stand", "/doc/qqpet_automation/qq-pet-macos/src/assets/Action/MM/Kid/Stand.swf"),
            ("GG_Egg_Stand", "/doc/qqpet_automation/qq-pet-macos/src/assets/Action/GG/Egg/Stand.swf"),
            ("MM_Egg_Stand", "/doc/qqpet_automation/qq-pet-macos/src/assets/Action/MM/Egg/Stand.swf"),
        ]:
            await page.evaluate(f'window.load("{url}")')
            await asyncio.sleep(0.6)
            shot = await page.locator("#pet").screenshot(omit_background=True)
            img = Image.open(io.BytesIO(shot))
            img.save(str(scratch / f"{name}.png"))
            print(f"Saved scratch/{name}.png")
        await b.close()
    server.shutdown()

if __name__ == "__main__":
    asyncio.run(main())
