#include "sound_manager.h"
#include <LittleFS.h>

SoundManager& g_sound = SoundManager::getInstance();

SoundManager::SoundManager() : currentVolume(128), isEnabled(true) {}

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
            uint8_t* buf = (uint8_t*)malloc(sz);
            if (buf) {
                f.read(buf, sz);
                f.close();
                M5.Speaker.playWav(buf, sz);
                free(buf);
            } else {
                f.close();
            }
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
