#pragma once
#ifndef HEATER_H
#define HEATER_H

#include <Arduino.h>

class Heater {
private:
    uint8_t _pin;
    float _targetKW;
    int _nominalWatt;
    int _lossWatt;
    int _currentDuty;

    void calculatePhysics();

public:
    Heater(uint8_t pin);
    void init();
    void setPowerKW(float kw);
    void setHardware(int nominal, int losses);
    void stop();
    void tick();

    // Геттеры
    float getTargetKW();
    int getNominal();
    int getLosses();
    int getDutyPercent();
};

#endif