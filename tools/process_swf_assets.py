import os, struct, subprocess
from pathlib import Path
from PIL import Image, ImageDraw

WORKSPACE = Path("d:/workspace/QQpet_StickS3")
ASSETS_DIR = WORKSPACE / "data/assets"
SOUNDS_DIR = ASSETS_DIR / "sounds"
COSTUMES_DIR = ASSETS_DIR / "costumes"

SOUNDS_DIR.mkdir(parents=True, exist_ok=True)
COSTUMES_DIR.mkdir(parents=True, exist_ok=True)

# 1. 转换并生成核心音效集 (16kHz 8-bit Mono WAV，轻量纯净)
AUDIO_MAPPING = {
    "1020030141.mp3": "study.wav",    # 上课翻书铃声
    "1020030241.mp3": "work.wav",     # 敲击搬砖音效
    "1020050111.mp3": "happy.wav",    # 企鹅开怀大笑/欢叫
    "1020050221.mp3": "snore.wav",    # 戴睡帽打呼噜声
    "1020050421.mp3": "eat.wav",      # 吧唧嘴进食
    "1020050521.mp3": "clean.wav",    # 洗澡揉泡泡水花
    "1020050611.mp3": "levelup.wav",  # 升级欢呼
    "1020050831.mp3": "coin.wav",     # 金币掉落
    "1020051111.mp3": "click.wav",    # 按键提示
    "1020051221.mp3": "sick.wav",     # 叹气咳嗽
}

def process_audio():
    print("Processing audio sound effects...")
    for src_mp3, target_wav in AUDIO_MAPPING.items():
        src_path = SOUNDS_DIR / src_mp3
        dst_path = SOUNDS_DIR / target_wav
        if src_path.exists():
            cmd = [
                'ffmpeg', '-y', '-i', str(src_path),
                '-ac', '1', '-ar', '16000', '-c:a', 'pcm_u8',
                '-t', '2.5', # 限制最大时长 2.5s 极速轻量
                str(dst_path)
            ]
            subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            if dst_path.exists():
                print(f"  Generated {target_wav}: {dst_path.stat().st_size} bytes")

# 2. 生成更多原版经典饰品
def generate_expanded_costumes():
    print("Generating expanded costumes...")
    # 9. 经典红白圣诞帽 (hat_xmas.png) 32x24
    im = Image.new("RGBA", (32, 24), (0,0,0,0))
    d = ImageDraw.Draw(im)
    # 红色锥形帽身 (微微向右弯垂)
    d.polygon([(6, 18), (16, 2), (28, 12), (24, 18)], fill=(220, 30, 30, 255))
    # 白色毛绒帽檐
    d.rounded_rectangle([4, 16, 28, 22], radius=3, fill=(255, 255, 255, 255), outline=(220, 230, 240, 255))
    # 帽尖白色小绒球
    d.ellipse([26, 10, 31, 15], fill=(255, 255, 255, 255), outline=(220, 230, 240, 255))
    im.save(COSTUMES_DIR / "hat_xmas.png")

    # 10. 魔法巫师帽 (hat_wizard.png) 32x26
    im = Image.new("RGBA", (32, 26), (0,0,0,0))
    d = ImageDraw.Draw(im)
    # 宽帽檐
    d.ellipse([2, 18, 30, 25], fill=(60, 40, 110, 255), outline=(180, 140, 255, 255))
    # 高锥顶
    d.polygon([(8, 20), (16, 2), (24, 20)], fill=(75, 50, 135, 255))
    # 金色星月腰带
    d.rectangle([8, 16, 24, 19], fill=(255, 215, 0, 255))
    # 金星闪烁
    d.ellipse([14, 8, 18, 12], fill=(255, 240, 100, 255))
    im.save(COSTUMES_DIR / "hat_wizard.png")

    # 11. 七彩飘空气球 (prop_balloon.png) 24x32
    im = Image.new("RGBA", (24, 32), (0,0,0,0))
    d = ImageDraw.Draw(im)
    # 气球绳
    d.line([(12, 16), (8, 24), (12, 30)], fill=(200, 200, 200, 255), width=1)
    # 鲜红大爱心/圆气球
    d.ellipse([3, 2, 21, 18], fill=(255, 80, 100, 255), outline=(230, 40, 70, 255))
    # 高光
    d.ellipse([6, 4, 10, 8], fill=(255, 200, 210, 255))
    im.save(COSTUMES_DIR / "prop_balloon.png")

    # 12. 晴雨折叠遮阳伞 (prop_umbrella.png) 26x28
    im = Image.new("RGBA", (26, 28), (0,0,0,0))
    d = ImageDraw.Draw(im)
    # 伞柄
    d.line([(13, 10), (13, 24)], fill=(120, 80, 40, 255), width=2)
    d.arc([11, 22, 15, 26], start=0, end=180, fill=(120, 80, 40, 255), width=2)
    # 蓝黄条纹伞面
    d.pieslice([2, 2, 24, 18], start=180, end=360, fill=(50, 150, 255, 255), outline=(30, 100, 200, 255))
    d.polygon([(10, 3), (16, 3), (18, 10), (8, 10)], fill=(255, 215, 0, 255))
    im.save(COSTUMES_DIR / "prop_umbrella.png")

    print("All 12 costumes generated successfully!")

