#include "web_server_portal.h"
#include "pet_core.h"
#include "storage_manager.h"
#include "display_engine.h"
#include "haptics.h"
#include <ArduinoJson.h>

WebServerPortal g_webPortal;

static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>QQ 宠物管家 - M5StickS3 控制台</title>
    <style>
        :root {
            --primary: #1890ff;
            --primary-hover: #40a9ff;
            --bg: #f0f5ff;
            --card: #ffffff;
            --text: #2c3e50;
            --subtext: #8c8c8c;
            --border: #e8e8e8;
            --success: #52c41a;
            --warning: #faad14;
            --danger: #ff4d4f;
        }
        * { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "PingFang SC", sans-serif; }
        body { background: var(--bg); color: var(--text); padding: 16px; display: flex; justify-content: center; }
        .container { width: 100%; max-width: 600px; display: flex; flex-direction: column; gap: 16px; }
        .header { background: linear-gradient(135deg, #1890ff, #096dd9); color: white; padding: 20px; border-radius: 16px; box-shadow: 0 4px 12px rgba(24,144,255,0.25); text-align: center; }
        .header h1 { font-size: 20px; font-weight: 600; margin-bottom: 4px; }
        .card { background: var(--card); border-radius: 14px; padding: 18px; box-shadow: 0 2px 8px rgba(0,0,0,0.04); }
        .card-title { font-size: 15px; font-weight: 600; color: #1f1f1f; margin-bottom: 14px; display: flex; justify-content: space-between; align-items: center; }
        .stat-group { display: flex; flex-direction: column; gap: 12px; }
        .stat-item { display: flex; flex-direction: column; gap: 4px; }
        .stat-header { display: flex; justify-content: space-between; font-size: 13px; font-weight: 500; }
        .progress-bg { background: #f0f0f0; border-radius: 6px; height: 10px; overflow: hidden; }
        .progress-bar { height: 100%; transition: width 0.3s ease; border-radius: 6px; }
        .btn-grid { display: grid; grid-template-columns: repeat(2, 1fr); gap: 10px; }
        .btn { background: var(--primary); color: white; border: none; padding: 12px; border-radius: 10px; font-size: 14px; font-weight: 500; cursor: pointer; transition: all 0.2s; }
        .btn:hover { background: var(--primary-hover); transform: translateY(-1px); }
        .btn-work { background: #fa8c16; }
        .btn-heal { background: var(--success); }
        .btn-shop { background: #722ed1; }
        .shop-grid { display: grid; grid-template-columns: repeat(2, 1fr); gap: 8px; }
        .shop-item { border: 1px solid var(--border); border-radius: 8px; padding: 10px; display: flex; justify-content: space-between; align-items: center; }
        .shop-btn { padding: 6px 12px; background: #722ed1; color: white; border: none; border-radius: 6px; cursor: pointer; font-size: 12px; }
        .form-group { display: flex; flex-direction: column; gap: 8px; margin-bottom: 12px; }
        .form-group label { font-size: 13px; color: var(--text); font-weight: 500; }
        .form-input { padding: 10px 12px; border: 1px solid var(--border); border-radius: 8px; font-size: 14px; outline: none; }
        .badge { font-size: 11px; padding: 2px 8px; border-radius: 10px; color: white; }
    </style>
</head>
<body>
<div class="container">
    <div class="header">
        <h1 id="pet-name">QQ 宠物 · Q宝</h1>
        <p id="pet-sub" style="font-size:12px; opacity:0.9;">主人：主人 | Lv.1 | 💰 500 元宝</p>
    </div>

    <!-- 状态监控 -->
    <div class="card">
        <div class="card-title">
            <span>🐾 宠物属性与资产</span>
            <span id="health-badge" class="badge" style="background:var(--success)">健康</span>
        </div>
        <div class="stat-group">
            <div class="stat-item">
                <div class="stat-header"><span>🍗 饥饿度</span><span id="hunger-val">0/0</span></div>
                <div class="progress-bg"><div id="hunger-bar" class="progress-bar" style="background:#ff9c6e; width:0%"></div></div>
            </div>
            <div class="stat-item">
                <div class="stat-header"><span>🧼 清洁度</span><span id="clean-val">0/0</span></div>
                <div class="progress-bg"><div id="clean-bar" class="progress-bar" style="background:#69c0ff; width:0%"></div></div>
            </div>
            <div class="stat-item">
                <div class="stat-header"><span>💖 心情值</span><span id="mood-val">0/1000</span></div>
                <div class="progress-bg"><div id="mood-bar" class="progress-bar" style="background:#ff85c0; width:0%"></div></div>
            </div>
            <div class="stat-item">
                <div class="stat-header"><span>⭐ 成长值 / 元宝</span><span id="growth-val">0 / 💰 500</span></div>
            </div>
        </div>
    </div>

    <!-- 快捷互动与打工 -->
    <div class="card">
        <div class="card-title">⚡ 一键养护与打工</div>
        <div class="btn-grid">
            <button class="btn" onclick="doAction('feed')">🍖 喂食 (+1000)</button>
            <button class="btn" onclick="doAction('bath')">🧼 洗澡 (+1000)</button>
            <button class="btn" onclick="doAction('play')">🎮 逗玩 (+150)</button>
            <button class="btn btn-work" onclick="doWork()">⚒️ 打工搬砖 (+150元宝)</button>
            <button class="btn" style="background:#2f54eb" onclick="doStudy()">📚 认真自习 (+智力/经验)</button>
            <button class="btn" style="background:#13c2c2" onclick="doTrip()">✈️ 背包旅行 (-100Y/满心情)</button>
            <button class="btn btn-heal" onclick="doAction('heal')" style="grid-column: span 2;">💊 一键对症吃药/复活</button>
        </div>
    </div>


    <!-- 元宝道具商城 -->
    <div class="card">
        <div class="card-title">🛒 道具商城 (消耗打工元宝)</div>
        <div class="shop-grid">
            <div class="shop-item">
                <div><b>美味小鱼</b><br><small style="color:#fa8c16">20 元宝</small></div>
                <button class="shop-btn" onclick="buyItem('food', 1)">购买</button>
            </div>
            <div class="shop-item">
                <div><b>泡泡香皂</b><br><small style="color:#fa8c16">20 元宝</small></div>
                <button class="shop-btn" onclick="buyItem('soap', 1)">购买</button>
            </div>
            <div class="shop-item">
                <div><b>板蓝根(感冒)</b><br><small style="color:#fa8c16">50 元宝</small></div>
                <button class="shop-btn" onclick="buyItem('10001', 1)">购买</button>
            </div>
            <div class="shop-item">
                <div><b>消食片(胀气)</b><br><small style="color:#fa8c16">50 元宝</small></div>
                <button class="shop-btn" onclick="buyItem('10002', 1)">购买</button>
            </div>
            <div class="shop-item">
                <div><b>枇杷糖浆(咳嗽)</b><br><small style="color:#fa8c16">50 元宝</small></div>
                <button class="shop-btn" onclick="buyItem('10003', 1)">购买</button>
            </div>
            <div class="shop-item">
                <div><b>还魂丹(复活)</b><br><small style="color:#fa8c16">300 元宝</small></div>
                <button class="shop-btn" onclick="buyItem('60001', 1)">购买</button>
            </div>
        </div>
    </div>

    <!-- 壁纸与装扮选择 -->
    <div class="card">
        <div class="card-title">🖼️ 企鹅领养身份与 16 款原版壁纸</div>
        <div class="form-group">
            <label>当前领养性别 (专属陪伴终生绑定)</label>
            <div id="disp-gender" style="font-size:14px; font-weight:bold; color:var(--primary); padding:8px 0;">👦 GG (帅哥男鹅)</div>
        </div>
        <div class="form-group">
            <label>选择桌面壁纸 (1~16 款原版壁纸 / 0 为经典极简)</label>

            <select id="sel-bg" class="form-input" onchange="setBg(this.value)">
                <option value="0">0 - 极简天蓝纯色</option>
                <option value="1">1 - 经典阳光草地</option>
                <option value="2">2 - 清新森林小道</option>
                <option value="3">3 - 暖阳浪漫海滩</option>
                <option value="4">4 - 静谧夜幕星空</option>
                <option value="5">5 - 温馨企鹅客厅</option>
                <option value="6">6 - 冰雪梦幻冰屋</option>
                <option value="7">7 - 金秋落叶枫林</option>
                <option value="8">8 - 梦幻童话乐园</option>
                <option value="9">9 - 蔚蓝深海世界</option>
                <option value="10">10 - 樱花飞舞庭院</option>
                <option value="11">11 - 奇幻魔法城堡</option>
                <option value="12">12 - 悠然农场庄园</option>
                <option value="13">13 - 浩瀚外太空星云</option>
                <option value="14">14 - 圣诞暖冬雪景</option>
                <option value="15">15 - 中国风新春庭阁</option>
                <option value="16">16 - 摩登都市天际线</option>
            </select>
        </div>
    </div>


    <!-- 系统配置 -->
    <div class="card">
        <div class="card-title">⚙️ 系统与 Wi-Fi 配网设置</div>
        <div class="form-group">
            <label>Wi-Fi SSID</label>
            <input id="cfg-ssid" class="form-input" placeholder="输入无线网络名称">
        </div>
        <div class="form-group">
            <label>Wi-Fi 密码</label>
            <input id="cfg-pwd" type="password" class="form-input" placeholder="无线密码">
        </div>
        <div class="form-group">
            <label>DeepSeek API Key</label>
            <input id="cfg-key" type="password" class="form-input" placeholder="sk-...">
        </div>
        <button class="btn" style="width:100%" onclick="saveConfig()">💾 保存配置并重启 Wi-Fi</button>
    </div>
</div>

<script>
function refresh() {
    fetch('/api/status').then(r => r.json()).then(data => {
        document.getElementById('pet-name').innerText = 'QQ 宠物 · ' + data.name + (data.gender == 1 ? ' (MM 妹子)' : ' (GG 帅哥)');
        document.getElementById('pet-sub').innerText = '主人: ' + data.host + ' | Lv.' + data.level + ' | 💰 ' + data.coins + ' 元宝';
        document.getElementById('hunger-val').innerText = data.hunger + '/' + data.max_hunger;
        document.getElementById('hunger-bar').style.width = (data.hunger / data.max_hunger * 100) + '%';
        document.getElementById('clean-val').innerText = data.clean + '/' + data.max_clean;
        document.getElementById('clean-bar').style.width = (data.clean / data.max_clean * 100) + '%';
        document.getElementById('mood-val').innerText = data.mood + '/1000';
        document.getElementById('mood-bar').style.width = (data.mood / 10) + '%';
        document.getElementById('growth-val').innerText = data.growth + ' (下一级: ' + data.next_growth + ') | 💰 ' + data.coins + ' 元宝';
        
        let badge = document.getElementById('health-badge');
        if (data.health == 0) {
            badge.innerText = '已死亡'; badge.style.background = 'var(--danger)';
        } else if (data.illness && data.illness.length > 0) {
            badge.innerText = '生病: ' + data.illness; badge.style.background = 'var(--warning)';
        } else {
            badge.innerText = '健康'; badge.style.background = 'var(--success)';
        }
        if (document.getElementById('sel-bg')) {
            document.getElementById('sel-bg').value = data.bg_id || 0;
        }
        if (document.getElementById('disp-gender')) {
            document.getElementById('disp-gender').innerText = (data.gender == 1) ? '👧 MM (甜美女企鹅 · 终生绑定)' : '👦 GG (帅气男企鹅 · 终生绑定)';
        }
    });
}

function resetAdoption() {
    if (!confirm('确定要重新开启领养仪式并选择新萌宠吗？')) return;
    fetch('/api/reset_adoption').then(r => r.json()).then(res => {
        alert(res.msg);
        refresh();
    });
}

function doAction(act) {
    fetch('/api/action?type=' + act).then(r => r.json()).then(res => {
        alert(res.msg);
        refresh();
    });
}



function doWork() {
    fetch('/api/work').then(r => r.json()).then(res => {
        alert(res.msg);
        refresh();
    });
}

function doStudy() {
    fetch('/api/study').then(r => r.json()).then(res => {
        alert(res.msg);
        refresh();
    });
}

function doTrip() {
    fetch('/api/trip').then(r => r.json()).then(res => {
        alert(res.msg);
        refresh();
    });
}


function buyItem(item, count) {
    fetch('/api/shop/buy?item=' + item + '&count=' + count).then(r => r.json()).then(res => {
        alert(res.msg);
        refresh();
    });
}

function setBg(bgId) {
    fetch('/api/set_bg?id=' + bgId).then(r => r.json()).then(res => {
        refresh();
    });
}

function saveConfig() {
    let payload = {
        ssid: document.getElementById('cfg-ssid').value,
        pwd: document.getElementById('cfg-pwd').value,
        key: document.getElementById('cfg-key').value
    };
    fetch('/api/config', {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify(payload)
    }).then(r => r.json()).then(res => {
        alert(res.msg);
    });
}

refresh();
setInterval(refresh, 3000);
</script>
</body>
</html>
)rawliteral";

WebServerPortal::WebServerPortal() : server(80) {}

void WebServerPortal::begin() {
    // 根路径
    server.on("/", HTTP_GET, [this]() {
        server.send_P(200, "text/html", INDEX_HTML);
    });

    // 状态 API
    server.on("/api/status", HTTP_GET, [this]() {
        const PetState& st = g_pet.getState();
        int minG = 0, nextG = 0;
        int level = calculateLevel(st.growth, minG, nextG);

        DynamicJsonDocument doc(1024);
        doc["name"] = st.name;
        doc["host"] = st.host;
        doc["gender"] = st.gender;
        doc["level"] = level;
        doc["growth"] = st.growth;
        doc["next_growth"] = nextG;
        doc["hunger"] = st.hunger;
        doc["max_hunger"] = g_pet.getMaxHunger();
        doc["clean"] = st.clean;
        doc["max_clean"] = g_pet.getMaxClean();
        doc["mood"] = st.mood;
        doc["health"] = st.health;
        doc["illness"] = st.illness;
        doc["coins"] = st.coins;
        doc["intellect"] = st.intellect;
        doc["charm"] = st.charm;
        doc["bg_id"] = st.bg_id;
        doc["food_count"] = st.food_count;
        doc["food_salmon"] = st.food_salmon;
        doc["food_icecream"] = st.food_icecream;
        doc["food_feast"] = st.food_feast;
        doc["soap_count"] = st.soap_count;
        doc["revival_count"] = st.revival_count;


        String out;
        serializeJson(doc, out);
        server.send(200, "application/json", out);
    });

    // 打工 API
    server.on("/api/work", HTTP_GET, [this]() {
        String msg;
        bool ok = g_pet.work(msg);
        g_storage.savePetState(g_pet.getState());
        g_display.showToast(msg, 3000);

        DynamicJsonDocument doc(256);
        doc["success"] = ok;
        doc["msg"] = msg;
        String out;
        serializeJson(doc, out);
        server.send(200, "application/json", out);
    });

    // 学习 API
    server.on("/api/study", HTTP_GET, [this]() {
        String msg;
        bool ok = g_pet.study(msg);
        g_storage.savePetState(g_pet.getState());
        g_display.showToast(msg, 3000);

        DynamicJsonDocument doc(256);
        doc["success"] = ok;
        doc["msg"] = msg;
        String out;
        serializeJson(doc, out);
        server.send(200, "application/json", out);
    });

    // 旅游 API
    server.on("/api/trip", HTTP_GET, [this]() {
        String msg;
        bool ok = g_pet.trip(msg);
        g_storage.savePetState(g_pet.getState());
        g_display.showToast(msg, 3000);

        DynamicJsonDocument doc(256);
        doc["success"] = ok;
        doc["msg"] = msg;
        String out;
        serializeJson(doc, out);
        server.send(200, "application/json", out);
    });


    // 道具商城购买 API
    server.on("/api/shop/buy", HTTP_GET, [this]() {
        String item = server.arg("item");
        int count = server.arg("count").toInt();
        if (count <= 0) count = 1;

        String msg;
        bool ok = g_pet.buyItem(item.c_str(), count, msg);
        g_storage.savePetState(g_pet.getState());
        g_display.showToast(msg, 3000);

        DynamicJsonDocument doc(256);
        doc["success"] = ok;
        doc["msg"] = msg;
        String out;
        serializeJson(doc, out);
        server.send(200, "application/json", out);
    });

    // 壁纸切换 API
    server.on("/api/set_bg", HTTP_GET, [this]() {
        int id = server.arg("id").toInt();
        if (id < 0) id = 0;
        if (id > 16) id = 16;

        PetState& st = g_pet.getState();
        st.bg_id = static_cast<uint8_t>(id);
        g_storage.savePetState(st);

        DynamicJsonDocument doc(256);
        doc["success"] = true;
        doc["bg_id"] = st.bg_id;
        String out;
        serializeJson(doc, out);
        server.send(200, "application/json", out);
    });

    // 重置并重新开启领养仪式 API (仅死亡或用户主动重置时允许)
    server.on("/api/reset_adoption", HTTP_GET, [this]() {
        g_pet.resetAdoption();
        g_storage.savePetState(g_pet.getState());

        DynamicJsonDocument doc(256);
        doc["success"] = true;
        doc["msg"] = "已重置宠物状态，设备屏幕已开启全新领养仪式！请在屏幕上选拔新萌宠~";
        String out;
        serializeJson(doc, out);
        server.send(200, "application/json", out);
    });



    // 互动动作 API
    server.on("/api/action", HTTP_GET, [this]() {
        String type = server.arg("type");
        DynamicJsonDocument doc(256);

        if (type == "feed") {
            if (g_pet.feed(1000)) {
                doc["success"] = true;
                doc["msg"] = "喂食成功！饥饿度恢复，成长值 +15！";
                g_display.showToast("美味小鱼！饱食度UP", 2500);
                g_haptics.trigger(HAPTIC_SUCCESS);
            } else {
                doc["success"] = false;
                doc["msg"] = "食物库存不足或宠物已死亡！";
            }
        } else if (type == "bath") {
            if (g_pet.bath(1000)) {
                doc["success"] = true;
                doc["msg"] = "洗澡成功！清洁度恢复，成长值 +15！";
                g_display.showToast("香皂泡泡浴！洁净度UP", 2500);
                g_haptics.trigger(HAPTIC_SUCCESS);
            } else {
                doc["success"] = false;
                doc["msg"] = "香皂库存不足或宠物已死亡！";
            }
        } else if (type == "play") {
            if (g_pet.play(150)) {
                doc["success"] = true;
                doc["msg"] = "逗玩开心！心情值提升，成长值 +10！";
                g_display.showToast("开怀大笑！心情UP", 2500);
                g_haptics.trigger(HAPTIC_SUCCESS);
            } else {
                doc["success"] = false;
                doc["msg"] = "宠物状态不佳或已死亡！";
            }
        } else if (type == "heal") {
            String hMsg;
            bool ok = g_pet.autoHeal(hMsg);
            doc["success"] = ok;
            doc["msg"] = hMsg;
            g_display.showToast(hMsg, 3000);
            if (ok) g_haptics.trigger(HAPTIC_SUCCESS);
        } else {
            doc["success"] = false;
            doc["msg"] = "未知指令";
        }

        g_storage.savePetState(g_pet.getState());
        String out;
        serializeJson(doc, out);
        server.send(200, "application/json", out);
    });

    // 配置保存 API
    server.on("/api/config", HTTP_POST, [this]() {
        String body = server.arg("plain");
        DynamicJsonDocument doc(512);
        deserializeJson(doc, body);

        PetState& st = g_pet.getState();
        if (doc.containsKey("ssid")) strncpy(st.wifi_ssid, doc["ssid"], sizeof(st.wifi_ssid) - 1);
        if (doc.containsKey("pwd")) strncpy(st.wifi_pwd, doc["pwd"], sizeof(st.wifi_pwd) - 1);
        if (doc.containsKey("key")) strncpy(st.deepseek_key, doc["key"], sizeof(st.deepseek_key) - 1);

        g_storage.savePetState(st);

        DynamicJsonDocument res(256);
        res["success"] = true;
        res["msg"] = "配置已保存！";
        String out;
        serializeJson(res, out);
        server.send(200, "application/json", out);
    });

    server.begin();
    Serial.println("[WebPortal] HTTP Server started.");
}

void WebServerPortal::update() {
    server.handleClient();
}
