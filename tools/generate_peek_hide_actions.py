import os
from pathlib import Path
from PIL import Image, ImageOps
import numpy as np

WORKSPACE = Path("d:/workspace/QQpet_StickS3")
DATA_DIR = WORKSPACE / "data/assets"

def generate_hide_actions():
    print("Generating hide_left and hide_right (peek-a-boo) animations...")

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
            # 1. hide_left (6 帧：往左躲藏，只探出半个萌脑袋和眼睛好奇偷看)
            # ----------------------------------------------------
            hl_dir = base_dir / "hide_left"
            hl_dir.mkdir(parents=True, exist_ok=True)
            for f in range(6):
                im = base_imgs[f % len(base_imgs)].copy()
                # 倾斜探头角度 (探出 12 度)
                angle = -8.0 + np.sin(f * np.pi / 3.0) * 3.0
                rot = im.rotate(angle, resample=Image.Resampling.BICUBIC, center=(25, 80))
                
                # 身体向左偏移 28px，大部分隐藏在边缘之外
                new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
                new_im.paste(rot, (-28, 0), rot)
                
                q = new_im.quantize(colors=16, method=Image.Quantize.FASTOCTREE)
                q.save(hl_dir / f"f_{f:02d}.png", optimize=True)

            # ----------------------------------------------------
            # 2. hide_right (6 帧：往右躲藏，右侧探头)
            # ----------------------------------------------------
            hr_dir = base_dir / "hide_right"
            hr_dir.mkdir(parents=True, exist_ok=True)
            for f in range(6):
                hl_frame = Image.open(hl_dir / f"f_{f:02d}.png").convert("RGBA")
                flipped = ImageOps.mirror(hl_frame)
                q = flipped.quantize(colors=16, method=Image.Quantize.FASTOCTREE)
                q.save(hr_dir / f"f_{f:02d}.png", optimize=True)

            print(f"Generated hide_left & hide_right for {gender}/{stage}!")

    print("\nPeek-a-boo hide animations created successfully!")

if __name__ == "__main__":
    generate_hide_actions()
