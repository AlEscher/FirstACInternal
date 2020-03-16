#include "pch.h"
#include "util.h"
#include <vector>
#include <iostream>

// clears a line in the console with spaces, needed because sometimes we overwrite
// the old text in the console with a shorter text / line
void clearLine(int limit)
{
	for (int i = 0; i < limit; i++)
	{
		std::cout << " ";
	}
	std::cout << std::endl;
}

void printCheatInfo(bool bHealth, bool bAmmo, bool bRecoil, int rapidFireMode, bool bTriggerBot, bool bFly, bool bNameChanger)
{
	std::cout << "---------------------------------------------" << std::endl;
	std::cout << "|-------Assault Cube Internal Trainer-------|" << std::endl;
	std::cout << "|---------------Made by Kekz----------------|" << std::endl;
	std::cout << "|-------------------------------------------|" << std::endl;
	std::cout << "| " << blue << "[NUM_PAD 1]" << white << " Godmode :";
	if (bHealth)
	{
		std::cout << green << "                ON" << white << "   |";
	}
	else
	{
		std::cout << red << "                OFF" << white << "  |";
	}
	clearLine(5);

	std::cout << "| " << blue << "[NUM_PAD 2]" << white << " Unlimited Ammo :";
	if (bAmmo)
	{
		std::cout << green << "         ON" << white << "   |";
	}
	else
	{
		std::cout << red << "         OFF" << white << "  |";
	}
	clearLine(5);

	std::cout << "| " << blue << "[NUM_PAD 3]" << white << " No Recoil :";
	if (bRecoil)
	{
		std::cout << green << "              ON" << white << "   |";
	}
	else
	{
		std::cout << red << "              OFF" << white << "  |";
	}
	clearLine(5);

	std::cout << "| " << blue << "[NUM_PAD 4]" << white << " Rapid Fire :";
	if (rapidFireMode == 1)
	{
		std::cout << red <<  "             OFF" << white << "  |";
	}
	else if (rapidFireMode == 16)
	{
		std::cout << yellow << "          INSANE" << white << "  |";
	}
	else
	{
		std::cout << green << "              x" << rapidFireMode << white << "  |";
	}
	clearLine(5);
	
	std::cout << "| " << blue << "[NUM_PAD 5]" << white << " Triggerbot :";
	if (bTriggerBot)
	{
		std::cout << green << "             ON" << white << "   |";
	}
	else
	{
		std::cout << red << "             OFF" << white << "  |";
	}
	clearLine(5);

	std::cout << "| " << blue << "[NUM_PAD 6]" << white << " Fly hack :";
	if (bFly)
	{
		std::cout << green << "               ON" << white << "   |";
	}
	else
	{
		std::cout << red << "               OFF" << white << "  |";
	}
	clearLine(5);

	std::cout << "| " << blue << "[NUM_PAD 0]" << white << " Name Changer :";
	if (bNameChanger)
	{
		std::cout << green << "           ON" << white << "   |";
	}
	else
	{
		std::cout << red << "           OFF" << white << "  |";
	}
	clearLine(5);
	std::cout << "| " << blue << "[NUM_PAD 7]" << white << " Add 10 frags to Scoreboard    |\n";
	std::cout << "| " << blue << "[NUM_PAD 8]" << white << " Set teleport coordinates      |\n";
	std::cout << "| " << blue << "[NUM_PAD 9]" << white << " Teleport to set coordinates   |\n";
	std::cout << "| " << blue << "[END]      " << white << " Eject                         |\n";
	std::cout << "---------------------------------------------" << std::endl;
}

bool Hook(void* toHook, void* ourFunct, int len, bool hook, std::vector<BYTE> *oldOpCodes) 
{
	(*oldOpCodes).resize(len);
	if (hook)
	{
		// the space we put our jump has to be at least 5 bytes
		if (len < 5)
		{
			return false;
		}

		DWORD curProtection;
		VirtualProtect(toHook, len, PAGE_EXECUTE_READWRITE, &curProtection);

		// save the current opCodes before placing our escape jump
		for (unsigned int i = 0; i < (*oldOpCodes).size(); i++)
		{
			(*oldOpCodes)[i] = *((BYTE*)toHook + i);
		}

		// nop out the instructions we want to overwrite
		memset(toHook, 0x90, len);

		// offset from the place where we hook to where we jump
		DWORD relativeAddress = ((DWORD)ourFunct - (DWORD)toHook) - 5;

		// E9 is jump
		*(BYTE*)toHook = 0xE9;
		// + 1 so that we don't overwrite E9
		*(DWORD*)((DWORD)toHook + 1) = relativeAddress;

		DWORD temp;
		VirtualProtect(toHook, len, curProtection, &temp);

		return true;
	}
	// revert hook
	else
	{
		DWORD curProtection;
		VirtualProtect(toHook, len, PAGE_EXECUTE_READWRITE, &curProtection);
		// rewrite the old opcodes
		for (unsigned int i = 0; i < (*oldOpCodes).size(); i++)
		{
			*((BYTE*)toHook + i) = (*oldOpCodes)[i];
		}
		DWORD temp;
		VirtualProtect(toHook, len, curProtection, &temp);
		return true;
	}
}