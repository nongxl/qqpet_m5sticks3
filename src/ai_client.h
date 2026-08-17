#pragma once
#include <Arduino.h>

class DeepSeekAiClient {
public:
    DeepSeekAiClient();
    void begin();
    void requestDialog(const char* sceneContext = "idle");
    void update();

private:
    bool isRequesting;
    uint32_t lastRequestTime;
    
    void fetchAiResponseAsync(const String& context);
};

extern DeepSeekAiClient g_ai;
