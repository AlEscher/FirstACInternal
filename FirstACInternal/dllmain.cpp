// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <windows.h>
#include <iostream>
#include <TlHelp32.h>
#include <vector>
#include <valarray>
#include "util.h"
#include "mem.h"
#include "entity.h"

void handleHealth(uintptr_t moduleBase, std::vector<BYTE> *oldOpCodes, bool bHealth);
void handleAmmo(uintptr_t moduleBase, std::vector<BYTE> *oldOpCodes, bool bAmmo);
void handleRecoil(uintptr_t moduleBase, std::vector<BYTE>* oldOpCodes, bool bRecoil);
void handleRapidFire(uintptr_t moduleBase, std::vector<BYTE>* oldOpCodes, int rapidFireMode);
void handleGrenade(uintptr_t moduleBase, std::vector<BYTE>* oldOpCodes, bool bNades);

int32_t *ammoAddr;
uintptr_t localPlayerASM;
Player* localPlayerPtr;
int rapidFireMode;

DWORD jmpBackAddyHealth;
DWORD jmpBackAddyAmmo;
DWORD jmpBackAddyRecoil;
DWORD jmpBackAddyRapid;
DWORD jmpBackAddyNades;
DWORD jmpBackAddySpeed;

typedef char* (__cdecl* _ACPrintF)(const char* sFormat, ...);
_ACPrintF ACPrintF;
// colors for the ingame chat
#define GREEN 0
#define BLUE 1
#define YELLOW 2
#define RED 3
#define GREY 4
#define WHITE 5
#define DARKRED 7
#define PINK 8
#define ORANGE 9

// naked -> no epilogue, no prologue, no extra assembly will be added during compilation
void __declspec(naked) godMode()
{
	__asm
	{
		// our code here
		// check if the game is trying to subtract health from us, the local player
		// the address of the entity that is being damaged is in ebx (localPlayerPtr + F4, found out through CheatEngine -> FInd what accesses)
		sub ebx, 0xF4
		cmp ebx, localPlayerASM
		// if it's us, skip the health subtraction
		je label
		// else execute the normal code
		add ebx, 0xF4
		sub [ebx + 0x04], edi
		label:
		// execute the instruction that we overwrite
		add ebx, 0xF4
		mov eax, edi
		jmp [jmpBackAddyHealth]
	}
}

void __declspec(naked) unlimAmmo()
{
	__asm
	{
		cmp esi, ammoAddr
		je label1
		dec [esi]
		label1:
		push edi
		mov edi, [esp+0x14]
		jmp [jmpBackAddyAmmo]
	}
}

void __declspec(naked) noRecoil()
{
	__asm
	{
		// do this over the ebx register since cmp [esi+8] apparently doesn't work
		push ebx
		mov ebx,[esi+8]
		cmp ebx, localPlayerASM
		pop ebx
		je skip
		// the op code we overwrote
		push eax
		lea ecx,[esp+0x1C]
		push ecx
		mov ecx,esi
		call edx
		// jump to after the recoil func call
		skip:
		jmp [jmpBackAddyRecoil]
		
	}
}

void __declspec(naked) rapidFire()
{
	__asm
	{
		// edx contains pointer to weaponWaittime inside of local entity
		// the wait times are written between offset 0x164 and 0x180
		sub edx, 0x180
		cmp edx, localPlayerASM
		jle firstCheck
		// add and sub must be after conditional jumps as it will overwrite flags
		add edx, 0x180
		mov [edx],ecx
		mov esi, [esi + 0x14]
		jmp [jmpBackAddyRapid]
		// upper limit check passed
		firstCheck:
		add edx, 0x180
		// check lower limit
		sub edx, 0x164
		cmp edx, localPlayerASM
		jge secondCheck
		add edx, 0x164
		mov [edx], ecx
		mov esi, [esi + 0x14]
		jmp [jmpBackAddyRapid]
		secondCheck:
		// if we're here it means that the weaponWaittime that is being written to is in our local entity between offsets 0x164 and 0x180
		add edx, 0x164
		// (1 if rapidFire is off, 16 is "insane" mode which skips the timeout completely)
		cmp rapidFireMode, 0xF
		je end
		push eax
		push edx
		// put the timeout value in eax and divide it by rapidFireMode
		mov eax, ecx
		// set edx to 0 since it's used in the division, otherwise the game crashes
		xor edx, edx
		div rapidFireMode
		// put the result back in ecx, restore all registers and load the wait time value
		mov ecx, eax
		pop edx
		pop eax
		mov [edx], ecx
		end:
		mov esi, [esi + 0x14]
		jmp [jmpBackAddyRapid]
	}
}

