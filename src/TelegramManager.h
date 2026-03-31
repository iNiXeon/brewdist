#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include "AppMenus.h" 

class TelegramManager {
private:
    unsigned long lastAutoSend = 0;
    DistillMenu distillMenu;
    MashMenu mashMenu;
    HeaterMenu heaterMenu;
    MenuBase *getActiveMenu();

public:
    void updateTelegram(bool force = false);
    
    void handleUpdates();
    
    void processMessage(const String &text);
};