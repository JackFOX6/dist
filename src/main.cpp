#include <windows.h>
#include <cstdio>
#include <cmath>
#include <iostream>
#include "MinHook.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_opengl3.h"
#include "../include/jni_helpers.h"

// =========================================================================================
// GLOBAL STATE
// =========================================================================================

typedef BOOL (WINAPI* wglSwapBuffers_t)(HDC);
wglSwapBuffers_t o_wglSwapBuffers = nullptr;

bool g_showMenu = false;
bool g_espEnabled = false;
bool g_playerEspEnabled = false;
bool g_vehicleEspEnabled = false;
bool g_animalEspEnabled = false;
bool g_imguiInitialized = false;
bool g_imguiInitAttempted = false;
WNDPROC g_originalWndProc = nullptr;
HWND g_hWnd = nullptr;
FILE* fLog = nullptr;
CRITICAL_SECTION g_imguiMutex;

int g_frameCount = 0;

bool g_playerManuallyInitialized = false;

bool g_autoAttach = false;
bool g_stealthMode = false;
int g_fpsLimit = 60;
float g_espDistanceRange = 500.0f;

char g_perkID[256] = "Axe";
int g_xpAmount = 100;

// =========================================================================================
// LOGGING
// =========================================================================================

void Log(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    
    if (fLog) {
        vfprintf(fLog, fmt, args);
        fprintf(fLog, "\n");
        fflush(fLog);
    }

    vprintf(fmt, args);
    printf("\n");
    
    va_end(args);
}

// =========================================================================================
// MINIMAP RENDERING FUNCTION
// =========================================================================================

