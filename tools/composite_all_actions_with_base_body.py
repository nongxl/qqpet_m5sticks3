#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
composite_all_actions_with_base_body.py
还原 QQ 宠物原版 Flash 双层渲染架构 (Base Body Avatar + Action Props Overlay)：
将所有局部动作/道具帧完整叠加在官方基准身躯上，彻底解决用餐、学习、跺脚、打工等动作的身躯残缺与透光问题！
"""

import os
import sys
import io
import struct
import subprocess
from pathlib import Path
from PIL import Image
import numpy as np

DATA_DIR = Path(__file__).resolve().parent.parent / "data" / "assets"
WORKSPACE = Path(__file__).resolve().parent.parent

def composite_act_file(act_path: Path, stand_imgs: list):
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
        
    out_png_bytes = []
    num_stand = len(stand_imgs)
    
    for f_idx, p_data in enumerate(raw_png_list):
        act_img = Image.open(io.BytesIO(p_data)).convert("RGBA")
        
        # 如果是 stand 自身，则直接保存
        if act_path.stem == "stand":
            final_img = act_img
        else:
            # 基础站立身躯 (底座)
            base_body = stand_imgs[f_idx % num_stand].copy()
            # 将动作覆盖在基础身躯上
            base_body.paste(act_img, (0, 0), act_img)
            final_img = base_body
            
        q = final_img.quantize(colors=64, method=Image.Quantize.FASTOCTREE)
        buf = io.BytesIO()
        q.save(buf, format="PNG", optimize=True)
        out_png_bytes.append(buf.getvalue())
        
    out_buf = bytearray(struct.pack("<4B", 0xAA, 0x01, len(out_png_bytes), 0))
    for b in out_png_bytes:
        out_buf.extend(struct.pack("<H", len(b)))
    for b in out_png_bytes:
        out_buf.extend(b)
        
    with open(act_path, "wb") as f:
        f.write(out_buf)

def main():
    for gender in ["MM", "GG"]:
        for stage in ["Egg", "Kid", "Adult"]:
            stage_dir = DATA_DIR / gender / stage
            stand_p = stage_dir / "stand.act"
            if not stand_p.exists():
                continue
                
            with open(stand_p, "rb") as f:
                data = f.read()
            count = data[2]
            sizes = struct.unpack(f"<{count}H", data[4:4 + count * 2])
            offset = 4 + count * 2
            stand_imgs = []
            for sz in sizes:
                stand_imgs.append(Image.open(io.BytesIO(data[offset:offset + sz])).convert("RGBA"))
                offset += sz
                
            act_files = list(stage_dir.glob("*.act"))
            print(f"Compositing {len(act_files)} actions for {gender}/{stage}...")
            for act_file in act_files:
                composite_act_file(act_file, stand_imgs)
                
    total = sum(f.stat().st_size for f in DATA_DIR.parent.rglob('*') if f.is_file())
    print(f"\nFinal Total Data Size: {total / (1024*1024):.2f} MB ({total / 1024:.1f} KB)")
    
    # 打包 LittleFS 镜像
    mklittlefs_exe = Path(os.environ['USERPROFILE']) / '.platformio/packages/tool-mklittlefs/mklittlefs.exe'
    out_bin = WORKSPACE / ".pio/build/m5stack-sticks3/littlefs.bin"
    out_bin.parent.mkdir(parents=True, exist_ok=True)
    
    print("\nPacking LittleFS image...")
    cmd = [str(mklittlefs_exe), '-c', 'data', '-s', '0x610000', '-b', '4096', '-p', '256', str(out_bin)]
    res = subprocess.run(cmd, capture_output=True, text=True, cwd=str(WORKSPACE))
    if res.stderr:
        print("mklittlefs stderr:", res.stderr)
    print("mklittlefs returncode:", res.returncode)
    
    # 验证文件列表
    cmd_l = [str(mklittlefs_exe), '-l', '-b', '4096', '-p', '256', '-s', '0x610000', str(out_bin)]
    res_l = subprocess.run(cmd_l, capture_output=True, text=True)
    lines = [l for l in res_l.stdout.strip().split('\n') if l]
    print(f"SUCCESS: LittleFS image contains {len(lines)} files!")

if __name__ == "__main__":
    main()
