// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <windows.h>
#include <iostream>
#include <TlHelp32.h>
#include <vector>
#include "util.h"
#include "mem.h"
#include "proc.h"

void handleHealth(uintptr_t moduleBase, std::vector<BYTE> *oldOpCodes, bool bHealth);
void handleAmmo(uintptr_t moduleBase, std::vector<BYTE> *oldOpCodes, bool bAmmo);
void handleRecoil(uintptr_t moduleBase, std::vector<BYTE>* oldOpCodes, bool bRecoil);
void handleRapidFire(uintptr_t moduleBase, std::vector<BYTE>* oldOpCodes, int rapidFireMode);
void handleGrenade(uintptr_t moduleBase, std::vector<BYTE>* oldOpCodes, bool bNades);
void handleSpeedHack(uintptr_t moduleBase, std::vector<BYTE>* oldOpCodes, bool bSpeed);
uintptr_t localPlayerForGodMode;
uintptr_t ammoAddr;
uintptr_t localPlayer;
int rapidFireMode;
DWORD jmpBackAddyHealth;
DWORD jmpBackAddyAmmo;
DWORD jmpBackAddyRecoil;
DWORD jmpBackAddyRapid;
DWORD jmpBackAddyNades;
DWORD jmpBackAddySpeed;

// naked -> no epilogue, no prologue, no extra assembly will be added during compilation
void __declspec(naked) godMode()
{
	__asm
	{
		// our code here
		// check if the game is trying to subtract health from us, the local player
		// the address of the entity that is being damaged is in ebx (localPlayer + F4, found out through CheatEngine -> FInd what accesses)
		cmp ebx, localPlayerForGodMode
		// if it's us, skip the health subtraction
		je label
		// else execute the normal code
		sub [ebx + 0x04], edi
		label:
		// execute the instruction that we overwrite
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
		cmp ebx, localPlayer
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
		cmp edx, localPlayer
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
		cmp edx, localPlayer
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
		cmp eax, localPlayer
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
		cmp ebx, localPlayer
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

	// Get module base
	uintptr_t moduleBase = (uintptr_t)GetModuleHandle(L"ac_client.exe");
	localPlayer = *(uintptr_t*)(moduleBase + 0x10F4F4);
	ammoAddr = mem::FindDMAAddy(moduleBase + 0x10F4F4, { 0x374, 0x14, 0x0 });
	localPlayerForGodMode = localPlayer + 0xF4;
	rapidFireMode = 1;

	bool bHealth = false, bAmmo = false, bRecoil = false, bNades = false, bSpeed = false, coordSet = false;

	std::vector<BYTE> healthOpcodes, ammoOpcodes, recoilOpcodes, rapidOpcodes, nadeOpcodes, speedOpcodes;
	std::vector<float> coordinates(3);
	

	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO bufferInfo;
	GetConsoleScreenBufferInfo(h, &bufferInfo);

	printCheatInfo(bHealth, bAmmo, bRecoil, rapidFireMode, bNades, bSpeed);

	// Hack loop
	while (true)
	{
		// Key input
		if (GetAsyncKeyState(VK_END) & 1)
		{
			// Undo all code patches before ejecting
			// Just setting all bools to false and then calling every "handle..." function doesn't work, makes the game crash.
			// I have no clue why.
			// Only works if all cheats are set to ON before uninjecting...
			bHealth = !bHealth;
			bAmmo = !bAmmo;
			bRecoil = !bRecoil;
			bNades = !bNades;
			bSpeed = !bSpeed;
			rapidFireMode = 1;

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
			if (rapidFireMode == 1)
			{
				handleRapidFire(moduleBase, &rapidOpcodes, rapidFireMode);
			}
			if (!bNades)
			{
				handleGrenade(moduleBase, &nadeOpcodes, bNades);
			}
			if (!bSpeed)
			{
				handleSpeedHack(moduleBase, &speedOpcodes, bSpeed);
			}
			break;
		}

		if (GetAsyncKeyState(VK_NUMPAD1) & 1)
		{
			bHealth = !bHealth;
			handleHealth(moduleBase, &healthOpcodes, bHealth);

			SetConsoleCursorPosition(h, bufferInfo.dwCursorPosition);
			printCheatInfo(bHealth, bAmmo, bRecoil, rapidFireMode, bNades, bSpeed);
		}

		if (GetAsyncKeyState(VK_NUMPAD2) & 1)
		{
			bAmmo = !bAmmo;
			handleAmmo(moduleBase, &ammoOpcodes, bAmmo);

			SetConsoleCursorPosition(h, bufferInfo.dwCursorPosition);
			printCheatInfo(bHealth, bAmmo, bRecoil, rapidFireMode, bNades, bSpeed);
		}

		if (GetAsyncKeyState(VK_NUMPAD3) & 1)
		{
			bRecoil = !bRecoil;
			handleRecoil(moduleBase, &recoilOpcodes, bRecoil);

			SetConsoleCursorPosition(h, bufferInfo.dwCursorPosition);
			printCheatInfo(bHealth, bAmmo, bRecoil, rapidFireMode, bNades, bSpeed);
		}

		if (GetAsyncKeyState(VK_NUMPAD4) & 1)
		{
			rapidFireMode = (rapidFireMode * 2) % 32;
			if (!rapidFireMode)
			{
				rapidFireMode = 1;
			}
			handleRapidFire(moduleBase, &rapidOpcodes, rapidFireMode);

			SetConsoleCursorPosition(h, bufferInfo.dwCursorPosition);
			printCheatInfo(bHealth, bAmmo, bRecoil, rapidFireMode, bNades, bSpeed);
		}

		if (GetAsyncKeyState(VK_NUMPAD5) & 1)
		{
			bNades = !bNades;
			handleGrenade(moduleBase, &nadeOpcodes, bNades);

			SetConsoleCursorPosition(h, bufferInfo.dwCursorPosition);
			printCheatInfo(bHealth, bAmmo, bRecoil, rapidFireMode, bNades, bSpeed);
		}

		// disabled for now since it's unstable
		if (GetAsyncKeyState(VK_NUMPAD6) & 1 && false)
		{
			bSpeed = !bSpeed;
			handleSpeedHack(moduleBase, &speedOpcodes, bSpeed);

			SetConsoleCursorPosition(h, bufferInfo.dwCursorPosition);
			printCheatInfo(bHealth, bAmmo, bRecoil, rapidFireMode, bNades, bSpeed);
		}

		if (GetAsyncKeyState(VK_NUMPAD7) & 1)
		{
			// increment # of kills by 10
			*(int*)(localPlayer + 0x1FC) += 10;
		}

		if (GetAsyncKeyState(VK_NUMPAD8) & 1)
		{
			coordinates[0] = *(float*)(localPlayer + 0x34);
			coordinates[1] = *(float*)(localPlayer + 0x38);
			coordinates[2] = *(float*)(localPlayer + 0x3C);
			coordSet = true;
		}

		if (GetAsyncKeyState(VK_NUMPAD9) & 1)
		{
			if (coordSet)
			{
				*(float*)(localPlayer + 0x34) = coordinates[0];
				*(float*)(localPlayer + 0x38) = coordinates[1];
				*(float*)(localPlayer + 0x3C) = coordinates[2];
			}
		}
		// Keep updating the ammo address so that it always points at the current weapon ammo
		ammoAddr = mem::FindDMAAddy(moduleBase + 0x10F4F4, { 0x374, 0x14, 0x0 });
		Sleep(10);
	}

	// Cleanup & eject
	if (f)
	{
		fclose(f);
	}
	FreeConsole();
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
	if (rapidFireMode == 1)
	{
		Hook((void*)hookAddress, rapidFire, hookLength, false, oldOpCodes);
	}
	else
	{
		Hook((void*)hookAddress, rapidFire, hookLength, true, oldOpCodes);
	}
}

void handleGrenade(uintptr_t moduleBase, std::vector<BYTE> *oldOpCodes, bool bNades)
{
	// Add a nade to the player's inventory if the grenade cheat was activated
	if (bNades)
	{
		*(int*)(localPlayer + 0x158) += 1;
	}
	// hook at grenadeAmmo reduction
	DWORD hookAddress = moduleBase + 0x63378;
	int hookLength = 5;
	jmpBackAddyNades = hookAddress + hookLength;
	Hook((void*)hookAddress, unlimNades, hookLength, bNades, oldOpCodes);
}

void handleSpeedHack(uintptr_t moduleBase, std::vector<BYTE>* oldOpCodes, bool bSpeed)
{
	// hook when current acceleration float is read / loaded
	DWORD hookAddress = moduleBase + 0x5B10C;
	int hookLength = 5;
	jmpBackAddySpeed = hookAddress + hookLength;
	Hook((void*)hookAddress, speedHack, hookLength, bSpeed, oldOpCodes);
}