void RenderMinimap() {
    auto& cfg = JNIHelper::g_minimapConfig;
    if (!cfg.enabled) return;
    if (!JNIHelper::g_playerInstance) return;
    
    JNIEnv* env = JNIHelper::GetEnv();
    if (!env) return;
    
    float playerX = 0.0f, playerY = 0.0f;
    if (JNIHelper::g_fidX && JNIHelper::g_fidY) {
        playerX = env->GetFloatField(JNIHelper::g_playerInstance, JNIHelper::g_fidX);
        playerY = env->GetFloatField(JNIHelper::g_playerInstance, JNIHelper::g_fidY);
    }
    if (playerX <= 0.0f) return;
    
    float mapSize = 200.0f;
    float halfMap = mapSize / 2.0f;
    float pixelsPerCell = mapSize / cfg.scale;
    
    ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    ImVec2 winPos;
    switch (cfg.position) {
        case 0: // Bottom-Right
            winPos = ImVec2(displaySize.x - mapSize - 15.0f, displaySize.y - mapSize - 15.0f);
            break;
        case 1: // Bottom-Left
            winPos = ImVec2(15.0f, displaySize.y - mapSize - 15.0f);
            break;
        case 2: // Floating
        default:
            winPos = ImVec2(displaySize.x - mapSize - 15.0f, displaySize.y / 2.0f - halfMap);
            break;
    }
    
    ImVec2 center = ImVec2(winPos.x + halfMap, winPos.y + halfMap);
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    
    dl->AddRectFilled(winPos, ImVec2(winPos.x + mapSize, winPos.y + mapSize),
        IM_COL32(10, 10, 10, (int)(cfg.alpha * 220)));
    
    dl->AddRect(winPos, ImVec2(winPos.x + mapSize, winPos.y + mapSize),
        IM_COL32(0, 200, 0, 180), 0.0f, 0, 1.5f);
    
    if (cfg.showGrid) {
        float gridSpacing = 10.0f * pixelsPerCell;
        if (gridSpacing > 5.0f) {
            float offsetX = fmodf(playerX, 10.0f) * pixelsPerCell;
            float offsetY = fmodf(playerY, 10.0f) * pixelsPerCell;
            
            for (float gx = -halfMap - gridSpacing; gx <= halfMap + gridSpacing; gx += gridSpacing) {
                float lineX = center.x + gx - offsetX;
                if (lineX >= winPos.x && lineX <= winPos.x + mapSize) {
                    dl->AddLine(ImVec2(lineX, winPos.y), ImVec2(lineX, winPos.y + mapSize),
                        IM_COL32(40, 40, 40, 100));
                }
            }
            for (float gy = -halfMap - gridSpacing; gy <= halfMap + gridSpacing; gy += gridSpacing) {
                float lineY = center.y + gy - offsetY;
                if (lineY >= winPos.y && lineY <= winPos.y + mapSize) {
                    dl->AddLine(ImVec2(winPos.x, lineY), ImVec2(winPos.x + mapSize, lineY),
                        IM_COL32(40, 40, 40, 100));
                }
            }
        }
    }
    
    dl->AddTriangleFilled(
        ImVec2(center.x, winPos.y + 4),
        ImVec2(center.x - 5, winPos.y + 14),
        ImVec2(center.x + 5, winPos.y + 14),
        IM_COL32(255, 255, 0, 200));
    dl->AddText(ImVec2(center.x - 3, winPos.y + 15), IM_COL32(255, 255, 0, 180), "N");
    
    std::vector<JNIHelper::ZombieInfo> entities = JNIHelper::GetZombiesForESP();
    
    int zombieCount = 0, playerCount = 0, vehicleCount = 0;
    
    for (const auto& e : entities) {
        if (e.type == 0 && !cfg.showZombies) continue;
        if (e.type == 1 && !cfg.showPlayers) continue;
        if (e.type == 2 && !cfg.showVehicles) continue;
        if (e.type == 0 && e.isDead) continue;
        
        float relX = (e.x - playerX) * pixelsPerCell;
        float relY = (e.y - playerY) * pixelsPerCell;
        
        float dotX = center.x + relX;
        float dotY = center.y + relY;
        
        if (dotX < winPos.x || dotX > winPos.x + mapSize ||
            dotY < winPos.y || dotY > winPos.y + mapSize) continue;
        
        ImU32 color;
        float dotSize;
        switch (e.type) {
            case 0:
                color = IM_COL32(255, 50, 50, 200);
                dotSize = 2.5f;
                zombieCount++;
                break;
            case 1:
                color = IM_COL32(50, 255, 50, 255);
                dotSize = 4.0f;
                playerCount++;
                break;
            case 2:
                color = IM_COL32(50, 150, 255, 230);
                dotSize = 5.0f;
                vehicleCount++;
                break;
            default:
                color = IM_COL32(255, 255, 0, 200);
                dotSize = 3.0f;
                break;
        }
        
        dl->AddCircleFilled(ImVec2(dotX, dotY), dotSize, color);
    }
    
    dl->AddCircleFilled(center, 4.0f, IM_COL32(255, 255, 0, 255));
    dl->AddCircle(center, 6.0f, IM_COL32(255, 255, 0, 200), 0, 1.5f);
    
    float legendY = winPos.y + mapSize - 16.0f;
    dl->AddRectFilled(ImVec2(winPos.x, legendY - 2), ImVec2(winPos.x + mapSize, winPos.y + mapSize),
        IM_COL32(0, 0, 0, 180));
    
    char legend[128];
    snprintf(legend, sizeof(legend), "Z:%d P:%d V:%d | %.0fm",
        zombieCount, playerCount, vehicleCount, cfg.scale);
    dl->AddText(ImVec2(winPos.x + 4, legendY), IM_COL32(0, 200, 0, 220), legend);
    
    dl->AddText(ImVec2(winPos.x + 4, winPos.y + 2), IM_COL32(0, 200, 0, 150), "[MINIMAP]");
}

// =========================================================================================
// PLAYER INFO ESP RENDERING
// =========================================================================================

