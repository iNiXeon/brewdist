#include "LocalUI.h"
#include "GlobalVars.h"
#include <GyverOLED.h>
#include <uButton.h>

// Внешние объекты (должны быть созданы в main.ino)
extern GyverOLED<SSD1306_128x64, OLED_NO_BUFFER> oled;
extern uButton btnUp, btnDown, btnOk, btnBack;

extern Config config;
extern State state;

void LocalUI::init()
{
    lastActionTime = millis();
    oled.setContrast((uint8_t)config.oledBrightness);
    draw();
}

void LocalUI::draw()
{
    oled.clear();
    switch (uiState)
    {
    case DASHBOARD:
        drawDash();
        break;
    case MAIN_MENU:
        drawMain();
        break;
    case SUB_HEATER:
        drawSubHeater();
        break;
    case SUB_KUB:
        drawSubKub();
        break;
    case SUB_SEL:
        drawSubSel();
        break;
    case SUB_COL:
        drawSubCol();
        break;
    case SUB_SRV:
        drawSubSrv();
        break;
    }
}

void LocalUI::tick()
{
    bool upClick = btnUp.release(), downClick = btnDown.release();
    bool okClick = btnOk.release(), backClick = btnBack.release();
    bool upStep = btnUp.step(), downStep = btnDown.step();
    bool action = upClick || downClick || okClick || backClick || upStep || downStep;

    if (action)
        lastActionTime = millis();

    // Автовыход в дашборд через 10 сек
    if (uiState != DASHBOARD && (millis() - lastActionTime > 10000))
    {
        uiState = DASHBOARD;
        isEditing = false;
        draw();
        return;
    }

    // Фоновое обновление температур на дашборде
    if (!action)
    {
        static uint32_t updTimer = 0;
        if (uiState == DASHBOARD && millis() - updTimer > 1000)
        {
            updTimer = millis();
            drawDash();
        }
        return;
    }

    // Логика редактирования значения
    if (isEditing && editVal != nullptr)
    {
        if (upStep)
            *editVal += 1.0;
        else if (downStep)
            *editVal -= 1.0;
        else if (upClick)
            *editVal += valStep;
        else if (downClick)
            *editVal -= valStep;

        *editVal = constrain(*editVal, valMin, valMax);

        if (editVal == &config.oledBrightness)
            oled.setContrast((uint8_t)config.oledBrightness);

        if (okClick || backClick)
        {
            isEditing = false;
            saveSettings();
            requestTgUpdate(true);
            if (uiState == SUB_HEATER) {
            syncHeater(); // Применяем новые настройки ТЭНа сразу после редактирования
    }
        }
        draw();
        return;
    }

    int nav = (upClick || upStep) ? -1 : (downClick || downStep ? 1 : 0);
    handleNavigation(nav, okClick, backClick);
    draw();
}

