#define GL_GLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <dlfcn.h>
#include <pthread.h>
#include <string>
#include <vector>

#include "../include/jni_helpers.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"

// =========================================================================================
// GLOBALS
// =========================================================================================

FILE *fLog = nullptr;
int g_frameCount = 0;

static bool g_showMenu = false;
static bool g_espEnabled = false;
static bool g_playerEspEnabled = false;
static bool g_vehicleEspEnabled = false;
static bool g_animalEspEnabled = false;
static bool g_imguiInitialized = false;
static bool g_jniReady = false;
static bool g_playerManuallyInitialized = false;

static Display *g_display = nullptr;
static float g_espDistanceRange = 500.0f;

static auto g_lastTime = std::chrono::steady_clock::now();

// =========================================================================================
// LOGGING
// =========================================================================================

static void Log(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  // [R-LOWSPEC] Sin fflush — el render thread no puede bloquearse en I/O de
  // disco. El buffer se vuelca cuando el OS lo decide o al cerrar el archivo en
  // hack_destructor.
  if (fLog) {
    vfprintf(fLog, fmt, args);
    fprintf(fLog, "\n");
    fflush(fLog);
  }
  va_end(args);
  
  va_start(args, fmt);
  vprintf(fmt, args);
  printf("\n");
  va_end(args);
}

// =========================================================================================
// X11 INPUT HELPERS (via separate XWayland connection)
// =========================================================================================

static Display *g_x11Display = nullptr;
static bool g_x11Attempted = false;
static bool g_eventsSelected = false;
static int g_mouseX = 0, g_mouseY = 0;
static bool g_mouse0 = false, g_mouse1 = false;
// forward-declared here, defined in the hook section below
static GLXDrawable g_hookDrawable = 0;
static int g_fbWidth = 0, g_fbHeight = 0;   // GLX drawable/framebuffer size
static int g_winWidth = 0, g_winHeight = 0; // actual X11 window size

static int x11SilentErrorHandler(Display *, XErrorEvent *) { return 0; }

static void EnsureX11Display() {
  if (g_x11Display || g_x11Attempted)
    return;
  g_x11Attempted = true;

  const char *disp = getenv("DISPLAY");
  if (disp)
    g_x11Display = XOpenDisplay(disp);
  if (!g_x11Display)
    g_x11Display = XOpenDisplay(":0");
  if (!g_x11Display)
    g_x11Display = XOpenDisplay(":1");

  if (g_x11Display) {
    XSetErrorHandler(x11SilentErrorHandler);
    Log("[+] X11 display opened for input: %s", disp ? disp : "fallback");
  } else {
    Log("[-] Could not open X11 display - keyboard input disabled");
  }
}

static ImGuiKey X11KeysymToImGuiKey(KeySym ks) {
  switch (ks) {
  case XK_Tab:
    return ImGuiKey_Tab;
  case XK_Left:
    return ImGuiKey_LeftArrow;
  case XK_Right:
    return ImGuiKey_RightArrow;
  case XK_Up:
    return ImGuiKey_UpArrow;
  case XK_Down:
    return ImGuiKey_DownArrow;
  case XK_Page_Up:
    return ImGuiKey_PageUp;
  case XK_Page_Down:
    return ImGuiKey_PageDown;
  case XK_Home:
    return ImGuiKey_Home;
  case XK_End:
    return ImGuiKey_End;
  case XK_Insert:
    return ImGuiKey_Insert;
  case XK_Delete:
    return ImGuiKey_Delete;
  case XK_BackSpace:
    return ImGuiKey_Backspace;
  case XK_Return:
    return ImGuiKey_Enter;
  case XK_KP_Enter:
    return ImGuiKey_KeypadEnter;
  case XK_Escape:
    return ImGuiKey_Escape;
  case XK_space:
    return ImGuiKey_Space;
  case XK_Control_L:
    return ImGuiKey_LeftCtrl;
  case XK_Control_R:
    return ImGuiKey_RightCtrl;
  case XK_Shift_L:
    return ImGuiKey_LeftShift;
  case XK_Shift_R:
    return ImGuiKey_RightShift;
  case XK_Alt_L:
    return ImGuiKey_LeftAlt;
  case XK_Alt_R:
    return ImGuiKey_RightAlt;
  case XK_a:
  case XK_A:
    return ImGuiKey_A;
  case XK_c:
  case XK_C:
    return ImGuiKey_C;
  case XK_v:
  case XK_V:
    return ImGuiKey_V;
  case XK_x:
  case XK_X:
    return ImGuiKey_X;
  case XK_y:
  case XK_Y:
    return ImGuiKey_Y;
  case XK_z:
  case XK_Z:
    return ImGuiKey_Z;
  default:
    return ImGuiKey_None;
  }
}

static void ProcessX11Event(XEvent *ev) {
  if (!ev)
    return;
  static int motionLogged = 0;
  if (ev->type == MotionNotify) {
    g_mouseX = ev->xmotion.x;
    g_mouseY = ev->xmotion.y;
    if (motionLogged < 3) {
      Log("[X11-MOTION] x=%d y=%d", g_mouseX, g_mouseY);
      motionLogged++;
    }
  } else if (ev->type == ButtonPress) {
    Log("[X11-CLICK] btn=%d x=%d y=%d", ev->xbutton.button, ev->xbutton.x,
        ev->xbutton.y);
    if (ev->xbutton.button == Button1)
      g_mouse0 = true;
    if (ev->xbutton.button == Button3)
      g_mouse1 = true;
  } else if (ev->type == ButtonRelease) {
    if (ev->xbutton.button == Button1)
      g_mouse0 = false;
    if (ev->xbutton.button == Button3)
      g_mouse1 = false;
  } else if (ev->type == ConfigureNotify) {
    if (ev->xconfigure.width > 0 && ev->xconfigure.height > 0) {
      g_winWidth = ev->xconfigure.width;
      g_winHeight = ev->xconfigure.height;
      Log("[X11-RESIZE] Window resized: %dx%d", g_winWidth, g_winHeight);
    }
  } else if (ev->type == KeyPress || ev->type == KeyRelease) {
    if (!ImGui::GetCurrentContext())
      return;
    ImGuiIO &io = ImGui::GetIO();
    bool isDown = (ev->type == KeyPress);
    unsigned int mods = ev->xkey.state;
    io.AddKeyEvent(ImGuiMod_Ctrl, (mods & ControlMask) != 0);
    io.AddKeyEvent(ImGuiMod_Shift, (mods & ShiftMask) != 0);
    io.AddKeyEvent(ImGuiMod_Alt, (mods & Mod1Mask) != 0);
    KeySym ks = XLookupKeysym(&ev->xkey, 0);
    ImGuiKey imkey = X11KeysymToImGuiKey(ks);
    if (imkey != ImGuiKey_None)
      io.AddKeyEvent(imkey, isDown);
    if (isDown) {
      char buf[32] = {};
      XLookupString(&ev->xkey, buf, sizeof(buf) - 1, nullptr, nullptr);
      for (int i = 0; buf[i]; i++) {
        unsigned char c = (unsigned char)buf[i];
        if (c >= 32)
          io.AddInputCharacter((unsigned int)c);
      }
    }
  }
}

static bool IsKeyDown(KeySym sym) {
  if (!g_x11Display)
    return false;
  KeyCode kc = XKeysymToKeycode(g_x11Display, sym);
  if (!kc)
    return false;
  char keys[32] = {};
  XQueryKeymap(g_x11Display, keys);
  return (keys[kc / 8] & (1 << (kc % 8))) != 0;
}

static bool g_insertWasDown = false;

// =========================================================================================
// JNI INIT THREAD
// =========================================================================================

static void *JNIInitThread(void *) {
  for (int i = 0; i < 60; i++) { // retry up to 60s
    if (JNIHelper::Initialize()) {
      g_jniReady = true;
      Log("[+] JNI initialized successfully");
      break;
    }
    sleep(1);
  }
  return nullptr;
}

// =========================================================================================
// IMGUI SETUP (runs once, inside the game's GL context)
// =========================================================================================