void RenderPlayerInfoESP() {
    if (!JNIHelper::g_playerEspActive) return;
    
    std::vector<PlayerInfoEx> players = JNIHelper::GetPlayersInfoForESP();
    if (players.empty()) return;
    
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    JNIHelper::ScreenInfo screen = JNIHelper::GetScreenInfo();
    
    JNIEnv* env = JNIHelper::GetEnv();
    if (!env || !JNIHelper::g_playerInstance) return;
    
    float playerX = env->GetFloatField(JNIHelper::g_playerInstance, JNIHelper::g_fidX);
    float playerY = env->GetFloatField(JNIHelper::g_playerInstance, JNIHelper::g_fidY);
    
    for (const auto& p : players) {
        float outX, outY;
        if (JNIHelper::WorldToScreen(p.x, p.y, p.z, outX, outY, screen)) {
            char distLabel[32];
            snprintf(distLabel, sizeof(distLabel), "[%.1fm]", p.distance);
            
            ImU32 nameColor;
            if (p.health > 75) {
                nameColor = IM_COL32(50, 255, 50, 255);
            } else if (p.health > 40) {
                nameColor = IM_COL32(255, 255, 50, 255);
            } else {
                nameColor = IM_COL32(255, 50, 50, 255);
            }
            
            dl->AddCircleFilled(ImVec2(outX, outY), 4.0f, IM_COL32(50, 150, 255, 255));
            dl->AddCircle(ImVec2(outX, outY), 6.0f, IM_COL32(50, 150, 255, 200), 0, 2.0f);
            
            dl->AddText(ImVec2(outX + 10, outY - 10), IM_COL32(0, 200, 0, 220), distLabel);
            dl->AddText(ImVec2(outX + 10, outY + 2), nameColor, p.username.c_str());
            
            if (p.health < 100.0f) {
                char healthLabel[32];
                snprintf(healthLabel, sizeof(healthLabel), "[%.0f%%]", p.health);
                dl->AddText(ImVec2(outX + 10, outY + 16), IM_COL32(255, 100, 100, 220), healthLabel);
            }
            
            if (!p.weapon.empty()) {
                char weaponLabel[64];
                snprintf(weaponLabel, sizeof(weaponLabel), "[%s]", p.weapon.c_str());
                dl->AddText(ImVec2(outX + 10, outY + 30), IM_COL32(255, 150, 50, 220), weaponLabel);
            }
            
            if (p.isDead) {
                dl->AddText(ImVec2(outX + 10, outY + 44), IM_COL32(100, 100, 100, 220), "[DEAD]");
            }
        }
    }
}

// =========================================================================================
// HOOK FUNCTION
// =========================================================================================

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Forward declaration for WndProc hook
LRESULT CALLBACK HookedWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

