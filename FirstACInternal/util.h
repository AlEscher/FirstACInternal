#pragma once
#include <vector>
#include "ConsoleColor.h"

void printCheatInfo(bool bHealth, bool bAmmo, bool bRecoil, int rapidFireMode, bool bTriggerBot, bool bFly, bool bNameChanger);
bool Hook(void* toHook, void* ourFunct, int len, bool hook, std::vector<BYTE> *oldOpCodes);