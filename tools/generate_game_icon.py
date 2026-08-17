from pathlib import Path
from PIL import Image, ImageDraw

WORKSPACE_DIR = Path("d:/workspace/QQpet_StickS3")
OUT_DIR = WORKSPACE_DIR / "data/assets/icons"
OUT_DIR.mkdir(parents=True, exist_ok=True)

def create_game_icons():
    # 1. play_norm.png (20x20) - 经典蓝色/深紫电竞游戏手柄
    im20 = Image.new("RGBA", (20, 20), (0, 0, 0, 0))
    d20 = ImageDraw.Draw(im20)
    # 手柄外壳
    d20.rounded_rectangle([1, 4, 18, 16], radius=4, fill=(50, 60, 110, 255), outline=(30, 40, 80, 255))
    # 十字方向键 (D-pad)
    d20.rectangle([3, 8, 7, 12], fill=(220, 230, 255, 255))
    d20.rectangle([4, 7, 6, 13], fill=(220, 230, 255, 255))
    # ABXY 按键 (彩色宝石圆点)
    d20.ellipse([12, 7, 14, 9], fill=(255, 60, 60, 255))   # 红
    d20.ellipse([15, 10, 17, 12], fill=(60, 200, 255, 255)) # 蓝
    d20.ellipse([12, 12, 14, 14], fill=(255, 215, 0, 255))  # 黄
    d20.ellipse([10, 10, 12, 12], fill=(80, 230, 90, 255))  # 绿
    im20.save(OUT_DIR / "play_norm.png")

    # 2. play_act.png (28x28) - 金黄发光高光游戏手柄
    im28 = Image.new("RGBA", (28, 28), (0, 0, 0, 0))
    d28 = ImageDraw.Draw(im28)
    # 外发光高亮底座
    d28.ellipse([2, 2, 26, 26], fill=(240, 245, 255, 220), outline=(70, 120, 255, 255), width=2)
    # 手柄主体
    d28.rounded_rectangle([4, 7, 24, 21], radius=5, fill=(45, 65, 140, 255), outline=(30, 45, 100, 255))
    # 摇杆/十字键
    d28.rectangle([7, 12, 11, 16], fill=(255, 255, 255, 255))
    d28.rectangle([8, 11, 10, 17], fill=(255, 255, 255, 255))
    # ABXY 彩色高光大圆点
    d28.ellipse([17, 10, 20, 13], fill=(255, 70, 70, 255))   # 红
    d28.ellipse([20, 13, 23, 16], fill=(50, 190, 255, 255))  # 蓝
    d28.ellipse([17, 16, 20, 19], fill=(255, 225, 40, 255))  # 黄
    d28.ellipse([14, 13, 17, 16], fill=(70, 230, 90, 255))   # 绿
    im28.save(OUT_DIR / "play_act.png")

    print("Created sharp gamepad icons for play_norm.png and play_act.png successfully!")

if __name__ == "__main__":
    create_game_icons()