static void InitImGui() {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.IniFilename = nullptr; // No persistent settings file

  ImGui::StyleColorsDark();
  ImGuiStyle &style = ImGui::GetStyle();

  // [ENI] DEDSEC THEME - Sharp, Hacker, Glitchy Aesthetic
  style.WindowRounding = 0.0f;
  style.FrameRounding = 0.0f;
  style.ScrollbarRounding = 0.0f;
  style.GrabRounding = 0.0f;
  style.TabRounding = 0.0f;
  style.WindowBorderSize = 1.0f;
  style.FrameBorderSize = 1.0f;
  style.PopupBorderSize = 1.0f;
  style.WindowPadding = ImVec2(10.0f, 10.0f);
  style.ItemSpacing = ImVec2(8.0f, 6.0f);
  style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
  style.Alpha = 0.95f;

  ImVec4 *colors = style.Colors;

  // Palette: Deep Dark backgrounds with electric magenta/cyan/green accents
  colors[ImGuiCol_Text] = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
  colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
  colors[ImGuiCol_WindowBg] = ImVec4(0.05f, 0.05f, 0.07f, 0.94f);
  colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
  colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.10f, 0.94f);
  colors[ImGuiCol_Border] =
      ImVec4(0.35f, 0.00f, 0.50f, 0.60f); // Magenta dark border
  colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
  colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.10f, 0.15f, 1.00f);
  colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.10f, 0.35f, 1.00f);
  colors[ImGuiCol_FrameBgActive] = ImVec4(0.35f, 0.00f, 0.50f, 1.00f);
  colors[ImGuiCol_TitleBg] = ImVec4(0.05f, 0.05f, 0.07f, 1.00f);
  colors[ImGuiCol_TitleBgActive] =
      ImVec4(0.35f, 0.00f, 0.50f, 1.00f); // Electric Magenta header
  colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
  colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
  colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
  colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
  colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
  colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
  colors[ImGuiCol_CheckMark] =
      ImVec4(0.00f, 1.00f, 0.80f, 1.00f); // Cyan checkmark
  colors[ImGuiCol_SliderGrab] = ImVec4(0.00f, 1.00f, 0.80f, 1.00f);
  colors[ImGuiCol_SliderGrabActive] = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
  colors[ImGuiCol_Button] = ImVec4(0.15f, 0.15f, 0.20f, 1.00f);
  colors[ImGuiCol_ButtonHovered] = ImVec4(0.35f, 0.00f, 0.50f, 1.00f);
  colors[ImGuiCol_ButtonActive] = ImVec4(0.60f, 0.00f, 0.80f, 1.00f);
  colors[ImGuiCol_Header] = ImVec4(0.35f, 0.00f, 0.50f, 0.50f);
  colors[ImGuiCol_HeaderHovered] = ImVec4(0.35f, 0.00f, 0.50f, 0.80f);
  colors[ImGuiCol_HeaderActive] = ImVec4(0.60f, 0.00f, 0.80f, 1.00f);
  colors[ImGuiCol_Separator] = ImVec4(0.35f, 0.00f, 0.50f, 0.60f);
  colors[ImGuiCol_SeparatorHovered] = ImVec4(0.60f, 0.00f, 0.80f, 0.78f);
  colors[ImGuiCol_SeparatorActive] = ImVec4(0.60f, 0.00f, 0.80f, 1.00f);
  colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 1.00f, 0.80f, 0.20f);
  colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.00f, 1.00f, 0.80f, 0.67f);
  colors[ImGuiCol_ResizeGripActive] = ImVec4(0.00f, 1.00f, 1.00f, 0.95f);
  colors[ImGuiCol_Tab] = ImVec4(0.15f, 0.15f, 0.20f, 1.00f);
  colors[ImGuiCol_TabHovered] = ImVec4(0.35f, 0.00f, 0.50f, 1.00f);
  colors[ImGuiCol_TabActive] = ImVec4(0.35f, 0.00f, 0.50f, 1.00f);
  colors[ImGuiCol_TabUnfocused] = ImVec4(0.07f, 0.10f, 0.15f, 0.97f);
  colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.20f, 0.28f, 1.00f);
  colors[ImGuiCol_TextSelectedBg] = ImVec4(0.00f, 1.00f, 0.80f, 0.35f);
  colors[ImGuiCol_NavHighlight] = ImVec4(0.00f, 1.00f, 0.80f, 1.00f);

  // Intentar fuentes comunes del sistema, preferiblemente monospaciadas para el
  // vibe DedSec
  static const ImWchar latinRanges[] = {0x0020, 0x00FF, 0};
  ImFontConfig fontCfg;
  fontCfg.OversampleH = 2;
  fontCfg.OversampleV = 2;
  fontCfg.SizePixels = 14.0f;
  const char *fontCandidates[] = {
      "/usr/share/fonts/truetype/ubuntu/UbuntuMono-R.ttf",
      "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
      "/usr/share/fonts/TTF/DejaVuSansMono.ttf",
      "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf",
      "/usr/share/fonts/liberation-sans/LiberationMono-Regular.ttf",
      "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", // fallback
      nullptr};
  bool fontLoaded = false;
  for (int i = 0; fontCandidates[i] != nullptr; i++) {
    // [ENI] IMPRESCINDIBLE verificar si el archivo existe antes, porque AddFontFromFileTTF 
    // lanza IM_ASSERT (abort) si el archivo no existe.
    if (access(fontCandidates[i], F_OK) == -1) {
        continue;
    }
    ImFont *f = io.Fonts->AddFontFromFileTTF(fontCandidates[i], 14.0f, &fontCfg,
                                             latinRanges);
    if (f) {
      Log("[+] ImGui font loaded: %s", fontCandidates[i]);
      fontLoaded = true;
      break;
    }
  }
  if (!fontLoaded) {
    io.Fonts->AddFontDefault();
    Log("[!] ImGui font: no TTF found, using built-in (sin soporte de "
        "acentos)");
  }

  ImGui_ImplOpenGL3_Init("#version 130");
  g_imguiInitialized = true;
  Log("[+] ImGui initialized (Linux/OpenGL3)");
}

// =========================================================================================
// MINIMAP OVERLAY
// =========================================================================================

// [R-LOWSPEC] Recibe las entidades pre-fetched desde RunHookFrame (cero scan
// JNI propio).
static void
RenderMinimapOverlay(const std::vector<JNIHelper::ZombieInfo> &entities) {
  auto &cfg = JNIHelper::g_minimapConfig;
  if (!cfg.enabled || !JNIHelper::g_playerInstance)
    return;
  JNIEnv *env = JNIHelper::GetEnv();
  if (!env || !JNIHelper::g_fidX || !JNIHelper::g_fidY)
    return;

  float px = env->GetFloatField(JNIHelper::g_playerInstance, JNIHelper::g_fidX);
  float py = env->GetFloatField(JNIHelper::g_playerInstance, JNIHelper::g_fidY);
  if (env->ExceptionCheck()) {
    env->ExceptionClear();
    return;
  }

  // entities viene por referencia — no se llama GetZombiesForESP() aqui

  float mapSize = cfg.scale;
  ImGuiIO &io = ImGui::GetIO();
  float sW = io.DisplaySize.x, sH = io.DisplaySize.y;

  float posX, posY;
  switch (cfg.position) {
  case 0:
    posX = sW - mapSize - 10.f;
    posY = sH - mapSize - 60.f;
    break;
  case 1:
    posX = 10.f;
    posY = sH - mapSize - 60.f;
    break;
  default:
    posX = sW - mapSize - 10.f;
    posY = 60.f;
    break;
  }

  ImDrawList *dl = ImGui::GetForegroundDrawList();
  ImVec2 p0(posX, posY), p1(posX + mapSize, posY + mapSize);
  ImU32 bgCol = IM_COL32(10, 10, 10, (int)(cfg.alpha * 200));
  ImU32 borderCol = IM_COL32(100, 100, 100, (int)(cfg.alpha * 255));
  dl->AddRectFilled(p0, p1, bgCol);
  dl->AddRect(p0, p1, borderCol, 2.0f, 0, 1.5f);

  if (cfg.showGrid) {
    ImU32 gridCol = IM_COL32(40, 40, 40, (int)(cfg.alpha * 180));
    for (int i = 1; i < 5; i++) {
      float t = mapSize / 5.f * i;
      dl->AddLine(ImVec2(posX + t, posY), ImVec2(posX + t, posY + mapSize),
                  gridCol);
      dl->AddLine(ImVec2(posX, posY + t), ImVec2(posX + mapSize, posY + t),
                  gridCol);
    }
  }

  float tileRadius = (g_espDistanceRange > 0 ? g_espDistanceRange : 50.f);
  float pixPerTile = (mapSize * 0.5f) / tileRadius;
  float cx = posX + mapSize * 0.5f;
  float cy = posY + mapSize * 0.5f;

  // Font pequeña para labels del minimap
  float fontSize = 9.0f;

  for (auto &e : entities) {
    if (e.isDead)
      continue;
    if (!cfg.showZombies && e.type == 0)
      continue;
    if (!cfg.showPlayers && e.type == 1)
      continue;
    if (!cfg.showVehicles && e.type == 2)
      continue;
    float sx = cx + (e.x - px) * pixPerTile;
    float sy = cy + (e.y - py) * pixPerTile;
    if (sx < posX || sx > posX + mapSize || sy < posY || sy > posY + mapSize)
      continue;

    ImU32 col;
    float r;
    if (e.type == 0) {
      col = IM_COL32(255, 50, 50, 220);
      r = 2.f; // Rojo uniforme
    } else if (e.type == 1) {
      col = IM_COL32(50, 220, 50, 230);
      r = 3.f;
    } else if (e.type == 2) {
      col = IM_COL32(50, 180, 255, 220);
      r = 3.f;
    } else {
      col = IM_COL32(255, 200, 50, 220);
      r = 2.f;
    }

    dl->AddCircleFilled(ImVec2(sx, sy), r, col);

    // Floor delta indicator
    if (e.floorDelta != 0 && e.type != 2) {
      char floorBuf[8];
      snprintf(floorBuf, sizeof(floorBuf), "%s%d", e.floorDelta > 0 ? "^" : "v",
               abs(e.floorDelta));
      ImU32 floorCol = e.floorDelta > 0 ? IM_COL32(200, 200, 255, 220)
                                        : IM_COL32(200, 160, 100, 220);
      dl->AddText(ImVec2(sx + 3.f, sy - 6.f), floorCol, floorBuf);
    }

    // Jugador: nombre encima del punto
    if (e.type == 1 && !e.vehicleName.empty()) {
      dl->AddText(ImVec2(sx - 12.f, sy - 12.f), IM_COL32(100, 255, 100, 210),
                  e.vehicleName.c_str());
    }

    // Vehículo: nombre de script abreviado
    if (e.type == 2 && !e.vehicleName.empty()) {
      const char *name = e.vehicleName.c_str();
      const char *dot = strrchr(name, '.');
      if (dot)
        name = dot + 1;
      char abbrev[16];
      snprintf(abbrev, sizeof(abbrev), "%.12s", name);
      dl->AddText(ImVec2(sx + 4.f, sy - 5.f), IM_COL32(100, 200, 255, 200),
                  abbrev);
    }
  }
  dl->AddCircleFilled(ImVec2(cx, cy), 4.f, IM_COL32(255, 255, 255, 255));
  dl->AddCircle(ImVec2(cx, cy), 5.f, IM_COL32(0, 0, 0, 220), 0, 1.5f);

  char scaleLbl[32];
  snprintf(scaleLbl, sizeof(scaleLbl), "r=%.0f tiles", tileRadius);
  dl->AddText(ImVec2(posX + 4, posY + 4), IM_COL32(160, 160, 160, 200),
              scaleLbl);
}

