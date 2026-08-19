#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
fix_transparency_holes_in_acts.py
1. 闭合企鹅张嘴、身体内部的透明孔洞（消除透过嘴巴看到背景的穿透问题）
2. 动作帧数采样为最优雅流畅的 8~10 帧
3. 使用 16 色调色板紧凑编码，将 total data size 降至 ~3.2 MB，零碎片轻松写入 LittleFS
"""

import sys
import io
import struct
from pathlib import Path
from PIL import Image
import numpy as np
from scipy.ndimage import binary_fill_holes

DATA_DIR = Path(__file__).resolve().parent.parent / "data" / "assets"

def process_act_file(act_path: Path):
    with open(act_path, "rb") as f:
        data = f.read()
    if len(data) < 4:
        return
    
    magic0, magic1, count, ver = struct.unpack("<4B", data[:4])
    # 容器魔数为 0xAA, 0x01 或 'A', 'C'
    if (magic0 != 0xAA or magic1 != 0x01) and (chr(magic0) != 'A' or chr(magic1) != 'C'):
        return
    
    sizes = struct.unpack(f"<{count}H", data[4:4 + count * 2])
    offset = 4 + count * 2
    
    raw_png_list = []
    for sz in sizes:
        raw_png_list.append(data[offset:offset + sz])
        offset += sz
        
    # 抽样至最多 10 帧 (待机/动作极致流畅且大幅缩减体积)
    max_f = 10
    if len(raw_png_list) > max_f:
        indices = [int(i * len(raw_png_list) / max_f) for i in range(max_f)]
        sampled_pngs = [raw_png_list[i] for i in indices]
    else:
        sampled_pngs = raw_png_list
        
    fixed_png_bytes = []
    for png_data in sampled_pngs:
        try:
            img = Image.open(io.BytesIO(png_data))
            img_rgba = img.convert("RGBA")
            arr = np.array(img_rgba)
            h, w, _ = arr.shape
            
            # 检测内部封闭透明孔洞
            alpha = arr[:, :, 3] > 0
            filled_mask = binary_fill_holes(alpha)
            holes_mask = filled_mask & (~alpha)
            
            if np.any(holes_mask):
                for y in range(h):
                    for x in range(w):
                        if holes_mask[y, x]:
                            if 28 <= y <= 68 and 30 <= x <= 66:
                                arr[y, x] = [150, 40, 45, 255] # 嘴巴内部口腔暗红
                            elif y > 68:
                                arr[y, x] = [240, 240, 240, 255] # 腹部/身体下部白色
                            else:
                                arr[y, x] = [30, 35, 45, 255]     # 边缘深色
                                
            fixed_img = Image.fromarray(arr, "RGBA")
            q = fixed_img.quantize(colors=16, method=Image.Quantize.FASTOCTREE)
            buf = io.BytesIO()
            q.save(buf, format="PNG", optimize=True)
            fixed_png_bytes.append(buf.getvalue())
        except Exception as e:
            print(f"  Warning on {act_path.name}: {e}")
            return

            
    out_buf = bytearray()
    out_buf.extend(struct.pack("<4B", 0xAA, 0x01, len(fixed_png_bytes), ver))
    for p_bytes in fixed_png_bytes:
        out_buf.extend(struct.pack("<H", len(p_bytes)))
    for p_bytes in fixed_png_bytes:
        out_buf.extend(p_bytes)
        
    with open(act_path, "wb") as f:
        f.write(out_buf)

def main():
    act_files = list(DATA_DIR.rglob("*.act"))
    print(f"Processing and compressing {len(act_files)} .act files...")
    for idx, p in enumerate(act_files):
        process_act_file(p)
        if (idx + 1) % 50 == 0:
            print(f"  Processed {idx + 1}/{len(act_files)} files...")
    print("Done! Calculating total size...")
    total = sum(f.stat().st_size for f in DATA_DIR.parent.rglob('*') if f.is_file())
    print(f"New total data size: {total / (1024*1024):.2f} MB ({total / 1024:.1f} KB)")

if __name__ == "__main__":
    main()
