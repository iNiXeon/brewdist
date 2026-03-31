#include "MenuBase.h"
#include "TGClient.h"
#include "GlobalVars.h"

extern void requestTgUpdate(bool force);
extern void saveSettings();

extern Config config;
extern State state;

String MenuBase::getNowTimeStr()
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
        return "00:00:00";
    char buffer[10];
    snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    return String(buffer);
}

const char *MenuBase::getNumpadKeyboard()
{
    return "{\"inline_keyboard\":["
           "[{\"text\":\"1\",\"callback_data\":\"/num_1\"},{\"text\":\"2\",\"callback_data\":\"/num_2\"},{\"text\":\"3\",\"callback_data\":\"/num_3\"}],"
           "[{\"text\":\"4\",\"callback_data\":\"/num_4\"},{\"text\":\"5\",\"callback_data\":\"/num_5\"},{\"text\":\"6\",\"callback_data\":\"/num_6\"}],"
           "[{\"text\":\"7\",\"callback_data\":\"/num_7\"},{\"text\":\"8\",\"callback_data\":\"/num_8\"},{\"text\":\"9\",\"callback_data\":\"/num_9\"}],"
           "[{\"text\":\".\",\"callback_data\":\"/num_dot\"},{\"text\":\"0\",\"callback_data\":\"/num_0\"},{\"text\":\"❌\",\"callback_data\":\"/num_clear\"}],"
           "[{\"text\":\"🔙 Отмена\",\"callback_data\":\"/num_cancel\"},{\"text\":\"✅ OK\",\"callback_data\":\"/num_ok\"}]"
           "]}";
}

void MenuBase::resetInputState()
{
    state.waitingForTargetT = false;
    state.waitingForHysteresis = false;
    state.waitingForPauseTemp = false;
    state.waitingForPauseDur = false;
    state.waitingForPower = false;
    state.waitingForNominal = false;
    state.waitingForLoss = false;
    state.inputBuffer = "";
    state.inputBuffer = "";
}

void MenuBase::handleNumpad(const String &data)
{
    String action = data.substring(5);
    if (action == "cancel")
    {
        resetInputState();
        requestTgUpdate(true);
        return;
    }
    if (action == "clear")
    {
        state.inputBuffer = "";
        requestTgUpdate(true);
        return;
    }
    if (action == "dot")
    {
        if (state.inputBuffer.indexOf('.') == -1)
            state.inputBuffer += ".";
        requestTgUpdate(true);
        return;
    }

    if (action == "ok")
    {
        float val = state.inputBuffer.toFloat();
        state.inputBuffer = "";

        if (state.waitingForTargetT)
        {
            config.targetCubeTemp = val;
            if (val == 0)
            {
                state.isWaitForHeat = false;
                TGClient::sendAlert("🔄 Цель куба сброшена");
            }
            else
            {
                state.isHeating = (state.currentT1 < config.targetCubeTemp);
                state.isWaitForHeat = true;
                TGClient::sendAlert((state.isHeating ? "🔥 Нагрев до: " : "❄️ Охлаждение до: ") + String(config.targetCubeTemp, 1) + "°C");
            }
            saveSettings();
        }
        else if (state.waitingForHysteresis)
        {
            config.hysteresis = val;
            TGClient::sendAlert("✅ Гистерезис: " + String(val, 2));
            saveSettings();
        }
        else if (state.waitingForPauseTemp)
        {
            if (val > 0)
            {
                state.tempMashTemp = val;
                state.waitingForPauseTemp = false;
                state.waitingForPauseDur = true;
                requestTgUpdate(true);
                return;
            }
        }
        else if (state.waitingForPauseDur)
        {
            if (val > 0)
            {
                state.pauses.push_back({state.tempMashTemp, (int)val});
                TGClient::sendAlert("✅ Добавлено: " + String(state.tempMashTemp, 1) + "°C на " + String((int)val) + " мин.");
            }
        }
        else if (state.waitingForPower)
        {
            config.targetPowerKW = val;
            syncHeater(); // Тот самый метод из main.ino
            saveSettings();
            TGClient::sendAlert("⚡ Мощность установлена: " + String(val, 2) + " кВт");
        }
        else if (state.waitingForNominal)
        {
            config.heaterPowerWatt = (int)val;
            syncHeater();
            saveSettings();
            TGClient::sendAlert("🔌 Номинал обновлен: " + String((int)val) + " Вт");
        }
        else if (state.waitingForLoss)
        {
            config.powerLossWatt = (int)val;
            syncHeater();
            saveSettings();
            TGClient::sendAlert("📉 Потери обновлены: " + String((int)val) + " Вт");
        }
        resetInputState();
        requestTgUpdate(true);
    }
    else if (state.inputBuffer.length() < 6)
    {
        state.inputBuffer += action;
        requestTgUpdate(true);
    }
}