// =========================================================================================
// ESP RENDERING
// =========================================================================================

// [R-LOWSPEC] Recibe las entidades pre-fetched desde RunHookFrame (cero scan
// JNI propio).
static void RenderESP(unsigned int winW, unsigned int winH,
                      const std::vector<JNIHelper::ZombieInfo> &entities) {
  JNIHelper::ScreenInfo screen = JNIHelper::GetScreenInfo();
  // entities llega por referencia const — sin copia, sin scan JNI extra.

  ImDrawList *dl = ImGui::GetBackgroundDrawList();

  // [R-LOWSPEC] Leer posición del player UNA sola vez antes del loop.
  // Antes se llamaba GetFloatField(x) y GetFloatField(y) por cada entidad
  // visible.
  float g_cachedPX = -1.0f, g_cachedPY = -1.0f;
  if (JNIHelper::g_playerInstance && JNIHelper::g_fidX && JNIHelper::g_fidY) {
    JNIEnv *env_ = JNIHelper::GetEnv();
    if (env_) {
      g_cachedPX =
          env_->GetFloatField(JNIHelper::g_playerInstance, JNIHelper::g_fidX);
      g_cachedPY =
          env_->GetFloatField(JNIHelper::g_playerInstance, JNIHelper::g_fidY);
    }
  }

  for (auto &e : entities) {
    if (!g_espEnabled && e.type == 0)
      continue;
    if (!g_playerEspEnabled && e.type == 1)
      continue;
    if (!g_vehicleEspEnabled && e.type == 2)
      continue;
    if (!g_animalEspEnabled && e.type == 3)
      continue;
    if (e.isDead)
      continue;

    float sx, sy;
    if (!JNIHelper::WorldToScreen(e.x, e.y, e.z, sx, sy, screen))
      continue;
    if (sx < 0 || sy < 0 || sx > (float)winW || sy > (float)winH)
      continue;

    float dist = 0.0f;
    if (g_cachedPX >= 0.0f) {
      float dx = e.x - g_cachedPX, dy = e.y - g_cachedPY;
      dist = sqrtf(dx * dx + dy * dy);
    }
    if (dist > g_espDistanceRange)
      continue;

    // Color uniforme para zombies
    ImU32 col;
    if (e.type == 0) {
      col = IM_COL32(255, 50, 50, 220); // Rojo uniforme
    } else {
      switch (e.type) {
      case 1:
        col = IM_COL32(50, 220, 50, 220);
        break; // Jugador — verde
      case 2:
        col = IM_COL32(50, 180, 255, 220);
        break; // Vehículo — azul
      case 3:
        col = IM_COL32(255, 200, 50, 220);
        break; // Animal  — amarillo
      default:
        col = IM_COL32(200, 200, 200, 200);
        break;
      }
    }

    float boxH = std::max(8.0f, 600.0f / (dist + 1.0f));
    float boxW = boxH * 0.5f;
    dl->AddRect(ImVec2(sx - boxW / 2, sy - boxH), ImVec2(sx + boxW / 2, sy),
                col, 0.0f, 0, 1.5f);

    // Sufijo de piso: ↑N o ↓N si la entidad no está en el mismo nivel
    char floorSuffix[12] = "";
    if (e.floorDelta != 0) {
      snprintf(floorSuffix, sizeof(floorSuffix), " %s%d",
               e.floorDelta > 0 ? "^" : "v", abs(e.floorDelta));
    }

    // Zombies: solo caja de color, sin texto (demasiado ruido visual con
    // hordas)
    if (e.type == 0) {
      // Solo mostrar piso si está en piso diferente
      if (e.floorDelta != 0) {
        dl->AddText(ImVec2(sx - boxW / 2, sy - boxH - 12),
                    IM_COL32(200, 200, 255, 200), floorSuffix);
      }
      continue; // No label de texto para zombies
    }

    char label[64];
    if (e.type == 1 && !e.vehicleName.empty()) {
      snprintf(label, sizeof(label), "%s %.0fm%s", e.vehicleName.c_str(), dist,
               floorSuffix);
    } else if (e.type == 2 && !e.vehicleName.empty()) {
      const char *vname = e.vehicleName.c_str();
      const char *dot = strrchr(vname, '.');
      if (dot)
      snprintf(label, sizeof(label), "%s %.0fm%s", e.vehicleName.c_str(), dist,
               floorSuffix);
    } else {
      snprintf(label, sizeof(label), "[%d] %.0fm%s", e.type, dist, floorSuffix);
    }

    ImVec2 textSize = ImGui::CalcTextSize(label);
    dl->AddText(ImVec2(sx - textSize.x / 2, sy - boxH - 14), col, label);
  }

  // --- LOOT ESP (Estructural) ---
  if (JNIHelper::g_lootEspActive && !JNIHelper::g_lootCache.empty()) {
    for (const auto &loot : JNIHelper::g_lootCache) {
      if (loot.items.empty()) continue; // Omitir contenedores vacíos (o filtrados)

      float sx, sy;
      if (!JNIHelper::WorldToScreen(loot.x, loot.y, loot.z, sx, sy, screen))
        continue;
      if (sx < 0 || sy < 0 || sx > (float)winW || sy > (float)winH)
        continue;

      // Calcular distancia opcional para fade/tamaño
      float dist = 0.0f;
      if (g_cachedPX >= 0.0f) {
        float dx = loot.x - g_cachedPX, dy = loot.y - g_cachedPY;
        dist = sqrtf(dx * dx + dy * dy);
      }
      
      // Dibujar lista apilada de items (Color verde táctico)
      ImU32 lootColor = IM_COL32(0, 255, 128, 255);
      float yOffset = sy; // Empezar donde está el tile en pantalla
      
      for (const auto& item : loot.items) {
          char itemLabel[64];
          snprintf(itemLabel, sizeof(itemLabel), "[%s]", item.name.c_str());
          ImVec2 textSize = ImGui::CalcTextSize(itemLabel);
          
          // Dibujar fondo negro semi-transparente para legibilidad
          dl->AddRectFilled(
              ImVec2(sx - textSize.x / 2 - 2, yOffset - 2), 
              ImVec2(sx + textSize.x / 2 + 2, yOffset + textSize.y + 2), 
              IM_COL32(0, 0, 0, 150)
          );
          // Dibujar texto
          dl->AddText(ImVec2(sx - textSize.x / 2, yOffset), lootColor, itemLabel);
          
          yOffset += textSize.y + 4.0f; // Siguiente item más abajo
      }
    }
  }
}

// =========================================================================================
// IMGUI MENU
// =========================================================================================

