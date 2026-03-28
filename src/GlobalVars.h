#pragma once
#include "ConfigStruct.h"

extern Config config;
extern State state;
extern void requestTgUpdate(bool force);
extern void saveSettings();