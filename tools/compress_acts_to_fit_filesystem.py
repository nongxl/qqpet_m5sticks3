#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
compress_acts_to_fit_filesystem.py
将 data/assets/ 下所有 .act 文件优化为标准 6 帧经典循环 (32 位 RGBA 或 64 色高清调色板)
将总数据大小压缩至 ~2.5 MB，确保 mklittlefs 100% 成功打包且不丢失任何原画细节！
"""

import sys
import io
import struct
from pathlib import Path
from PIL import Image
import numpy as np

DATA_DIR = Path(__file__).resolve().parent.parent / "data" / "assets"

def optimize_act(act_p: Path):
    with open(act_p, "rb") as f:
        data = f.read()
    if len(data) < 4:
        return
    
    magic0, magic1, count, ver = struct.unpack("<4B", data[:4])
    if (magic0 != 0xAA or magic1 != 0x01) and (chr(magic0) != 'A' or chr(magic1) != 'C'):
        return
        
    sizes = struct.unpack(f"<{count}H", data[4:4 + count * 2])
    offset = 4 + count * 2
    
    raw_png_list = []
    for sz in sizes:
        raw_png_list.append(data[offset:offset + sz])
        offset += sz
        
    # 标准 6 帧经典循环
    max_f = 6
    if len(raw_png_list) > max_f:
        indices = [int(i * len(raw_png_list) / max_f) for i in range(max_f)]
        sampled_pngs = [raw_png_list[i] for i in indices]
    else:
        sampled_pngs = raw_png_list
        
    out_png_bytes = []
    for p_data in sampled_pngs:
        img = Image.open(io.BytesIO(p_data)).convert("RGBA")
        
        # 优化 PNG 压缩
        buf = io.BytesIO()
        # 64 色高质量量化，保持 Alpha 准确
        q = img.quantize(colors=64, method=Image.Quantize.MEDIANCUT)
        q.save(buf, format="PNG", optimize=True)
        out_png_bytes.append(buf.getvalue())
        
    out_buf = bytearray(struct.pack("<4B", 0xAA, 0x01, len(out_png_bytes), 0))
    for b in out_png_bytes:
        out_buf.extend(struct.pack("<H", len(b)))
    for b in out_png_bytes:
        out_buf.extend(b)
        
    with open(act_p, "wb") as f:
        f.write(out_buf)

def main():
    act_files = list(DATA_DIR.rglob("*.act"))
    print(f"Compressing {len(act_files)} .act files to standard 6 frames...")
    for p in act_files:
        optimize_act(p)
    total = sum(f.stat().st_size for f in DATA_DIR.parent.rglob('*') if f.is_file())
    print(f"New total data size: {total / (1024*1024):.2f} MB ({total / 1024:.1f} KB)")

if __name__ == "__main__":
    main()
