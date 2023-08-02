#include "pch.h"
#include <Windows.h>
#include <stdio.h>
// Add additional includes if needed
// using namespace std;

// for logging. format: log_file << ... << std::endl;
#include <fstream> 
using std::ofstream;
ofstream log_file("log.txt", std::ios::app);

// Typedef for the hooked function signature, such as:
typedef char*(*FUNC_PTR)(int);
typedef void (*FUNC_PTR2)();
//(WINAPI is for _stdcall)
// or in the simple case:


// Ptr to the original function
FUNC_PTR original_malloc;
FUNC_PTR2 original_exit;
// Global variables
DWORD lpProtect = 0;
LPVOID IAT_malloc;
LPVOID IAT_exit;
FUNC_PTR JumpTo_malloc;
FUNC_PTR JumpTo_exit;
void* addr;
// Helper function to remove the hook (for the current call):
void _stdcall remove_malloc_hook() {
	VirtualProtect((char*)IAT_malloc, 0x4, PAGE_EXECUTE_READWRITE, &lpProtect);
	memcpy(IAT_malloc, &original_malloc, 0x4);
	VirtualProtect((char*)IAT_malloc, 0x4, PAGE_EXECUTE_READ, &lpProtect);
}

// Helper function to restore the hook (for the next calls)
void _stdcall restore_malloc_hook() {
	VirtualProtect((char*)IAT_malloc, 0x4, PAGE_EXECUTE_READWRITE, &lpProtect);
	memcpy(IAT_malloc, &JumpTo_malloc, 0x4);
	VirtualProtect((char*)IAT_malloc, 0x4, PAGE_EXECUTE_READ, &lpProtect);
}
void _stdcall remove_exit_hook() {
	VirtualProtect((char*)IAT_exit, 0x4, PAGE_EXECUTE_READWRITE, &lpProtect);
	memcpy(IAT_exit, &original_exit, 0x4);
	VirtualProtect((char*)IAT_exit, 0x4, PAGE_EXECUTE_READ, &lpProtect);
}

// Helper function to restore the hook (for the next calls)
void _stdcall restore_exit_hook() {
	VirtualProtect((char*)IAT_exit, 0x4, PAGE_EXECUTE_READWRITE, &lpProtect);
	memcpy(IAT_exit, &JumpTo_exit, 0x4);
	VirtualProtect((char*)IAT_exit, 0x4, PAGE_EXECUTE_READ, &lpProtect);
}
// Hook function. Might use helper functions in C, i.e. void _stdcall helper(int num) {}
// Might even use only a funcHook c function instead!
void* _cdecl mallocHook(int size) {
	// Restore overriden bytes
	
	addr = original_malloc(size);
	log_file << std::hex << "0x" << addr << " " << "0x" << size << std::endl;
	// Assembly part. Might call restore_hook somewhere inside
	return addr;
		
}
void _cdecl exitHook() {
	
	//for (size_t i = 0; i < tbl_size; i++)
	//{
		//log_file << "value in index " << i << ": " << ((char*)addr)[0] << std::endl;
	//}
	Sleep(1000000);
	original_exit();
}
void setHook() {
	
	HMODULE h = GetModuleHandle(L"crackme.exe");
	HMODULE h2 = GetModuleHandle(L"msvcrt.dll");
	if ((h == NULL) || (h2 == NULL)) { return; }

	original_malloc = (FUNC_PTR)GetProcAddress(h2, "malloc");
	if (original_malloc == NULL) {
		log_file << "failed finding malloc" << std::endl;
		return;
	}
	original_exit = (FUNC_PTR2)GetProcAddress(h2, "exit");
	if (original_malloc == NULL) {
		log_file << "failed finding cexit" << std::endl;
		return;
	}
	int addr_beginning_of_our_exe = 0x400000; // change!
	int addr_func_to_hook_in_IAT_malloc = 0x40E1DC; // change!
	int addr_func_to_hook_in_IAT_exit = 0x40E1C4; // change!
	IAT_malloc = h + (addr_func_to_hook_in_IAT_malloc - addr_beginning_of_our_exe) / 4; // Calc address of address to override in IAT
	IAT_exit = h + (addr_func_to_hook_in_IAT_exit - addr_beginning_of_our_exe) / 4; // Calc address of address to override in IAT
	JumpTo_malloc = (FUNC_PTR)((char*)&mallocHook);
	JumpTo_exit = (FUNC_PTR)((char*)&exitHook);
	restore_malloc_hook();
	restore_exit_hook();
}


BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		setHook();
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}