BOOL WINAPI hk_wglSwapBuffers(HDC hdc) {
    g_frameCount++;
    
    static bool logged60 = false;
    if (!logged60 && g_frameCount == 60) {
        logged60 = true;
        Log("[+] Hook stable - 60 frames rendered");
        Log("[i] Press INSERT to show menu");
    }
    
    // ESP Entity Cache Update in Real-Time (60 fps)
    if (g_espEnabled || JNIHelper::g_minimapConfig.enabled || g_showMenu) {
        if (JNIHelper::g_playerInstance) {
            JNIHelper::GetZombiesForESP();
        }
    }
    
    if (g_showMenu || g_espEnabled) {
        if (!g_imguiInitialized && !g_imguiInitAttempted && g_frameCount > 60) {
            g_imguiInitAttempted = true;
            Log("[*] Initializing ImGui...");
            
            try {
                IMGUI_CHECKVERSION();
                ImGui::CreateContext();
                ImGuiIO& io = ImGui::GetIO();
                io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
                io.IniFilename = nullptr;
                Log("[+] ImGui context created");
                
                if (!ImGui_ImplWin32_Init(g_hWnd)) {
                    Log("[!] Win32 backend failed");
                    ImGui::DestroyContext();
                    g_showMenu = false;
                    return o_wglSwapBuffers(hdc);
                }
                Log("[+] Win32 backend initialized");
                
                if (!ImGui_ImplOpenGL3_Init("#version 130")) {
                    Log("[!] OpenGL3 backend failed");
                    ImGui_ImplWin32_Shutdown();
                    ImGui::DestroyContext();
                    g_showMenu = false;
                    return o_wglSwapBuffers(hdc);
                }
                Log("[+] OpenGL3 backend initialized");

                ImGuiStyle& style = ImGui::GetStyle();
                style.WindowRounding = 5.0f;
                style.FrameRounding = 3.0f;
                style.Colors[ImGuiCol_Text] = ImVec4(0.00f, 1.00f, 0.00f, 1.00f);
                style.Colors[ImGuiCol_WindowBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.95f);
                style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
                style.Colors[ImGuiCol_Button] = ImVec4(0.10f, 0.30f, 0.10f, 1.00f);
                style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.20f, 0.60f, 0.20f, 1.00f);
                style.Colors[ImGuiCol_Header] = ImVec4(0.15f, 0.40f, 0.15f, 1.00f);
                style.Colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
                
                g_imguiInitialized = true;
                Log("[+] ImGui fully initialized");
            }
            catch (const std::exception& e) {
                Log("[!] Exception during initialization: %s", e.what());
                g_showMenu = false;
            }
            catch (...) {
                Log("[!] Unknown exception during initialization");
                g_showMenu = false;
            }
        }

        if (g_imguiInitialized) {
            EnterCriticalSection(&g_imguiMutex);
            
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();
                
                if (g_espEnabled) {
                     ImDrawList* drawList = ImGui::GetBackgroundDrawList();
                     JNIHelper::ScreenInfo screen = JNIHelper::GetScreenInfo();
                     std::vector<JNIHelper::ZombieInfo> entities = JNIHelper::GetZombiesForESP();
                      
                     float localZ = 0.0f;
                     JNIEnv* env = JNIHelper::GetEnv();
                     if (env && JNIHelper::g_playerInstance && JNIHelper::g_fidZ) {
                         localZ = env->GetFloatField(JNIHelper::g_playerInstance, JNIHelper::g_fidZ);
                     }
                      
                     for (const auto& z : entities) {
                         if (z.type == 0 && !g_espEnabled) continue;
                         if (z.type == 1 && !g_playerEspEnabled) continue;
                         if (z.type == 2 && !g_vehicleEspEnabled) continue;
                         if (z.type == 3 && !g_animalEspEnabled) continue;
                         if (z.type == 0 && z.isDead) continue;
                          
                         float sx, sy;
                         if (JNIHelper::WorldToScreen(z.x, z.y, z.z, sx, sy, screen)) {
                             float safeZoom = (screen.zoom > 0.001f) ? screen.zoom : 1.0f;
                              
                             float boxHeight = 170.0f / safeZoom;
                             float boxWidth = 90.0f / safeZoom;

                             ImVec2 topLeft = ImVec2(sx - (boxWidth / 2.0f), sy - boxHeight);
                             ImVec2 bottomRight = ImVec2(sx + (boxWidth / 2.0f), sy);

                             ImU32 boxColor;
                             const char* label;
                             switch(z.type) {
                                 case 0: boxColor = IM_COL32(255, 0, 0, 255); label = "Z"; break;
                                 case 1: boxColor = IM_COL32(0, 255, 0, 255); label = "P"; break;
                                 case 2: boxColor = IM_COL32(0, 150, 255, 255); label = "V"; break;
                                 case 3: boxColor = IM_COL32(255, 150, 0, 255); label = "A"; break;
                                 default: boxColor = IM_COL32(255, 255, 0, 255); label = "?";
                             }

                             drawList->AddRect(topLeft, bottomRight, boxColor, 0.0f, 0, 2.0f);
                             drawList->AddText(ImVec2(topLeft.x, topLeft.y - 14), IM_COL32(255, 255, 255, 255), label);
                              
                             float zDiff = z.z - localZ;
                             if (zDiff > 0.5f) {
                                  drawList->AddText(ImVec2(topLeft.x, topLeft.y - 28), IM_COL32(0, 100, 255, 255), "UP");
                             } else if (zDiff < -0.5f) {
                                  drawList->AddText(ImVec2(topLeft.x, topLeft.y - 28), IM_COL32(150, 150, 150, 255), "DWN");
                             }
                         }
                     }
                }
                
                RenderMinimap();
                RenderPlayerInfoESP();
                
                ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_FirstUseEver);
                ImGui::SetNextWindowSize(ImVec2(500, 700), ImGuiCond_FirstUseEver);
                
                bool window_open = g_showMenu;
                if (g_showMenu && ImGui::Begin("ABAÑOÑE v3.0 - For RDH PZ B42", &window_open, ImGuiWindowFlags_NoCollapse)) {
                    ImGui::TextColored(ImVec4(1, 1, 1, 1), "  ____  ____  _   _ ");
                    ImGui::TextColored(ImVec4(1, 1, 1, 1), " |  _ \\|  _ \\| | | |");
                    ImGui::TextColored(ImVec4(1, 1, 1, 1), " | |_) | | | | |_| |");
                    ImGui::TextColored(ImVec4(1, 1, 1, 1), " |  _ <| |_| |  _  |");
                    ImGui::TextColored(ImVec4(1, 1, 1, 1), " |_| \\_\\____/|_| |_|");
                    ImGui::Separator();
                    ImGui::Text("Made by RDH Team");
                    ImGui::Separator();

                    if (ImGui::CollapsingHeader("VISUALS", ImGuiTreeNodeFlags_DefaultOpen)) {
                        ImGui::Checkbox("Enable Zombie ESP", &g_espEnabled);
                        ImGui::Checkbox("Enable Player ESP", &g_playerEspEnabled);
                        ImGui::Checkbox("Enable Vehicle ESP", &g_vehicleEspEnabled);
                        ImGui::Checkbox("Enable Animal ESP", &g_animalEspEnabled);
                        
                        ImGui::Separator();
                        
                        if (ImGui::CollapsingHeader("MINIMAP", ImGuiTreeNodeFlags_DefaultOpen)) {
                            ImGui::Checkbox("Enable Minimap", &JNIHelper::g_minimapConfig.enabled);
                            
                            if (JNIHelper::g_minimapConfig.enabled) {
                                ImGui::Combo("Position##MMP", &JNIHelper::g_minimapConfig.position, 
                                    "Bottom-Right\0Bottom-Left\0Floating\0");
                                ImGui::SliderFloat("Scale", &JNIHelper::g_minimapConfig.scale, 50.0f, 300.0f);
                                ImGui::SliderFloat("Opacity", &JNIHelper::g_minimapConfig.alpha, 0.3f, 1.0f);
                                ImGui::Separator();
                                ImGui::Text("Show:");
                                ImGui::Checkbox("Zombies##MMP", &JNIHelper::g_minimapConfig.showZombies);
                                ImGui::Checkbox("Players##MMP", &JNIHelper::g_minimapConfig.showPlayers);
                                ImGui::Checkbox("Vehicles##MMP", &JNIHelper::g_minimapConfig.showVehicles);
                                ImGui::Checkbox("Animals##MMP", &JNIHelper::g_minimapConfig.showAnimals);
                                ImGui::Checkbox("Grid##MMP", &JNIHelper::g_minimapConfig.showGrid);
                            }
                        }
                    }

                    if (ImGui::CollapsingHeader("SETTINGS", ImGuiTreeNodeFlags_DefaultOpen)) {
                        ImGui::Checkbox("Auto-Attach", &g_autoAttach);
                        ImGui::Checkbox("Stealth Mode", &g_stealthMode);
                        ImGui::Separator();
                        ImGui::SliderInt("FPS Limit", &g_fpsLimit, 30, 144);
                        ImGui::SliderFloat("ESP Distance", &g_espDistanceRange, 100.0f, 2000.0f);
                    }
                    
                    if (ImGui::CollapsingHeader("STATUS", ImGuiTreeNodeFlags_DefaultOpen)) {
                        bool jniConnected = (JNIHelper::g_javaVm != nullptr);
                        ImGui::TextColored(jniConnected ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0, 0, 1),
                            "JNI Connection: %s", jniConnected ? "Connected" : "Disconnected");
                        
                        int cachedClasses = 0;
                        if (JNIHelper::g_cachedIsoPlayerClass) cachedClasses++;
                        if (JNIHelper::g_cachedIsoGameCharacterClass) cachedClasses++;
                        if (JNIHelper::g_cachedPlayerCheatsClass) cachedClasses++;
                        if (JNIHelper::g_cachedIsoWorldClass) cachedClasses++;
                        if (JNIHelper::g_cachedIsoCellClass) cachedClasses++;
                        if (JNIHelper::g_cachedIsoZombieClass) cachedClasses++;
                        if (JNIHelper::g_cachedIsoVehicleClass) cachedClasses++;
                        ImGui::Text("Cached Classes: %d", cachedClasses);
                        
                        ImGui::Text("Active Hooks: %s", o_wglSwapBuffers ? "1 (wglSwapBuffers)" : "0");
                        ImGui::Text("Memory (Est): ~%zu KB", sizeof(JNIHelper) + sizeof(ZombieInfo) * 100);
                    }

                    ImGui::End();
                }
                if (g_showMenu) g_showMenu = window_open;
                
                ImGui::EndFrame();  // Cerrar el frame de ImGui
                ImGui::Render();
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            }
            LeaveCriticalSection(&g_imguiMutex);
        }
    
    return o_wglSwapBuffers(hdc);
}