void __declspec(naked) unlimNades()
{
	__asm
	{
		// grenadeAmmo address is stored in eax, offset 0x158 in local entity
		sub eax, 0x158
		cmp eax, localPlayerASM
		// add and sub must be after conditional jumps as it will overwrite flags
		je skip
		add eax, 0x158
		dec [eax]
		jmp end
		skip:
		add eax, 0x158	
		end:
		// execute stolen bytes
		mov eax, [esi + 0x0C]
		jmp [jmpBackAddyNades]
	}
}

void __declspec(naked) speedHack()
{
	__asm
	{
		// local player is stored in ebx
		fld DWORD ptr[ebx + 0x10]
		cmp ebx, localPlayerASM
		jne normal
		// imul uses both registers for the result of the multiplication
		push eax
		push edx
		mov edx, 2
		// imul uses eax as first operand
		mov eax,[ebx+0x10]
		imul edx
		mov [ebx+0x10],eax
		pop edx
		pop eax
		fmul st(0), st(2)
		jmp [jmpBackAddySpeed]
		// execute overwritten instruction
		normal:
		fmul st(0), st(2)
		jmp [jmpBackAddySpeed]
	}
}

DWORD WINAPI HackThread(HMODULE hModule)
{
	// Create console
	AllocConsole();
	FILE* f;
	freopen_s(&f, "CONOUT$", "w", stdout);

	// Setup the variables we need
	uintptr_t moduleBase = (uintptr_t)GetModuleHandle(L"ac_client.exe");
	localPlayerPtr =  nullptr;
	localPlayerASM = *(uintptr_t*)(moduleBase + 0x10F4F4);
	localPlayerPtr = *(Player**)(moduleBase + 0x10F4F4);
	// fill the player name with spaces (i believe the name can be max 15 characters, not sure though)
	for (unsigned int i = strlen(localPlayerPtr->name); i < 15 - strlen(localPlayerPtr->name); i++)
	{
		localPlayerPtr->name[i] = ' ';
	}
	std::valarray<char> playerName(localPlayerPtr->name, strlen(localPlayerPtr->name));
	std::string nameBackup(localPlayerPtr->name, strlen(localPlayerPtr->name));
	int nameChangerCtr = 0;
	
	// needed for our injected assembly code
	ammoAddr = localPlayerPtr->currentWeaponPtr->ammo;
	rapidFireMode = 1;

	bool bHealth = false, bAmmo = false, bRecoil = false, bNades = false, bFly = false, coordSet = false, bNameChanger = false;

	std::vector<BYTE> healthOpcodes, ammoOpcodes, recoilOpcodes, rapidOpcodes, nadeOpcodes, speedOpcodes;
	std::vector<float> coordinates(3);
	
	// Prints text to the ingame chat
	ACPrintF = (_ACPrintF)(moduleBase + 0x6B060);
	// %d are flags for the color of the following strings
	const char* sMessageFormat = "\f%d%s:\f%d %s";
	const char* settingUpdate = "\f%d%s: \f%d%s\f%d %s";
	const char* cheatName = "[AC_Internal]";

	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO bufferInfo;
	GetConsoleScreenBufferInfo(h, &bufferInfo);

	printCheatInfo(bHealth, bAmmo, bRecoil, rapidFireMode, bNades, bFly, bNameChanger);
	ACPrintF(sMessageFormat, GREY, cheatName, GREEN, "Injection & Setup successful!");

	// Hack loop
	while (true)
	{
		if (!localPlayerPtr)
		{
			continue;
		}
		// Key input
		if (GetAsyncKeyState(VK_END) & 1)
		{
			ACPrintF(sMessageFormat, GREY, cheatName, YELLOW, "Uninjecting...");
			// Undo all code patches before ejecting
			bHealth = !bHealth;
			bAmmo = !bAmmo;
			bRecoil = !bRecoil;
			bNades = !bNades;
			bFly = !bFly;
			rapidFireMode = 1;

			// we only want to unpatch if the specific setting was activated at least once during runtime
			// if we don't do this and uninject without having activated the godmode for example, it's gonna write a bunch of 0s in the code of the game,
			// since the opcode vectors aren't initialized and the old opcodes were never read
			if (!bHealth)
			{
				handleHealth(moduleBase, &healthOpcodes, bHealth);
			}
			if (!bAmmo)
			{
				handleAmmo(moduleBase, &ammoOpcodes, bAmmo);
			}
			if (!bRecoil)
			{
				handleRecoil(moduleBase, &recoilOpcodes, bRecoil);
			}
			// only unpatch rapid fire if we have opcodes saved (in other words if the setting was activated at least once)
			if (rapidOpcodes.size() > 0)
			{
				handleRapidFire(moduleBase, &rapidOpcodes, rapidFireMode);
			}
			if (!bNades)
			{
				handleGrenade(moduleBase, &nadeOpcodes, bNades);
			}
			if (!bFly)
			{
				localPlayerPtr->fly = 0;
			}
			// reset the name
			for (unsigned int i = 0; i < nameBackup.size(); i++)
			{
				localPlayerPtr->name[i] = nameBackup[i];
			}

			break;
		}

		if (GetAsyncKeyState(VK_NUMPAD1) & 1)
		{
			bHealth = !bHealth;
			handleHealth(moduleBase, &healthOpcodes, bHealth);

			if (bHealth)
			{
				ACPrintF(settingUpdate, GREY, cheatName, WHITE, "Godmode", GREEN, "ON");
			}
			else
			{
				ACPrintF(settingUpdate, GREY, cheatName, WHITE, "Godmode", RED, "OFF");
			}

			SetConsoleCursorPosition(h, bufferInfo.dwCursorPosition);
			printCheatInfo(bHealth, bAmmo, bRecoil, rapidFireMode, bNades, bFly, bNameChanger);
		}

		if (GetAsyncKeyState(VK_NUMPAD2) & 1)
		{
			bAmmo = !bAmmo;
			handleAmmo(moduleBase, &ammoOpcodes, bAmmo);

			if (bAmmo)
			{
				ACPrintF(settingUpdate, GREY, cheatName, WHITE, "Unlimited Ammo", GREEN, "ON");
			}
			else
			{
				ACPrintF(settingUpdate, GREY, cheatName, WHITE, "Unlimited Ammo", RED, "OFF");
			}

			SetConsoleCursorPosition(h, bufferInfo.dwCursorPosition);
			printCheatInfo(bHealth, bAmmo, bRecoil, rapidFireMode, bNades, bFly, bNameChanger);
		}

		if (GetAsyncKeyState(VK_NUMPAD3) & 1)
		{
			bRecoil = !bRecoil;
			handleRecoil(moduleBase, &recoilOpcodes, bRecoil);

			if (bRecoil)
			{
				ACPrintF(settingUpdate, GREY, cheatName, WHITE, "No Recoil", GREEN, "ON");
			}
			else
			{
				ACPrintF(settingUpdate, GREY, cheatName, WHITE, "No Recoil", RED, "OFF");
			}

			SetConsoleCursorPosition(h, bufferInfo.dwCursorPosition);
			printCheatInfo(bHealth, bAmmo, bRecoil, rapidFireMode, bNades, bFly, bNameChanger);
		}

		if (GetAsyncKeyState(VK_NUMPAD4) & 1)
		{
			rapidFireMode = (rapidFireMode * 2) % 32;
			if (rapidFireMode == 0)
			{
				rapidFireMode = 1;
				ACPrintF(settingUpdate, GREY, cheatName, WHITE, "Rapid Fire", RED, "OFF");
			}
			else if (rapidFireMode < 16)
			{
				char fireMode[2];
				_itoa_s(rapidFireMode, fireMode, 2, 10);

				ACPrintF(settingUpdate, GREY, cheatName, WHITE, "Rapid Fire x", GREEN, fireMode);
			}
			else
			{
				ACPrintF(settingUpdate, GREY, cheatName, WHITE, "Rapid Fire", DARKRED, "INSANE");
			}
			handleRapidFire(moduleBase, &rapidOpcodes, rapidFireMode);

			SetConsoleCursorPosition(h, bufferInfo.dwCursorPosition);
			printCheatInfo(bHealth, bAmmo, bRecoil, rapidFireMode, bNades, bFly, bNameChanger);
		}

		if (GetAsyncKeyState(VK_NUMPAD5) & 1)
		{
			bNades = !bNades;
			handleGrenade(moduleBase, &nadeOpcodes, bNades);

			if (bNades)
			{
				ACPrintF(settingUpdate, GREY, cheatName, WHITE, "Unlimited Grenades", GREEN, "ON");
			}
			else
			{
				ACPrintF(settingUpdate, GREY, cheatName, WHITE, "Unlimited Grenades", RED, "OFF");
			}

			SetConsoleCursorPosition(h, bufferInfo.dwCursorPosition);
			printCheatInfo(bHealth, bAmmo, bRecoil, rapidFireMode, bNades, bFly, bNameChanger);
		}

		if (GetAsyncKeyState(VK_NUMPAD6) & 1)
		{
			bFly = !bFly;
			if (bFly)
			{
				localPlayerPtr->fly = 5;
				ACPrintF(settingUpdate, GREY, cheatName, WHITE, "Fly Hack", GREEN, "ON");
			}
			else
			{
				localPlayerPtr->fly = 0;
				ACPrintF(settingUpdate, GREY, cheatName, WHITE, "Fly Hack", RED, "OFF");
			}

			SetConsoleCursorPosition(h, bufferInfo.dwCursorPosition);
			printCheatInfo(bHealth, bAmmo, bRecoil, rapidFireMode, bNades, bFly, bNameChanger);
		}

		if (GetAsyncKeyState(VK_NUMPAD7) & 1)
		{
			// increment # of kills by 10
			localPlayerPtr->numOfKills += 10;
		}

		if (GetAsyncKeyState(VK_NUMPAD8) & 1)
		{
			coordinates[0] = localPlayerPtr->posCoord.x;
			coordinates[1] = localPlayerPtr->posCoord.y;
			coordinates[2] = localPlayerPtr->posCoord.z;
			coordSet = true;
			ACPrintF(sMessageFormat, GREY, cheatName, YELLOW, "Teleporter coordinates set");
		}

		if (GetAsyncKeyState(VK_NUMPAD9) & 1)
		{
			if (coordSet)
			{
				localPlayerPtr->posCoord.x = coordinates[0];
				localPlayerPtr->posCoord.y = coordinates[1];
				localPlayerPtr->posCoord.z = coordinates[2];
				ACPrintF(sMessageFormat, GREY, cheatName, GREEN, "Teleported");
			}
		}

		if (GetAsyncKeyState(VK_NUMPAD0) & 1)
		{
			bNameChanger = !bNameChanger;

			if (!bNameChanger)
			{
				// reset name
				for (unsigned int i = 0; i < nameBackup.size(); i++)
				{
					localPlayerPtr->name[i] = nameBackup[i];
				}

				ACPrintF(settingUpdate, GREY, cheatName, WHITE, "Name Changer", RED, "OFF");
			}
			else
			{
				ACPrintF(settingUpdate, GREY, cheatName, WHITE, "Name Changer", GREEN, "ON");
			}

			SetConsoleCursorPosition(h, bufferInfo.dwCursorPosition);
			printCheatInfo(bHealth, bAmmo, bRecoil, rapidFireMode, bNades, bFly, bNameChanger);
		}
		// Keep updating the ammo address so that it always points at the current weapon ammo
		ammoAddr = localPlayerPtr->currentWeaponPtr->ammo;
		Sleep(5);

		if (bNameChanger)
		{
			// every 40 cycles (~200 ms) shift the name by 1 char
			if (nameChangerCtr == 40)
			{
				// rotate the name by 1 to the left (to the right would be -1)
				playerName = playerName.cshift(1);
				// write the shifted name into memory
				for (unsigned int i = 0; i < playerName.size(); i++)
				{
					localPlayerPtr->name[i] = playerName[i];
				}
				nameChangerCtr = 0;
			}
			else
			{
				nameChangerCtr++;
			}
		}
	}

	// Cleanup & eject
	if (f)
	{
		fclose(f);
	}
	FreeConsole();
	ACPrintF(sMessageFormat, GREY, cheatName, GREEN, "Successfully uninjected!");
	FreeLibraryAndExitThread(hModule, 0);
	return 0;
}

