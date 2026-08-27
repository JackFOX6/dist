#define _GNU_SOURCE
#include <sys/ptrace.h>
#include <stdarg.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dlfcn.h>
#include <link.h>
#include <elf.h>
#include <fcntl.h>
#include <dirent.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <vector>

#define LOG_FILE "/tmp/injector_linux.log"

static FILE* fLog = nullptr;

static void Log(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    if (fLog) { vfprintf(fLog, fmt, args); fprintf(fLog, "\n"); fflush(fLog); }
    vprintf(fmt, args);
    printf("\n");
    va_end(args);
}

struct RemoteContext {
    void* dlopen_addr;
    void* dlclose_addr;
    void* free_addr;
    char path[256];
    void* handle;
};

static bool GetProcessIdByName(const char* name, pid_t& outPid) {
    DIR* dir = opendir("/proc");
    if (!dir) return false;

    dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        int pid = atoi(entry->d_name);
        if (pid <= 1) continue;

        char commPath[256], cmdlinePath[256];
        snprintf(commPath, sizeof(commPath), "/proc/%d/comm", pid);
        FILE* f = fopen(commPath, "r");
        if (f) {
            char comm[256];
            if (fgets(comm, sizeof(comm), f)) {
                comm[strcspn(comm, "\n")] = 0;
                if (strcmp(comm, name) == 0) { outPid = pid; fclose(f); closedir(dir); return true; }
            }
            fclose(f);
        }

        snprintf(cmdlinePath, sizeof(cmdlinePath), "/proc/%d/cmdline", pid);
        f = fopen(cmdlinePath, "r");
        if (f) {
            char buf[1024];
            if (fgets(buf, sizeof(buf), f)) {
                if (strstr(buf, name)) { outPid = pid; fclose(f); closedir(dir); return true; }
            }
            fclose(f);
        }
    }
    closedir(dir);
    return false;
}

static pid_t FindTargetProcess() {
    const char* names[] = {"java", "javaw", "ProjectZomboid", nullptr};
    for (int i = 0; names[i]; i++) {
        pid_t pid = 0;
        if (GetProcessIdByName(names[i], pid)) return pid;
    }
    return 0;
}

static bool ResolveRemoteSymbol(pid_t pid, const char* libName, const char* symName, void*& outAddr) {
    outAddr = dlsym(RTLD_NEXT, symName);
    return outAddr != nullptr;
}

static bool InjectLibrary(pid_t pid, const char* libPath) {
    Log("[*] Attaching to PID %d", pid);

    if (ptrace(PTRACE_ATTACH, pid, nullptr, nullptr) < 0) {
        Log("[-] PTRACE_ATTACH failed");
        return false;
    }

    int status;
    waitpid(pid, &status, 0);

    user_regs_struct regs;
    if (ptrace(PTRACE_GETREGS, pid, nullptr, &regs) < 0) {
        Log("[-] PTRACE_GETREGS failed");
        ptrace(PTRACE_DETACH, pid, nullptr, nullptr);
        return false;
    }

    Log("[+] Registers read successfully");

    void* dlopen_addr = (void*)dlsym(RTLD_NEXT, "dlopen");
    if (!dlopen_addr) {
        Log("[-] Could not resolve dlopen");
        ptrace(PTRACE_DETACH, pid, nullptr, nullptr);
        return false;
    }

    Log("[+] dlopen address: %p", dlopen_addr);

    size_t pathLen = strlen(libPath) + 1;
    std::vector<char> remotePath(pathLen + 8);
    memcpy(remotePath.data(), libPath, pathLen);

    long pagesize = sysconf(_SC_PAGE_SIZE);
    long remoteStack = regs.rsp - 0x1000;

    for (size_t i = 0; i < pathLen; i += sizeof(long)) {
        long val = 0;
        memcpy(&val, libPath + i, std::min(sizeof(long), pathLen - i));
        ptrace(PTRACE_POKEDATA, pid, remoteStack + i, val);
    }

    regs.rdi = remoteStack;
    regs.rsi = RTLD_NOW;
    regs.rdx = 0;
    regs.rip = (unsigned long)dlopen_addr;

    Log("[*] Setting up remote call to dlopen");

    if (ptrace(PTRACE_SETREGS, pid, nullptr, &regs) < 0) {
        Log("[-] PTRACE_SETREGS failed");
        ptrace(PTRACE_DETACH, pid, nullptr, nullptr);
        return false;
    }

    Log("[+] Resuming execution");
    ptrace(PTRACE_DETACH, pid, nullptr, nullptr);

    return true;
}

int main(int argc, char* argv[]) {
    fLog = fopen(LOG_FILE, "w");
    if (!fLog) fLog = stderr;

    Log("=== Linux Injector (ptrace) ===");

    const char* libPath = "./zomboid_hack.so";
    if (argc > 1) libPath = argv[1];

    if (access(libPath, R_OK) != 0) {
        Log("[-] Library not found: %s", libPath);
        return 1;
    }

    pid_t pid = FindTargetProcess();
    if (pid == 0) {
        Log("[-] Target process not found");
        Log("[-] Looking for: java, ProjectZomboid processes");
        return 1;
    }

    Log("[+] Found target PID: %d", pid);

    if (!InjectLibrary(pid, libPath)) {
        Log("[-] Injection failed");
        return 1;
    }

    Log("[SUCCESS] Injection completed");
    return 0;
}