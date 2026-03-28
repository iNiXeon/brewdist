#include "LocalUI.h"
#include "GlobalVars.h"
#include <GyverOLED.h> 
#include <uButton.h>

// Внешние объекты (должны быть созданы в main.ino)
extern GyverOLED<SSD1306_128x64, OLED_NO_BUFFER> oled;
extern uButton btnUp, btnDown, btnOk, btnBack;

extern Config config;
extern State state;


void LocalUI::init() {
    lastActionTime = millis();
    oled.setContrast((uint8_t)config.oledBrightness);
    draw();
}

void LocalUI::draw() {
    oled.clear();
    switch (uiState) {
        case DASHBOARD: drawDash(); break;
        case MAIN_MENU: drawMain(); break;
        case SUB_KUB:   drawSubKub(); break;
        case SUB_SEL:   drawSubSel(); break;
        case SUB_COL:   drawSubCol(); break;
        case SUB_SRV:   drawSubSrv(); break;
    }
}

void LocalUI::tick() {
    bool upClick = btnUp.release(), downClick = btnDown.release();
    bool okClick = btnOk.release(), backClick = btnBack.release();
    bool upStep = btnUp.step(), downStep = btnDown.step();
    bool action = upClick || downClick || okClick || backClick || upStep || downStep;

    if (action) lastActionTime = millis();

    // Автовыход в дашборд через 10 сек
    if (uiState != DASHBOARD && (millis() - lastActionTime > 10000)) {
        uiState = DASHBOARD;
        isEditing = false;
        draw();
        return;
    }

    // Фоновое обновление температур на дашборде
    if (!action) {
        static uint32_t updTimer = 0;
        if (uiState == DASHBOARD && millis() - updTimer > 1000) {
            updTimer = millis();
            drawDash();
        }
        return;
    }

    // Логика редактирования значения
    if (isEditing && editVal != nullptr) {
        if (upStep) *editVal += 1.0;
        else if (downStep) *editVal -= 1.0;
        else if (upClick) *editVal += valStep;
        else if (downClick) *editVal -= valStep;

        *editVal = constrain(*editVal, valMin, valMax);
        
        if (editVal == &config.oledBrightness)
            oled.setContrast((uint8_t)config.oledBrightness);

        if (okClick || backClick) {
            isEditing = false;
            saveSettings();
            requestTgUpdate(true);
        }
        draw();
        return;
    }

    int nav = (upClick || upStep) ? -1 : (downClick || downStep ? 1 : 0);
    handleNavigation(nav, okClick, backClick);
    draw();
}

void LocalUI::handleNavigation(int nav, bool okClick, bool backClick) {
    switch (uiState) {
        case DASHBOARD:
            if (okClick) { uiState = MAIN_MENU; cursorMain = 0; }
            break;

        case MAIN_MENU:
            if (nav != 0) cursorMain = constrain(cursorMain + nav, 0, 3);
            if (backClick) uiState = DASHBOARD;
            if (okClick) {
                cursorSub = 0;
                uiState = (MenuState)(SUB_KUB + cursorMain);
            }
            break;

        case SUB_KUB:
            if (nav != 0) cursorSub = constrain(cursorSub + nav, 0, 1);
            if (backClick) uiState = MAIN_MENU;
            if (okClick) {
                if (cursorSub == 0) {
                    state.isWaitForHeat = !state.isWaitForHeat;
                    if (state.isWaitForHeat) state.isHeating = (state.currentT1 < config.targetCubeTemp);
                    saveSettings(); requestTgUpdate(true);
                } else startEdit(&config.targetCubeTemp, 20, 100, 0.1);
            }
            break;

        case SUB_SEL:
            if (nav != 0) cursorSub = constrain(cursorSub + nav, 0, 2);
            if (backClick) uiState = MAIN_MENU;
            if (okClick) {
                if (cursorSub == 0) {
                    config.isSelectionActive = !config.isSelectionActive;
                    if (config.isSelectionActive) {
                        state.hysteresisReached = false;
                        config.baseSelectionTemp = state.currentT2;
                    }
                    saveSettings(); requestTgUpdate(true);
                } else if (cursorSub == 2) startEdit(&config.hysteresis, 0.1, 5.0, 0.1);
            }
            break;

        case SUB_SRV:
            if (nav != 0) cursorSub = constrain(cursorSub + nav, 0, 2);
            if (backClick) uiState = MAIN_MENU;
            if (okClick && cursorSub == 1) startEdit(&config.oledBrightness, 10, 255, 10);
            break;
            
        default: break;
    }
}

