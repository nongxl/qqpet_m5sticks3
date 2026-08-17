import os
import re
from pathlib import Path

WORKSPACE = Path("d:/workspace/QQpet_StickS3")
SMALLTALK_DIR = WORKSPACE / "doc/qqpet_automation/qq-pet-macos/src/assets/Communication/SmallTalk"
GAME_DATA_CPP = WORKSPACE / "src/game_data.cpp"
GAME_DATA_H = WORKSPACE / "src/game_data.h"

def get_smalltalk_quotes():
    quotes = []
    # 按照数字 1..101 顺序读取
    for i in range(1, 102):
        f = SMALLTALK_DIR / f"{i}.txt"
        if f.exists():
            text = f.read_text(encoding="utf-8", errors="ignore").strip()
            # 清理换行和空白
            text = " ".join(text.split())
            # 转义双引号
            text = text.replace('"', '\\"')
            if text and text not in quotes:
                quotes.append(text)
    return quotes

def update_game_data():
    quotes = get_smalltalk_quotes()
    print(f"Loaded {len(quotes)} original SmallTalk quotes.")

    quotes_cpp_str = "const char* QUOTES_IDLE[] = {\n"
    for q in quotes:
        quotes_cpp_str += f'    "{q}",\n'
    quotes_cpp_str += "};\n"
    quotes_cpp_str += f"const size_t QUOTES_IDLE_COUNT = {len(quotes)};\n"

    # 读取当前 game_data.cpp 并替换 QUOTES_IDLE 定义
    cpp_text = GAME_DATA_CPP.read_text(encoding="utf-8")
    pattern = r"const char\* QUOTES_IDLE\[\]\s*=\s*\{.*?\};\s*const size_t QUOTES_IDLE_COUNT\s*=\s*\d+;"
    new_cpp_text = re.sub(pattern, quotes_cpp_str.strip(), cpp_text, flags=re.DOTALL)
    GAME_DATA_CPP.write_text(new_cpp_text, encoding="utf-8")
    print("Updated game_data.cpp with all 101 SmallTalk quotes successfully!")

if __name__ == "__main__":
    update_game_data()
