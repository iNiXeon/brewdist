#include "Heater.h"
#include "GlobalVars.h"

extern Config config;
extern State state;

Heater::Heater(uint8_t pin) : _pin(pin), _targetKW(0), _nominalWatt(3000), _lossWatt(200), _currentDuty(0) {}

void Heater::init() {
    pinMode(_pin, OUTPUT);
    stop();
}

void Heater::setPowerKW(float kw) {
    _targetKW = kw;
    calculatePhysics();
}

void Heater::setHardware(int nominal, int losses) {
    _nominalWatt = nominal;
    _lossWatt = losses;
    calculatePhysics();
}

void Heater::calculatePhysics() {
    if (_targetKW <= 0.001) {
        _currentDuty = 0;
    } else {
        float requiredWatt = (_targetKW * 1000.0) + (float)_lossWatt;
        requiredWatt = constrain(requiredWatt, 0, _nominalWatt);
        _currentDuty = (int)((requiredWatt / (float)_nominalWatt) * 255.0);
    }
}

void Heater::stop() {
    _targetKW = 0;
    _currentDuty = 0;
    analogWrite(_pin, 0);
}

void Heater::tick() {
    analogWrite(_pin, _currentDuty);
}

// Реализация геттеров...
float Heater::getTargetKW() { return _targetKW; }
int Heater::getNominal() { return _nominalWatt; }
int Heater::getLosses() { return _lossWatt; }
int Heater::getDutyPercent() { return map(_currentDuty, 0, 255, 0, 100); }