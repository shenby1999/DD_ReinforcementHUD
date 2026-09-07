#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <string>

int main(int argc, char** argv)
{
    if (argc < 2) { fprintf(stderr, "usage: injector <pid> [dllpath]\n"); return 1; }
    DWORD pid = (DWORD)atoi(argv[1]);
    std::string dll = argc >= 3 ? argv[2] : "ReinforcementHudColor.dll";

    HANDLE h = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!h) { fprintf(stderr, "OpenProcess error %lu\n", GetLastError()); return 1; }

    size_t len = dll.size() + 1;
    void* remote = VirtualAllocEx(h, NULL, len, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
    if (!remote) { fprintf(stderr, "VirtualAllocEx error %lu\n", GetLastError()); CloseHandle(h); return 1; }
    if (!WriteProcessMemory(h, remote, dll.c_str(), len, NULL)) { fprintf(stderr, "WriteProcessMemory error %lu\n", GetLastError()); return 1; }

    HMODULE k32 = GetModuleHandleA("kernel32.dll");
    FARPROC loadLib = GetProcAddress(k32, "LoadLibraryA");
    HANDLE thread = CreateRemoteThread(h, NULL, 0, (LPTHREAD_START_ROUTINE)loadLib, remote, 0, NULL);
    if (!thread) { fprintf(stderr, "CreateRemoteThread error %lu\n", GetLastError()); return 1; }
    WaitForSingleObject(thread, INFINITE);
    DWORD exitCode = 0;
    GetExitCodeThread(thread, &exitCode);
    printf("LoadLibrary thread exit = 0x%lX\n", exitCode);
    VirtualFreeEx(h, remote, 0, MEM_RELEASE);
    CloseHandle(thread);
    CloseHandle(h);
    return 0;
}
