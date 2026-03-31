#include "GlobalVars.h"
#include "Heater.h"

Heater::Heater(uint8_t pin) : 
    _pin(pin), 
    _targetKW(0.0), 
    _nominalWatt(config.heaterPowerWatt), 
    _lossWatt(config.powerLossWatt), 
    _windowStartTime(0),
    _onTimeMs(0) 
{}

void Heater::init() {
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, LOW); // Используем digitalWrite вместо analogWrite
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
    if (_targetKW <= 0.01) {
        state.heaterOn = false; 
        _onTimeMs = 0;
    } else {
        state.heaterOn = true; 
        float requiredWatt = (_targetKW * 1000.0) + (float)_lossWatt;
        requiredWatt = constrain(requiredWatt, 0, _nominalWatt);
        // Вычисляем время включения ТЭНа в миллисекундах (от 0 до _pwmPeriod)
        _onTimeMs = (unsigned long)((requiredWatt / (float)_nominalWatt) * _pwmPeriod);
    }
}

void Heater::stop() {
    _targetKW = 0;
    _onTimeMs = 0;
    digitalWrite(_pin, LOW);
    state.heaterOn = false; 
}

void Heater::tick() {
    // 1. ГЛАВНЫЙ РУБИЛЬНИК
    // Если нагрев выключен в интерфейсе (Telegram или OLED) — 
    // жестко глушим пин и выходим. ШИМ не работает.
    if (!state.heaterOn) {
        digitalWrite(_pin, LOW);
        return;
    }
    // 2. Защита от нулевой мощности (и float-погрешностей)
    // Если задали 0 кВт, ТЭН не должен включаться, но state.heaterOn 
    // остается true (мы просто находимся в режиме ожидания или паузы).
    if (_targetKW < 0.1 || _onTimeMs == 0) {
        digitalWrite(_pin, LOW);
        return;
    }

    // 3. Обработка 100% мощности (без потерь времени на ШИМ)
    if (_onTimeMs >= _pwmPeriod) {
        digitalWrite(_pin, HIGH);
        return;
    }

    // 4. СТАНДАРТНЫЙ АЛГОРИТМ МЕДЛЕННОГО ШИМ
    unsigned long now = millis();
    
    // Если окно закончилось, начинаем новое
    if (now - _windowStartTime >= _pwmPeriod) {
        _windowStartTime = now; 
    }

    // Внутри окна: до _onTimeMs держим HIGH, после — LOW
    if (now - _windowStartTime < _onTimeMs) {
        digitalWrite(_pin, HIGH);
    } else {
        digitalWrite(_pin, LOW);
    }
}

// Реализация геттеров...
float Heater::getTargetKW() { return _targetKW; }
int Heater::getNominal() { return _nominalWatt; }
int Heater::getLosses() { return _lossWatt; }
int Heater::getDutyPercent() { 
    // Возвращаем процент мощности от 0 до 100
    return (_onTimeMs * 100) / _pwmPeriod; 
}