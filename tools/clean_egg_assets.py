import os
from pathlib import Path
from PIL import Image, ImageDraw
import numpy as np

WORKSPACE = Path("d:/workspace/QQpet_StickS3")
DATA_DIR = WORKSPACE / "data/assets"

def clean_egg_animations():
    print("Cleaning and refining Egg stage animations for MM and GG...")

    # 对 MM/Egg 和 GG/Egg 的 play 和 happy 帧进行精细画质修复，去除突兀月亮/杂质残影
    for gender in ["MM", "GG"]:
        # 1. 修复 play (生成纯净萌宝左右摇摆扑腾动作)
        play_dir = DATA_DIR / gender / "Egg" / "play"
        stand_dir = DATA_DIR / gender / "Egg" / "stand"
        happy_dir = DATA_DIR / gender / "Egg" / "happy"

        stand_frames = sorted(list(stand_dir.glob("*.png")))
        if not stand_frames:
            continue

        base_im = Image.open(stand_frames[0]).convert("RGBA")

        # 生成 8 帧可爱的宝宝摇摆扑腾 play 动画
        for f in range(8):
            # 以 stand 帧为基底，进行生动可爱的左右摇摆与呼吸微位移
            shift_x = int(np.sin(f * np.pi / 4.0) * 2.5)
            shift_y = int(np.cos(f * np.pi / 4.0) * 1.5)
            
            im = Image.open(stand_frames[f % len(stand_frames)]).convert("RGBA")
            
            # 创建平滑移动帧
            new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
            new_im.paste(im, (shift_x, shift_y), im)
            
            # 保存量化 32 色 PNG
            out_path = play_dir / f"f_{f:02d}.png"
            q = new_im.quantize(colors=32, method=Image.Quantize.FASTOCTREE)
            q.save(out_path, optimize=True)

        # 2. 修复 happy (生成纯净萌宝开怀欢呼/跳跳动作)
        for f in range(8):
            jump_y = -3 if f in [2, 3, 4] else (0 if f in [0, 7] else -1)
            im = Image.open(stand_frames[f % len(stand_frames)]).convert("RGBA")
            
            new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
            new_im.paste(im, (0, jump_y), im)
            
            # 在头顶添加两个小红心微光 (萌系开心表达)
            d = ImageDraw.Draw(new_im)
            if f in [2, 3, 4, 5]:
                hx = 54 + (f - 2) * 2
                hy = 22 - (f - 2) * 2
                d.ellipse([hx, hy, hx+4, hy+4], fill=(255, 100, 150, 220))
                d.ellipse([hx+3, hy, hx+7, hy+4], fill=(255, 100, 150, 220))
                d.polygon([(hx, hy+2), (hx+7, hy+2), (hx+3, hy+6)], fill=(255, 100, 150, 220))
            
            out_path = happy_dir / f"f_{f:02d}.png"
            q = new_im.quantize(colors=32, method=Image.Quantize.FASTOCTREE)
            q.save(out_path, optimize=True)

    print("All Egg stage animations refined and cleaned successfully!")

if __name__ == "__main__":
    clean_egg_animations()
