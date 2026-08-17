import os
from pathlib import Path
from PIL import Image

WORKSPACE_DIR = Path("d:/workspace/QQpet_StickS3")
ICONS_DIR = WORKSPACE_DIR / "doc/qqpet_automation/qq-pet-macos/src/assets/control/icons"
OUT_DATA_DIR = WORKSPACE_DIR / "data/assets/icons"

MENU_ICONS = [
    {"id": "feed", "file": "weishi.png"},
    {"id": "bath", "file": "qingjie.png"},
    {"id": "play", "file": "wanshua.png"},
    {"id": "work", "file": "dagong.png"},
    {"id": "study", "file": "xuexi.png"},
    {"id": "trip", "file": "lvyou.png"},
    {"id": "cure", "file": "zhibing.png"},
    {"id": "shop", "file": "richang.png"},
    {"id": "status", "file": "chongwu.png"},
    {"id": "web", "file": "guanli.png"},
]

def pack_icons():
    OUT_DATA_DIR.mkdir(parents=True, exist_ok=True)

    for item in MENU_ICONS:
        icon_id = item["id"]
        icon_path = ICONS_DIR / item["file"]
        
        img = Image.open(icon_path).convert("RGBA")
        
        # 1. 正常尺寸 20x20
        normal_img = img.resize((20, 20), Image.Resampling.LANCZOS)
        normal_out = OUT_DATA_DIR / f"{icon_id}_norm.png"
        normal_img.save(normal_out)

        # 2. 放大选中尺寸 28x28
        active_img = img.resize((28, 28), Image.Resampling.LANCZOS)
        active_out = OUT_DATA_DIR / f"{icon_id}_act.png"
        active_img.save(active_out)

    print("All 10 menu icons saved to data/assets/icons successfully!")

if __name__ == "__main__":
    pack_icons()

