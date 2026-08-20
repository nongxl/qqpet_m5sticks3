#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
apply_custom_action_mappings.py
极速并发多 Worker 提取与固件烧录：
1. 增量智能缓存机制（无变动动作 0 毫秒跳过）；
2. 6 线程/并发 Worker 高速并行渲染提取 Flash 长动画；
3. 一键打包 6.06MB LittleFS 镜像并自动烧录至 ESP32-S3 (COM6, 0x1E0000)！
"""

import os
import sys
import io
import json
import time
import struct
import shutil
import asyncio
import http.server
import socketserver
import threading
import subprocess
from pathlib import Path

# 强制使用 UTF-8 输出，杜绝 Windows 控制台 GBK 乱码与编码报错
if hasattr(sys.stdout, 'reconfigure'):
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')
if hasattr(sys.stderr, 'reconfigure'):
    sys.stderr.reconfigure(encoding='utf-8', errors='replace')

from PIL import Image, ImageDraw, ImageOps
import numpy as np
from scipy.ndimage import binary_fill_holes
from playwright.async_api import async_playwright

WORKSPACE = Path(__file__).resolve().parent.parent
DATA_DIR = WORKSPACE / "data/assets"
ACTION_ROOT = WORKSPACE / "doc/qqpet_automation/qq-pet-macos/src/assets/Action"
MAPPING_FILE = WORKSPACE / "tools/action_mappings.json"
CACHE_FILE = WORKSPACE / "tools/.build_cache.json"
PORT = 8998
NUM_WORKERS = 6

HTML_DYNAMIC = """<!DOCTYPE html>
<html>
<head>
  <script src="/doc/qqpet_automation/qq-pet-macos/src/windows/js/ruffle/ruffle.js"></script>
  <style>
    body { margin: 0; background: transparent; overflow: hidden; }
    #pet { width: 260px; height: 260px; }
  </style>
</head>
<body>
  <div id="pet"></div>
  <script>
    window.loadSwf = function(url) {
      const r = window.RufflePlayer.newest();
      const p = r.createPlayer();
      p.style.width = '100%';
      p.style.height = '100%';
      p.config = {
        autoplay: 'on',
        unmuteOverlay: 'hidden',
        letterbox: 'off',
        backgroundColor: '#000000',
        wmode: 'transparent'
      };
      const pet = document.getElementById('pet');
      pet.innerHTML = '';
      pet.appendChild(p);
      return p.load(url);
    };
  </script>
