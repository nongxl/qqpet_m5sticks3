#include "sound_manager.h"
#include <LittleFS.h>

SoundManager& g_sound = SoundManager::getInstance();

// 专用常驻音频缓冲区 (在 PSRAM 或系统静态内存中，最大 48KB)
static uint8_t s_audioBuffer[48 * 1024];
static size_t s_currentAudioSize = 0;

SoundManager::SoundManager() : currentVolume(120), isEnabled(true) {}

void SoundManager::begin() {
    M5.Speaker.begin();
    M5.Speaker.setVolume(currentVolume);
}

void SoundManager::setVolume(uint8_t vol) {
    currentVolume = vol;
    M5.Speaker.setVolume(currentVolume);
}

void SoundManager::stop() {
    M5.Speaker.stop();
}

void SoundManager::playWav(const char* filename) {
    if (!isEnabled || currentVolume == 0 || !filename) return;

    String path = String("/assets/sounds/") + filename;
    if (LittleFS.exists(path)) {
        File f = LittleFS.open(path, "r");
        if (f) {
            size_t sz = f.size();
            if (sz > sizeof(s_audioBuffer)) {
                sz = sizeof(s_audioBuffer);
            }
            // 停止当前正在播放的声音，等待通道复位
            M5.Speaker.stop();
            
            f.read(s_audioBuffer, sz);
            f.close();
            s_currentAudioSize = sz;
            
            // 播放 WAV 数据：使用常驻缓冲区，设置 stop_current_sound = true
            M5.Speaker.playWav(s_audioBuffer, s_currentAudioSize, 1, 0, true);
        }
    }
}

void SoundManager::playSound(SoundType type) {
    switch (type) {
        case SOUND_CLICK:
            playWav("click.wav");
            break;
        case SOUND_EAT:
            playWav("eat.wav");
            break;
        case SOUND_CLEAN:
            playWav("clean.wav");
            break;
        case SOUND_LEVELUP:
            playWav("levelup.wav");
            break;
        case SOUND_WORK:
            playWav("work.wav");
            break;
        case SOUND_STUDY:
            playWav("study.wav");
            break;
        case SOUND_SNORE:
            playWav("snore.wav");
            break;
        case SOUND_HAPPY:
            playWav("happy.wav");
            break;
        case SOUND_COIN:
            playWav("coin.wav");
            break;
        case SOUND_SICK:
            playWav("sick.wav");
            break;
        default:
            break;
    }
}
