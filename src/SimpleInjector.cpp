#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <string>

#define LOG_FILE "simple_injector.log"

void Log(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    FILE* fLog = fopen(LOG_FILE, "a");
    if (fLog) {
        vfprintf(fLog, fmt, args);
        fprintf(fLog, "\n");
        fclose(fLog);
    }

    vprintf(fmt, args);
    printf("\n");

    va_end(args);
}

DWORD GetProcessIdByName(const char* processName) {
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        Log("[-] CreateToolhelp32Snapshot failed: %lu", GetLastError());
        return 0;
    }

    if (Process32First(hSnapshot, &pe32)) {
        do {
            if (strcmp(pe32.szExeFile, processName) == 0) {
                CloseHandle(hSnapshot);
                return pe32.th32ProcessID;
            }
        } while (Process32Next(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);
    return 0;
}

bool InjectDLL(DWORD pid, const char* dllPath) {
    Log("[*] Starting LoadLibrary Injection for PID %lu", pid);

    // 1. Open Process
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProcess) {
        Log("[-] OpenProcess failed: %lu", GetLastError());
        return false;
    }
    Log("[+] Opened process");

    // 2. Get full path
    char fullPath[MAX_PATH];
    if (!GetFullPathNameA(dllPath, MAX_PATH, fullPath, nullptr)) {
        Log("[-] GetFullPathNameA failed: %lu", GetLastError());
        CloseHandle(hProcess);
        return false;
    }
    Log("[+] Full DLL path: %s", fullPath);

    // 3. Allocate memory for path
    SIZE_T pathLen = strlen(fullPath) + 1;
    LPVOID remotePath = VirtualAllocEx(hProcess, nullptr, pathLen, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remotePath) {
        Log("[-] VirtualAllocEx failed: %lu", GetLastError());
        CloseHandle(hProcess);
        return false;
    }
    Log("[+] Allocated memory at %p", remotePath);

    // 4. Write path to remote process
    if (!WriteProcessMemory(hProcess, remotePath, fullPath, pathLen, nullptr)) {
        Log("[-] WriteProcessMemory failed: %lu", GetLastError());
        VirtualFreeEx(hProcess, remotePath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }
    Log("[+] Wrote DLL path to remote process");

    // 5. Get LoadLibraryA address
    HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
    if (!hKernel32) {
        Log("[-] GetModuleHandleA(kernel32.dll) failed");
        VirtualFreeEx(hProcess, remotePath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    LPVOID loadLibAddr = (LPVOID)GetProcAddress(hKernel32, "LoadLibraryA");
    if (!loadLibAddr) {
        Log("[-] GetProcAddress(LoadLibraryA) failed");
        VirtualFreeEx(hProcess, remotePath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }
    Log("[+] LoadLibraryA address: %p", loadLibAddr);

    // 6. Create remote thread
    HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0, (LPTHREAD_START_ROUTINE)loadLibAddr, remotePath, 0, nullptr);
    if (!hThread) {
        Log("[-] CreateRemoteThread failed: %lu", GetLastError());
        VirtualFreeEx(hProcess, remotePath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }
    Log("[+] Created remote thread");

    // 7. Wait for thread
    WaitForSingleObject(hThread, INFINITE);
    
    DWORD exitCode = 0;
    GetExitCodeThread(hThread, &exitCode);
    Log("[+] Thread finished with exit code: %lu (0x%X)", exitCode, exitCode);

    // NOTE: On x64, LoadLibraryA returns a 64-bit HMODULE but GetExitCodeThread
    // only captures the low 32 bits. A high-address load can make it look like 0.
    // Verify by scanning the module list instead.
    Sleep(500);
    bool dllFound = false;
    {
        HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
        if (hSnap != INVALID_HANDLE_VALUE) {
            MODULEENTRY32 me32;
            me32.dwSize = sizeof(MODULEENTRY32);
            // Extract just the filename from dllPath for comparison
            const char* dllFile = strrchr(dllPath, '\\');
            if (!dllFile) dllFile = strrchr(dllPath, '/');
            dllFile = dllFile ? dllFile + 1 : dllPath;
            if (Module32First(hSnap, &me32)) {
                do {
                    if (_stricmp(me32.szModule, dllFile) == 0) {
                        dllFound = true;
                        Log("[+] Confirmed: DLL loaded at base 0x%p", me32.modBaseAddr);
                        break;
                    }
                } while (Module32Next(hSnap, &me32));
            }
            CloseHandle(hSnap);
        }
    }

    if (!dllFound && exitCode == 0) {
        Log("[-] LoadLibrary failed in remote process");
        CloseHandle(hThread);
        VirtualFreeEx(hProcess, remotePath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    Log("[+] DLL loaded successfully (exit code may be truncated on x64)");

    // Cleanup
    CloseHandle(hThread);
    VirtualFreeEx(hProcess, remotePath, 0, MEM_RELEASE);
    CloseHandle(hProcess);

    return true;
}

int main(int argc, char* argv[]) {
    Log("=== Simple LoadLibrary Injector ===");

    // Determine DLL path (use arg if provided)
    const char* dllName = "ZomboidHack.dll";
    if (argc > 1) {
        dllName = argv[1];
        Log("[*] Using custom DLL path: %s", dllName);
    }

    // Find window
    HWND hWnd = FindWindowA(nullptr, "Project Zomboid");
    DWORD pid = 0;

    if (!hWnd) {
        Log("[-] Window 'Project Zomboid' not found. Trying process scan...");
        pid = GetProcessIdByName("ProjectZomboid64.exe");
        if (!pid) pid = GetProcessIdByName("ProjectZomboid.exe");
        if (!pid) pid = GetProcessIdByName("java.exe");

        if (!pid) {
            Log("[-] Process not found. Ensure game is running.");
            system("pause");
            return 1;
        }
        Log("[+] Found process by scan: PID %lu", pid);
    } else {
        GetWindowThreadProcessId(hWnd, &pid);
        Log("[+] Found window. PID %lu", pid);
    }

    // Inject
    if (InjectDLL(pid, dllName)) {
        Log("[SUCCESS] Injection complete!");
    } else {
        Log("[FAIL] Injection failed.");
    }

    system("pause");
    return 0;
}
