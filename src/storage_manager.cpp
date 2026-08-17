#include "storage_manager.h"

StorageManager g_storage;

StorageManager::StorageManager() {}

bool StorageManager::begin() {
    return prefs.begin("qqpet", false);
}

bool StorageManager::savePetState(const PetState& state) {
    size_t written = prefs.putBytes("state", &state, sizeof(PetState));
    return written == sizeof(PetState);
}

bool StorageManager::loadPetState(PetState& state) {
    if (!prefs.isKey("state")) {
        return false;
    }
    size_t storedLen = prefs.getBytesLength("state");
    if (storedLen == 0) return false;

    memset(&state, 0, sizeof(PetState));
    size_t readLen = prefs.getBytes("state", &state, sizeof(PetState));
    
    if (readLen > 0) {
        // 固件结构体升级自适应兼容：当读取旧版数据长度时，自动兜底并升级
        if (readLen < sizeof(PetState)) {
            if (state.food_salmon <= 0) state.food_salmon = 10;
            if (state.food_icecream <= 0) state.food_icecream = 5;
            if (state.food_feast <= 0) state.food_feast = 2;
            if (state.growth > 0) state.is_adopted = true; // 已有经验值则终生保持领养
            savePetState(state); // 自动平滑升级为新版本结构体
        }
        return true;
    }
    return false;
}


void StorageManager::saveWifiConfig(const char* ssid, const char* pwd) {
    prefs.putString("wifi_ssid", ssid);
    prefs.putString("wifi_pwd", pwd);
}

void StorageManager::saveAiConfig(const char* apiKey) {
    prefs.putString("ds_key", apiKey);
}
