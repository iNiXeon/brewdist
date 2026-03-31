#include "TelegramManager.h"
#include "TGClient.h"
#include "GlobalVars.h"

extern Config config;
extern State state;

MenuBase* TelegramManager::getActiveMenu() {
    if (state.heaterMode) return (MenuBase *)&heaterMenu;
    return state.mashMode ? (MenuBase *)&mashMenu : (MenuBase *)&distillMenu;
}

void TelegramManager::updateTelegram(bool force) {
    if (!force && (millis() - lastAutoSend < 30000)) return;

    MenuBase *currentMenu = getActiveMenu();
    String statusText = currentMenu->generateStatusText();

    if (config.mainMonitorMsgId == 0) {
        String r = TGClient::sendRequest("sendMessage", currentMenu->buildMessageJson(statusText));
        if (r.indexOf("\"ok\":true") != -1) {
            JsonDocument rDoc;
            deserializeJson(rDoc, r);
            config.mainMonitorMsgId = rDoc["result"]["message_id"];
            saveSettings();
        }
    } else {
        String r = TGClient::sendRequest("editMessageText", currentMenu->buildMessageJson(statusText, config.mainMonitorMsgId));
        // Если сообщение было удалено вручную в чате, сбрасываем ID, чтобы создать новое
        if (r.indexOf("\"ok\":false") != -1 && r.indexOf("message not found") != -1) {
            config.mainMonitorMsgId = 0;
            saveSettings();
        }
    }
    lastAutoSend = millis();
}

void TelegramManager::handleUpdates() {
    String offsetValue = (state.lastUpdateId <= 0) ? "-1" : String(state.lastUpdateId + 1);
    String resp = TGClient::sendRequest("getUpdates", "{\"offset\":" + offsetValue + ", \"timeout\":10}", 15);

    if (resp.indexOf("\"ok\":true") == -1) return;

    JsonDocument doc;
    if (deserializeJson(doc, resp)) return;

    JsonArray results = doc["result"].as<JsonArray>();
    if (results.size() == 0 && state.lastUpdateId <= 0) {
        state.lastUpdateId = 1;
        return;
    }

    for (JsonObject item : results) {
        state.lastUpdateId = item["update_id"].as<long>();
        
        if (item["message"]) {
            processMessage(item["message"]["text"].as<String>());
        }
        
        if (item["callback_query"]) {
            String data = item["callback_query"]["data"].as<String>();
            int msgId = item["callback_query"]["message"]["message_id"].as<int>();
            getActiveMenu()->processCallback(data, msgId);
        }
    }
}

void TelegramManager::processMessage(const String &text) {
    if (text == "/start" || text == "/reset") {
        config.mainMonitorMsgId = 0;
        updateTelegram(true);
    } 
    else if (text == "0") {
        config.targetCubeTemp = 0;
        state.isWaitForHeat = false;
        TGClient::sendAlert("🔄 Нагрев остановлен, цели сброшены");
        updateTelegram(true);
    }
}