String MenuBase::generateStatusText()
{
    String statusText;
    statusText.reserve(1024);
    if (state.waitingForTargetT || state.waitingForHysteresis || state.waitingForPauseTemp || state.waitingForPauseDur || state.waitingForPower || state.waitingForNominal || state.waitingForLoss)
    {
        statusText = "🔢 *Ввод значения*\n\n";
        if (state.waitingForTargetT)
            statusText += "🌡 Целевая температура куба (T1)\n";
        else if (state.waitingForHysteresis)
            statusText += "⚙️ Гистерезис (T2)\n";
        else if (state.waitingForPauseTemp)
            statusText += "🍺 Затирание (Шаг 1/2): Температура\n";
        else if (state.waitingForPauseDur)
            statusText += "🍺 Затирание (Шаг 2/2): Время (мин)\n";
        if (state.waitingForPower)
            statusText += "⚡ Текущая мощность (кВт)\n";
        else if (state.waitingForNominal)
            statusText += "🔌 Номинальная мощность ТЭНа (Вт)\n";
        else if (state.waitingForLoss)
            statusText += "📉 Теплопотери системы (Вт)\n";
        statusText += "\nЗначение: `" + (state.inputBuffer.isEmpty() ? "_" : state.inputBuffer) + "`";
        return statusText;
    }

    statusText = "🕒 " + getNowTimeStr() + " (BrewDist)\n────────────────\n";
    statusText += "🔥 Куб (T1): `" + String(state.currentT1, 1) + "°C`\n";
    statusText += "❄️ Выход (T2): `" + String(state.currentT2, 1) + "°C`\n";
    statusText += "────────────────\n\n" + getSpecificStatusText();
    return statusText;
}

String MenuBase::buildMessageJson(const String &text, int msgId)
{
    JsonDocument doc;
    doc["chat_id"] = config.chat_id;
    doc["text"] = text;
    doc["parse_mode"] = "Markdown";
    if (msgId != 0)
        doc["message_id"] = msgId;
    bool isInput = state.waitingForTargetT || state.waitingForHysteresis || state.waitingForPauseTemp || state.waitingForPauseDur ||
                   state.waitingForPower || state.waitingForNominal || state.waitingForLoss;
    doc["reply_markup"] = serialized(isInput ? getNumpadKeyboard() : getKeyboard());
    String out;
    serializeJson(doc, out);
    return out;
}

void MenuBase::processCallback(const String &data, int msgId)
{
    if (data.startsWith("/num_"))
    {
        handleNumpad(data);
        return;
    }
    if (data == "/status")
    {
        state.mashMode = false;
        state.heaterMode = false; // Сбрасываем режим ТЭНа
        requestTgUpdate(true);
        return;
    }
    if (data == "/m_menu")
    {
        state.mashMode = true;
        state.heaterMode = false;
        requestTgUpdate(true);
        return;
    }
    if (data == "/h_menu")
    {
        state.heaterMode = true;
        state.mashMode = false;
        requestTgUpdate(true);
        return;
    }
    if (data == "/delete_msg")
    {
        JsonDocument doc;
        doc["chat_id"] = config.chat_id;
        doc["message_id"] = msgId;
        String body;
        serializeJson(doc, body);
        TGClient::sendRequest("deleteMessage", body);
        return;
    }
    processSpecificCallback(data);
}