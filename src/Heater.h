#pragma once
#include <Arduino.h>

class Heater {
private:
    uint8_t _pin;
    float _targetKW;
    int _nominalWatt;
    int _lossWatt;
    
    // Переменные для алгоритма медленного ШИМ
    unsigned long _windowStartTime;
    unsigned long _onTimeMs;
    const unsigned long _pwmPeriod = 1000; // Период ШИМ: 1 секунда (1000 мс)

    void calculatePhysics();

public:
    Heater(uint8_t pin);
    void init();
    void setPowerKW(float kw);
    void setHardware(int nominal, int losses);
    void stop();
    void tick();

    float getTargetKW();
    int getNominal();
    int getLosses();
    int getDutyPercent();
};