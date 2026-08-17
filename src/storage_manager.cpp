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
    size_t readLen = prefs.getBytes("state", &state, sizeof(PetState));
    return readLen == sizeof(PetState);
}

void StorageManager::saveWifiConfig(const char* ssid, const char* pwd) {
    prefs.putString("wifi_ssid", ssid);
    prefs.putString("wifi_pwd", pwd);
}

void StorageManager::saveAiConfig(const char* apiKey) {
    prefs.putString("ds_key", apiKey);
}