static void RenderMenu() {
  ImGuiIO &io = ImGui::GetIO();
  ImGui::SetNextWindowSize(ImVec2(500, 700), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowPos(ImVec2(50.f, 50.f), ImGuiCond_FirstUseEver);

  bool open = true;
  ImGui::Begin(":: [ DEDSEC // ROOT_ACCESS ] ::", &open,
               ImGuiWindowFlags_NoCollapse);

  // ASCII DedSec Header
  ImGui::TextColored(
      ImVec4(0.0f, 1.0f, 0.8f, 1.0f),
      "      :::     :::::::::   ::::::::   ::::::::  :::::::::: :::::::: ");
  ImGui::TextColored(
      ImVec4(0.0f, 1.0f, 0.8f, 1.0f),
      "     :+: :+:   :+:    :+: :+:    :+: :+:    :+: :+:       :+:    :+: ");
  ImGui::TextColored(
      ImVec4(0.0f, 1.0f, 0.8f, 1.0f),
      "    +:+   +:+  +:+    +:+ +:+    +:+ +:+        +:+       +:+        ");
  ImGui::TextColored(
      ImVec4(0.35f, 0.0f, 0.50f, 1.0f),
      "  +#++:++#++: +#++:++#+  +#++:++#+  :#:        +#++:++#  +#+         ");
  ImGui::TextColored(
      ImVec4(0.35f, 0.0f, 0.50f, 1.0f),
      " +#+     +#+ +#+    +#+ +#+        +#+   +#+# +#+       +#+          ");
  ImGui::TextColored(
      ImVec4(0.35f, 0.0f, 0.50f, 1.0f),
      "###     ### #########  ###         ########  ########## ########     ");
  ImGui::Separator();

  bool playerReady = g_jniReady && JNIHelper::g_playerInstance;

  // --- Character Creation ---
  if (ImGui::CollapsingHeader("Character Creation")) {
    if (!g_jniReady) {
      ImGui::TextColored(ImVec4(1, 0.4f, 0.2f, 1), "JNI not ready");
    } else {
      int pts = JNIHelper::GetCharacterFreePoints();
      ImGui::Text("Current Points: %d", pts);
      static int pointsToSet = 0;
      ImGui::SetNextItemWidth(100);
      ImGui::InputInt("##Points", &pointsToSet);
      ImGui::SameLine();
      if (ImGui::Button("Set"))
        JNIHelper::SetCharacterFreePoints(pointsToSet);
      ImGui::SameLine();
      if (ImGui::Button("MAX (100)")) {
        pointsToSet = 100;
        JNIHelper::SetCharacterFreePoints(100);
      }
      if (ImGui::Button("Sync Lua"))
        JNIHelper::SyncCharacterFreePoints();
      ImGui::SameLine();
      if (ImGui::Button("Reset"))
        JNIHelper::ResetCharacterFreePoints();
    }
  }

  // --- Visuals ---
  if (ImGui::CollapsingHeader("VISUALS")) {
    ImGui::Checkbox("Zombie ESP", &g_espEnabled);
    ImGui::Checkbox("Player ESP", &g_playerEspEnabled);
    ImGui::Checkbox("Vehicle ESP", &g_vehicleEspEnabled);
    ImGui::Checkbox("Animal ESP", &g_animalEspEnabled);
    ImGui::SliderFloat("ESP Distance", &g_espDistanceRange, 100.f, 2000.f,
                       "%.0f");
    ImGui::Separator();
    if (ImGui::CollapsingHeader("MINIMAP")) {
      ImGui::Checkbox("Enable Minimap", &JNIHelper::g_minimapConfig.enabled);
      if (JNIHelper::g_minimapConfig.enabled) {
        ImGui::Combo("Position##MMP", &JNIHelper::g_minimapConfig.position,
                     "Bottom-Right\0Bottom-Left\0Floating\0");
        ImGui::SliderFloat("Scale", &JNIHelper::g_minimapConfig.scale, 50.f,
                           300.f);
        ImGui::SliderFloat("Opacity", &JNIHelper::g_minimapConfig.alpha, 0.3f,
                           1.f);
        ImGui::Checkbox("Zombies##MMP",
                        &JNIHelper::g_minimapConfig.showZombies);
        ImGui::Checkbox("Players##MMP",
                        &JNIHelper::g_minimapConfig.showPlayers);
        ImGui::Checkbox("Vehicles##MMP",
                        &JNIHelper::g_minimapConfig.showVehicles);
        ImGui::Checkbox("Grid##MMP", &JNIHelper::g_minimapConfig.showGrid);
      }
    }
  }

  // --- Ghost Commander ---
  if (ImGui::CollapsingHeader("GHOST COMMANDER",
                              ImGuiTreeNodeFlags_DefaultOpen)) {
    if (playerReady) {
      ImGui::TextColored(ImVec4(0, 1, 0, 1), "Status: Player Connected");
      ImGui::Text("User: %s", JNIHelper::GetPlayerName().c_str());
      float hp = JNIHelper::GetPlayerHealthDirect();
      if (hp >= 0.f)
        ImGui::Text("Health: %.1f", hp * 100.f);

      // --- Edit Zombie Kills & Days Survived ---
      ImGui::Separator();
      ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.8f, 1.0f), "[ Player Stats Editor ]");
      
      static int tempKills = -1;
      static float tempDays = -1.0f;
      if (tempKills == -1) {
        tempKills = JNIHelper::GetZombieKills();
        tempDays = (float)(JNIHelper::GetHoursSurvived() / 24.0);
      }
      
      ImGui::Text("Actual Kills: %d | Days: %.2f", JNIHelper::GetZombieKills(), JNIHelper::GetHoursSurvived() / 24.0);
      
      ImGui::SetNextItemWidth(120);
      ImGui::InputInt("Kills", &tempKills);
      ImGui::SameLine();
      if (ImGui::Button("Set Kills")) {
        JNIHelper::SetZombieKills(tempKills);
      }
      
      ImGui::SetNextItemWidth(120);
      ImGui::InputFloat("Days", &tempDays, 0.1f, 1.0f, "%.2f");
      ImGui::SameLine();
      if (ImGui::Button("Set Days")) {
        JNIHelper::SetHoursSurvived(tempDays * 24.0);
      }
      
      ImGui::SameLine();
      if (ImGui::Button("Sync Stats")) {
        tempKills = JNIHelper::GetZombieKills();
        tempDays = (float)(JNIHelper::GetHoursSurvived() / 24.0);
      }
      ImGui::Separator();

      if (ImGui::TreeNode("Moodles & Nutrition Editor")) {
        JNIHelper::UpdateMoodleCache();
        
        // Moodles
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.8f, 1.0f), "[ Moodles (Client-Safe) ]");
        
        float hunger = JNIHelper::g_moodleInfo.hunger;
        if (ImGui::SliderFloat("Hunger", &hunger, 0.0f, 1.0f, "%.2f")) {
          JNIHelper::SetPlayerStat(0, hunger);
        }
        
        float thirst = JNIHelper::g_moodleInfo.thirst;
        if (ImGui::SliderFloat("Thirst", &thirst, 0.0f, 1.0f, "%.2f")) {
          JNIHelper::SetPlayerStat(1, thirst);
        }
        
        float fatigue = JNIHelper::g_moodleInfo.fatigue;
        if (ImGui::SliderFloat("Fatigue", &fatigue, 0.0f, 1.0f, "%.2f")) {
          JNIHelper::SetPlayerStat(2, fatigue);
        }
        
        float endurance = JNIHelper::g_moodleInfo.endurance;
        if (ImGui::SliderFloat("Endurance", &endurance, 0.0f, 1.0f, "%.2f")) {
          JNIHelper::SetPlayerStat(5, endurance);
        }
        
        float panic = JNIHelper::g_moodleInfo.panic;
        if (ImGui::SliderFloat("Panic", &panic, 0.0f, 100.0f, "%.1f")) {
          JNIHelper::SetPlayerStat(4, panic);
        }
        
        float pain = (float)JNIHelper::g_moodleInfo.painLevel / 100.0f;
        if (ImGui::SliderFloat("Pain", &pain, 0.0f, 1.0f, "%.2f")) {
          JNIHelper::SetPlayerStat(3, pain);
        }
        
        float temp = JNIHelper::g_moodleInfo.temperature;
        if (ImGui::SliderFloat("Temperature", &temp, 35.0f, 42.0f, "%.1f C")) {
          JNIHelper::SetPlayerStat(6, temp);
        }
        
        // Nutrition
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.8f, 1.0f), "[ Nutrition (Client-Safe) ]");
        
        static float weightVal = -1.0f;
        static float caloriesVal = 0.0f;
        static float proteinsVal = 0.0f;
        static float carbsVal = 0.0f;
        static float lipidsVal = 0.0f;
        
        if (weightVal == -1.0f) {
          weightVal = JNIHelper::GetNutritionStat(0);
          caloriesVal = JNIHelper::GetNutritionStat(1);
          proteinsVal = JNIHelper::GetNutritionStat(2);
          carbsVal = JNIHelper::GetNutritionStat(3);
          lipidsVal = JNIHelper::GetNutritionStat(4);
        }
        
        ImGui::Text("Actual Weight: %.2f", JNIHelper::GetNutritionStat(0));
        
        ImGui::SetNextItemWidth(120);
        ImGui::InputFloat("Weight", &weightVal, 0.1f, 1.0f, "%.2f");
        ImGui::SameLine();
        if (ImGui::Button("Set Weight")) JNIHelper::SetNutritionStat(0, weightVal);
        
        ImGui::SetNextItemWidth(120);
        ImGui::InputFloat("Calories", &caloriesVal, 10.0f, 100.0f, "%.1f");
        ImGui::SameLine();
        if (ImGui::Button("Set Calories")) JNIHelper::SetNutritionStat(1, caloriesVal);
        
        ImGui::SetNextItemWidth(120);
        ImGui::InputFloat("Proteins", &proteinsVal, 1.0f, 10.0f, "%.1f");
        ImGui::SameLine();
        if (ImGui::Button("Set Proteins")) JNIHelper::SetNutritionStat(2, proteinsVal);
        
        ImGui::SetNextItemWidth(120);
        ImGui::InputFloat("Carbs", &carbsVal, 1.0f, 10.0f, "%.1f");
        ImGui::SameLine();
        if (ImGui::Button("Set Carbs")) JNIHelper::SetNutritionStat(3, carbsVal);
        
        ImGui::SetNextItemWidth(120);
        ImGui::InputFloat("Lipids", &lipidsVal, 1.0f, 10.0f, "%.1f");
        ImGui::SameLine();
        if (ImGui::Button("Set Lipids")) JNIHelper::SetNutritionStat(4, lipidsVal);
        
        if (ImGui::Button("Sync Nutrition")) {
          weightVal = JNIHelper::GetNutritionStat(0);
          caloriesVal = JNIHelper::GetNutritionStat(1);
          proteinsVal = JNIHelper::GetNutritionStat(2);
          carbsVal = JNIHelper::GetNutritionStat(3);
          lipidsVal = JNIHelper::GetNutritionStat(4);
        }
        
        ImGui::TreePop();
      }
      ImGui::Separator();

      if (ImGui::Button("Full Heal"))
        JNIHelper::RestoreToFullHealth();
      ImGui::SameLine();
      ImGui::TextColored(ImVec4(1, 1, 0, 1), "(Server Auth)");
      if (ImGui::Button("Refresh Instance"))
        JNIHelper::GetLocalPlayer();
      ImGui::Separator();

      // ── LOGGEABLE (visibles al admin via ISVersionWaterMark + AdminPanel)
      // ──────
      ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.2f, 1.0f),
                         "[ LOGGEABLE - Visible al Admin ]");
      ImGui::BeginDisabled(true);
      ImGui::Checkbox("God Mode (Invincible) [VISIBLE]",
                      &JNIHelper::g_godModeActive);
      ImGui::SameLine();
      ImGui::TextDisabled("isCheatSet() -> watermark");
      ImGui::Checkbox("Phantom (Invisible) [VISIBLE]",
                      &JNIHelper::g_invisibleActive);
      ImGui::SameLine();
      ImGui::TextDisabled("AdminPanel: setInvisible()");
      ImGui::Checkbox("Ghost (NoClip) [VISIBLE]", &JNIHelper::g_noClipActive);
      ImGui::SameLine();
      ImGui::TextDisabled("isGhostMode() -> AdminPanel");
      ImGui::EndDisabled();

      // ── SEGUROS - no pasan por CheatType EnumSet observable
      // ─────────────────
      ImGui::Separator();
      ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f),
                         "[ SERVER-SAFE - No Loggeable ]");
      if (ImGui::Checkbox("Unlimited Ammo", &JNIHelper::g_unlimitedAmmoActive))
        JNIHelper::ToggleUnlimitedAmmo(JNIHelper::g_unlimitedAmmoActive);
      if (ImGui::Checkbox("Unlimited Carry",
                          &JNIHelper::g_unlimitedCarryActive))
        JNIHelper::ToggleUnlimitedCarry(JNIHelper::g_unlimitedCarryActive);
      if (ImGui::Checkbox("Unlimited Endurance",
                          &JNIHelper::g_unlimitedEnduranceActive))
        JNIHelper::ToggleUnlimitedEndurance(
            JNIHelper::g_unlimitedEnduranceActive);
      ImGui::SameLine();
      ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f),
                         "(server rollback en MP)");

      ImGui::Separator();
      ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f),
                         "[ RANGED PRECISION ]");
      if (ImGui::Checkbox("Soft Aim (Ranged Only)",
                          &JNIHelper::g_softAimActive))
        JNIHelper::ToggleSoftAim(JNIHelper::g_softAimActive);
      ImGui::SameLine();
      ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f),
                         "hitChance=100 / spread=0 / noJam");
      if (JNIHelper::g_softAimActive)
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f),
                           "  >> ACTIVO - Equipa un arma ranged");

      if (ImGui::Checkbox("Critical Override (Headshot)",
                          &JNIHelper::g_criticalOverrideActive))
        JNIHelper::ToggleCriticalOverride(JNIHelper::g_criticalOverrideActive);
      ImGui::SameLine();
      ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.1f, 1.0f),
                         "critChance=100%% / dmg x3");
      if (JNIHelper::g_criticalOverrideActive)
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f),
                           "  >> ACTIVO - Disparo critico garantizado");

      ImGui::Separator();
      ImGui::TextColored(ImVec4(0.6f, 1.0f, 0.4f, 1.0f), "[ ANIMALS ]");
      if (ImGui::Checkbox("Animal Cheat (Tame + Extra)",
                          &JNIHelper::g_animalCheatActive))
        JNIHelper::ToggleAnimalCheat(JNIHelper::g_animalCheatActive);
      ImGui::SameLine();
      ImGui::TextColored(ImVec4(0.7f, 1.0f, 0.5f, 1.0f),
                         "ANIMAL + ANIMAL_EXTRA_VALUES");
      if (JNIHelper::g_animalCheatActive)
        ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.3f, 1.0f),
                           "  >> ACTIVO - Taming libre, stress=0, stats extra");

      // ── COMBAT SUPREMACY — field overrides directos, cero logging
      // ───────────
      ImGui::Separator();
      ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f),
                         "[ COMBAT SUPREMACY ]");
      ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f),
                         "Field overrides directos - Invisible al admin");
      if (ImGui::Checkbox("Combat Supremacy",
                          &JNIHelper::g_combatSupremacyActive))
        JNIHelper::ToggleCombatSupremacy(JNIHelper::g_combatSupremacyActive);
      if (JNIHelper::g_combatSupremacyActive) {
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), "  >> ACTIVO");
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 0.9f),
                           "     ignoreStaggerBack  blurFactor=0");
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 0.9f),
                           "     recoilDelay=0  swingTime=min");
      }

      // ── STEALTH & PERCEPTION — local-only, zero network
      // ──────────────────────
      ImGui::Separator();
      ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.8f, 1.0f),
                         "[ STEALTH & PERCEPTION ]");
      ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f),
                         "100%% local - Sin sync de servidor");
      if (ImGui::Checkbox("Night Vision", &JNIHelper::g_nightVisionActive))
        JNIHelper::ToggleNightVision(JNIHelper::g_nightVisionActive);
      ImGui::SameLine();
      ImGui::TextDisabled("setWearingNightVisionGoggles");
      if (ImGui::Checkbox("Lightfoot (Stealth)", &JNIHelper::g_lightfootActive))
        JNIHelper::ToggleLightfoot(JNIHelper::g_lightfootActive);
      ImGui::SameLine();
      ImGui::TextDisabled("wornItemsHearingModifier=0");

      // ── KNOW ALL RECIPES — CheatType local
      // ───────────────────────────────────
      ImGui::Separator();
      ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.3f, 1.0f), "[ CRAFTING ]");
      if (ImGui::Checkbox("Know All Recipes",
                          &JNIHelper::g_knowAllRecipesActive))
        JNIHelper::ToggleKnowAllRecipes(JNIHelper::g_knowAllRecipesActive);
      ImGui::SameLine();
      ImGui::TextDisabled("CheatType flag - UI local");

      // ── LOOT ESP
      // ──────────────────────────────────────────────────────────────
      ImGui::Separator();
      ImGui::TextColored(ImVec4(0.8f, 0.6f, 1.0f, 1.0f), "[ LOOT ESP ]");
      ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f),
                         "Read-only - Sin paquetes al servidor");
      if (ImGui::Checkbox("Loot ESP Activo", &JNIHelper::g_lootEspActive)) {
      }
      
      ImGui::Checkbox("Weapons", &JNIHelper::g_lootFilterWeapon);
      ImGui::SameLine();
      ImGui::Checkbox("Ammo", &JNIHelper::g_lootFilterAmmo);
      ImGui::SameLine();
      ImGui::Checkbox("Medical", &JNIHelper::g_lootFilterMedical);
      
      ImGui::Checkbox("Food", &JNIHelper::g_lootFilterFood);
      ImGui::SameLine();
      ImGui::Checkbox("Mods/Other", &JNIHelper::g_lootFilterMods);
      
      ImGui::InputText("Custom Filter", JNIHelper::g_lootFilterCustom, sizeof(JNIHelper::g_lootFilterCustom));
      
      static int s_lootRefreshCounter = 0;
      if (JNIHelper::g_lootEspActive) {
        // Actualizar cada 120 frames (~2s) para no sobrecargar
        if (s_lootRefreshCounter++ >= 120) {
          JNIHelper::g_lootCache = JNIHelper::GetNearbyLoot(15);
          s_lootRefreshCounter = 0;
        }
      } else {
        s_lootRefreshCounter = 0;
        if (!JNIHelper::g_lootCache.empty()) JNIHelper::g_lootCache.clear();
      }
      
      ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.5f, 1.0f),
                         "Containers con loot: %d", (int)JNIHelper::g_lootCache.size());
      if (ImGui::BeginChild("##LootList", ImVec2(-FLT_MIN, 100.f), true)) {
        for (const auto &loot : JNIHelper::g_lootCache) {
          char posStr[24];
          snprintf(posStr, sizeof(posStr), "(%.0f,%.0f)", loot.x, loot.y);
          if (ImGui::TreeNode(posStr)) {
            for (const auto &item : loot.items)
              ImGui::TextDisabled("  %s", item.name.c_str());
            ImGui::TreePop();
          }
        }
      }
      ImGui::EndChild();
    } else {
      ImGui::TextColored(ImVec4(1, 0, 0, 1), "Status: Disconnected");
      if (ImGui::Button("Hook Player Instance"))
        JNIHelper::GetLocalPlayer();
    }
  }


