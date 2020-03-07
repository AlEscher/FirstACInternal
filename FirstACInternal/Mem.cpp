#include "pch.h"
#include "Mem.h"

// dst for the Bytes to be overwritten, src pointer to a BYTE array
void mem::PatchEx(BYTE* dst, BYTE* src, unsigned int size, HANDLE hProcess)
{
	DWORD oldProtect;
	// External version of VirtualProtect
	// Changes the page access rights for the instructions we want to patch, since Code is executable but not writable
	// If you only use WRITE permission, it can create problems if those instructions are being executed
	VirtualProtectEx(hProcess, dst, size, PAGE_EXECUTE_READWRITE, &oldProtect);
	// patch the bytes
	WriteProcessMemory(hProcess, dst, src, size, nullptr);
	// restore old memory access permissions
	VirtualProtectEx(hProcess, dst, size, oldProtect, &oldProtect);
}

void mem::NopEx(BYTE* dst, unsigned int size, HANDLE hProcess)
{
	// BYTE array on heap with number of bytes we want to NOP
	BYTE* nopArray = new BYTE[size];
	// Set every Byte to NOP
	memset(nopArray, 0x90, size);

	// Patch bytes
	PatchEx(dst, nopArray, size, hProcess);
	// Delete array on Heap
	delete[] nopArray;
}

// work through a multidimensional pointer
uintptr_t mem::FindDMAAddy(HANDLE hProc, uintptr_t ptr, std::vector<unsigned int> offsets)
{
	uintptr_t addr = ptr;
	for (unsigned int i = 0; i < offsets.size(); i++)
	{
		// read what's (BYTE*)addr and store it in addr (dereference the pointer)
		ReadProcessMemory(hProc, (BYTE*)addr, &addr, sizeof(addr), 0);
		// add offset
		addr += offsets[i];
	}
	return addr;
}



// Internal version

// dst for the Bytes to be overwritten, src pointer to a BYTE array
void mem::Patch(BYTE* dst, BYTE* src, unsigned int size)
{
	DWORD oldProtect;
	// Changes the page access rights for the instructions we want to patch, since Code is executable but not writable
	// If you only use WRITE permission, it can create problems if those instructions are being executed
	VirtualProtect(dst, size, PAGE_EXECUTE_READWRITE, &oldProtect);
	// patch the bytes
	memcpy(dst, src, size);
	// restore old memory access permissions
	VirtualProtect(dst, size, oldProtect, &oldProtect);
}

void mem::Nop(BYTE* dst, unsigned int size)
{
	DWORD oldProtect;
	VirtualProtect(dst, size, PAGE_EXECUTE_READWRITE, &oldProtect);
	// patch the bytes
	memset(dst, 0x90, size);
	// restore old memory access permissions
	VirtualProtect(dst, size, oldProtect, &oldProtect);
}

// work through a multidimensional pointer
uintptr_t mem::FindDMAAddy(uintptr_t ptr, std::vector<unsigned int> offsets)
{
	uintptr_t addr = ptr;
	for (unsigned int i = 0; i < offsets.size(); i++)
	{
		// read what's in addr and store it in addr (dereference the pointer)
		addr = *(uintptr_t*)addr;
		// add offset
		addr += offsets[i];
	}
	return addr;
}