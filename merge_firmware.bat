@echo off
chcp 65001 >nul
echo ========================================================
echo        QQPet-StickS3 8MB 全量固件合并工具 (含 LittleFS 素材)
echo ========================================================

set BOOTLOADER=.pio\build\m5stack-sticks3\bootloader.bin
set PARTITIONS=.pio\build\m5stack-sticks3\partitions.bin
set BOOT_APP0=%USERPROFILE%\.platformio\packages\framework-arduinoespressif32\tools\partitions\boot_app0.bin
set FIRMWARE=.pio\build\m5stack-sticks3\firmware.bin
set LITTLEFS=.pio\build\m5stack-sticks3\littlefs.bin
set OUTPUT_BIN=QQpet-StickS3-Merged-8MB.bin

set PIO_PY=%USERPROFILE%\.platformio\penv\Scripts\python.exe
set ESPTOOL_PY=%USERPROFILE%\.platformio\packages\tool-esptoolpy\esptool.py

if not exist "%BOOTLOADER%" (
    echo [错误] 未找到固件编译文件，请先运行: pio run
    exit /b 1
)

if not exist "%LITTLEFS%" (
    echo [错误] 未找到 LittleFS 素材镜像，请先运行: pio run --target buildfs
    exit /b 1
)

echo 正在合并生成完整 8MB 一键烧录固件: %OUTPUT_BIN% ...

"%PIO_PY%" "%ESPTOOL_PY%" --chip esp32s3 merge_bin -o "%OUTPUT_BIN%" --flash_mode dio --flash_size 8MB 0x0000 "%BOOTLOADER%" 0x8000 "%PARTITIONS%" 0xe000 "%BOOT_APP0%" 0x10000 "%FIRMWARE%" 0x290000 "%LITTLEFS%"

if %ERRORLEVEL% equ 0 (
    echo.
    echo ========================================================
    echo  [成功] 8MB 全量一体化固件已生成: %OUTPUT_BIN%
    echo  可直接在 0x0 偏移处一键烧录整颗芯片！
    echo ========================================================
) else (
    echo.
    echo [失败] 固件合并失败，请检查 PlatformIO 环境。
)