// --- World Manipulation ---
if (ImGui::CollapsingHeader("WORLD MANIPULATION")) {
  static char itemFilter[128] = "";
  ImGui::InputText("Search Item", itemFilter, sizeof(itemFilter));
  if (ImGui::Button("Load Item List"))
    JNIHelper::GetAllItems();
  if (!JNIHelper::g_itemCache.empty()) {
    if (ImGui::BeginListBox(
            "##ItemList",
            ImVec2(-FLT_MIN, 5 * ImGui::GetTextLineHeightWithSpacing()))) {
      for (const auto &item : JNIHelper::g_itemCache) {
        if (itemFilter[0] && !strstr(item.c_str(), itemFilter))
          continue;
        bool sel = (JNIHelper::g_selectedItem == item);
        if (ImGui::Selectable(item.c_str(), sel)) {
          JNIHelper::g_selectedItem = item;
          size_t d = item.find(" | ");
          std::string id = (d != std::string::npos) ? item.substr(0, d) : item;
          strncpy(JNIHelper::g_targetItemID, id.c_str(),
                  sizeof(JNIHelper::g_targetItemID) - 1);
        }
        if (sel)
          ImGui::SetItemDefaultFocus();
      }
      ImGui::EndListBox();
    }
  }
  ImGui::InputText("Target ID", JNIHelper::g_targetItemID,
                   sizeof(JNIHelper::g_targetItemID));
  static int qty = 1;
  ImGui::InputInt("Quantity", &qty);
  if (qty < 1)
    qty = 1;
  // [ENI] Desactivado en B42 Stable por incompatibilidad
  // if (ImGui::Button("Spawn Item (Ground)"))
  //   JNIHelper::SpawnItem(JNIHelper::g_targetItemID, qty);
    
  ImGui::Separator();
  ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "[ Mass AoE Injection ]");
  ImGui::SliderInt("Radius (Tiles)##AoE", &JNIHelper::corpseAoERadius, 1, 50);
  
  if (ImGui::Button("Inject Loot to ALL Corpses in Radius (AoE)")) {
    JNIHelper::InjectLootToCorpsesAoE(JNIHelper::corpseAoERadius);
  }
  if (ImGui::Button("Inject Loot to ALL Containers in Radius (AoE)")) {
    JNIHelper::InjectLootToContainersAoE(JNIHelper::corpseAoERadius);
  }

  // [ENI-P7] Dynamic Vehicle Key Spawner
  ImGui::Separator();
  static std::string nearCar = "";
  static int throttle = 0;
  if (throttle++ % 60 == 0) {
    nearCar = JNIHelper::GetNearVehicleScriptName();
  }

  if (!nearCar.empty()) {
    ImGui::PushStyleColor(ImGuiCol_Button,
                          ImVec4(0.0f, 1.0f, 0.8f, 0.4f)); // Cyan transparente
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                          ImVec4(0.0f, 1.0f, 0.8f, 0.7f));
    if (ImGui::Button(("Hotwire: " + nearCar).c_str())) {
      JNIHelper::SpawnKeyForNearVehicle();
    }
    ImGui::PopStyleColor(2);
  } else {
    ImGui::PushStyleColor(ImGuiCol_Button,
                          ImVec4(0.5f, 0.0f, 0.0f, 0.4f)); // Rojo transparente
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
    ImGui::Button("Hotwire: No Vehicle Found");
    ImGui::PopStyleColor(2);
  }
}

