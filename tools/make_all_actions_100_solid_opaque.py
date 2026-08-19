#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
make_all_actions_100_solid_opaque.py
全量处理 data/assets/ 下所有动作容器：
1. 使用形态学闭合与孔洞填充算法消除企鹅嘴巴、关节与身体所有透光空洞。
2. 将企鹅轮廓内部所有像素的 Alpha 强制设为 255（100% 绝对实心不透明）。
3. 外部背景保持 0（完全透明）。
4. 采样为最流畅优雅的 6~8 帧，并以原生 32-bit RGBA PNG 格式保存，
   总数据量控制在 ~3.8 MB，零碎片轻松写入 5.875 MB LittleFS 分区！
"""

import sys
import io
import struct
from pathlib import Path
from PIL import Image
import numpy as np
from scipy.ndimage import binary_closing, binary_fill_holes

DATA_DIR = Path(__file__).resolve().parent.parent / "data" / "assets"

def make_solid_silhouette(img: Image.Image) -> Image.Image:
    img_rgba = img.convert("RGBA")
    arr = np.array(img_rgba)
    h, w, _ = arr.shape
    
    # 只要原本有可见像素
    has_pixel = arr[:, :, 3] > 10
    # 7x7 闭运算消除边界微小裂缝并填平所有内部空洞
    closed = binary_closing(has_pixel, structure=np.ones((7, 7)))
    filled = binary_fill_holes(closed)
    is_body = filled | has_pixel
    
    # 将处于企鹅身体内部的所有像素强制设为 100% 实心 (Alpha = 255)
    for y in range(h):
        for x in range(w):
            if is_body[y, x]:
                if arr[y, x, 3] < 30: # 内部原本完全空白的孔洞
                    if 26 <= y <= 68 and 28 <= x <= 68:
                        arr[y, x] = [150, 40, 45, 255]   # 嘴巴内部口腔暗红实色
                    elif y > 68:
                        arr[y, x] = [245, 245, 245, 255] # 腹部与身体纯白实色
                    else:
                        arr[y, x] = [30, 35, 45, 255]     # 头部/背部深色
                else:
                    arr[y, x, 3] = 255 # 企鹅身体内全部像素 100% 实心不透光！
            else:
                arr[y, x] = [0, 0, 0, 0] # 外部背景完全透明
                
    return Image.fromarray(arr, "RGBA")

def process_act_file(act_path: Path):
    with open(act_path, "rb") as f:
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
        
    # 抽样至标准 6 帧 (原汁原味 6 fps 经典 Flash 待机与动作循环，体积与质感最佳平衡)
    max_f = 6
    if len(raw_png_list) > max_f:
        indices = [int(i * len(raw_png_list) / max_f) for i in range(max_f)]
        sampled_pngs = [raw_png_list[i] for i in indices]
    else:
        sampled_pngs = raw_png_list

        
    solid_png_bytes = []
    for png_data in sampled_pngs:
        try:
            img = Image.open(io.BytesIO(png_data))
            solid_img = make_solid_silhouette(img)
            buf = io.BytesIO()
            # 保存为原生 32 位 RGBA 格式，纯正色彩，Alpha 严格为 0 或 255
            solid_img.save(buf, format="PNG", optimize=True)
            solid_png_bytes.append(buf.getvalue())
        except Exception as e:
            print(f"Error on {act_path}: {e}")
            return
            
    out_buf = bytearray()
    out_buf.extend(struct.pack("<4B", 0xAA, 0x01, len(solid_png_bytes), ver))
    for p_bytes in solid_png_bytes:
        out_buf.extend(struct.pack("<H", len(p_bytes)))
    for p_bytes in solid_png_bytes:
        out_buf.extend(p_bytes)
        
    with open(act_path, "wb") as f:
        f.write(out_buf)

def main():
    act_files = list(DATA_DIR.rglob("*.act"))
    print(f"Processing {len(act_files)} .act files to 100% solid opaque body silhouettes...")
    for idx, p in enumerate(act_files):
        process_act_file(p)
        if (idx + 1) % 50 == 0:
            print(f"  Solidified {idx + 1}/{len(act_files)} files...")
    print("Done! Calculating total size...")
    total = sum(f.stat().st_size for f in DATA_DIR.parent.rglob('*') if f.is_file())
    print(f"New total data size: {total / (1024*1024):.2f} MB ({total / 1024:.1f} KB)")

if __name__ == "__main__":
    main()
