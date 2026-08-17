#include "ai_client.h"
#include "pet_core.h"
#include "display_engine.h"
#include "game_data.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

DeepSeekAiClient g_ai;

DeepSeekAiClient::DeepSeekAiClient() : isRequesting(false), lastRequestTime(0) {}

void DeepSeekAiClient::begin() {}

struct AiTaskParams {
    String context;
    String apiKey;
    String prompt;
};

static void aiRequestTask(void* parameter) {
    AiTaskParams* params = static_cast<AiTaskParams*>(parameter);
    
    bool success = false;
    String replyText = "";

    if (WiFi.status() == WL_CONNECTED && params->apiKey.length() > 5) {
        WiFiClientSecure client;
        client.setInsecure(); // 简化嵌入式 SSL 握手
        client.setTimeout(6000);

        HTTPClient https;
        if (https.begin(client, "https://api.deepseek.com/chat/completions")) {
            https.addHeader("Content-Type", "application/json");
            https.addHeader("Authorization", "Bearer " + params->apiKey);

            StaticJsonDocument<768> reqDoc;
            reqDoc["model"] = "deepseek-chat";
            reqDoc["max_tokens"] = 60;
            reqDoc["temperature"] = 0.8;

            JsonArray messages = reqDoc.createNestedArray("messages");
            
            JsonObject sysMsg = messages.createNestedObject();
            sysMsg["role"] = "system";
            sysMsg["content"] = "你是QQ宠物企鹅，请用可爱、简短的一句话(20字以内)对主人说话，可带拟声词或Emoji。";

            JsonObject userMsg = messages.createNestedObject();
            userMsg["role"] = "user";
            userMsg["content"] = params->prompt;

            String reqPayload;
            serializeJson(reqDoc, reqPayload);

            int httpCode = https.POST(reqPayload);
            if (httpCode == HTTP_CODE_OK) {
                String resPayload = https.getString();
                DynamicJsonDocument resDoc(1024);
                DeserializationError err = deserializeJson(resDoc, resPayload);
                if (!err) {
                    const char* content = resDoc["choices"][0]["message"]["content"];
                    if (content && strlen(content) > 0) {
                        replyText = String(content);
                        replyText.trim();
                        success = true;
                    }
                }
            }
            https.end();
        }
    }

    if (!success) {
        // 回退到原版经典硬编码台词
        replyText = getRandomClassicQuote(params->context.c_str());
    }

    g_display.showBubble(replyText, 5500);

    delete params;
    vTaskDelete(NULL);
}

void DeepSeekAiClient::requestDialog(const char* sceneContext) {
    if (isRequesting && millis() - lastRequestTime < 8000) return;
    lastRequestTime = millis();

    const PetState& st = g_pet.getState();

    // 如果未配置 Key 或未连网，直接秒回本地台词
    if (strlen(st.deepseek_key) < 5 || WiFi.status() != WL_CONNECTED) {
        const char* quote = getRandomClassicQuote(sceneContext);
        g_display.showBubble(quote, 5000);
        return;
    }

    AiTaskParams* params = new AiTaskParams();
    params->context = sceneContext;
    params->apiKey = st.deepseek_key;

    // 组装宠物实时状态 Prompt 上下文
    String prompt = "我的名字是" + String(st.name) + "，主人叫" + String(st.host) + "。";
    prompt += "当前等级: " + String(g_pet.getLevel()) + "级。";
    prompt += "状态: 饥饿值" + String(st.hunger) + "/" + String(g_pet.getMaxHunger()) + "，";
    prompt += "清洁值" + String(st.clean) + "/" + String(g_pet.getMaxClean()) + "，";
    prompt += "心情值" + String(st.mood) + "/1000。";

    if (g_pet.isSick()) {
        prompt += "我当前生病了: " + String(st.illness) + "。";
    } else if (g_pet.isDead()) {
        prompt += "我已经死亡了，需要还魂丹救我。";
    }

    if (strcmp(sceneContext, "hungry") == 0) prompt += "我现在肚子很饿想吃东西！";
    else if (strcmp(sceneContext, "dirty") == 0) prompt += "我现在身上很脏想洗澡！";
    else if (strcmp(sceneContext, "happy") == 0) prompt += "主人正在跟我摇晃玩耍，我非常开心！";
    else if (strcmp(sceneContext, "levelup") == 0) prompt += "我刚刚升级了！向主人炫耀一下！";
    else prompt += "日常闲聊互动。";

    params->prompt = prompt;

    // 启动 FreeRTOS 异步线程，不卡顿主屏动画
    xTaskCreatePinnedToCore(aiRequestTask, "ai_task", 4096, params, 1, NULL, 0);
}

void DeepSeekAiClient::update() {}
