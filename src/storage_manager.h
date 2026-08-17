#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "pet_models.h"

class StorageManager {
public:
    StorageManager();
    bool begin();
    bool savePetState(const PetState& state);
    bool loadPetState(PetState& state);
    
    // 辅助设置保存
    void saveWifiConfig(const char* ssid, const char* pwd);
    void saveAiConfig(const char* apiKey);

private:
    Preferences prefs;
};

extern StorageManager g_storage;