// =========================================================================================
// WINDOW MESSAGE HANDLER
// =========================================================================================

LRESULT CALLBACK HookedWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_KEYDOWN && wParam == VK_INSERT) {
        g_showMenu = !g_showMenu;
        Log("[*] INSERT pressed - Menu %s", g_showMenu ? "SHOWN" : "HIDDEN");
        return 1;
    }
    
    if (msg == WM_KEYDOWN && wParam == VK_DELETE) {
        Log("[*] DELETE pressed - Cleanup triggered");
        return 1;
    }
    
    if (msg == WM_KEYDOWN && wParam == VK_HOME) {
        g_espEnabled = !g_espEnabled;
        g_playerEspEnabled = g_espEnabled;
        g_vehicleEspEnabled = g_espEnabled;
        g_animalEspEnabled = g_espEnabled;
        Log("[*] HOME pressed - ESP %s", g_espEnabled ? "ENABLED" : "DISABLED");
        return 1;
    }

    if (g_showMenu) {
        if (g_imguiInitialized && ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
            return 1;

        if (msg == WM_KEYUP || msg == WM_SYSKEYUP) {
            return CallWindowProc(g_originalWndProc, hWnd, msg, wParam, lParam);
        }

        return 1;
    }

    return CallWindowProc(g_originalWndProc, hWnd, msg, wParam, lParam);
}

