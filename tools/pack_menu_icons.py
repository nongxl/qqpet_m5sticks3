import os
from pathlib import Path
from PIL import Image

WORKSPACE_DIR = Path("d:/workspace/QQpet_StickS3")
ICONS_DIR = WORKSPACE_DIR / "doc/qqpet_automation/qq-pet-macos/src/assets/control/icons"
OUT_HEADER = WORKSPACE_DIR / "src/menu_icon_assets.h"
OUT_CPP = WORKSPACE_DIR / "src/menu_icon_assets.cpp"

MENU_ICONS = [
    {"id": "feed", "file": "weishi.png"},
    {"id": "bath", "file": "qingjie.png"},
    {"id": "play", "file": "wanshua.png"},
    {"id": "cure", "file": "zhibing.png"},
    {"id": "status", "file": "chongwu.png"},
    {"id": "web", "file": "guanli.png"},
]

def pack_icons():
    header_lines = [
        "#pragma once",
        "#include <Arduino.h>",
        "",
        "struct MenuIconData {",
        "    const uint8_t* normalData;",
        "    size_t normalLen;",
        "    const uint8_t* activeData;",
        "    size_t activeLen;",
        "};",
        "",
        "extern const MenuIconData ICON_FEED;",
        "extern const MenuIconData ICON_BATH;",
        "extern const MenuIconData ICON_PLAY;",
        "extern const MenuIconData ICON_CURE;",
        "extern const MenuIconData ICON_STATUS;",
        "extern const MenuIconData ICON_WEB;",
        "",
        "const MenuIconData* getMenuIcon(int optionIndex);",
    ]

    cpp_lines = ['#include "menu_icon_assets.h"\n']

    for item in MENU_ICONS:
        icon_id = item["id"]
        icon_path = ICONS_DIR / item["file"]
        
        img = Image.open(icon_path).convert("RGBA")
        
        # 1. 正常尺寸 20x20
        normal_img = img.resize((20, 20), Image.Resampling.LANCZOS)
        normal_out = WORKSPACE_DIR / f"tools/temp_{icon_id}_normal.png"
        normal_img.save(normal_out)
        normal_bytes = normal_out.read_bytes()

        # 2. 放大选中尺寸 28x28
        active_img = img.resize((28, 28), Image.Resampling.LANCZOS)
        active_out = WORKSPACE_DIR / f"tools/temp_{icon_id}_active.png"
        active_img.save(active_out)
        active_bytes = active_out.read_bytes()

        norm_hex = ", ".join(f"0x{b:02x}" for b in normal_bytes)
        act_hex = ", ".join(f"0x{b:02x}" for b in active_bytes)

        cpp_lines.append(f"static const uint8_t ICON_{icon_id.upper()}_NORM_PNG[] PROGMEM = {{{norm_hex}}};")
        cpp_lines.append(f"static const uint8_t ICON_{icon_id.upper()}_ACT_PNG[] PROGMEM = {{{act_hex}}};")
        cpp_lines.append(f"const MenuIconData ICON_{icon_id.upper()} = {{ICON_{icon_id.upper()}_NORM_PNG, sizeof(ICON_{icon_id.upper()}_NORM_PNG), ICON_{icon_id.upper()}_ACT_PNG, sizeof(ICON_{icon_id.upper()}_ACT_PNG)}};\n")

    cpp_lines.append("""
const MenuIconData* getMenuIcon(int optionIndex) {
    switch (optionIndex) {
        case 0: return &ICON_FEED;
        case 1: return &ICON_BATH;
        case 2: return &ICON_PLAY;
        case 3: return &ICON_CURE;
        case 4: return &ICON_STATUS;
        case 5: return &ICON_WEB;
        default: return &ICON_FEED;
    }
}
""")

    OUT_HEADER.write_text("\n".join(header_lines), encoding="utf-8")
    OUT_CPP.write_text("\n".join(cpp_lines), encoding="utf-8")
    print("menu_icon_assets.h & .cpp generated successfully!")

if __name__ == "__main__":
    pack_icons()
