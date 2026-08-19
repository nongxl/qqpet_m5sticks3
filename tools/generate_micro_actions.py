import os
from pathlib import Path
from PIL import Image, ImageDraw, ImageOps
import numpy as np

WORKSPACE = Path("d:/workspace/QQpet_StickS3")
DATA_DIR = WORKSPACE / "data/assets"

def generate_micro_actions():
    print("Generating sneeze, yawn, angry, and shy animations for all stages...")

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
            # 1. sneeze (6 帧：后仰蓄力 -> 阿嚏猛前倾 -> 揉鼻子)
            # ----------------------------------------------------
            sneeze_dir = base_dir / "sneeze"
            sneeze_dir.mkdir(parents=True, exist_ok=True)
            for f in range(6):
                im = base_imgs[f % len(base_imgs)].copy()
                if f in [0, 1]:
                    # 后仰蓄力 (向后倾斜 6 度，微向上抬)
                    rot = im.rotate(-6.0, resample=Image.Resampling.BICUBIC, center=(48, 85))
                    new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
                    new_im.paste(rot, (0, -2), rot)
                elif f in [2, 3]:
                    # “阿嚏！”猛烈前俯 (向前倾斜 10 度，向下冲 4px)
                    rot = im.rotate(10.0, resample=Image.Resampling.BICUBIC, center=(48, 85))
                    new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
                    new_im.paste(rot, (0, 4), rot)
                    # 绘制小喷嚏白点气流
                    draw = ImageDraw.Draw(new_im)
                    draw.ellipse([64, 48, 70, 54], fill=(240, 245, 255, 200))
                    draw.ellipse([72, 44, 76, 48], fill=(220, 235, 255, 180))
                else:
                    # 恢复并微歪头吸鼻子
                    rot = im.rotate(2.0, resample=Image.Resampling.BICUBIC, center=(48, 85))
                    new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
                    new_im.paste(rot, (0, 0), rot)

                q = new_im.quantize(colors=16, method=Image.Quantize.FASTOCTREE)
                q.save(sneeze_dir / f"f_{f:02d}.png", optimize=True)

            # ----------------------------------------------------
            # 2. yawn (6 帧：揉眼惺忪 -> 张嘴大哈欠 -> 揉揉眼)
            # ----------------------------------------------------
            yawn_dir = base_dir / "yawn"
            yawn_dir.mkdir(parents=True, exist_ok=True)
            for f in range(6):
                im = base_imgs[f % len(base_imgs)].copy()
                new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
                if f in [1, 2, 3]:
                    # 头部微仰打哈欠
                    rot = im.rotate(-4.0, resample=Image.Resampling.BICUBIC, center=(48, 85))
                    new_im.paste(rot, (0, -2), rot)
                    draw = ImageDraw.Draw(new_im)
                    # 大张嘴
                    draw.ellipse([44, 48, 52, 56], fill=(220, 60, 60, 220))
                    # 困倦泪花小水滴
                    draw.ellipse([34, 42, 38, 46], fill=(160, 220, 255, 200))
                else:
                    new_im.paste(im, (0, 0), im)
                    draw = ImageDraw.Draw(new_im)
                    # 揉眼睛小动作
                    draw.ellipse([36, 44, 42, 50], fill=(245, 245, 245, 180))

                q = new_im.quantize(colors=16, method=Image.Quantize.FASTOCTREE)
                q.save(yawn_dir / f"f_{f:02d}.png", optimize=True)

            # ----------------------------------------------------
            # 3. angry (6 帧：气鼓鼓跺脚、身体起伏)
            # ----------------------------------------------------
            angry_dir = base_dir / "angry"
            angry_dir.mkdir(parents=True, exist_ok=True)
            for f in range(6):
                im = base_imgs[f % len(base_imgs)].copy()
                new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
                
                # 身体横向略微拉宽压扁（气鼓鼓鼓腮帮子）
                w, h = im.size
                bulge_w = int(w * 1.06)
                bulged = im.resize((bulge_w, h), Image.Resampling.BILINEAR)
                offset_x = (96 - bulge_w) // 2
                
                # 跺脚上下急促震动
                shake_y = -3 if (f % 2 == 1) else 1
                new_im.paste(bulged, (offset_x, shake_y), bulged)
                
                # 头顶小怒火交叉十字符号
                draw = ImageDraw.Draw(new_im)
                fx, fy = 66, 22 + (f % 2) * 2
                draw.line([(fx - 4, fy - 4), (fx + 4, fy + 4)], fill=(255, 60, 40, 230), width=2)
                draw.line([(fx + 4, fy - 4), (fx - 4, fy + 4)], fill=(255, 60, 40, 230), width=2)

                q = new_im.quantize(colors=16, method=Image.Quantize.FASTOCTREE)
                q.save(angry_dir / f"f_{f:02d}.png", optimize=True)

            # ----------------------------------------------------
            # 4. shy (6 帧：粉红脸颊、左右娇羞微晃)
            # ----------------------------------------------------
            shy_dir = base_dir / "shy"
            shy_dir.mkdir(parents=True, exist_ok=True)
            for f in range(6):
                im = base_imgs[f % len(base_imgs)].copy()
                new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
                
                # 左右小娇羞摇晃
                sway = np.sin(f * np.pi / 3.0) * 3.0
                rot = im.rotate(sway, resample=Image.Resampling.BICUBIC, center=(48, 85))
                new_im.paste(rot, (0, 0), rot)
                
                # 左右脸颊两团粉红红晕
                draw = ImageDraw.Draw(new_im)
                draw.ellipse([32, 46, 42, 54], fill=(255, 130, 160, 180))
                draw.ellipse([54, 46, 64, 54], fill=(255, 130, 160, 180))

                q = new_im.quantize(colors=16, method=Image.Quantize.FASTOCTREE)
                q.save(shy_dir / f"f_{f:02d}.png", optimize=True)

            print(f"Generated sneeze, yawn, angry, shy for {gender}/{stage}!")

    print("\nAll micro-actions created successfully!")

if __name__ == "__main__":
    generate_micro_actions()
