# ⚡ KahluaBridge — C++17 Runtime Instrumentation & JVM Interoperability Engine

[![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20Windows-blue.svg)](https://github.com/JackFOX6/dist)
[![Architecture](https://img.shields.io/badge/Architecture-C%2B%2B17%20%2F%20JNI%20%2F%20OpenGL-orange.svg)](https://github.com/JackFOX6/dist)
[![Standard](https://img.shields.io/badge/Standard-Zero--Overhead%20%2F%20Thread--Safe-green.svg)](https://github.com/JackFOX6/dist)
[![License](https://img.shields.io/badge/License-MIT-purple.svg)](LEGAL.md)

## 📌 Executive Summary

**KahluaBridge** is a high-performance native systems engineering framework written in modern **C++17** and **JNI (Java Native Interface)**. It provides real-time runtime instrumentation, low-latency memory inspection, and cross-language interoperability between native C++ modules, the Java Virtual Machine (JVM), and embedded Kahlua/Lua scripting runtimes.

Designed specifically for high-throughput, low-latency game engines and simulation runtimes (such as Project Zomboid Build 42), KahluaBridge achieves deterministic 60Hz telemetry and spatial debugging overlays with zero garbage collection overhead.

---

## 🚀 Key Architectural Pillars

### 1. ⚙️ Dual-Platform Native Subsystem
* **Linux Subsystem**:
  * Shared object (`.so`) runtime instrumentation.
  * Dynamic graphics API context hooking (`glXSwapBuffers` in X11 and `eglSwapBuffers` in Wayland/EGL) via symbol interception.
  * Low-level X11 input queue dispatching and event filtering.
* **Windows Subsystem**:
  * Native Dynamic Link Library (`.dll`) architecture.
  * OpenGL swapchain interception (`wglSwapBuffers`) leveraging MinHook.
  * Automated build pipeline via PowerShell (`build.ps1`).

---

### 2. ⚡ Zero-Overhead JNI Reflection Caching (60Hz Engine)
* **GlobalRef Initialization**: Pre-caches `jclass`, `jfieldID`, and `jmethodID` references during engine bootstrap (`Initialize()`), entirely eliminating runtime reflection lookups (`FindClass` / `GetMethodID`) in the hot loop.
* **Thread-Safe JVM Monitors**: Enforces strict synchronization boundaries across native threads and the JVM to prevent `ConcurrentModificationException` during spatial queries.
* **Immutable Snapshots**: Converts live entity collections to static buffers, decoupling rendering passes from active JVM garbage collection cycles.

---

### 3. 🔍 Spatial Telemetry & Debugging Pipeline (ImGui OpenGL3)
* **Real-time Spatial Tracking**: High-efficiency 3D bounding calculations (X, Y, Z coordinates, velocity vectors, floor deltas, and state flags).
* **Hardware-Accelerated Overlay**: Direct OpenGL rendering pipeline with zero frame latency and minimal memory footprint.
* **Entity Telemetry**: Diagnostic inspection of live character stats, vehicle state metrics, container states, and inventory trees.

---

### 4. 🔄 Cross-Language Scripting Bridge (LuaManager / JVM Interop)
* **Bidirectional State Sync**: Real-time synchronization between Java simulation classes (`SandboxOptions`) and Lua runtime environments (`SandboxVars`).
* **Synchronous Native Execution**: Native API dispatch interface for automated integration testing and runtime diagnostics.

---

## 🛠️ Repository Structure

```text
.
├── CMakeLists.txt              # Cross-Platform CMake configuration
├── build.ps1                   # Automated build & packaging script (Windows)
├── include/
│   └── jni_helpers.h           # Unified JNI headers, telemetry structs & prototypes
├── linux/
│   ├── CMakeLists.txt          # Target configuration for Linux shared objects (.so)
│   ├── main_linux.cpp          # Linux entry point, OpenGL hooks & ImGui pipeline
│   └── injector_ptrace.cpp     # Process attachment & memory instrumentation
├── src/
│   ├── main.cpp                # Native harness test runner
│   └── jni_helpers.cpp         # JNI cache implementation, JVM manipulation & reflection
├── vendor/                     # Third-party SDKs (ImGui, MinHook, GLAD, GL3W, JNI Headers)
└── LEGAL.md                    # Research, security analysis & licensing disclaimer
```

---

## ⚙️ Building and Compilation

### Linux Target (Shared Object)

```bash
# 1. Enter the linux build tree
cd linux
mkdir -p build && cd build

# 2. Configure and compile with CMake
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

# Output: libkahluabridge.so
```

### Windows Target (Native DLL)

```powershell
# Execute the automated build script
.\build.ps1
```

---

## ⚖️ License & Disclaimer

This project is distributed under the [MIT License](LEGAL.md) and was developed for systems engineering research, runtime performance optimization, and cross-platform native interoperability studies.
