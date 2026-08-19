from pathlib import Path
from PIL import Image

WORKSPACE = Path("d:/workspace/QQpet_StickS3")
SRC_DIR = WORKSPACE / "doc/QQ_NEW_SWF/icons"
OUT_DIR = WORKSPACE / "data/assets/icons"
OUT_DIR.mkdir(parents=True, exist_ok=True)

# 映射 11 款官方菜单图标
ICON_MAPPINGS = {
    "feed": SRC_DIR / "weishi.png",
    "bath": SRC_DIR / "qingjie.png",
    "play": SRC_DIR / "wanshua.png",
    "wardrobe": SRC_DIR / "fenzhuan.png",
    "work": SRC_DIR / "dagong.png",
    "study": SRC_DIR / "xuexi.png",
    "trip": SRC_DIR / "lvyou.png",
    "cure": SRC_DIR / "zhibing.png",
    "shop": SRC_DIR / "richang.png",
    "status": SRC_DIR / "chongwu.png",
    "web": SRC_DIR / "guanli.png"
}

def convert_icons():
    print("Converting official 26x26 / 40x40 icons to pristine 20x20 norm & 28x28 act PNGs...")
    
    for name, src_path in ICON_MAPPINGS.items():
        if not src_path.exists():
            print(f"Warning: {src_path} not found!")
            continue

        im = Image.open(src_path).convert("RGBA")
        
        # 1. 裁剪有效透明边界 (消除边缘留白)
        bbox = im.getbbox()
        if bbox:
            im = im.crop(bbox)

        # 2. 生成 20x20 常态图标 (_norm.png)
        w, h = im.size
        scale_norm = min(18 / w, 18 / h)
        nw_norm, nh_norm = max(1, int(w * scale_norm)), max(1, int(h * scale_norm))
        im_norm = im.resize((nw_norm, nh_norm), Image.Resampling.LANCZOS)
        
        canvas_norm = Image.new("RGBA", (20, 20), (0, 0, 0, 0))
        canvas_norm.paste(im_norm, ((20 - nw_norm) // 2, (20 - nh_norm) // 2), im_norm)
        
        out_norm = OUT_DIR / f"{name}_norm.png"
        canvas_norm.save(out_norm, optimize=True)

        # 3. 生成 28x28 选中放大高亮图标 (_act.png)
        scale_act = min(26 / w, 26 / h)
        nw_act, nh_act = max(1, int(w * scale_act)), max(1, int(h * scale_act))
        im_act = im.resize((nw_act, nh_act), Image.Resampling.LANCZOS)
        
        canvas_act = Image.new("RGBA", (28, 28), (0, 0, 0, 0))
        canvas_act.paste(im_act, ((28 - nw_act) // 2, (28 - nh_act) // 2), im_act)
        
        out_act = OUT_DIR / f"{name}_act.png"
        canvas_act.save(out_act, optimize=True)

        print(f"  Processed {name:10s} -> norm(20x20) & act(28x28) from {src_path.name}")

    print("\nAll 11 official icons converted and written to data/assets/icons/ successfully!")

if __name__ == "__main__":
    convert_icons()
