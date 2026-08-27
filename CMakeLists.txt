cmake_minimum_required(VERSION 3.10)
project(Prototipo2)

set(CMAKE_CXX_STANDARD 17)

# =========================================================================================
# OPTIMIZACIÓN PARA i3-2100 + RX 550 (LOW-SPEC TARGET)
# =========================================================================================
# Objetivo: Minimizar FPS drops y sluttering manteniendo funcionalidad
# Hardware: Intel Core i3-2100 / 4GB RAM / RX 550 2GB
# =========================================================================================

# --- NINJA BUILD SYSTEM (Más rápido que NMake) ---
# Nota: Usar 'cmake -G Ninja ..' para compilar
# Si Ninja no está disponible,会自动 usar NMake

# --- BUILD TYPE: MinSizeRel (DLL más pequeño + optimizado) ---
# Opciones: Debug, Release, MinSizeRel, RelWithDebInfo
# MinSizeRel: Optimizado para tamaño mínimo (~30% más pequeño que Release)
# Generator: Visual Studio 18 2026 (reemplazar si hay otra versión disponible)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES AND NOT DEFINED CMAKE_GENERATOR)
    message(STATUS "Setting build type to 'MinSizeRel' as none was specified.")
    set(CMAKE_BUILD_TYPE "MinSizeRel" CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# --- COMPILER OPTIMIZATION FLAGS ---
if(MSVC)
    # Flags específicos para CPUs Intel sin AVX (i3-2100 = SSSE3 max)
    set(CMAKE_CXX_FLAGS_INIT "/DWIN32 /D_WINDOWS /W3 /GR /EHsc")
    
    # Configuración por build type
    set(CMAKE_CXX_FLAGS_DEBUG "/MTd /Zi /Ob0 /Od /RTC1")
    set(CMAKE_CXX_FLAGS_MINSIZEREL "/MT /O1 /Ob1 /DNDEBUG /GL /Gy")
    set(CMAKE_CXX_FLAGS_RELEASE "/MT /O2 /Ob2 /DNDEBUG /GL /Gy")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/MT /Zi /O2 /Ob1 /DNDEBUG /GL /Gy")
    
    # Link-Time Optimization (LTCG) para MSVC
    set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "/LTCG /OPT:ICF /OPT:REF /INCREMENTAL:NO")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/LTCG /OPT:ICF /OPT:REF /INCREMENTAL:NO")
    set(CMAKE_SHARED_LINKER_FLAGS_MINSIZEREL "/LTCG /OPT:ICF /OPT:REF /INCREMENTAL:NO")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "/LTCG /OPT:ICF /OPT:REF /INCREMENTAL:NO")
    
    message(STATUS "Using MSVC compiler")
elseif(MINGW)
    # Flags específicos para MinGW/GCC (RX 550 + i3-2100)
    set(CMAKE_CXX_FLAGS_INIT "-DWIN32 -D_WINDOWS -Wall")
    
    # Configuración por build type para MinGW
    set(CMAKE_CXX_FLAGS_MINSIZEREL "-O2 -DNDEBUG -ffunction-sections -fdata-sections -static -static-libgcc -static-libstdc++ -Wl,-Bstatic -lstdc++ -lpthread -lwinpthread -Wl,-Bdynamic")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -ffunction-sections -fdata-sections -static-libgcc -static-libstdc++")
    
    # Link-Time Optimization para MinGW (equivalent a /LTCG)
    set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "-Wl,--gc-sections")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-Wl,--gc-sections")
    set(CMAKE_SHARED_LINKER_FLAGS_MINSIZEREL "-Wl,--gc-sections -static-libgcc -static-libstdc++ -Wl,-Bstatic -lstdc++ -lpthread -lwinpthread -Wl,-Bdynamic")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "-Wl,--gc-sections")
    
    message(STATUS "Using MinGW/GCC compiler")
else()
    # GCC/Clang por defecto
    set(CMAKE_CXX_FLAGS_INIT "-DWIN32 -D_WINDOWS -Wall")
    set(CMAKE_CXX_FLAGS_MINSIZEREL "-O2 -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
    message(STATUS "Using default GCC/Clang compiler")
endif()

# --- COMPILER MEMORY OPTIMIZATION (R-LOWSPEC) ---
# Reducir uso de memoria durante compilación
set(CMAKE_CXX_USE_RESPONSE_FILE_FOR_OBJECTS 0)
set(CMAKE_C_USE_RESPONSE_FILE_FOR_OBJECTS 0)
set(CMAKE_RC_USE_RESPONSE_FILE_FOR_OBJECTS 0)

# --- DEPENDENCY: MINHOOK (SOURCE BUILD) ---
# Compiling from source ensures ABI compatibility with the current toolchain
add_subdirectory(vendor/minhook)

# --- MAIN TARGET ---
add_library(Prototipo2 SHARED
    src/main.cpp
    src/jni_helpers.cpp
    # ImGui Sources (Direct compilation for LTO optimization)
    vendor/imgui/imgui.cpp
    vendor/imgui/imgui_draw.cpp
    vendor/imgui/imgui_tables.cpp
    vendor/imgui/imgui_widgets.cpp
    vendor/imgui/backends/imgui_impl_win32.cpp
    vendor/imgui/backends/imgui_impl_opengl3.cpp
)

# --- MSVC: Disable specific warnings that bloat binary with info ---
if(MSVC)
    target_compile_options(Prototipo2 PRIVATE
        /wd4100  # unreferenced formal parameter
        /wd4127  # conditional expression is constant
        /wd4189  # local variable is initialized but not referenced
        /wd4201  # nameless struct/union
        /wd4244  # conversion, possible loss of data
        /wd4389  # signed/unsigned mismatch
    )
endif()

# --- INJECTOR TARGET (Manual Map - Advanced) ---
add_executable(ZomboidInjector
    src/Injector.cpp
)
if(MINGW)
    target_link_libraries(ZomboidInjector PRIVATE -ldbghelp -lpsapi -lkernel32)
else()
    target_link_libraries(ZomboidInjector PRIVATE Dbghelp.lib Psapi.lib kernel32.lib)
endif()

# --- SIMPLE INJECTOR TARGET (LoadLibrary - Reliable) ---
add_executable(SimpleInjector
    src/SimpleInjector.cpp
)
target_link_libraries(SimpleInjector PRIVATE kernel32.lib)

# --- INCLUDES ---
target_include_directories(Prototipo2 PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/java/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/java/include/win32"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/backends"
)

# Link directly to the minhook target defined in the subdirectory
# MinHook handles its own include directories via 'target_include_directories(... PUBLIC ...)'
target_link_libraries(Prototipo2 PRIVATE minhook)

# --- STATIC LINKING: Embed GCC/C++ runtime into the DLL ---
# Eliminates libstdc++-6.dll dependency. libwinpthread-1.dll is handled
# by deploying it to the Proton prefix (drive_c/windows/system32/).
if(MINGW)
    target_link_options(Prototipo2 PRIVATE -static-libgcc -static-libstdc++)
endif()

# --- OUTPUT CONFIG ---
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_SHARED_LIBRARY_PREFIX "")
set_target_properties(Prototipo2 PROPERTIES OUTPUT_NAME "ZomboidHack")

# --- WINDOWS SPECIFIC ---
if(WIN32)
    target_compile_definitions(Prototipo2 PRIVATE _WINDOWS)
    target_link_libraries(Prototipo2 PRIVATE
        user32.lib
        gdi32.lib
        opengl32.lib
        dwmapi.lib
        d3d11.lib
    )
endif()