// --- Skills & XP ---
if (ImGui::CollapsingHeader("SKILLS & XP")) {
  static char perkBuf[128] = "Axe";
  ImGui::InputText("Perk ID", perkBuf, sizeof(perkBuf));
  static float xpAmt = 1000.f;
  ImGui::InputFloat("XP Amount", &xpAmt);
  ImGui::BeginDisabled(true);
  if (ImGui::Button("Add XP"))
    JNIHelper::AddXP(perkBuf, xpAmt);
  ImGui::EndDisabled();
  ImGui::SameLine();
  ImGui::TextDisabled("(disabled - WIP)");
}

// --- Advanced ---
if (ImGui::CollapsingHeader("ADVANCED")) {
  if (ImGui::Button("Dump IsoGameCharacter"))
    JNIHelper::DumpClassInfo("zombie/characters/IsoGameCharacter");
  ImGui::SameLine();
  if (ImGui::Button("Dump PlayerCheats"))
    JNIHelper::DumpClassInfo("zombie/characters/PlayerCheats");
  if (ImGui::Button("Dump CheatType"))
    JNIHelper::DumpClassInfo("zombie/characters/CheatType");
  ImGui::SameLine();
  if (ImGui::Button("Dump IsoPlayer"))
    JNIHelper::DumpClassInfo("zombie/characters/IsoPlayer");
}

if (ImGui::CollapsingHeader("REVERSE ENGINEERING", ImGuiTreeNodeFlags_DefaultOpen)) {
  ImGui::TextColored(ImVec4(0.8f, 0.2f, 1.0f, 1.0f), "[ JVMTI DEEP DUMPER ]");
  ImGui::TextWrapped("Exporta todas las clases de la B42 cargadas en memoria a un JSON.");
  if (ImGui::Button("Dump B42 Classes (JSON)", ImVec2(-1, 30))) {
    JNIHelper::DumpAllLoadedClasses();
  }
  ImGui::Separator();
  ImGui::TextColored(ImVec4(0.8f, 0.2f, 1.0f, 1.0f), "[ NATIVE DEBUG ]");
  static bool s_forceDebugMode = false;
  if (ImGui::Checkbox("Force Game Debug Mode (F11)", &s_forceDebugMode)) {
    JNIHelper::ToggleDebugMode(s_forceDebugMode);
  }
}

// --- Status ---
if (ImGui::CollapsingHeader("STATUS", ImGuiTreeNodeFlags_DefaultOpen)) {
  bool jniOk = (JNIHelper::g_javaVm != nullptr);
  ImGui::TextColored(jniOk ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0, 0, 1), "JNI: %s",
                     jniOk ? "Connected" : "Disconnected");
  ImGui::Text("Player: %s", JNIHelper::g_playerInstance ? "OK" : "Not found");
  ImGui::Text("Frame: %d", g_frameCount);
  int cached = 0;
  if (JNIHelper::g_cachedIsoPlayerClass)
    cached++;
  if (JNIHelper::g_cachedIsoGameCharacterClass)
    cached++;
  if (JNIHelper::g_cachedPlayerCheatsClass)
    cached++;
  if (JNIHelper::g_cachedIsoWorldClass)
    cached++;
  if (JNIHelper::g_cachedIsoCellClass)
    cached++;
  if (JNIHelper::g_cachedIsoZombieClass)
    cached++;
  ImGui::Text("Cached Classes: %d", cached);
}

ImGui::End();
if (!open)
  g_showMenu = false;
}

// =========================================================================================
// CORE FRAME HOOK — shared by EGL and GLX paths
// =========================================================================================

static bool g_inHook = false;
static GLXContext g_glxContext = nullptr;
static Display *g_hookDpy = nullptr;
// g_hookDrawable declared at top (needed by EnsureEventSubscription)
static bool g_jniThreadLaunched = false;