</body>
</html>"""

class QuietHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=str(WORKSPACE), **kwargs)
    def log_message(self, format, *args):
        pass

def pack_png_frames_to_act(png_bytes_list, out_file):
    out_file.parent.mkdir(parents=True, exist_ok=True)
    count = len(png_bytes_list)
    buf = bytearray()
    buf.extend(struct.pack("<4B", 0xAA, 0x01, count, 0))
    for p in png_bytes_list:
        buf.extend(struct.pack("<H", len(p)))
    for p in png_bytes_list:
        buf.extend(p)
    with open(out_file, "wb") as f:
        f.write(buf)

def process_frame_image(screenshot_bytes, stage):
    img = Image.open(io.BytesIO(screenshot_bytes)).convert("RGBA")
    arr_test = np.array(img)
    red_err = (arr_test[:, :, 0] > 170) & (arr_test[:, :, 1] < 70) & (arr_test[:, :, 2] < 70)
    if np.sum(red_err) > 300:
        return None

    target_size = 72 if stage == "Egg" else (82 if stage == "Kid" else 90)
    bbox = img.getbbox()
    if bbox:
        cropped = img.crop(bbox)
        w, h = cropped.size
        scale = min(target_size / w, target_size / h)
        nw = max(1, int(w * scale))
        nh = max(1, int(h * scale))

        resized = cropped.resize((nw, nh), Image.Resampling.LANCZOS)
        final_img = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
        paste_x = (96 - nw) // 2
        paste_y = 96 - nh - 2
        final_img.paste(resized, (paste_x, paste_y), mask=resized)

        # 闭合空洞填充
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
        return buf.getvalue()
    else:
        final_img = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
        buf = io.BytesIO()
        final_img.save(buf, format="PNG", optimize=True)
        return buf.getvalue()

async def worker_task(worker_id, queue, browser, total_tasks, progress_state, lock):
    page = await browser.new_page(viewport={"width": 260, "height": 260})
    await page.goto(f"http://127.0.0.1:{PORT}/tools/render_dynamic.html")
    await asyncio.sleep(0.5)

    while not queue.empty():
        try:
            item, out_file, swf_file, cache_key, num_f, fps_val = queue.get_nowait()
        except asyncio.QueueEmpty:
            break

        gender = item.get("gender")
        stage = item.get("stage")
        rel_path = item.get("rel_path")
        act_id = item.get("action_id")
        swf_full = f"{gender}/{stage}/{rel_path}"
        act_url = f"/doc/qqpet_automation/qq-pet-macos/src/assets/Action/{swf_full}".replace("\\", "/")
        frame_delay = 1.0 / max(1, fps_val)

        async with lock:
            progress_state["current"] += 1
            idx = progress_state["current"]
            pct = int(idx * 80 / max(1, total_tasks))
            print(f"PROGRESS: {idx}/{total_tasks} | {pct}% | [{idx}/{total_tasks} W{worker_id}] 正在提取: {gender}/{stage}/{act_id} ({rel_path})", flush=True)

        try:
            await page.evaluate(f'window.loadSwf("{act_url}")')
            await asyncio.sleep(0.35)

            png_bytes_list = []
            failed = False

            for _ in range(num_f):
                shot_bytes = await page.locator("#pet").screenshot(omit_background=True)
                p_bytes = await asyncio.to_thread(process_frame_image, shot_bytes, stage)
                if p_bytes is None:
                    failed = True
                    break
                png_bytes_list.append(p_bytes)
                await asyncio.sleep(frame_delay)

            if not failed and png_bytes_list:
                await asyncio.to_thread(pack_png_frames_to_act, png_bytes_list, out_file)
                async with lock:
                    progress_state["cache"][cache_key] = {
                        "swf_rel": rel_path,
                        "num_frames": num_f,
                        "fps": fps_val,
                        "swf_mtime": swf_file.stat().st_mtime if swf_file.exists() else 0,
                        "stage": stage
                    }
                    print(f"  [OK] 提取完成: {out_file.name} ({out_file.stat().st_size} 字节)", flush=True)
            else:
                print(f"  [SKIP] 无法提取或出现异常: {rel_path}", flush=True)

        except Exception as e:
            print(f"  [ERR] 提取异常 {rel_path}: {e}", flush=True)

        queue.task_done()

    await page.close()

async def run_async_build():
    if not MAPPING_FILE.exists():
        print(f"ERROR: {MAPPING_FILE} not found!")
        return False

    with open(MAPPING_FILE, "r", encoding="utf-8") as f:
        mappings = json.load(f)

    build_cache = {}
    if CACHE_FILE.exists():
        try:
            with open(CACHE_FILE, "r", encoding="utf-8") as f:
                build_cache = json.load(f)
        except Exception:
            pass

    enabled_items = [item for item in mappings.values() if item.get("enabled", True) and item.get("action_id")]
    total_to_process = len(enabled_items)

    tasks_to_extract = []
    cached_count = 0

    for item in enabled_items:
        gender = item.get("gender")
        stage = item.get("stage")
        rel_path = item.get("rel_path")
        act_id = item.get("action_id")
        num_f = int(item.get("num_frames", 28))
        fps_val = int(item.get("fps", 12))
        
        out_file = DATA_DIR / gender / stage / f"{act_id}.act"
        swf_file = ACTION_ROOT / gender / stage / rel_path
        cache_key = f"{gender}/{stage}/{act_id}.act"
        
        needs_build = True
        if out_file.exists() and out_file.stat().st_size > 500 and swf_file.exists():
            c_info = build_cache.get(cache_key, {})
            if (c_info.get("swf_rel") == rel_path and
                c_info.get("num_frames") == num_f and
                c_info.get("fps") == fps_val and
                c_info.get("swf_mtime") == swf_file.stat().st_mtime and
                c_info.get("stage") == stage):
                needs_build = False

        if needs_build:
            tasks_to_extract.append((item, out_file, swf_file, cache_key, num_f, fps_val))
        else:
            cached_count += 1

    print(f"PROGRESS: 0/{total_to_process} | 0% | 总计 {total_to_process} 个动作 [缓存命中 {cached_count} 个, 待提取 {len(tasks_to_extract)} 个 (启用 {NUM_WORKERS} 并发加速)]", flush=True)

    if tasks_to_extract:
        (WORKSPACE / "tools/render_dynamic.html").write_text(HTML_DYNAMIC, encoding="utf-8")
        server = socketserver.TCPServer(("", PORT), QuietHandler)
        threading.Thread(target=server.serve_forever, daemon=True).start()

        queue = asyncio.Queue()
        for t in tasks_to_extract:
            queue.put_nowait(t)

        progress_state = {
            "current": cached_count,
            "cache": build_cache
        }
        lock = asyncio.Lock()

        async with async_playwright() as p:
            browser = await p.chromium.launch(channel="msedge", headless=True)
            workers = [
                asyncio.create_task(worker_task(w_id + 1, queue, browser, total_to_process, progress_state, lock))
                for w_id in range(min(NUM_WORKERS, len(tasks_to_extract)))
            ]
            await asyncio.gather(*workers)
            await browser.close()

        server.shutdown()

        with open(CACHE_FILE, "w", encoding="utf-8") as f:
            json.dump(build_cache, f, ensure_ascii=False, indent=2)
    else:
        print("PROGRESS: 80% | 所有动作均处于最新状态，无需重新渲染！", flush=True)

    # 打包 LittleFS 镜像
    mklittlefs_exe = Path(os.environ['USERPROFILE']) / '.platformio/packages/tool-mklittlefs/mklittlefs.exe'
    out_bin = WORKSPACE / ".pio/build/m5stack-sticks3/littlefs.bin"
    out_bin.parent.mkdir(parents=True, exist_ok=True)
    
    print("\nPROGRESS: 85% | 正在打包 6.06MB LittleFS 文件系统镜像 (mklittlefs)...", flush=True)
    cmd = [str(mklittlefs_exe), '-c', 'data', '-s', '0x610000', '-b', '4096', '-p', '256', str(out_bin)]
    res_pack = subprocess.run(cmd, capture_output=True, text=True, cwd=str(WORKSPACE))
    if res_pack.stderr:
        print(res_pack.stderr, flush=True)
    print(f"  [OK] LittleFS 镜像生成完成: {out_bin.stat().st_size / (1024*1024):.2f} MB", flush=True)
    
    # 烧录到 ESP32
    print("\nPROGRESS: 90% | 正在连接串口 COM6 烧录 LittleFS 分区 (0x1E0000)...", flush=True)
    esptool_py = Path(os.environ['USERPROFILE']) / '.platformio/packages/tool-esptoolpy/esptool.py'
    python_exe = Path(os.environ['USERPROFILE']) / '.platformio/penv/Scripts/python.exe'
    
    cmd_flash = [
        str(python_exe), str(esptool_py),
        '--chip', 'esp32s3', '-p', 'COM6', '-b', '460800',
        'write_flash', '0x1E0000', str(out_bin)
    ]
    res_f = subprocess.run(cmd_flash, capture_output=True, text=True)
    print(res_f.stdout, flush=True)
    if res_f.returncode == 0:
        print("\nPROGRESS: 100% | SUCCESS | 恭喜！全部长动画已成功编译并烧录至 M5StickS3！设备已自动重启生效！", flush=True)
        return True
    else:
        print(f"\nPROGRESS: ERROR | 烧录失败：{res_f.stderr}", flush=True)
        return False

def main():
    ok = asyncio.run(run_async_build())
    if not ok:
        sys.exit(1)

if __name__ == "__main__":
    main()
