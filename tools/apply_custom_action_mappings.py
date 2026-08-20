#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
apply_custom_action_mappings.py
根据用户在 Action Studio Web 页面中配置的 action_mappings.json：
1. 提取所有启用的 SWF 动画，生成纯单层实心 .act 文件；
2. 打包生成 LittleFS 镜像；
3. 全量烧录至 ESP32-S3 设备 (COM6, 0x1E0000)！
"""

import os
import sys
import io
import json
import time
import struct
import shutil
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
from scipy.ndimage import binary_fill_holes
from playwright.sync_api import sync_playwright


WORKSPACE = Path(__file__).resolve().parent.parent
DATA_DIR = WORKSPACE / "data/assets"
ACTION_ROOT = WORKSPACE / "doc/qqpet_automation/qq-pet-macos/src/assets/Action"
MAPPING_FILE = WORKSPACE / "tools/action_mappings.json"
PORT = 8998

class QuietHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=str(WORKSPACE), **kwargs)
    def log_message(self, format, *args):
        pass

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

def pack_png_frames_to_act(png_bytes_list, out_file: Path):
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

def extract_single_swf(page, swf_rel, out_act_file, stage, num_frames=6, frame_delay=0.08):
    act_url = f"/doc/qqpet_automation/qq-pet-macos/src/assets/Action/{swf_rel}".replace("\\", "/")
    html_code = HTML_TEMPLATE.format(swf_url=act_url)
    (WORKSPACE / "tools/render_apply.html").write_text(html_code, encoding="utf-8")

    page.goto(f"http://127.0.0.1:{PORT}/tools/render_apply.html")
    try:
        page.wait_for_selector("#pet ruffle-player", timeout=8000)
    except Exception:
        return False
    time.sleep(0.35)

    target_size = 72 if stage == "Egg" else (82 if stage == "Kid" else 90)
    png_bytes_list = []

    for f in range(num_frames):
        screenshot_bytes = page.locator("#pet").screenshot(omit_background=True)
        img = Image.open(io.BytesIO(screenshot_bytes)).convert("RGBA")
        
        arr_test = np.array(img)
        red_err = (arr_test[:, :, 0] > 170) & (arr_test[:, :, 1] < 70) & (arr_test[:, :, 2] < 70)
        if np.sum(red_err) > 300:
            return False

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
            png_bytes_list.append(buf.getvalue())
        else:
            final_img = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
            buf = io.BytesIO()
            final_img.save(buf, format="PNG", optimize=True)
            png_bytes_list.append(buf.getvalue())

        time.sleep(frame_delay)

    pack_png_frames_to_act(png_bytes_list, out_act_file)
    return True

def main():
    if not MAPPING_FILE.exists():
        print("No action_mappings.json found!")
        return

    with open(MAPPING_FILE, "r", encoding="utf-8") as f:
        mappings = json.load(f)

    CACHE_FILE = WORKSPACE / "tools/.build_cache.json"
    build_cache = {}
    if CACHE_FILE.exists():
        try:
            with open(CACHE_FILE, "r", encoding="utf-8") as f:
                build_cache = json.load(f)
        except Exception:
            pass

    enabled_items = [item for item in mappings.values() if item.get("enabled", True) and item.get("action_id")]
    total_to_process = len(enabled_items)
    
    # 分析哪些需要重新提取，哪些命中缓存
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

    print(f"PROGRESS: 0/{total_to_process} | 0% | 总计 {total_to_process} 个动作 [缓存命中 {cached_count} 个, 待提取 {len(tasks_to_extract)} 个]", flush=True)


    if tasks_to_extract:
        server = socketserver.TCPServer(("", PORT), QuietHandler)
        threading.Thread(target=server.serve_forever, daemon=True).start()

        processed_count = cached_count
        with sync_playwright() as p:
            browser = p.chromium.launch(channel="msedge", headless=True)
            page = browser.new_page(viewport={"width": 260, "height": 260})

            for item, out_file, swf_file, cache_key, num_f, fps_val in tasks_to_extract:
                gender = item.get("gender")
                stage = item.get("stage")
                rel_path = item.get("rel_path")
                act_id = item.get("action_id")
                swf_full = f"{gender}/{stage}/{rel_path}"
                frame_delay = 1.0 / max(1, fps_val)

                processed_count += 1
                pct = int(processed_count * 80 / max(1, total_to_process))
                print(f"PROGRESS: {processed_count}/{total_to_process} | {pct}% | [{processed_count}/{total_to_process}] 正在提取: {gender}/{stage}/{act_id} ({rel_path})", flush=True)

                ok = extract_single_swf(page, swf_full, out_file, stage, num_frames=num_f, frame_delay=frame_delay)
                if ok:
                    print(f"  [OK] 生成成功: {out_file.name} ({out_file.stat().st_size} 字节)", flush=True)
                    build_cache[cache_key] = {
                        "swf_rel": rel_path,
                        "num_frames": num_f,
                        "fps": fps_val,
                        "swf_mtime": swf_file.stat().st_mtime if swf_file.exists() else 0,
                        "stage": stage
                    }
                else:
                    print(f"  [SKIP] 无法提取: {rel_path}", flush=True)

            browser.close()
        server.shutdown()

        # 保存更新后的构建缓存
        with open(CACHE_FILE, "w", encoding="utf-8") as f:
            json.dump(build_cache, f, ensure_ascii=False, indent=2)
    else:
        print("PROGRESS: 80% | 所有动作均已处于最新状态，无需重新提取渲染！", flush=True)


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
    else:
        print(f"\nPROGRESS: ERROR | 烧录失败：{res_f.stderr}", flush=True)
        sys.exit(1)

    print(res_f.stdout)
    if res_f.returncode == 0:
        print("\nSUCCESSFULLY FLASHED TO M5STICKS3!")

if __name__ == "__main__":
    main()