static void RunHookFrame() {
  if (g_inHook)
    return;

  // Soportar tanto X11 (GLX) como Wayland (EGL)
  bool hasGLX =
      (glXGetCurrentContext != nullptr && glXGetCurrentContext() != nullptr);
  bool hasEGL =
      (eglGetCurrentContext != nullptr && eglGetCurrentContext() != nullptr);

  if (!hasGLX && !hasEGL)
    return;

  // Trackear el contexto actual para ImGui
  void *currentContext =
      hasGLX ? (void *)glXGetCurrentContext() : (void *)eglGetCurrentContext();

  if (g_imguiInitialized && currentContext != g_glxContext) {
    return;
  }

  g_inHook = true;
  g_frameCount++;
  EnsureX11Display();

  auto now = std::chrono::steady_clock::now();
  float dt = std::chrono::duration<float>(now - g_lastTime).count();
  g_lastTime = now;
  if (dt <= 0.f || dt > 0.5f)
    dt = 1.f / 60.f;

  // [R-LOWSPEC] Throttle XQueryKeymap — es un round-trip IPC al servidor X11.
  // Verificar cada 4 frames da respuesta de ~66ms, suficiente para toggle de
  // menu.
  static int s_keyCheckFrame = 0;
  static bool s_insertState = false;
  if ((g_frameCount - s_keyCheckFrame) >= 4) {
    s_insertState = IsKeyDown(XK_Insert);
    s_keyCheckFrame = g_frameCount;
  }
  bool insertNow = s_insertState;
  if (insertNow && !g_insertWasDown) {
    g_showMenu = !g_showMenu;
    Log("[*] Menu %s", g_showMenu ? "SHOWN" : "HIDDEN");
  }
  g_insertWasDown = insertNow;

  if (!g_imguiInitialized && g_frameCount > 60) {
    Log("[*] Initializing ImGui at frame %d", g_frameCount);
    GLint s_prog = 0, s_vao = 0, s_vbo = 0, s_ebo = 0, s_tex = 0, s_atex = 0,
          s_ua = 4, s_ur = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &s_prog);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &s_vao);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &s_vbo);
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &s_ebo);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &s_atex);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &s_tex);
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &s_ua);
    glGetIntegerv(GL_UNPACK_ROW_LENGTH, &s_ur);
    InitImGui();
    glUseProgram(s_prog);
    glBindVertexArray(s_vao);
    glBindBuffer(GL_ARRAY_BUFFER, s_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_ebo);
    glActiveTexture(s_atex);
    glBindTexture(GL_TEXTURE_2D, s_tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, s_ua);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, s_ur);
    g_glxContext = (GLXContext)currentContext;
    Log("[+] ImGui ready, GL state restored");
  }

  if (!g_jniReady && g_imguiInitialized && !g_jniThreadLaunched) {
    g_jniThreadLaunched = true;
    pthread_t t;
    pthread_create(
        &t, nullptr,
        [](void *) -> void * {
          for (int i = 0; i < 60; i++) {
            if (JNIHelper::Initialize()) {
              g_jniReady = true;
              Log("[+] JNI ready — click 'Hook Player Instance' to connect");
              if (JNIHelper::g_javaVm) {
                  JNIHelper::g_javaVm->DetachCurrentThread();
              }
              return nullptr;
            }
            sleep(1);
          }
          return nullptr;
        },
        nullptr);
    pthread_detach(t);
  }

  // Auto-hook player instance if missing (Solves JVM execution time delay)
  if (g_jniReady && !JNIHelper::g_playerInstance) {
    static int hookTimer = 0;
    if (++hookTimer >= 60) {
      JNIHelper::GetLocalPlayer();
      hookTimer = 0;
    }
  }

  // Apply active cheats each frame when player is available
  if (g_jniReady && JNIHelper::g_playerInstance) {
    JNIHelper::ApplyActiveCheats();
  }

  if (g_imguiInitialized) {
    GLint vp[4] = {0, 0, 1920, 1080};
    glGetIntegerv(GL_VIEWPORT, vp);
    unsigned int winW = vp[2] < 1 ? 1920u : (unsigned int)vp[2];
    unsigned int winH = vp[3] < 1 ? 1080u : (unsigned int)vp[3];

    float dispW = (g_winWidth > 0) ? (float)g_winWidth : (float)winW;
    float dispH = (g_winHeight > 0) ? (float)g_winHeight : (float)winH;

    ImGuiIO &io = ImGui::GetIO();
    io.DisplaySize = ImVec2(dispW, dispH);
    io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
    io.DeltaTime = dt;

    io.MousePos = ImVec2((float)g_mouseX, (float)g_mouseY);
    io.MouseDown[0] = g_mouse0;
    io.MouseDown[1] = g_mouse1;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();
    if (g_showMenu)
      RenderMenu();

    // [R-LOWSPEC] Fetch UNICO de entidades por frame — ambas funciones de
    // render reciben la misma referencia const. Sin doble scan JNI, sin doble
    // copia de vector.
    bool anyEspNeeded =
        g_jniReady && JNIHelper::g_playerInstance &&
        (g_espEnabled || g_playerEspEnabled || g_vehicleEspEnabled ||
         g_animalEspEnabled || JNIHelper::g_minimapConfig.enabled);
    const std::vector<JNIHelper::ZombieInfo> frameEntities =
        anyEspNeeded ? JNIHelper::GetZombiesForESP()
                     : std::vector<JNIHelper::ZombieInfo>{};

    if (g_espEnabled || g_playerEspEnabled || g_vehicleEspEnabled ||
        g_animalEspEnabled)
      RenderESP((unsigned int)dispW, (unsigned int)dispH, frameEntities);
    RenderMinimapOverlay(frameEntities);
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  }
  g_inHook = false;
}

// =========================================================================================
// dlsym OVERRIDE — required because GLFW calls dlsym("glXSwapBuffers") at
// runtime
// =========================================================================================

// Forward declarations
static void hook_glXSwapBuffers(Display *, GLXDrawable);
static EGLBoolean hook_eglSwapBuffers(EGLDisplay, EGLSurface);

typedef void *(*dlsym_fn_t)(void *, const char *);
static dlsym_fn_t real_dlsym = nullptr;

static void init_real_dlsym() {
  real_dlsym = (dlsym_fn_t)dlvsym(RTLD_NEXT, "dlsym", "GLIBC_2.34");
  if (!real_dlsym)
    real_dlsym = (dlsym_fn_t)dlvsym(RTLD_NEXT, "dlsym", "GLIBC_2.2.5");
}

extern "C" __attribute__((visibility("default"))) void *
dlsym(void *handle, const char *symbol) {
  if (!real_dlsym)
    init_real_dlsym();
  if (!real_dlsym)
    return nullptr;
  if (symbol) {
    if (strcmp(symbol, "glXSwapBuffers") == 0)
      return (void *)hook_glXSwapBuffers;
    if (strcmp(symbol, "eglSwapBuffers") == 0)
      return (void *)hook_eglSwapBuffers;
    if (strcmp(symbol, "XNextEvent") == 0)
      return (void *)XNextEvent;
    if (strcmp(symbol, "XCheckWindowEvent") == 0)
      return (void *)XCheckWindowEvent;
    if (strcmp(symbol, "XCheckMaskEvent") == 0)
      return (void *)XCheckMaskEvent;
    if (strcmp(symbol, "XCheckTypedWindowEvent") == 0)
      return (void *)XCheckTypedWindowEvent;
    if (strcmp(symbol, "XCheckIfEvent") == 0)
      return (void *)XCheckIfEvent;
  }
  return real_dlsym(handle, symbol);
}

// =========================================================================================
// DIRECT SYMBOL HOOKS (for libraries that link glXSwapBuffers directly)
// =========================================================================================

// 1. EGL Hook (Wayland / Niri)
typedef EGLBoolean (*real_eglSwapBuffers_t)(EGLDisplay, EGLSurface);
static real_eglSwapBuffers_t real_eglSwapBuffers = nullptr;

static EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
  if (!real_eglSwapBuffers) {
    if (!real_dlsym)
      init_real_dlsym();
    real_eglSwapBuffers =
        (real_eglSwapBuffers_t)real_dlsym(RTLD_NEXT, "eglSwapBuffers");
  }
  RunHookFrame();
  return real_eglSwapBuffers ? real_eglSwapBuffers(dpy, surface) : EGL_FALSE;
}

extern "C" __attribute__((visibility("default"))) EGLBoolean
eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
  return hook_eglSwapBuffers(dpy, surface);
}

// 2. GLX Hook (X11 Fallback)
typedef void (*real_glXSwapBuffers_t)(Display *, GLXDrawable);
static real_glXSwapBuffers_t real_glXSwapBuffers = nullptr;

static void hook_glXSwapBuffers(Display *dpy, GLXDrawable drawable) {
  if (!real_glXSwapBuffers) {
    if (!real_dlsym)
      init_real_dlsym();
    real_glXSwapBuffers =
        (real_glXSwapBuffers_t)real_dlsym(RTLD_NEXT, "glXSwapBuffers");
  }
  g_hookDpy = dpy;
  g_hookDrawable = drawable;
  // Query framebuffer size (GLX drawable) and actual window size
  if (g_fbWidth == 0) {
    unsigned int dw = 0, dh = 0;
    glXQueryDrawable(dpy, drawable, GLX_WIDTH, &dw);
    glXQueryDrawable(dpy, drawable, GLX_HEIGHT, &dh);
    if (dw > 0 && dh > 0) {
      g_fbWidth = (int)dw;
      g_fbHeight = (int)dh;
      Log("[+] Drawable (fb) size: %dx%d", g_fbWidth, g_fbHeight);
    }
  }
  // g_winWidth/g_winHeight come from ConfigureNotify events intercepted in
  // ProcessX11Event
  RunHookFrame();
  if (real_glXSwapBuffers)
    real_glXSwapBuffers(dpy, drawable);
}

