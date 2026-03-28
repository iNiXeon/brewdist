#include "AppMenus.h"
#include "TGClient.h"
#include "GlobalVars.h" 

extern Config config;
extern State state;

String DistillMenu::getKeyboard() {
    String selectText = config.isSelectionActive ? "🛑 Стоп отбор" : "🚀 Начать отбор";
    return "{\"inline_keyboard\":["
           "[{\"text\":\"🌡 Цель Куб\",\"callback_data\":\"/set_target\"},{\"text\":\"🍺 Затирание\",\"callback_data\":\"/m_menu\"}],"
           "[{\"text\":\"" + selectText + "\",\"callback_data\":\"/toggle_select\"},"
           "{\"text\":\"⚙️ Гист. " + String(config.hysteresis, 2) + "\",\"callback_data\":\"/set_hyst\"}],"
           "[{\"text\":\"🔄 Статус\",\"callback_data\":\"/status\"}]]}";
}

String DistillMenu::getSpecificStatusText() {
    String statusText = "";
    if (state.isWaitForHeat) {
        statusText += "\n" + String(state.isHeating ? "🔥 *НАГРЕВ" : "❄️ *ОХЛАЖДЕНИЕ") + " КУБА ДО* `" + String(config.targetCubeTemp, 1) + "°C`";
        statusText += "\n";
    }

    if (config.isSelectionActive) {
        statusText += "\n🟢 *ИДЕТ ОТБОР*\n🌡 База: `" + String(config.baseSelectionTemp, 1) + "°C`\n🛡 Гист. (Т2): `" + String(config.hysteresis, 2) + "°C`";
        if (state.hysteresisReached) statusText += "\n⚠️ *СТОП ОТБОР (ГИСТЕРЕЗИС)*";
    }

    if (!state.isWaitForHeat && !config.isSelectionActive) {
        statusText += "\n⚪️ *РЕЖИМ ОЖИДАНИЯ*";
    }
    return statusText;
}

void DistillMenu::processSpecificCallback(const String &data) {
    if (data == "/set_hyst") {
        resetInputState();
        state.waitingForHysteresis = true;
        requestTgUpdate(true);
    } else if (data == "/set_target") {
        resetInputState();
        state.waitingForTargetT = true;
        requestTgUpdate(true);
    } else if (data == "/toggle_select") {
        if (!config.isSelectionActive) {
            config.baseSelectionTemp = state.currentT2;
            config.isSelectionActive = true;
            state.hysteresisReached = false;
            TGClient::sendAlert("🚀 Отбор запущен! База: " + String(config.baseSelectionTemp, 1) + "°C");
        } else {
            config.isSelectionActive = false;
            TGClient::sendAlert("🛑 Отбор остановлен.");
        }
        saveSettings();
        requestTgUpdate(true);
    }
}

String MashMenu::getKeyboard() {
    String startStop = state.mashActive ? "🛑 Стоп" : "▶️ Старт";
    String cmd = state.mashActive ? "/m_stop" : "/m_start";
    return "{\"inline_keyboard\":["
           "[{\"text\":\"" + startStop + "\", \"callback_data\":\"" + cmd + "\"},"
           "{\"text\":\"➕ Добавить паузу\", \"callback_data\":\"/m_add\"}],"
           "[{\"text\":\"🧹 Очистить список\", \"callback_data\":\"/m_clear\"}],"
           "[{\"text\":\"⬅️ Назад\", \"callback_data\":\"/status\"}]]}";
}

String MashMenu::getSpecificStatusText() {
    String statusText = "🍺 *РЕЖИМ ЗАТИРАНИЯ*\n";
    if (state.pauses.empty()) {
        statusText += "_Список пауз пуст._\n";
    } else {
        for (size_t i = 0; i < state.pauses.size(); i++) {
            statusText += (i == (size_t)state.currentPauseIdx ? "▶️ " : "▪️ ") + 
                          String(i + 1) + ". " + String(state.pauses[i].temp, 1) + 
                          "°C (" + String(state.pauses[i].duration) + " мин)\n";
        }
    }

    if (state.mashActive && state.currentPauseIdx != -1) {
        if (state.pauseTimer == 0) {
            statusText += "\n📈 *Нагрев до паузы " + String(state.currentPauseIdx + 1) + "...*\n📍 Цель: " + String(state.pauses[state.currentPauseIdx].temp, 1) + "°C";
        } else {
            int left = state.pauses[state.currentPauseIdx].duration - ((millis() - state.pauseTimer) / 60000);
            statusText += "\n⏳ *Пауза " + String(state.currentPauseIdx + 1) + " активна*\nОсталось: " + String(left) + " мин";
        }
    }
    return statusText;
}

void MashMenu::processSpecificCallback(const String &data) {
    if (data == "/m_add") {
        resetInputState();
        state.waitingForPauseTemp = true;
        requestTgUpdate(true);
    } else if (data == "/m_clear") {
        state.pauses.clear();
        state.mashActive = false;
        state.currentPauseIdx = -1;
        TGClient::sendAlert("🗑 Список очищен.");
        requestTgUpdate(true);
    } else if (data == "/m_stop") {
        state.mashActive = false;
        state.currentPauseIdx = -1;
        state.pauseTimer = 0;
        TGClient::sendAlert("🛑 Затирание остановлено.");
        requestTgUpdate(true);
    } else if (data == "/m_start") {
        if (state.pauses.empty()) {
            TGClient::sendAlert("❌ Список пуст!");
        } else {
            state.mashActive = true;
            state.currentPauseIdx = 0;
            state.pauseTimer = 0;
            state.warningSent = false;
            TGClient::sendAlert("🚀 Затирание запущено!");
            requestTgUpdate(true);
        }
    }
}