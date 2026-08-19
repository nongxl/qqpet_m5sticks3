import os
import shutil
from pathlib import Path
from PIL import Image, ImageDraw, ImageEnhance, ImageOps
import numpy as np

WORKSPACE = Path("d:/workspace/QQpet_StickS3")
DATA_DIR = WORKSPACE / "data/assets"

def generate_actions_and_vfx():
    print("Generating walk_left, walk_right, and drag animations for all stages...")

    for gender in ["MM", "GG"]:
        for stage in ["Egg", "Kid", "Adult"]:
            base_dir = DATA_DIR / gender / stage
            stand_dir = base_dir / "stand"
            if not stand_dir.exists():
                continue

            stand_frames = sorted(list(stand_dir.glob("*.png")))
            if not stand_frames:
                continue

            base_imgs = [Image.open(f).convert("RGBA") for f in stand_frames]

            # ----------------------------------------------------
            # 1. walk_right (8 帧左右鸭子步右行)
            # ----------------------------------------------------
            walk_r_dir = base_dir / "walk_right"
            walk_r_dir.mkdir(parents=True, exist_ok=True)
            for f in range(8):
                im = base_imgs[f % len(base_imgs)].copy()
                # 倾斜摆动 + 垂直起伏模拟小碎步
                angle = -3.0 + np.sin(f * np.pi / 2.0) * 4.0
                rot = im.rotate(angle, resample=Image.Resampling.BICUBIC, center=(48, 85))
                shift_y = int(np.abs(np.sin(f * np.pi / 2.0)) * -3.0)
                
                new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
                new_im.paste(rot, (0, shift_y), rot)
                
                q = new_im.quantize(colors=32, method=Image.Quantize.FASTOCTREE)
                q.save(walk_r_dir / f"f_{f:02d}.png", optimize=True)

            # ----------------------------------------------------
            # 2. walk_left (8 帧水平镜像左行)
            # ----------------------------------------------------
            walk_l_dir = base_dir / "walk_left"
            walk_l_dir.mkdir(parents=True, exist_ok=True)
            for f in range(8):
                rf = walk_r_dir / f"f_{f:02d}.png"
                im = Image.open(rf).convert("RGBA")
                flipped = ImageOps.mirror(im)
                q = flipped.quantize(colors=32, method=Image.Quantize.FASTOCTREE)
                q.save(walk_l_dir / f"f_{f:02d}.png", optimize=True)

            # ----------------------------------------------------
            # 3. drag (8 帧悬空提溜扑腾挣扎动作)
            # ----------------------------------------------------
            drag_dir = base_dir / "drag"
            drag_dir.mkdir(parents=True, exist_ok=True)
            for f in range(8):
                im = base_imgs[f % len(base_imgs)].copy()
                # 身体悬空拉长，小翅膀快速左右扑腾
                w, h = im.size
                stretch_h = int(h * 1.05)
                stretched = im.resize((w, stretch_h), Image.Resampling.LANCZOS)
                
                # 左右疯狂小摇摆
                flap_angle = np.sin(f * np.pi) * 8.0
                rot = stretched.rotate(flap_angle, resample=Image.Resampling.BICUBIC, center=(48, 20))
                
                # 悬空向上提升 8px
                new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
                new_im.paste(rot, (0, -8), rot)
                
                q = new_im.quantize(colors=32, method=Image.Quantize.FASTOCTREE)
                q.save(drag_dir / f"f_{f:02d}.png", optimize=True)

            print(f"Generated walk_left, walk_right, drag for {gender}/{stage}!")

    print("\nAll action sequences created successfully!")

if __name__ == "__main__":
    generate_actions_and_vfx()
