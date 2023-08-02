#include <windows.h>
#include <stdio.h>
#include <tlhelp32.h>
#include <Shlwapi.h>
#include <Windows.h>
#include <stdio.h>
#include <fstream> 
using std::ofstream;
ofstream log_file("log.txt");
// LPSTR DLL_PATH; // If name of dll is passed as argument to exe
#define DLL_PATH "HeapRead_dll.dll" // Otherwise (built-in in exe)
#define true 1
#define false 0

BOOL dllInjector(const char* dllpath, DWORD pID);


int main(int argc, char** argv)
{
    // 1) Create Process SUSPENDED
    PROCESS_INFORMATION pi;
    STARTUPINFOA Startup;
    ZeroMemory(&Startup, sizeof(Startup));
    ZeroMemory(&pi, sizeof(pi));

    // 2) Get command line. Two options:

    // 2a) Get command line from current process + arguments, check validity of args:
    //LPSTR lpCmdLine = GetCommandLineA();
    //if (argc < 3) {
    //    printf("Usage: %s prog_name dll_name\n", argv[0]);
    //    return 1;
    //}
    //LPSTR lpCmdLine = (LPSTR)argv[1];
    //DLL_PATH = (LPSTR)argv[2];

    // OR 2b) Fix a specific command line (can include arguments):
    LPSTR lpCmdLine = (LPSTR)"crackme.exe ""U""";

    // 3) Create process to run cmdline
    if (CreateProcessA("crackme.exe", (LPSTR)"crackme.exe U", NULL, NULL, NULL, CREATE_SUSPENDED, NULL, NULL, &Startup, &pi) == FALSE) {
        printf("couldnt open process %s\n", lpCmdLine);
        
        return 1;
    }
    
    // 4) Inject DLL (call setHook, etc.)
    if (!(dllInjector(DLL_PATH, pi.dwProcessId))) {
        printf("couldnt inject dll");
        return 1;
    }

    // 5) Wait and finish injection
    Sleep(1000); // Let the DLL finish loading
    ResumeThread(pi.hThread);
    log_file << pi.dwProcessId << std::endl;
    //printf("Injected dll successfully\n");
    return 0;
}

BOOL dllInjector(const char* dllpath, DWORD pID)
{
    HANDLE pHandle;
    LPVOID remoteString;
    LPVOID remoteLoadLib;

    pHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pID);

    if (!pHandle) {
        printf("couldnt open proccess with perms\n");
        return false;
    }


    remoteLoadLib = (LPVOID)GetProcAddress(GetModuleHandle(L"kernel32.dll"), "LoadLibraryA");

    remoteString = (LPVOID)VirtualAllocEx(pHandle, NULL, strlen(DLL_PATH) + 1, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    WriteProcessMemory(pHandle, (LPVOID)remoteString, dllpath, strlen(dllpath), NULL);
    if (NULL == CreateRemoteThread(pHandle, NULL, NULL, (LPTHREAD_START_ROUTINE)remoteLoadLib, (LPVOID)remoteString, NULL, NULL)) {
        return false;
    }
    CloseHandle(pHandle);

    return true;
}