import os
from pathlib import Path
from PIL import Image, ImageDraw
import numpy as np

WORKSPACE = Path("d:/workspace/QQpet_StickS3")
DATA_DIR = WORKSPACE / "data/assets"

def generate_weather_and_hospital_actions():
    print("Generating umbrella, cold, summer, tiwenji, injection animations...")

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
            # 1. umbrella (4 帧：撑小花伞，小手微晃)
            # ----------------------------------------------------
            umb_dir = base_dir / "umbrella"
            umb_dir.mkdir(parents=True, exist_ok=True)
            for f in range(4):
                im = base_imgs[f % len(base_imgs)].copy()
                new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
                # 企鹅微缩并微移
                new_im.paste(im, (0, 4), im)
                draw = ImageDraw.Draw(new_im)
                
                # 伞柄
                ux, uy = 62, 45 + (f % 2)
                draw.line([(ux, uy), (ux - 12, uy + 26)], fill=(120, 70, 30, 240), width=2)
                # 伞面 (红黄相间小花伞)
                draw.pieslice([ux - 28, uy - 26, ux + 28, uy + 14], 180, 360, fill=(240, 70, 60, 240))
                draw.pieslice([ux - 16, uy - 26, ux + 16, uy + 14], 210, 330, fill=(255, 220, 60, 240))
                # 伞顶小尖
                draw.line([(ux, uy - 26), (ux, uy - 30)], fill=(120, 70, 30, 240), width=2)

                q = new_im.quantize(colors=16, method=Image.Quantize.FASTOCTREE)
                q.save(umb_dir / f"f_{f:02d}.png", optimize=True)

            # ----------------------------------------------------
            # 2. cold (4 帧：冬雪搓手哈白气)
            # ----------------------------------------------------
            cold_dir = base_dir / "cold"
            cold_dir.mkdir(parents=True, exist_ok=True)
            for f in range(4):
                im = base_imgs[f % len(base_imgs)].copy()
                new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
                # 身体寒颤微抖
                shake_x = 1 if (f % 2 == 1) else -1
                new_im.paste(im, (shake_x, 0), im)
                draw = ImageDraw.Draw(new_im)
                
                # 厚毛绒蓝色大围巾
                draw.rounded_rectangle([32, 54, 64, 66], radius=4, fill=(40, 120, 220, 240))
                # 嘴前哈出白雾气团
                gx, gy = 56 + f * 2, 44 - f * 2
                draw.ellipse([gx, gy, gx + 8 + f * 2, gy + 8 + f * 2], fill=(240, 250, 255, 160))

                q = new_im.quantize(colors=16, method=Image.Quantize.FASTOCTREE)
                q.save(cold_dir / f"f_{f:02d}.png", optimize=True)

            # ----------------------------------------------------
            # 3. summer (4 帧：夏日吃冰棍/吹电风扇)
            # ----------------------------------------------------
            sum_dir = base_dir / "summer"
            sum_dir.mkdir(parents=True, exist_ok=True)
            for f in range(4):
                im = base_imgs[f % len(base_imgs)].copy()
                new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
                new_im.paste(im, (0, 0), im)
                draw = ImageDraw.Draw(new_im)
                
                # 迷你小风扇 (带旋转扇叶)
                fx, fy = 70, 65
                draw.rectangle([fx + 4, fy + 8, fx + 6, fy + 22], fill=(100, 100, 100, 240))
                draw.ellipse([fx, fy + 20, fx + 10, fy + 24], fill=(60, 60, 60, 240)) # 底座
                draw.ellipse([fx - 4, fy - 6, fx + 14, fy + 12], outline=(0, 180, 240, 220), width=1) # 罩子
                # 扇叶
                ang = (f * 90) % 360
                draw.line([(fx + 5, fy + 3), (fx + 5 + 6 * np.cos(np.radians(ang)), fy + 3 + 6 * np.sin(np.radians(ang)))], fill=(0, 220, 255, 255), width=2)
                # 凉风气流
                draw.arc([fx - 14, fy - 2, fx - 2, fy + 8], 120, 240, fill=(180, 240, 255, 180), width=1)

                q = new_im.quantize(colors=16, method=Image.Quantize.FASTOCTREE)
                q.save(sum_dir / f"f_{f:02d}.png", optimize=True)

            # ----------------------------------------------------
            # 4. tiwenji (4 帧：嘴叼体温计、头顶冰袋微颤)
            # ----------------------------------------------------
            tiw_dir = base_dir / "tiwenji"
            tiw_dir.mkdir(parents=True, exist_ok=True)
            for f in range(4):
                im = base_imgs[f % len(base_imgs)].copy()
                new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
                # 虚弱微缩
                new_im.paste(im, (0, 2), im)
                draw = ImageDraw.Draw(new_im)
                
                # 头顶蓝色退热冰袋
                draw.ellipse([38, 14, 58, 28], fill=(100, 180, 255, 230))
                draw.ellipse([44, 10, 52, 16], fill=(255, 180, 0, 240)) # 冰袋扎口
                
                # 嘴叼玻璃水银体温计
                tx, ty = 54, 52
                draw.line([(tx, ty), (tx + 16, ty - 6)], fill=(220, 240, 255, 240), width=3)
                draw.line([(tx, ty), (tx + 8, ty - 3)], fill=(255, 40, 40, 255), width=2) # 红色水银芯
                draw.ellipse([tx + 14, ty - 8, tx + 18, ty - 4], fill=(200, 220, 240, 240)) # 水银球

                q = new_im.quantize(colors=16, method=Image.Quantize.FASTOCTREE)
                q.save(tiw_dir / f"f_{f:02d}.png", optimize=True)

            # ----------------------------------------------------
            # 5. injection (4 帧：巨型针筒扎屁股特写，泪花狂飙)
            # ----------------------------------------------------
            inj_dir = base_dir / "injection"
            inj_dir.mkdir(parents=True, exist_ok=True)
            for f in range(4):
                im = base_imgs[f % len(base_imgs)].copy()
                new_im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
                
                # 企鹅趴下受惊 (向前倾斜 15 度)
                rot = im.rotate(15.0, resample=Image.Resampling.BICUBIC, center=(35, 80))
                new_im.paste(rot, (-6, 6), rot)
                draw = ImageDraw.Draw(new_im)
                
                # 巨型医用针筒 (从右上方刺向屁股)
                sx, sy = 56 + (f % 2) * 2, 60
                draw.line([(sx, sy), (sx + 26, sy - 22)], fill=(220, 240, 255, 240), width=6)
                draw.line([(sx - 4, sy + 3), (sx, sy)], fill=(180, 190, 200, 255), width=2) # 细针头
                draw.line([(sx + 6, sy - 5), (sx + 18, sy - 15)], fill=(0, 220, 120, 240), width=4) # 绿色药水
                
                # 眼角狂飙蓝色泪花
                draw.ellipse([20 - f * 2, 40 + f * 2, 26 - f * 2, 46 + f * 2], fill=(120, 200, 255, 230))
                draw.ellipse([14 - f * 3, 36 + f * 3, 19 - f * 3, 41 + f * 3], fill=(160, 220, 255, 200))

                q = new_im.quantize(colors=16, method=Image.Quantize.FASTOCTREE)
                q.save(inj_dir / f"f_{f:02d}.png", optimize=True)

            print(f"Generated weather & hospital actions for {gender}/{stage}!")

    print("\nWeather & Hospital actions created successfully!")

if __name__ == "__main__":
    generate_weather_and_hospital_actions()
