import os
from pathlib import Path
from PIL import Image, ImageDraw

WORKSPACE_DIR = Path("d:/workspace/QQpet_StickS3")
OUT_DIR = WORKSPACE_DIR / "data/assets/costumes"
OUT_DIR.mkdir(parents=True, exist_ok=True)

def create_costumes():
    # 1. 🎓 博士帽 (Graduation Cap) - 32x20
    im = Image.new("RGBA", (32, 20), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    # 菱形帽顶
    d.polygon([(16, 2), (30, 8), (16, 14), (2, 8)], fill=(30, 35, 50, 255), outline=(15, 20, 30, 255))
    # 帽身
    d.rectangle([10, 11, 22, 17], fill=(25, 30, 45, 255))
    # 金色流苏
    d.line([(16, 8), (27, 10), (27, 18)], fill=(255, 215, 0, 255), width=2)
    d.ellipse([25, 17, 29, 19], fill=(255, 200, 0, 255))
    im.save(OUT_DIR / "hat_grad.png")

    # 2. 🕶️ 酷炫墨镜 (Cool Sunglasses) - 30x12
    im = Image.new("RGBA", (30, 12), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    # 左右黑镜片
    d.rounded_rectangle([2, 2, 13, 10], radius=3, fill=(20, 25, 30, 255), outline=(210, 180, 50, 255))
    d.rounded_rectangle([17, 2, 28, 10], radius=3, fill=(20, 25, 30, 255), outline=(210, 180, 50, 255))
    # 中间鼻梁金架
    d.line([(13, 4), (17, 4)], fill=(230, 195, 60, 255), width=2)
    # 镜面白色高光反光
    d.line([(4, 4), (8, 8)], fill=(120, 180, 240, 200), width=1)
    d.line([(19, 4), (23, 8)], fill=(120, 180, 240, 200), width=1)
    im.save(OUT_DIR / "glass_cool.png")

    # 3. 🧣 鲜艳红领巾 (Red Scarf) - 26x16
    im = Image.new("RGBA", (26, 16), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    # 领结
    d.polygon([(4, 2), (22, 2), (18, 7), (8, 7)], fill=(230, 30, 30, 255))
    d.ellipse([11, 4, 15, 8], fill=(255, 60, 60, 255))
    # 三角飘带
    d.polygon([(11, 7), (15, 7), (16, 15), (10, 15)], fill=(220, 20, 20, 255))
    d.polygon([(13, 7), (17, 7), (20, 13), (15, 13)], fill=(200, 15, 15, 255))
    im.save(OUT_DIR / "scarf_red.png")

    # 4. 👼 天使金色光环 (Angel Halo) - 28x12
    im = Image.new("RGBA", (28, 12), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    d.ellipse([2, 2, 26, 10], outline=(255, 215, 0, 255), width=2)
    d.ellipse([4, 3, 24, 9], outline=(255, 245, 140, 200), width=1)
    im.save(OUT_DIR / "hat_halo.png")

    # 5. 🎀 萌粉蝴蝶结 (Pink Bowtie) - 24x16
    im = Image.new("RGBA", (24, 16), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    d.polygon([(3, 3), (12, 8), (3, 13)], fill=(255, 110, 160, 255), outline=(230, 60, 120, 255))
    d.polygon([(21, 3), (12, 8), (21, 13)], fill=(255, 110, 160, 255), outline=(230, 60, 120, 255))
    d.ellipse([10, 6, 14, 10], fill=(255, 180, 210, 255), outline=(230, 60, 120, 255))
    im.save(OUT_DIR / "bow_pink.png")

    # 6. 👑 尊贵黄金皇冠 (Gold Crown) - 26x16
    im = Image.new("RGBA", (26, 16), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    # 底座
    d.rectangle([4, 11, 22, 14], fill=(255, 200, 0, 255), outline=(210, 150, 0, 255))
    # 皇冠尖顶
    d.polygon([(4, 11), (4, 4), (9, 8), (13, 2), (17, 8), (22, 4), (22, 11)], fill=(255, 215, 20, 255), outline=(210, 150, 0, 255))
    # 红绿宝石
    d.ellipse([12, 1, 14, 3], fill=(255, 40, 40, 255))
    d.ellipse([3, 3, 5, 5], fill=(40, 200, 255, 255))
    d.ellipse([21, 3, 23, 5], fill=(40, 200, 255, 255))
    im.save(OUT_DIR / "hat_crown.png")

    # 7. 🎒 探险家小背包 (Explorer Backpack) - 20x22
    im = Image.new("RGBA", (20, 22), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    d.rounded_rectangle([2, 3, 18, 19], radius=3, fill=(180, 100, 40, 255), outline=(130, 65, 20, 255))
    d.rounded_rectangle([4, 9, 16, 17], radius=2, fill=(210, 130, 60, 255), outline=(130, 65, 20, 255))
    d.line([(5, 3), (5, 0), (15, 0), (15, 3)], fill=(120, 60, 20, 255), width=2) # 提手
    d.ellipse([9, 12, 11, 14], fill=(255, 215, 0, 255)) # 铜扣
    im.save(OUT_DIR / "pack_explorer.png")

    # 8. 🪄 魔法星月魔杖 (Magic Wand) - 18x24
    im = Image.new("RGBA", (18, 24), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    # 木质魔杖柄
    d.line([(4, 22), (13, 8)], fill=(140, 85, 40, 255), width=2)
    # 闪耀黄星
    d.ellipse([10, 3, 17, 10], fill=(255, 225, 40, 255))
    d.ellipse([12, 5, 15, 8], fill=(255, 250, 180, 255))
    im.save(OUT_DIR / "wand_magic.png")

    print(f"Generated 8 costume assets successfully in {OUT_DIR}")

if __name__ == "__main__":
    create_costumes()