BOOL APIENTRY DllMain( HMODULE hModule,
					   DWORD  ul_reason_for_call,
					   LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	{
		// Only creat thread here, no other code
		HANDLE tmp = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)HackThread, hModule, 0, nullptr);
		if (tmp)
		{
			CloseHandle(tmp);
		}
	}
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

void handleHealth(uintptr_t moduleBase, std::vector<BYTE> *oldOpCodes, bool bHealth)
{
	// hook at health subtraction
	DWORD hookAddress = moduleBase + 0x29D1F;
	int hookLength = 5;
	jmpBackAddyHealth = hookAddress + hookLength;
	Hook((void*)hookAddress, godMode, hookLength, bHealth, oldOpCodes);
}

void handleAmmo(uintptr_t moduleBase, std::vector<BYTE> *oldOpCodes, bool bAmmo)
{
	// hook at ammo subtraction
	DWORD hookAddress = moduleBase + 0x637E9;
	int hookLength = 7;
	jmpBackAddyAmmo = hookAddress + hookLength;
	Hook((void*)hookAddress, unlimAmmo, hookLength, bAmmo, oldOpCodes);
}

void handleRecoil(uintptr_t moduleBase, std::vector<BYTE> *oldOpCodes, bool bRecoil)
{
	// hook before the recoil function is called
	DWORD hookAddress = moduleBase + 0x63786;
	int hookLength = 10;
	jmpBackAddyRecoil = hookAddress + hookLength;
	Hook((void*)hookAddress, noRecoil, hookLength, bRecoil, oldOpCodes);
}

