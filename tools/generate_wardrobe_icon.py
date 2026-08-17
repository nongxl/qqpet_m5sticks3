from pathlib import Path
from PIL import Image, ImageDraw

WORKSPACE_DIR = Path("d:/workspace/QQpet_StickS3")
OUT_DIR = WORKSPACE_DIR / "data/assets/icons"
OUT_DIR.mkdir(parents=True, exist_ok=True)

def create_wardrobe_icons():
    # 1. wardrobe_norm.png (20x20)
    im20 = Image.new("RGBA", (20, 20), (0, 0, 0, 0))
    d20 = ImageDraw.Draw(im20)
    # 衣架金色挂钩
    d20.arc([8, 1, 12, 5], start=180, end=360, fill=(255, 215, 0, 255), width=2)
    # 粉色洋装上身
    d20.polygon([(6, 6), (14, 6), (12, 10), (8, 10)], fill=(255, 105, 180, 255))
    # 裙摆 (喇叭裙)
    d20.polygon([(8, 10), (12, 10), (16, 18), (4, 18)], fill=(255, 140, 200, 255), outline=(230, 80, 150, 255))
    # 蝴蝶结装饰
    d20.ellipse([9, 9, 11, 11], fill=(255, 240, 100, 255))
    im20.save(OUT_DIR / "wardrobe_norm.png")

    # 2. wardrobe_act.png (28x28)
    im28 = Image.new("RGBA", (28, 28), (0, 0, 0, 0))
    d28 = ImageDraw.Draw(im28)
    # 高亮外发光圆底
    d28.ellipse([2, 2, 26, 26], fill=(255, 245, 250, 220), outline=(255, 105, 180, 255), width=2)
    # 衣架金色挂钩
    d28.arc([11, 4, 17, 9], start=180, end=360, fill=(255, 200, 0, 255), width=2)
    # 粉色洋装上身
    d28.polygon([(8, 10), (20, 10), (17, 15), (11, 15)], fill=(255, 90, 170, 255))
    # 裙摆
    d28.polygon([(11, 15), (17, 15), (22, 24), (6, 24)], fill=(255, 130, 195, 255), outline=(230, 60, 140, 255))
    # 裙边白色花边
    d28.line([(7, 23), (21, 23)], fill=(255, 255, 255, 255), width=1)
    # 蝴蝶结
    d28.ellipse([12, 13, 16, 16], fill=(255, 235, 80, 255))
    im28.save(OUT_DIR / "wardrobe_act.png")

    print("Created wardrobe icons successfully!")

if __name__ == "__main__":
    create_wardrobe_icons()
