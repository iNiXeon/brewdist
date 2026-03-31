#pragma once
#include "MenuBase.h"

class DistillMenu : public MenuBase {
public:
    String getKeyboard() override;
    String getSpecificStatusText() override;
    void processSpecificCallback(const String &data) override;
};

class MashMenu : public MenuBase {
public:
    String getKeyboard() override;
    String getSpecificStatusText() override;
    void processSpecificCallback(const String &data) override;
};

class HeaterMenu : public MenuBase {
public:
        String getKeyboard() override;
    String getSpecificStatusText() override;
    void processSpecificCallback(const String &data) override;
};