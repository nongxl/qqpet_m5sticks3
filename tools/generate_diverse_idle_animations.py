import os
import shutil
from pathlib import Path
from PIL import Image, ImageDraw, ImageEnhance
import numpy as np

WORKSPACE = Path("d:/workspace/QQpet_StickS3")
DATA_DIR = WORKSPACE / "data/assets"

def create_diverse_idle_animations():
    print("Generating comprehensive, diverse idle animation suite for all stages and genders...")

    for gender in ["MM", "GG"]:
        for stage in ["Egg", "Kid", "Adult"]:
            base_dir = DATA_DIR / gender / stage
            stand_dir = base_dir / "stand"
            if not stand_dir.exists():
                continue

            stand_frames = sorted(list(stand_dir.glob("*.png")))
            if not stand_frames:
                continue

            # 1. 彻底清除旧的 play、happy 等文件夹中多余的遗留帧
            for act in ["play", "happy", "look", "stretch", "wobble", "yawn"]:
                target_dir = base_dir / act
                if target_dir.exists():
                    shutil.rmtree(target_dir)
                target_dir.mkdir(parents=True, exist_ok=True)

            # 读取 8 帧基准 stand 图像
            base_imgs = [Image.open(f).convert("RGBA") for f in stand_frames]

            # ----------------------------------------------------
            # 动作 1: 【look 左右歪头打量主人】(8 帧)
            # ----------------------------------------------------
            look_dir = base_dir / "look"
            for f in range(8):
                # 身体轻微倾斜与眼珠/脑袋左右张望
                im = base_imgs[f % len(base_imgs)].copy()
                tilt_angle = np.sin(f * np.pi / 4.0) * 4.0 # 左右倾斜 4 度
                rotated = im.rotate(tilt_angle, resample=Image.Resampling.BICUBIC, center=(48, 80))
                
                out_path = look_dir / f"f_{f:02d}.png"
                q = rotated.quantize(colors=32, method=Image.Quantize.FASTOCTREE)
                q.save(out_path, optimize=True)

            # ----------------------------------------------------
            # 动作 2: 【wobble 左右憨态蹒跚摇晃扑腾】(8 帧)
            # ----------------------------------------------------
            wobble_dir = base_dir / "wobble"
            for f in range(8):
                im = base_imgs[f % len(base_imgs)].copy()
                # 左右水平小位移 + 呼吸起伏
                shift_x = int(np.sin(f * np.pi / 4.0) * 3.5)
                shift_y = int(np.abs(np.cos(f * np.pi / 4.0)) * -2.0)
                
                new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
                new_im.paste(im, (shift_x, shift_y), im)
                
                out_path = wobble_dir / f"f_{f:02d}.png"
                q = new_im.quantize(colors=32, method=Image.Quantize.FASTOCTREE)
                q.save(out_path, optimize=True)

            # ----------------------------------------------------
            # 动作 3: 【stretch 伸个舒服小懒腰】(8 帧)
            # ----------------------------------------------------
            stretch_dir = base_dir / "stretch"
            for f in range(8):
                im = base_imgs[f % len(base_imgs)].copy()
                # 垂直拉伸与回落
                stretch_factor = 1.0 + (np.sin(f * np.pi / 8.0) * 0.08)
                w, h = im.size
                new_h = int(h * stretch_factor)
                scaled = im.resize((w, new_h), Image.Resampling.LANCZOS)
                
                new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
                paste_y = 96 - new_h
                new_im.paste(scaled, (0, paste_y), scaled)
                
                out_path = stretch_dir / f"f_{f:02d}.png"
                q = new_im.quantize(colors=32, method=Image.Quantize.FASTOCTREE)
                q.save(out_path, optimize=True)

            # ----------------------------------------------------
            # 动作 4: 【happy / bounce 开心蹦跳跳跃】(8 帧)
            # ----------------------------------------------------
            happy_dir = base_dir / "happy"
            for f in range(8):
                im = base_imgs[f % len(base_imgs)].copy()
                jump_y = -5 if f in [2, 3, 4] else (0 if f in [0, 7] else -2)
                
                new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
                new_im.paste(im, (0, jump_y), im)
                
                # 伴随头顶萌萌小爱心
                d = ImageDraw.Draw(new_im)
                if f in [2, 3, 4, 5]:
                    hx = 54 + (f - 2) * 2
                    hy = 20 - (f - 2) * 2
                    d.ellipse([hx, hy, hx+4, hy+4], fill=(255, 120, 160, 230))
                    d.ellipse([hx+3, hy, hx+7, hy+4], fill=(255, 120, 160, 230))
                    d.polygon([(hx, hy+2), (hx+7, hy+2), (hx+3, hy+6)], fill=(255, 120, 160, 230))
                
                out_path = happy_dir / f"f_{f:02d}.png"
                q = new_im.quantize(colors=32, method=Image.Quantize.FASTOCTREE)
                q.save(out_path, optimize=True)

            # ----------------------------------------------------
            # 动作 5: 【play 玩耍拍球/转圈互动】(8 帧)
            # ----------------------------------------------------
            play_dir = base_dir / "play"
            for f in range(8):
                im = base_imgs[f % len(base_imgs)].copy()
                shift_x = int(np.cos(f * np.pi / 4.0) * 2.5)
                shift_y = int(np.sin(f * np.pi / 4.0) * 1.5)
                
                new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
                new_im.paste(im, (shift_x, shift_y), im)
                
                out_path = play_dir / f"f_{f:02d}.png"
                q = new_im.quantize(colors=32, method=Image.Quantize.FASTOCTREE)
                q.save(out_path, optimize=True)

            print(f"Generated 5 distinct idle actions for {gender}/{stage} successfully!")

    # 补充 MM/Adult/work 如果缺失
    mm_adult_work = DATA_DIR / "MM/Adult/work"
    gg_adult_work = DATA_DIR / "GG/Adult/work"
    if not any(mm_adult_work.glob("*.png")) and gg_adult_work.exists():
        mm_adult_work.mkdir(parents=True, exist_ok=True)
        for f in gg_adult_work.glob("*.png"):
            shutil.copy(f, mm_adult_work / f.name)
        print("Copied MM/Adult/work frames from GG/Adult/work.")

    print("\nAll diverse idle animations generated and LittleFS assets refreshed perfectly!")

if __name__ == "__main__":
    create_diverse_idle_animations()
