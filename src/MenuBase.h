#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>

class MenuBase {
protected:
    String getNowTimeStr();
    const char *getNumpadKeyboard();
    void resetInputState();
    void handleNumpad(const String &data);

public:
    virtual ~MenuBase() {}
    virtual String getKeyboard() = 0;
    virtual String getSpecificStatusText() = 0;
    virtual void processSpecificCallback(const String &data) = 0;

    String generateStatusText();
    String buildMessageJson(const String &text, int msgId = 0);
    void processCallback(const String &data, int msgId);
};