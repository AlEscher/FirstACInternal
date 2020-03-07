#include "pch.h"
#include "Proc.h"

DWORD GetProcId(const wchar_t* procName)
{
	DWORD procID = 0;
	// Take snapshot of the processes (first argument)
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	// error checking for CreateToolhelp32Snapshot()
	if (hSnap != INVALID_HANDLE_VALUE)
	{
		PROCESSENTRY32 procEntry;
		// need to set this size (see documentation)
		procEntry.dwSize = sizeof(procEntry);

			// grab first process in snapshot and store it in procEntry
		if (Process32First(hSnap, &procEntry))
		{
			do 
			{
				// compare filename with our given process name
				// string compare with wide chars (case-insensitive)
				if (!_wcsicmp(procEntry.szExeFile, procName))
				{
					// found our process
					procID = procEntry.th32ProcessID;
					break;
				}
					// loop through processes in snapshot
			} while (Process32Next(hSnap, &procEntry));

		}
	}
	// close handle, avoids memory leaks
	CloseHandle(hSnap);
	return procID;
}

// uinptr_t compiles to 32 bit on 32bit-architecture and 64 bit on 64bit
uintptr_t GetModuleBaseAddress(DWORD procId, const wchar_t* modName)
{
	uintptr_t modBaseAddr = 0;
	// the bit-or means to give 32bit and 64bit modules
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, procId);

	if (hSnap != INVALID_HANDLE_VALUE)
	{
		MODULEENTRY32 modEntry;
		modEntry.dwSize = sizeof(modEntry);
		if (Module32First(hSnap, &modEntry))
		{
			do 
			{
				if (!_wcsicmp(modEntry.szModule, modName))
				{
					modBaseAddr = (uintptr_t)modEntry.modBaseAddr;
					break;
				}
			} while (Module32Next(hSnap, &modEntry));
		}
	}
	CloseHandle(hSnap);
	return modBaseAddr;
}