// =========================================================================================
// INITIALIZATION
// =========================================================================================

void InitializeHooks() {
    Log("[INFO] Inicializando sistema...");

    Log("[*] Buscando ventana de Project Zomboid...");
    for (int i = 0; i < 30; i++) {
        g_hWnd = FindWindowA(nullptr, "Project Zomboid");
        if (g_hWnd) {
            Log("[+] Ventana encontrada: %p", g_hWnd);
            break;
        }
        Sleep(500);
    }

    if (!g_hWnd) {
        Log("[ERROR] No se encontro ventana de Project Zomboid");
        return;
    }

    Log("[*] Hooking WndProc...");
    g_originalWndProc = (WNDPROC)SetWindowLongPtr(g_hWnd, GWLP_WNDPROC, (LONG_PTR)HookedWndProc);
    if (!g_originalWndProc) {
        Log("[ERROR] Fallo al hookear WndProc");
        return;
    }
    Log("[+] WndProc hooked successfully");

    Log("[*] Inicializando MinHook...");
    if (MH_Initialize() != MH_OK) {
        Log("[ERROR] Fallo al inicializar MinHook");
        return;
    }

    void* targetFunc = (void*)GetProcAddress(GetModuleHandleA("opengl32.dll"), "wglSwapBuffers");
    if (!targetFunc) {
        Log("[ERROR] No se pudo encontrar wglSwapBuffers");
        return;
    }
    Log("[INFO] Target wglSwapBuffers encontrado en: %p", targetFunc);

    MH_STATUS createStatus = MH_CreateHook(
        targetFunc,
        reinterpret_cast<LPVOID>(hk_wglSwapBuffers),
        reinterpret_cast<LPVOID*>(&o_wglSwapBuffers)
    );

    if (createStatus != MH_OK) {
        Log("[ERROR] MH_CreateHook fallo. Status: %d", createStatus);
        return;
    }

    if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) {
        Log("[ERROR] MH_EnableHook fallo");
        return;
    }

    Log("[SUCCESS] Hook instalado correctamente.");
    Log("[INFO] Sistema listo. Presiona INSERT para abrir menu.");
    
    Log("[*] Inicializando JNI...");
    if (JNIHelper::Initialize()) {
        Log("[+] JNI initialized successfully");
    } else {
        Log("[!] JNI initialization failed");
    }
}