extern "C" __attribute__((visibility("default"))) void
glXSwapBuffers(Display *dpy, GLXDrawable drawable) {
  hook_glXSwapBuffers(dpy, drawable);
}

// ---- X11 Event Hooks ----
// Intercept GLFW's X11 event loop to feed mouse state to ImGui.
// All events pass through unchanged so GLFW/the game is unaffected.

static bool IsMouseEventType(int t) {
  return t == ButtonPress || t == ButtonRelease; // Never block MotionNotify
}
static bool IsKeyboardEventType(int t) {
  return t == KeyPress || t == KeyRelease;
}
static bool ImGuiWantsMouse() {
  return ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse;
}
static bool ImGuiWantsKeyboard() {
  return ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureKeyboard;
}

typedef int (*real_XNextEvent_t)(Display *, XEvent *);
static real_XNextEvent_t real_XNextEvent = nullptr;
extern "C" __attribute__((visibility("default"))) int XNextEvent(Display *dpy,
                                                                 XEvent *ev) {
  if (!real_XNextEvent)
    real_XNextEvent = (real_XNextEvent_t)real_dlsym(RTLD_NEXT, "XNextEvent");
  int r = real_XNextEvent(dpy, ev);
  ProcessX11Event(ev);
  if ((ImGuiWantsMouse() && IsMouseEventType(ev->type)) ||
      (ImGuiWantsKeyboard() && IsKeyboardEventType(ev->type))) {
    ev->type = 0;
  }
  return r;
}

typedef Bool (*real_XCheckWindowEvent_t)(Display *, Window, long, XEvent *);
static real_XCheckWindowEvent_t real_XCheckWindowEvent = nullptr;
extern "C" __attribute__((visibility("default"))) Bool
XCheckWindowEvent(Display *dpy, Window w, long mask, XEvent *ev) {
  if (!real_XCheckWindowEvent)
    real_XCheckWindowEvent =
        (real_XCheckWindowEvent_t)real_dlsym(RTLD_NEXT, "XCheckWindowEvent");
  Bool r = real_XCheckWindowEvent(dpy, w, mask, ev);
  if (r) {
    ProcessX11Event(ev);
    if ((ImGuiWantsMouse() && IsMouseEventType(ev->type)) ||
        (ImGuiWantsKeyboard() && IsKeyboardEventType(ev->type))) {
      ev->type = 0;
    }
  }
  return r;
}

typedef Bool (*real_XCheckMaskEvent_t)(Display *, long, XEvent *);
static real_XCheckMaskEvent_t real_XCheckMaskEvent = nullptr;
extern "C" __attribute__((visibility("default"))) Bool
XCheckMaskEvent(Display *dpy, long mask, XEvent *ev) {
  if (!real_XCheckMaskEvent)
    real_XCheckMaskEvent =
        (real_XCheckMaskEvent_t)real_dlsym(RTLD_NEXT, "XCheckMaskEvent");
  Bool r = real_XCheckMaskEvent(dpy, mask, ev);
  if (r) {
    ProcessX11Event(ev);
    if ((ImGuiWantsMouse() && IsMouseEventType(ev->type)) ||
        (ImGuiWantsKeyboard() && IsKeyboardEventType(ev->type))) {
      ev->type = 0;
    }
  }
  return r;
}

typedef Bool (*real_XCheckTypedWindowEvent_t)(Display *, Window, int, XEvent *);
static real_XCheckTypedWindowEvent_t real_XCheckTypedWindowEvent = nullptr;
extern "C" __attribute__((visibility("default"))) Bool
XCheckTypedWindowEvent(Display *dpy, Window w, int type, XEvent *ev) {
  if (!real_XCheckTypedWindowEvent)
    real_XCheckTypedWindowEvent = (real_XCheckTypedWindowEvent_t)real_dlsym(
        RTLD_NEXT, "XCheckTypedWindowEvent");
  Bool r = real_XCheckTypedWindowEvent(dpy, w, type, ev);
  if (r) {
    ProcessX11Event(ev);
    if ((ImGuiWantsMouse() && IsMouseEventType(ev->type)) ||
        (ImGuiWantsKeyboard() && IsKeyboardEventType(ev->type))) {
      ev->type = 0;
    }
  }
  return r;
}

typedef Bool (*real_XCheckIfEvent_t)(Display *, XEvent *,
                                     Bool (*)(Display *, XEvent *, XPointer),
                                     XPointer);
static real_XCheckIfEvent_t real_XCheckIfEvent = nullptr;
extern "C" __attribute__((visibility("default"))) Bool
XCheckIfEvent(Display *dpy, XEvent *ev,
              Bool (*pred)(Display *, XEvent *, XPointer), XPointer arg) {
  if (!real_XCheckIfEvent)
    real_XCheckIfEvent =
        (real_XCheckIfEvent_t)real_dlsym(RTLD_NEXT, "XCheckIfEvent");
  Bool r = real_XCheckIfEvent(dpy, ev, pred, arg);
  if (r) {
    ProcessX11Event(ev);
    if ((ImGuiWantsMouse() && IsMouseEventType(ev->type)) ||
        (ImGuiWantsKeyboard() && IsKeyboardEventType(ev->type))) {
      ev->type = 0;
    }
  }
  return r;
}

// 3. Interceptar las búsquedas internas de LWJGL/SDL2
// Note: eglGetProcAddress returns __eglMustCastToProperFunctionPointerType (not
// void*)
typedef __eglMustCastToProperFunctionPointerType (*real_eglGetProcAddress_t)(
    const char *);
static real_eglGetProcAddress_t real_eglGetProcAddress = nullptr;

extern "C" __attribute__((visibility("default")))
__eglMustCastToProperFunctionPointerType
eglGetProcAddress(const char *procname) {
  if (!real_eglGetProcAddress)
    real_eglGetProcAddress =
        (real_eglGetProcAddress_t)dlsym(RTLD_NEXT, "eglGetProcAddress");
  if (procname && strcmp(procname, "eglSwapBuffers") == 0)
    return (__eglMustCastToProperFunctionPointerType)hook_eglSwapBuffers;
  return real_eglGetProcAddress ? real_eglGetProcAddress(procname) : nullptr;
}

// Note: glXGetProcAddress returns __GLXextFuncPtr (not void*)
typedef __GLXextFuncPtr (*real_glXGetProcAddress_t)(const GLubyte *);
static real_glXGetProcAddress_t real_glXGetProcAddress = nullptr;

extern "C" __attribute__((visibility("default"))) __GLXextFuncPtr
glXGetProcAddress(const GLubyte *procname) {
  if (!real_glXGetProcAddress)
    real_glXGetProcAddress =
        (real_glXGetProcAddress_t)dlsym(RTLD_NEXT, "glXGetProcAddress");
  if (procname && strcmp((const char *)procname, "glXSwapBuffers") == 0)
    return (__GLXextFuncPtr)hook_glXSwapBuffers;
  return real_glXGetProcAddress ? real_glXGetProcAddress(procname) : nullptr;
}

typedef __GLXextFuncPtr (*real_glXGetProcAddressARB_t)(const GLubyte *);
static real_glXGetProcAddressARB_t real_glXGetProcAddressARB = nullptr;

extern "C" __attribute__((visibility("default"))) __GLXextFuncPtr
glXGetProcAddressARB(const GLubyte *procname) {
  if (!real_glXGetProcAddressARB)
    real_glXGetProcAddressARB =
        (real_glXGetProcAddressARB_t)dlsym(RTLD_NEXT, "glXGetProcAddressARB");
  if (procname && strcmp((const char *)procname, "glXSwapBuffers") == 0)
    return (__GLXextFuncPtr)hook_glXSwapBuffers;
  return real_glXGetProcAddressARB ? real_glXGetProcAddressARB(procname)
                                   : nullptr;
}

// =========================================================================================
// CONSTRUCTOR
// =========================================================================================

__attribute__((constructor)) static void full_constructor() {
  fLog = fopen("/home/j4ck/Dev/Prototipo-2.1 (Limpio) FULL FEATURES/logs/zomboid_full_linux.log", "w");
  Log("=== [Prototipo-Full] Linux LD_PRELOAD ===");
  Log("Toggle menu: INSERT key");
  Log("[*] Hooks active (EGL/GLX/GetProcAddress)");
  // JNI is lazy-loaded from the render thread after GL context is stable
}

__attribute__((destructor)) static void full_destructor() {
  if (g_imguiInitialized) {
    // ImGui::DestroyContext(); // Comentado para evitar JVM crash SIGSEGV al destruir el contexto
  }
  if (fLog)
    fclose(fLog);
}
