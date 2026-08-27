cmake_minimum_required(VERSION 3.16)
project(PrototipoFullLinux CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Paths
set(ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(JVM_INCLUDE "/usr/lib/jvm/java-26-openjdk/include")

# ImGui sources (renderer only - no Win32 backend)
set(IMGUI_SRC
    ${ROOT}/vendor/imgui/imgui.cpp
    ${ROOT}/vendor/imgui/imgui_draw.cpp
    ${ROOT}/vendor/imgui/imgui_tables.cpp
    ${ROOT}/vendor/imgui/imgui_widgets.cpp
    ${ROOT}/vendor/imgui/backends/imgui_impl_opengl3.cpp
)

add_library(zomboid_full SHARED
    main_linux.cpp
    ${ROOT}/src/jni_helpers.cpp
    ${IMGUI_SRC}
)

target_include_directories(zomboid_full PRIVATE
    ${ROOT}/include
    ${ROOT}/vendor/imgui
    ${ROOT}/vendor/imgui/backends
    ${JVM_INCLUDE}
    ${JVM_INCLUDE}/linux
)

target_link_libraries(zomboid_full PRIVATE
    GL
    EGL
    X11
    dl
    pthread
)

target_compile_options(zomboid_full PRIVATE
    -O3
    -fvisibility=hidden
    -fvisibility-inlines-hidden
    -ffunction-sections
    -fdata-sections
)

target_link_options(zomboid_full PRIVATE
    -Wl,--strip-all
    -Wl,-s
    -Wl,--gc-sections
)

set_target_properties(zomboid_full PROPERTIES
    OUTPUT_NAME "zomboid_full"
    PREFIX ""
    SUFFIX ".so"
)