void LocalUI::handleNavigation(int nav, bool okClick, bool backClick)
{
    switch (uiState)
    {
    case DASHBOARD:
        if (okClick)
        {
            uiState = MAIN_MENU;
            cursorMain = 0;
        }
        break;

    case MAIN_MENU:
        if (nav != 0)
            cursorMain = constrain(cursorMain + nav, 0, 4); // У тебя 5 пунктов в drawMain, значит до 4
        if (backClick)
            uiState = DASHBOARD;
        if (okClick)
        {
            cursorSub = 0;
            // Должно начинаться с SUB_HEATER, так как это первый пункт в списке
            uiState = (MenuState)(SUB_HEATER + cursorMain);
        }
        break;

    case SUB_HEATER:
        if (nav != 0)
            cursorSub = constrain(cursorSub + nav, 0, 4);
        if (backClick)
            uiState = MAIN_MENU;

        if (okClick)
        {
            if (cursorSub == 0)
            {
                // Логика кнопки Вкл/Выкл
                state.heaterOn = !state.heaterOn;
                if (!state.heaterOn) {
                    config.targetPowerKW = 0.0; // Если выключаем, обнуляем целевую мощность
                }
                saveSettings();
            }
            else if (cursorSub == 1)
            {
                // Редактируем целевую мощность (кВт)
                // Разумно ограничить ввод сверху физическим максимумом
                float maxAllowedKW = (config.heaterPowerWatt - config.powerLossWatt) / 1000.0;
                startEdit(&config.targetPowerKW, 0.0, maxAllowedKW, 0.1);
            }
            else if (cursorSub == 2)
            {
                // Редактируем номинал ТЭНа (Вт)
                // ВНИМАНИЕ: Если startEdit принимает только float*,
                // тебе понадобится перегрузка функции для int*
                startEdit(&config.heaterPowerWatt, 1000, 5000, 100);
            }
            else if (cursorSub == 3)
            {
                // Редактируем теплопотери (Вт)
                startEdit(&config.powerLossWatt, 0, 1000, 10);
            }
            requestTgUpdate(true); // Если нужно уведомлять Telegram
        }
        break;

    case SUB_KUB:
        if (nav != 0)
            cursorSub = constrain(cursorSub + nav, 0, 1);
        if (backClick)
            uiState = MAIN_MENU;
        if (okClick)
        {
            if (cursorSub == 0)
            {
                state.isWaitForHeat = !state.isWaitForHeat;
                if (state.isWaitForHeat)
                    state.isHeating = (state.currentT1 < config.targetCubeTemp);
                saveSettings();
                requestTgUpdate(true);
            }
            else
                startEdit(&config.targetCubeTemp, 20, 100, 0.1);
        }
        break;

    case SUB_SEL:
        if (nav != 0)
            cursorSub = constrain(cursorSub + nav, 0, 2);
        if (backClick)
            uiState = MAIN_MENU;
        if (okClick)
        {
            if (cursorSub == 0)
            {
                config.isSelectionActive = !config.isSelectionActive;
                if (config.isSelectionActive)
                {
                    state.hysteresisReached = false;
                    config.baseSelectionTemp = state.currentT2;
                }
                saveSettings();
                requestTgUpdate(true);
            }
            else if (cursorSub == 2)
                startEdit(&config.hysteresis, 0.1, 5.0, 0.1);
        }
        break;

    case SUB_SRV:
        if (nav != 0)
            cursorSub = constrain(cursorSub + nav, 0, 2);
        if (backClick)
            uiState = MAIN_MENU;
        if (okClick && cursorSub == 1)
            startEdit(&config.oledBrightness, 10, 255, 10);
        break;

    default:
        break;
    }
}

// Вспомогательные методы (отрисовка)
void LocalUI::startEdit(float *val, float min, float max, float step)
{
    isEditing = true;
    editVal = val;
    valMin = min;
    valMax = max;
    valStep = step;
}

void LocalUI::printCursor(uint8_t line, uint8_t current)
{
    oled.setCursor(0, line + 2); // Смещение под заголовок
    oled.print((line == current) ? ">" : " ");
}

void LocalUI::drawMenuHeader(const char *title)
{
    oled.home();
    oled.setScale(1);
    oled.invertText(true);
    oled.print(title);
    oled.invertText(false);
}

void LocalUI::drawValue(float val, bool highlight, const char *unit, int decimals)
{
    if (highlight)
        oled.invertText(true);
    oled.print(val, decimals);
    oled.invertText(false);
    oled.print(unit);
}

void LocalUI::drawDash()
{
    oled.setCursor(0, 0);
    oled.setScale(2);

    // 1. Приоритет: Затирание
    if (state.mashActive) {
        oled.print("ЗАТОР П:");
        // Выводим номер текущей паузы (индекс начинается с 0, поэтому +1)
        oled.print(state.currentPauseIdx + 1);
        oled.print("  "); 
    }
    // 2. Приоритет: Отбор (Ректификация/Дистилляция)
    else if (config.isSelectionActive) {
        oled.print("ОТБОР ");
        oled.print(config.baseSelectionTemp, 1);
        oled.print(" ");
    }
    // 3. Приоритет: Ожидание целевой температуры куба
    else if (state.isWaitForHeat) {
        // "ОХЛАЖДЕНИЕ" слишком длинное для scale(2), лучше сократить, 
        // иначе может поехать верстка (влезает ~10-11 символов)
        oled.print(state.isHeating ? "НАГРЕВ... " : "ОХЛАЖДЕНИЕ... ");
    }
    // 4. Приоритет: Просто включен ТЭН в ручном режиме
    else if (state.heaterOn) {
        oled.print("ТЭН ВКЛ.  ");
    }
    // 5. Дефолтное состояние: Простой
    else {
        oled.print("ОЖИДАНИЕ...    ");
    }

    // Вывод температур (с запасом пробелов для очистки хвостов)
    oled.setCursor(0, 2);
    oled.print("T1: ");
    oled.print(state.currentT1, 1);
    oled.print("      ");

    oled.setCursor(0, 5);
    oled.print("T2: ");
    oled.print(state.currentT2, 1);
    oled.print("      ");
}

