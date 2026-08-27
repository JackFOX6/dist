# 🚀 Runtime Preload & Launch Guide — KahluaBridge

Technical guide for attaching and loading the native shared library (`libkahluabridge.so` / `zomboid_safemode.so`) on Linux environments.

---

## 1. Dynamic Preloading via Linux Dynamic Linker

Location of the launch script: `<Game_or_Engine_Path>/projectzomboid.sh`

### A. Enabling Runtime Instrumentation
Preload the native dynamic library before the JVM initializes:

```bash
XMODIFIERS= LD_PRELOAD="${HOME}/Dev/zomboid_safemode.so:${LD_PRELOAD}:${JSIG}:libPZXInitThreads64.so" ./ProjectZomboid64 "$@" >"${LOGFILE}"
```

### B. Standard Clean Execution
Factory default execution without instrumentation:

```bash
XMODIFIERS= LD_PRELOAD="${LD_PRELOAD}:${JSIG}:libPZXInitThreads64.so" ./ProjectZomboid64 "$@" >"${LOGFILE}"
```

---

## 2. Environment Variable Control (`ENABLE_SAFEMODE`)

You can control preloading dynamically via an environment flag:

```bash
if [ "$ENABLE_SAFEMODE" = "1" ]; then
    PRELOAD_LIST="${HOME}/Dev/zomboid_safemode.so:${LD_PRELOAD}:${JSIG}:libPZXInitThreads64.so"
else
    PRELOAD_LIST="${LD_PRELOAD}:${JSIG}:libPZXInitThreads64.so"
fi

XMODIFIERS= LD_PRELOAD="${PRELOAD_LIST}" ./ProjectZomboid64 "$@" >"${LOGFILE}"
```

### Terminal Commands:
* **Launch with Instrumentation:** `ENABLE_SAFEMODE=1 ./projectzomboid.sh`
* **Clean Launch:** `./projectzomboid.sh`

---

## 3. Real-Time Console Debugging

To view native diagnostics directly in the terminal output without log redirection:

```bash
XMODIFIERS= LD_PRELOAD="${HOME}/Dev/zomboid_safemode.so:${LD_PRELOAD}:${JSIG}:libPZXInitThreads64.so" ./ProjectZomboid64 "$@"
```