void handleRapidFire(uintptr_t moduleBase, std::vector<BYTE>* oldOpCodes, int rapidFireMode)
{
	// hook when the timeout value is loaded
	DWORD hookAddress = moduleBase + 0x637E4;
	int hookLength = 5;
	jmpBackAddyRapid = hookAddress + hookLength;
	// only save the current Opcodes if rapidFire was just turned on (went from 1 to 2)
	if (rapidFireMode == 2)
	{
		Hook((void*)hookAddress, rapidFire, hookLength, true, oldOpCodes);
	}
	// if rapidFire was just turned off, rewrite the old opcodes
	// also check if we have opcodes saved (in other words if the setting was ever activated)
	else if (rapidFireMode == 1)
	{
		Hook((void*)hookAddress, rapidFire, hookLength, false, oldOpCodes);
	}
	// if rapidFireMode was turned from e.g. 4 to 8, don't do anything as the function is already hooked
	// we do not want to read / write opcodes in this case
	else
	{
		return;
	}
}

void handleGrenade(uintptr_t moduleBase, std::vector<BYTE> *oldOpCodes, bool bNades)
{
	// Add a nade to the player's inventory if the grenade cheat was activated
	if (bNades)
	{
		localPlayerPtr->grenadeAmmo += 1;
	}
	// hook at grenadeAmmo reduction
	DWORD hookAddress = moduleBase + 0x63378;
	int hookLength = 5;
	jmpBackAddyNades = hookAddress + hookLength;
	Hook((void*)hookAddress, unlimNades, hookLength, bNades, oldOpCodes);
}

