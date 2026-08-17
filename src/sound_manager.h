#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include <M5Unified.h>

enum SoundType {
    SOUND_NONE = 0,
    SOUND_CLICK,
    SOUND_EAT,
    SOUND_CLEAN,
    SOUND_LEVELUP,
    SOUND_WORK,
    SOUND_STUDY,
    SOUND_SNORE,
    SOUND_HAPPY,
    SOUND_COIN,
    SOUND_SICK
};

class SoundManager {
public:
    static SoundManager& getInstance() {
        static SoundManager instance;
        return instance;
    }

    void begin();
    void playSound(SoundType type);
    void playWav(const char* filename);
    void setVolume(uint8_t vol);
    uint8_t getVolume() const { return currentVolume; }
    void stop();

private:
    SoundManager();
    uint8_t currentVolume;
    bool isEnabled;
};

extern SoundManager& g_sound;
