#pragma once
#include <Arduino.h>
#include "ConfigStruct.h"

// Состояния меню для навигации
enum MenuState {
    DASHBOARD,
    MAIN_MENU,
    SUB_HEATER,
    SUB_KUB,
    SUB_SEL,
    SUB_COL,
    SUB_SRV
};

class LocalUI {
public:
    MenuState uiState = DASHBOARD;
    uint8_t cursorMain = 0, cursorSub = 0;
    bool isEditing = false;
    uint32_t lastActionTime = 0;
    float *editVal = nullptr;
    float valMin = 0, valMax = 120, valStep = 0.1;

    void init();
    void draw();
    void tick();

private:
    void handleNavigation(int nav, bool okClick, bool backClick);
    void startEdit(float *val, float min, float max, float step);
    
    // Вспомогательные функции отрисовки
    void printCursor(uint8_t line, uint8_t current);
    void drawMenuHeader(const char *title);
    void drawValue(float val, bool highlight, const char *unit = "C", int decimals = 1);
    
    // Экраны
    void drawDash();
    void drawMain();
    void drawSubHeater();
    void drawSubKub();
    void drawSubSel();
    void drawSubCol();
    void drawSubSrv();
};