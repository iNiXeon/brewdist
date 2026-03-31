#pragma once
#include <Arduino.h>
#include <vector>

// Структура для одной паузы затирания
struct MashPause
{
    float temp;   // Температура паузы
    int duration; // Длительность в минутах
};

// Настройки, которые обычно сохраняются в энергонезависимую память (LittleFS/EEPROM)
struct Config
{
    String ssid = "";
    String password = "";
    
    String botToken = "";
    String chat_id = "";
    String urlWorker = "";
    int tzOffset = 3;
    int mainMonitorMsgId = 0;

    float targetPowerKW = 0.0;  // Целевая полезная мощность (кВт)
    float heaterPowerWatt = 3000; // Номинал ТЭНа (Вт)
    float powerLossWatt = 200;    // Теплопотери (Вт)

    float targetCubeTemp = 0.0;     // Т1 стоп/уведомление
    float baseSelectionTemp = 0.0;  // Т2 базовая (стабилизация)
    float hysteresis = 0.2;         // Гистерезис для Т2
    bool isSelectionActive = false; // Флаг: идет ли отбор
    float targetColTemp = 78.0;      // Т3 целевая температура колонны
    bool isAutoMode = true;          // Флаг: автоматический режим
    // Интерфейс
    float oledBrightness = 128;
};

struct State
{
    // Датчики
    float currentT1 = 0.0;
    float currentT2 = 0.0;

    bool heaterOn = false;       // Включен ли нагрев
    bool isHeating = false;         // Идет ли активный нагрев сейчас
    bool isWaitForHeat = false;     // Ждем ли достижения целевой Т1
    bool hysteresisReached = false; // Превышен ли порог по Т2

    bool mashMode = false;   // Открыто ли меню затирания в TG
    bool mashActive = false; // Идет ли процесс автоматических пауз
    std::vector<MashPause> pauses;
    int currentPauseIdx = -1;
    unsigned long pauseTimer = 0;
    float tempMashTemp = 0.0; // Временная переменная для ввода новой паузы
    bool warningSent = false;

    bool waitingForTargetT = false;
    bool waitingForHysteresis = false;
    bool waitingForPauseTemp = false;
    bool waitingForPauseDur = false;

    // ... существующие поля ...
    bool heaterMode = false;         // Открыто ли меню ТЭНа в TG
    
    // Флаги ввода для ТЭНа
    bool waitingForPower = false;    // Ввод текущей мощности
    bool waitingForNominal = false;  // Ввод номинала ТЭНа
    bool waitingForLoss = false;     // Ввод теплопотерь
    
    String inputBuffer = "";

    long lastUpdateId = 0;
};