#pragma once
#include <vector>

void printCheatInfo(bool bHealth, bool bAmmo, bool bRecoil, int rapidFireMode, bool bNades, bool bSpeed, bool bNameChanger);
bool Hook(void* toHook, void* ourFunct, int len, bool hook, std::vector<BYTE> *oldOpCodes);