// Вспомогательные методы (отрисовка)
void LocalUI::startEdit(float *val, float min, float max, float step) {
    isEditing = true; editVal = val; valMin = min; valMax = max; valStep = step;
}

void LocalUI::printCursor(uint8_t line, uint8_t current) {
    oled.setCursor(0, line + 2); // Смещение под заголовок
    oled.print((line == current) ? ">" : " ");
}

void LocalUI::drawMenuHeader(const char *title) {
    oled.home();
    oled.setScale(1); 
    oled.invertText(true);
    oled.print(title);
    oled.invertText(false);
}

void LocalUI::drawValue(float val, bool highlight, const char *unit, int decimals) {
    if (highlight) oled.invertText(true);
    oled.print(val, decimals);
    oled.invertText(false);
    oled.print(unit);
}

void LocalUI::drawDash() {
    oled.setCursor(0, 0);
    oled.setScale(2);
    if (state.isWaitForHeat)
        oled.print(state.isHeating ? "НАГРЕВ...  " : "ОХЛАЖДЕНИЕ...  ");
    else if (config.isSelectionActive)
        oled.print(state.mashActive ? "ЗАТОР...  " : (config.isSelectionActive ? "ОТБОР " + String(config.baseSelectionTemp, 1) : "СТОП...   "));
    else oled.print("ОЖИДАНИЕ... ");
    
    oled.setCursor(0, 2);
    oled.print("T1:"); oled.print(state.currentT1, 1);
    oled.setCursor(0, 5);
    oled.print("T2:"); oled.print(state.currentT2, 1);
}

void LocalUI::drawMain() {
    drawMenuHeader("ГЛАВНОЕ МЕНЮ");
    printCursor(0, cursorMain);
    oled.print("1.ЦЕЛЬ КУБ [");
    oled.print(state.isWaitForHeat ? "ВКЛ" : "ВЫКЛ");
    oled.println("]");
    printCursor(1, cursorMain);
    oled.print("2.ОТБОР [");
    oled.print(config.isSelectionActive ? "ПУСК" : "СТОП");
    oled.println("]");
    printCursor(2, cursorMain);
    oled.println("3.ЦЕЛЬ ЦАРГА");
    printCursor(3, cursorMain);
    oled.println("4.СЕРВИС");
}

void LocalUI::drawSubKub() {
    drawMenuHeader("ЦЕЛЬ КУБ:");
    printCursor(0, cursorSub);
    oled.println(state.isWaitForHeat ? "[ ОСТАНОВИТЬ ]" : "[ ЗАПУСТИТЬ ]");
    printCursor(1, cursorSub);
    oled.print("Установить: ");
    drawValue(config.targetCubeTemp, isEditing && cursorSub == 1);
}

void LocalUI::drawSubSel() {
    drawMenuHeader("ОТБОР:");
    printCursor(0, cursorSub);
    oled.println(config.isSelectionActive ? "[ ОСТАНОВИТЬ ]" : "[ ЗАПУСТИТЬ ]");
    printCursor(1, cursorSub);
    oled.println(config.isAutoMode ? "Режим: АВТО" : "Режим: РУЧНОЙ");
    printCursor(2, cursorSub);
    oled.print("Гист.: ");
    drawValue(config.hysteresis, isEditing && cursorSub == 2);
}

void LocalUI::drawSubCol() {
    drawMenuHeader("ЦЕЛЬ ЦАРГА:");
    printCursor(0, cursorSub);
    oled.print("Текущая: ");
    oled.print(state.currentT2, 1);
    oled.println("C");
    printCursor(1, cursorSub);
    oled.print("Установить: ");
    drawValue(config.targetColTemp, isEditing && cursorSub == 1);
}

void LocalUI::drawSubSrv() {
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