// =========================================================================================
// MAIN THREAD
// =========================================================================================

DWORD WINAPI MainThread(LPVOID lpReserved) {
    char logPath[MAX_PATH];
    HMODULE hMod = nullptr;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCSTR)&MainThread, &hMod);
    GetModuleFileNameA(hMod, logPath, MAX_PATH);
    
    char* lastSlash = strrchr(logPath, '\\');
    if (lastSlash) {
        *lastSlash = '\0'; // Remove ZomboidHack.dll
        char* buildSlash = strrchr(logPath, '\\');
        if (buildSlash && _stricmp(buildSlash, "\\build") == 0) {
            *buildSlash = '\0'; // Remove build
        }
        strcat_s(logPath, MAX_PATH, "\\logs");
        CreateDirectoryA(logPath, NULL);
        strcat_s(logPath, MAX_PATH, "\\zomboid_hack_debug.log");
    } else {
        snprintf(logPath, sizeof(logPath), "zomboid_hack_debug.log");
    }

    fopen_s(&fLog, logPath, "w");

    Log("=== [ZomboidHack] ENI Control Panel Started ===");
    Log("Author: ENI (Elite Neural Intelligence)");
    Log("Target: Project Zomboid Build 42 (Unstable)");
    Log("Hardware Opt: i3-2100 (JNI Caching Active)");
    Log("------------------------------------------------");
    
    InitializeHooks();

    return 0;
}

// =========================================================================================
// DLL MAIN
// =========================================================================================

BOOL WINAPI DllMain(HMODULE hMod, DWORD dwReason, LPVOID lpReserved) {
    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hMod);
        InitializeCriticalSection(&g_imguiMutex);
        CreateThread(nullptr, 0, MainThread, hMod, 0, nullptr);
        break;
    case DLL_PROCESS_DETACH:
        DeleteCriticalSection(&g_imguiMutex);
        if (g_imguiInitialized) {
            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
        }
        if (g_originalWndProc && g_hWnd) {
            SetWindowLongPtr(g_hWnd, GWLP_WNDPROC, (LONG_PTR)g_originalWndProc);
        }
        MH_DisableHook(MH_ALL_HOOKS);
        MH_Uninitialize();
        JNIHelper::Cleanup();
        if (fLog) fclose(fLog);
        break;
    }
    return TRUE;
}