# 3. 睡觉与打呼噜动画序列 (sleep)
def generate_sleep_animations():
    print("Generating sleep & snoring animation frames...")
    stages = [("Egg", 24), ("Kid", 28), ("Adult", 34)]
    genders = ["GG", "MM"]

    for gender in genders:
        for stage_name, r in stages:
            out_dir = ASSETS_DIR / gender / stage_name / "sleep"
            out_dir.mkdir(parents=True, exist_ok=True)

            # 生成 10 帧循环深睡冒泡动画
            for f in range(10):
                im = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
                d = ImageDraw.Draw(im)

                # 企鹅身体卧姿 (微扁呼气吸气)
                breath_y = 1 if f in [3, 4, 5, 6] else 0
                body_cy = 54 + breath_y
                body_color = (35, 45, 60, 255) if gender == "GG" else (50, 40, 65, 255)
                belly_color = (255, 255, 255, 255)

                # 卧倒身体与白肚皮
                d.ellipse([48 - r, body_cy - r + 4, 48 + r, body_cy + r], fill=body_color)
                d.ellipse([48 - int(r*0.65), body_cy - int(r*0.5) + 4, 48 + int(r*0.65), body_cy + int(r*0.85)], fill=belly_color)

                # 闭眼弯弯笑眼 / 睡觉闭眼线 (zzZ)
                d.arc([42, body_cy - 4, 46, body_cy], start=0, end=180, fill=(30, 30, 30, 255), width=2)
                d.arc([50, body_cy - 4, 54, body_cy], start=0, end=180, fill=(30, 30, 30, 255), width=2)

                # 橙黄色小嘴
                d.polygon([(45, body_cy + 1), (51, body_cy + 1), (48, body_cy + 4)], fill=(255, 140, 0, 255))

                # 戴条纹睡帽
                cap_color = (70, 130, 240, 255) if gender == "GG" else (255, 130, 180, 255)
                d.polygon([(36, body_cy - r + 8), (56, body_cy - r + 8), (62, body_cy - r - 4)], fill=cap_color)
                d.ellipse([60, body_cy - r - 6, 66, body_cy - r], fill=(255, 255, 255, 255)) # 帽顶白球

                # 鼻涕泡泡呼吸变化 (f 0~9 逐渐变大然后破裂)
                if f < 8:
                    bubble_r = 2 + f
                    bx = 52 + int(f * 0.8)
                    by = body_cy - 2 - int(f * 0.5)
                    d.ellipse([bx - bubble_r, by - bubble_r, bx + bubble_r, by + bubble_r], 
                              fill=(200, 230, 255, 160), outline=(130, 180, 255, 220))
                    d.ellipse([bx - int(bubble_r*0.5), by - int(bubble_r*0.5), bx - int(bubble_r*0.2), by - int(bubble_r*0.2)],
                              fill=(255, 255, 255, 240))
                else: # 泡泡破裂星星微光
                    bx = 58
                    by = body_cy - 6
                    d.line([(bx-4, by), (bx+4, by)], fill=(180, 220, 255, 200), width=1)
                    d.line([(bx, by-4), (bx, by+4)], fill=(180, 220, 255, 200), width=1)

                # "Z z Z" 浮动文字
                z_offset = (f * 2) % 12
                d.text((64, body_cy - 18 - z_offset), "z", fill=(100, 150, 240, 200))
                d.text((70, body_cy - 26 - z_offset), "Z", fill=(70, 120, 220, 240))

                im.save(out_dir / f"f_{f:02d}.png")

    print("Sleep animation frames generated successfully!")

if __name__ == "__main__":
    process_audio()
    generate_expanded_costumes()
    generate_sleep_animations()