void LocalUI::drawMain()
{
    drawMenuHeader("ГЛАВНОЕ МЕНЮ");
    printCursor(0, cursorMain);
    oled.print("1.ТЭН [");
    oled.print(state.heaterOn ? "ВКЛ" : "ВЫКЛ");
    oled.println("]");
    printCursor(1, cursorMain);
    oled.print("2.ЦЕЛЬ КУБ [");
    oled.print(state.isWaitForHeat ? "ВКЛ" : "ВЫКЛ");
    oled.println("]");
    printCursor(2, cursorMain);
    oled.print("3.ОТБОР [");
    oled.print(config.isSelectionActive ? "ПУСК" : "СТОП");
    oled.println("]");
    printCursor(3, cursorMain);
    oled.println("4.ЦЕЛЬ ЦАРГА");
    printCursor(4, cursorMain);
    oled.println("5.СЕРВИС");
}

void LocalUI::drawSubKub()
{
    drawMenuHeader("ЦЕЛЬ КУБ:");
    printCursor(0, cursorSub);
    oled.println(state.isWaitForHeat ? "[ ОСТАНОВИТЬ ]" : "[ ЗАПУСТИТЬ ]");
    printCursor(1, cursorSub);
    oled.print("Установить: ");
    drawValue(config.targetCubeTemp, isEditing && cursorSub == 1);
}

void LocalUI::drawSubSel()
{
    drawMenuHeader("ОТБОР:");
    printCursor(0, cursorSub);
    oled.println(config.isSelectionActive ? "[ ОСТАНОВИТЬ ]" : "[ ЗАПУСТИТЬ ]");
    printCursor(1, cursorSub);
    oled.println(config.isAutoMode ? "Режим: АВТО" : "Режим: РУЧНОЙ");
    printCursor(2, cursorSub);
    oled.print("Гист.: ");
    drawValue(config.hysteresis, isEditing && cursorSub == 2);
}

void LocalUI::drawSubHeater()
{
    drawMenuHeader("ТЭН:");
    printCursor(0, cursorSub);
    oled.println(state.heaterOn ? "НАГРЕВ: [ ИДЕТ ]" : "НАГРЕВ: [ ВЫКЛ ]");
    printCursor(1, cursorSub);
    oled.print("Мощность: ");
    drawValue(config.targetPowerKW, isEditing && cursorSub == 1, " кВт");
    printCursor(2, cursorSub);
    oled.print("Max: ");
    drawValue(config.heaterPowerWatt, isEditing && cursorSub == 2, " Вт", 0);
    printCursor(3, cursorSub);
    oled.print("Потери: ");
    drawValue(config.powerLossWatt, isEditing && cursorSub == 3, " Вт", 0);
    oled.update();
}

void LocalUI::drawSubCol()
{
    drawMenuHeader("ЦЕЛЬ ЦАРГА:");
    printCursor(0, cursorSub);
    oled.print("Текущая: ");
    oled.print(state.currentT2, 1);
    oled.println("C");
    printCursor(1, cursorSub);
    oled.print("Установить: ");
    drawValue(config.targetColTemp, isEditing && cursorSub == 1);
}

void LocalUI::drawSubSrv()
{
    drawMenuHeader("СЕРВИС:");
    printCursor(0, cursorSub);
    oled.print("WiFi: ");
    oled.println("OK"); // Или выведи IP
    printCursor(1, cursorSub);
    oled.print("Яркость: ");
    drawValue(config.oledBrightness, isEditing && cursorSub == 1, "", 0);
    printCursor(2, cursorSub);
    oled.println("Сброс настроек");
}