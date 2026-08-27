#include "../include/jni_helpers.h"
#include <cmath>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

// Static member initialization
JavaVM *JNIHelper::g_javaVm = nullptr;
JNIEnv *JNIHelper::g_jniEnv = nullptr;

bool JNIHelper::g_godModeActive = false;
bool JNIHelper::g_invisibleActive = false;
bool JNIHelper::g_zombiesDontAttackActive = false;
int JNIHelper::corpseAoERadius = 15;
bool JNIHelper::g_ghostModeActive = false;
bool JNIHelper::g_noClipActive = false;
bool JNIHelper::g_unlimitedAmmoActive = false;
bool JNIHelper::g_unlimitedCarryActive = false;
bool JNIHelper::g_unlimitedEnduranceActive = false;
bool JNIHelper::g_softAimActive          = false; // [ENI] Soft Aim
bool JNIHelper::g_criticalOverrideActive = false; // [ENI] Critical Override

// [ENI] Soft Aim + Critical Override — HandWeapon cache
jclass    JNIHelper::g_cachedHandWeaponClass    = nullptr;
jfieldID  JNIHelper::g_fidHWHitChance           = nullptr;
jfieldID  JNIHelper::g_fidHWMinAngle            = nullptr;
jfieldID  JNIHelper::g_fidHWMaxAngle            = nullptr;
jfieldID  JNIHelper::g_fidHWProjectileSpread    = nullptr;
jfieldID  JNIHelper::g_fidHWJamGunChance        = nullptr;
jmethodID JNIHelper::g_midHWIsRanged            = nullptr;
jmethodID JNIHelper::g_midHWIsAimedFirearm      = nullptr;
jfieldID  JNIHelper::g_fidHWCriticalChance      = nullptr; // [ENI] Critical Override
jfieldID  JNIHelper::g_fidHWCriticalDmgMult     = nullptr; // [ENI] Critical Override
jfieldID  JNIHelper::g_fidHWAimingPerkCritMod   = nullptr; // [ENI] Critical Override
jfieldID  JNIHelper::g_fidHWSwingTime           = nullptr; // [ENI] Combat Supremacy (HandWeapon)
jfieldID  JNIHelper::g_fidHWMinSwingTime        = nullptr; // [ENI] Combat Supremacy (HandWeapon)
jfieldID  JNIHelper::g_fidHWRecoilDelayHW       = nullptr; // [ENI] Combat Supremacy (HandWeapon)
// [ENI] Combat Supremacy — setters públicos de IGC (reemplazo de field sets directos)
jmethodID JNIHelper::g_midSetIgnoreStaggerBack  = nullptr; // setIgnoreStaggerBack(Z)V
jmethodID JNIHelper::g_midSetStaggerTimeMod     = nullptr; // setStaggerTimeMod(F)V
jmethodID JNIHelper::g_midSetRecoilDelay        = nullptr; // setRecoilDelay(F)V
jfieldID  JNIHelper::g_fidBlurFactor            = nullptr; // blurFactor (sin setter público)
jfieldID  JNIHelper::g_fidBlurFactorTarget      = nullptr; // blurFactorTarget

bool JNIHelper::g_playerEspActive = false;
bool JNIHelper::g_vehicleEspActive = false;

// [ENI] Minimap Configuration
JNIHelper::MinimapConfig JNIHelper::g_minimapConfig = {
    false,  // enabled
    150.0f, // scale
    0.75f,  // alpha
    0,      // position (0=Bottom-Right)
    true,   // showGrid
    true,   // showZombies
    true,   // showPlayers
    true,   // showVehicles
    false   // showContainers
};

// [ENI] Entity Cache for FPS Optimization
JNIHelper::EntityCache JNIHelper::g_entityCache = {
    {},   // entities
    0,    // frameLastUpdate
    false // isValid
};

// [ENI] Moodle Configuration
JNIHelper::MoodleDisplayConfig JNIHelper::g_moodleConfig = {
    true,  // enabled
    0,     // position (0=Top-Left)
    0.85f, // alpha
    true,  // showHunger
    true,  // showThirst
    true,  // showFatigue
    true,  // showSick
    true,  // showInjured
    true   // showPanic
};

// [ENI] Moodle Info (cached data)
JNIHelper::MoodleInfo JNIHelper::g_moodleInfo = {
    0.0f,  // hunger
    0.0f,  // thirst
    0.0f,  // fatigue
    0.0f,  // panic
    37.0f, // temperature
    false, // isSick
    false, // isInjured
    false, // isBleeding
    false, // isCold
    0      // painLevel
};

std::vector<std::string> JNIHelper::g_itemCache;
std::string JNIHelper::g_selectedItem = "";
char JNIHelper::g_targetItemID[128] = "Base.Axe";

jobject JNIHelper::g_playerInstance = nullptr;

// ClassLoader and class reference caching
jobject JNIHelper::g_classLoader = nullptr;
jclass JNIHelper::g_classLoaderClass = nullptr;

jclass JNIHelper::g_cachedIsoPlayerClass = nullptr;
jclass JNIHelper::g_cachedIsoGameCharacterClass = nullptr;
jclass JNIHelper::g_cachedPlayerCheatsClass = nullptr;
jclass JNIHelper::g_cachedStatsClass = nullptr;
jclass JNIHelper::g_cachedCharacterStatClass = nullptr;
jclass JNIHelper::g_cachedBodyDamageClass = nullptr;
jclass JNIHelper::g_cachedScriptManagerClass = nullptr;
jclass JNIHelper::g_cachedItemClass = nullptr;
jclass JNIHelper::g_cachedArrayListClass = nullptr;
jclass JNIHelper::g_cachedSandboxOptionsClass = nullptr;
jclass JNIHelper::g_cachedIntegerConfigOptionClass = nullptr;
jclass JNIHelper::g_cachedLuaManagerClass = nullptr;
jclass JNIHelper::g_cachedKahluaTableClass = nullptr;
jclass JNIHelper::g_cachedDoubleClass = nullptr;
jclass JNIHelper::g_cachedNutritionClass = nullptr;

// GameState Caching
jclass JNIHelper::g_cachedGameWindowClass = nullptr;
jclass JNIHelper::g_cachedGameStateMachineClass = nullptr;
jclass JNIHelper::g_cachedGameStateClass = nullptr;
jclass JNIHelper::g_cachedMainScreenStateClass = nullptr;

// Field IDs
static jfieldID g_mainScreenStateInstanceFieldId = nullptr;

jfieldID JNIHelper::g_statesFieldId = nullptr;
jfieldID JNIHelper::g_currentFieldId = nullptr;
jfieldID JNIHelper::g_fidLuaEnv = nullptr;

jmethodID JNIHelper::g_hasInstanceMethodId = nullptr;
jmethodID JNIHelper::g_getInstanceMethodId = nullptr;
jmethodID JNIHelper::g_midGetStats = nullptr;
jmethodID JNIHelper::g_midRestoreToFullHealth = nullptr;
jmethodID JNIHelper::g_midAddXP = nullptr;
jmethodID JNIHelper::g_midGetPerk = nullptr;
jmethodID JNIHelper::g_midGetCurrentSquare = nullptr;
jmethodID JNIHelper::g_midCreateItem = nullptr;
jmethodID JNIHelper::g_midAddWorldInventoryItem = nullptr;
jmethodID JNIHelper::g_midTransmitCompleteItemToServer = nullptr;
jmethodID JNIHelper::g_midGetAllItems = nullptr;
jmethodID JNIHelper::g_midGetFullName = nullptr;
jmethodID JNIHelper::g_midGetDisplayName = nullptr;
jmethodID JNIHelper::g_midArrayListSize = nullptr;
jmethodID JNIHelper::g_midArrayListGet = nullptr;
jmethodID JNIHelper::g_midRawGet = nullptr;
jmethodID JNIHelper::g_midRawSet = nullptr;
jmethodID JNIHelper::g_midDoubleInit = nullptr;

// [ENI] Cached IDs for 60Hz loop
jfieldID JNIHelper::g_fidInvincible = nullptr;
jfieldID JNIHelper::g_fidAvoidDamage = nullptr;
jfieldID JNIHelper::g_fidGhostMode = nullptr;
jfieldID JNIHelper::g_fidCheats = nullptr;
jfieldID JNIHelper::g_fidEnumSet = nullptr;
jmethodID JNIHelper::g_midPlayerCheatsSet = nullptr;
jmethodID JNIHelper::g_midPlayerCheatsIsSet = nullptr;
jmethodID JNIHelper::g_midPlayerCheatsSetPrivate = nullptr;
jmethodID JNIHelper::g_midPlayerCheatsUnsetPrivate = nullptr;
jfieldID JNIHelper::g_fidRightHandItem = nullptr;
jmethodID JNIHelper::g_midGetMaxAmmo = nullptr;
jmethodID JNIHelper::g_midSetCurrentAmmo = nullptr;
jfieldID JNIHelper::g_fidBodyDamage = nullptr;
jfieldID JNIHelper::g_fidXP = nullptr;
jfieldID JNIHelper::g_fidEndurance = nullptr;
jfieldID JNIHelper::g_fidFatigue = nullptr;
jfieldID JNIHelper::g_fidStats = nullptr;
jfieldID JNIHelper::g_fidSandboxInstance = nullptr;
jfieldID JNIHelper::g_fidCharacterFreePoints = nullptr;
jfieldID JNIHelper::g_fidIntOptionValue = nullptr;

jobject JNIHelper::g_valGodMode = nullptr;
jobject JNIHelper::g_valInvisible = nullptr;
jobject JNIHelper::g_valZombiesDontAttack = nullptr;
jobject JNIHelper::g_valGhostMode = nullptr;
jobject JNIHelper::g_valNoClip = nullptr;
jobject JNIHelper::g_valUnlimitedAmmo = nullptr;
jobject JNIHelper::g_valUnlimitedCarry = nullptr;
jobject JNIHelper::g_valAnimal = nullptr;            // [ENI] CheatType::ANIMAL
jobject JNIHelper::g_valAnimalExtraValues = nullptr; // [ENI] CheatType::ANIMAL_EXTRA_VALUES
bool JNIHelper::g_animalCheatActive = false;         // [ENI] Animal Cheat
bool JNIHelper::g_combatSupremacyActive = false;     // [ENI] Combat Supremacy
// [ENI] Combat Supremacy — ELIMINAR fieldIDs de IGC (ahora usamos setters)
// g_fidIgnoreStaggerBack, g_fidRecoilDelayIGC, g_fidStaggerTimeMod -> reemplazados por methodIDs
// [ENI] Vehicle Tools — BaseVehicle
jclass    JNIHelper::g_cachedBaseVehicleClass   = nullptr;
jfieldID  JNIHelper::g_fidVehicleHotwired       = nullptr;
jfieldID  JNIHelper::g_fidVehicleAlarmed        = nullptr;
jfieldID  JNIHelper::g_fidVehicleKeysInIgnition = nullptr;
jfieldID  JNIHelper::g_fidVehicleIgnitionSwitch = nullptr;
jmethodID JNIHelper::g_midIsSeatedInVehicle     = nullptr;
jmethodID JNIHelper::g_midGetVehicleFromPlayer  = nullptr;
jmethodID JNIHelper::g_midSetHotwired           = nullptr;
jmethodID JNIHelper::g_midSetAlarmed            = nullptr;
jmethodID JNIHelper::g_midSetKeysInIgnition     = nullptr;
jmethodID JNIHelper::g_midCreateVehicleKey      = nullptr;
jmethodID JNIHelper::g_midIsHotwired            = nullptr;
jmethodID JNIHelper::g_midGetScriptName         = nullptr;
jmethodID JNIHelper::g_midItemContainerAddItem  = nullptr;
jmethodID JNIHelper::g_midCheatHotwire          = nullptr;
// [ENI] Nuevas features server-safe
bool      JNIHelper::g_knowAllRecipesActive     = false;
bool      JNIHelper::g_nightVisionActive        = false;
bool      JNIHelper::g_lightfootActive          = false;
bool      JNIHelper::g_lootEspActive            = false;
bool      JNIHelper::g_lootFilterWeapon         = true;
bool      JNIHelper::g_lootFilterAmmo           = true;
bool      JNIHelper::g_lootFilterMedical        = true;
bool      JNIHelper::g_lootFilterFood           = true;
bool      JNIHelper::g_lootFilterMods           = false;
char      JNIHelper::g_lootFilterCustom[128]    = "";
std::vector<JNIHelper::LootInfo> JNIHelper::g_lootCache;

bool      JNIHelper::g_claimBypassActive        = false;
jmethodID JNIHelper::g_midSetNightVision        = nullptr; // IsoPlayer.setWearingNightVisionGoggles
jfieldID  JNIHelper::g_fidWornItemsHearing      = nullptr; // IGC.wornItemsHearingModifier
jobject   JNIHelper::g_valKnowAllRecipes        = nullptr; // CheatType::KNOW_ALL_RECIPES
jfieldID  JNIHelper::g_fidIsoObjectContainer    = nullptr; // IsoObject.container
jmethodID JNIHelper::g_midItemContainerGetItems = nullptr; // ItemContainer.getItems()
jmethodID JNIHelper::g_midItemGetType           = nullptr; // InventoryItem.getType()
jmethodID JNIHelper::g_midItemGetName           = nullptr; // InventoryItem.getName()
jmethodID JNIHelper::g_midItemGetCategory       = nullptr; // InventoryItem.getCategory()
jmethodID JNIHelper::g_midGridSquareGetObjects  = nullptr; // IsoGridSquare.getObjects()
jmethodID JNIHelper::g_midGridSquareGetStaticMovingObjects = nullptr; // IsoGridSquare.getStaticMovingObjects()

jmethodID JNIHelper::g_midSetAdd = nullptr;
jmethodID JNIHelper::g_midSetRemove = nullptr;
jmethodID JNIHelper::g_midSetIntOptionValue = nullptr;
jmethodID JNIHelper::g_midInitSandboxVars = nullptr;
jmethodID JNIHelper::g_midRunLua = nullptr;

// Zombie Kills & Survived Time method IDs
jmethodID JNIHelper::g_midGetZombieKills = nullptr;
jmethodID JNIHelper::g_midSetZombieKills = nullptr;
jmethodID JNIHelper::g_midGetLastZombieKills = nullptr;
jmethodID JNIHelper::g_midSetLastZombieKills = nullptr;
jmethodID JNIHelper::g_midGetHoursSurvived = nullptr;
jmethodID JNIHelper::g_midSetHoursSurvived = nullptr;
jmethodID JNIHelper::g_midSavePlayer = nullptr;

// Nutrition JNI cached variables
jmethodID JNIHelper::g_midGetNutrition = nullptr;
jmethodID JNIHelper::g_midGetWeight = nullptr;
jmethodID JNIHelper::g_midSetWeight = nullptr;
jmethodID JNIHelper::g_midGetCalories = nullptr;
jmethodID JNIHelper::g_midSetCalories = nullptr;
jmethodID JNIHelper::g_midGetProteins = nullptr;
jmethodID JNIHelper::g_midSetProteins = nullptr;
jmethodID JNIHelper::g_midGetCarbohydrates = nullptr;
jmethodID JNIHelper::g_midSetCarbohydrates = nullptr;
jmethodID JNIHelper::g_midGetLipids = nullptr;
jmethodID JNIHelper::g_midSetLipids = nullptr;

// [ENI] ESP - Zombie Scanner & Core
jclass JNIHelper::g_cachedIsoWorldClass = nullptr;
jclass JNIHelper::g_cachedIsoCellClass = nullptr;
jclass JNIHelper::g_cachedIsoZombieClass = nullptr;
jclass JNIHelper::g_cachedIsoDeadBodyClass = nullptr;
jclass JNIHelper::g_cachedInventoryItemFactoryClass = nullptr;
jclass JNIHelper::g_cachedIsoVehicleClass = nullptr;
jclass JNIHelper::g_cachedCoreClass = nullptr;
jclass JNIHelper::g_cachedIsoCameraClass = nullptr;
jclass JNIHelper::g_cachedGameClientClass = nullptr;
jclass JNIHelper::g_cachedSetClass = nullptr;

jfieldID JNIHelper::g_fidWorldInstance = nullptr;
jfieldID JNIHelper::g_fidCurrentCell = nullptr;
jfieldID JNIHelper::g_fidZombieList = nullptr;
jfieldID JNIHelper::g_fidRemoteSurvivorList = nullptr; // [ENI] Player List
jfieldID JNIHelper::g_fidVehicleList = nullptr;
jfieldID JNIHelper::g_fidGameClientInstance = nullptr;

jmethodID JNIHelper::g_midGetRemoteSurvivorList = nullptr;
jmethodID JNIHelper::g_midGetGameClientPlayers = nullptr;
jmethodID JNIHelper::g_midGetVehicles = nullptr;
jmethodID JNIHelper::g_midGetAnimals = nullptr;
jmethodID JNIHelper::g_midSetToArray = nullptr;
jfieldID JNIHelper::g_fidX = nullptr;
jfieldID JNIHelper::g_fidY = nullptr;
jfieldID JNIHelper::g_fidZ = nullptr;
jmethodID JNIHelper::g_midGetX = nullptr;
jmethodID JNIHelper::g_midGetY = nullptr;
jmethodID JNIHelper::g_midGetZ = nullptr;

// [B42] Stats API via CharacterStat enum
jmethodID JNIHelper::g_midStatsGet = nullptr;
jmethodID JNIHelper::g_midStatsSet = nullptr;
jobject JNIHelper::g_valStatHunger = nullptr;
jobject JNIHelper::g_valStatThirst = nullptr;
jobject JNIHelper::g_valStatFatigue = nullptr;
jobject JNIHelper::g_valStatPain = nullptr;
jobject JNIHelper::g_valStatTemperature = nullptr;
jobject JNIHelper::g_valStatEndurance = nullptr;
jobject JNIHelper::g_valStatPanic = nullptr;

// [ENI] Player Info ESP Method IDs
jmethodID JNIHelper::g_midGetUsername = nullptr;
jmethodID JNIHelper::g_midGetHealth = nullptr;
jmethodID JNIHelper::g_midGetPrimaryHandItem = nullptr;

jmethodID JNIHelper::g_midCoreInstance = nullptr;
jmethodID JNIHelper::g_midGetOffX = nullptr;
jmethodID JNIHelper::g_midGetOffY = nullptr;
jmethodID JNIHelper::g_midGetZoom = nullptr;
jmethodID JNIHelper::g_midIsDead = nullptr;
jmethodID JNIHelper::g_midIsoCameraGetOffX = nullptr;
jmethodID JNIHelper::g_midIsoCameraGetOffY = nullptr;
jfieldID  JNIHelper::g_fidZombieSpeedType  = nullptr; // [ENI] IsoZombie.speedType
jfieldID  JNIHelper::g_fidZombieCrawling   = nullptr; // [ENI] IsoZombie.crawling

// =========================================================================================
// JNI ENVIRONMENT & INITIALIZATION
// =========================================================================================

bool JNIHelper::Initialize() {
  // Try to get existing JVM - cross-platform via dlsym/GetProcAddress
  typedef jint(JNICALL * JNI_GetCreatedJavaVMs_t)(JavaVM **, jsize, jsize *);
#ifdef _WIN32
  HMODULE jvmDll = GetModuleHandleA("jvm.dll");
  if (!jvmDll)
    return false;
  JNI_GetCreatedJavaVMs_t jniGetCreatedJavaVMs =
      (JNI_GetCreatedJavaVMs_t)GetProcAddress(jvmDll, "JNI_GetCreatedJavaVMs");
#else
  // On Linux, libjvm.so is already loaded by the game - search the global
  // symbol table
  JNI_GetCreatedJavaVMs_t jniGetCreatedJavaVMs =
      (JNI_GetCreatedJavaVMs_t)dlsym(RTLD_DEFAULT, "JNI_GetCreatedJavaVMs");
#endif
  if (!jniGetCreatedJavaVMs)
    return false;

  jsize vmCount = 0;
  jniGetCreatedJavaVMs(nullptr, 0, &vmCount);
  if (vmCount == 0)
    return false;

  JavaVM *javaVm = nullptr;
  jniGetCreatedJavaVMs(&javaVm, 1, nullptr);
  if (!javaVm)
    return false;

  g_javaVm = javaVm;

  JNIEnv *env = GetEnv();
  if (env) {
    JNI_LOG("[+] JNI initialized successfully");
    CacheClasses();
    return true;
  }
  return false;
}

JNIEnv *JNIHelper::GetEnv() {
  if (!g_javaVm)
    return g_jniEnv;

  JNIEnv *env = nullptr;
  jint res = g_javaVm->GetEnv((void **)&env, JNI_VERSION_1_6);

  if (res == JNI_EDETACHED) {
    if (g_javaVm->AttachCurrentThread((void **)&env, nullptr) != JNI_OK) {
      return nullptr;
    }
  }

  return env;
}

// =========================================================================================
// Generic JNI Collection Helpers (Type-Safe for ArrayList, PZArrayList, Set)
// =========================================================================================

jint JNIHelper::GetListSize(JNIEnv *env, jobject listObj) {
  if (!env || !listObj)
    return 0;
  jclass cls = env->GetObjectClass(listObj);
  if (!cls) {
    env->ExceptionClear();
    return 0;
  }
  jmethodID mid = env->GetMethodID(cls, "size", "()I");
  if (!mid) {
    env->ExceptionClear();
    env->DeleteLocalRef(cls);
    return 0;
  }
  jint sz = env->CallIntMethod(listObj, mid);
  if (env->ExceptionCheck())
    env->ExceptionClear();
  env->DeleteLocalRef(cls);
  return sz;
}

jobject JNIHelper::GetListItem(JNIEnv *env, jobject listObj, jint index) {
  if (!env || !listObj)
    return nullptr;
  jclass cls = env->GetObjectClass(listObj);
  if (!cls) {
    env->ExceptionClear();
    return nullptr;
  }
  jmethodID mid = env->GetMethodID(cls, "get", "(I)Ljava/lang/Object;");
  if (!mid) {
    env->ExceptionClear();
    env->DeleteLocalRef(cls);
    return nullptr;
  }
  jobject item = env->CallObjectMethod(listObj, mid, index);
  if (env->ExceptionCheck())
    env->ExceptionClear();
  env->DeleteLocalRef(cls);
  return item;
}

jobjectArray JNIHelper::CollectionToArray(JNIEnv *env, jobject collObj) {
  if (!env || !collObj)
    return nullptr;
  jclass cls = env->GetObjectClass(collObj);
  if (!cls) {
    env->ExceptionClear();
    return nullptr;
  }
  jmethodID mid = env->GetMethodID(cls, "toArray", "()[Ljava/lang/Object;");
  if (!mid) {
    env->ExceptionClear();
    env->DeleteLocalRef(cls);
    return nullptr;
  }
  jobjectArray arr = (jobjectArray)env->CallObjectMethod(collObj, mid);
  if (env->ExceptionCheck())
    env->ExceptionClear();
  env->DeleteLocalRef(cls);
  return arr;
}

// =========================================================================================
// Class Loading Helpers
// =========================================================================================

jclass JNIHelper::FindClassEx(const char *className) {
  JNIEnv *env = GetEnv();
  if (!env)
    return nullptr;

  jclass cls = env->FindClass(className);
  if (cls)
    return cls;

  env->ExceptionClear();
  return nullptr;
}

bool JNIHelper::CacheClasses() {
  JNIEnv *env = GetEnv();
  if (!env)
    return false;

  auto SafeCacheClass = [&](jclass &globalRef, const char *name) {
    if (globalRef)
      return;
    jclass local = env->FindClass(name);
    if (local) {
      globalRef = (jclass)env->NewGlobalRef(local);
      env->DeleteLocalRef(local);
    } else {
      env->ExceptionClear();
      JNI_LOG("[ERROR] Could not find class: %s", name);
    }
  };

  SafeCacheClass(g_cachedIsoPlayerClass, "zombie/characters/IsoPlayer");
  SafeCacheClass(g_cachedIsoGameCharacterClass,
                 "zombie/characters/IsoGameCharacter");
  SafeCacheClass(g_cachedStatsClass, "zombie/characters/Stats");
  SafeCacheClass(g_cachedScriptManagerClass, "zombie/scripting/ScriptManager");
  SafeCacheClass(g_cachedItemClass, "zombie/scripting/objects/Item");
  SafeCacheClass(g_cachedArrayListClass, "java/util/ArrayList");
  SafeCacheClass(g_cachedSandboxOptionsClass, "zombie/SandboxOptions");
  SafeCacheClass(g_cachedIntegerConfigOptionClass,
                 "zombie/config/IntegerConfigOption");
  SafeCacheClass(g_cachedLuaManagerClass, "zombie/Lua/LuaManager");

  // [ENI] ESP Classes
  SafeCacheClass(g_cachedIsoWorldClass, "zombie/iso/IsoWorld");
  SafeCacheClass(g_cachedIsoCellClass, "zombie/iso/IsoCell");
  SafeCacheClass(g_cachedIsoZombieClass, "zombie/characters/IsoZombie");
  SafeCacheClass(g_cachedIsoDeadBodyClass, "zombie/iso/objects/IsoDeadBody");
  SafeCacheClass(g_cachedInventoryItemFactoryClass, "zombie/inventory/InventoryItemFactory");
  SafeCacheClass(g_cachedIsoPlayerClass, "zombie/characters/IsoPlayer");
  SafeCacheClass(g_cachedIsoVehicleClass, "zombie/vehicles/IsoVehicle");
  SafeCacheClass(g_cachedNutritionClass, "zombie/characters/BodyDamage/Nutrition");
  SafeCacheClass(g_cachedCoreClass, "zombie/core/Core");
  SafeCacheClass(g_cachedIsoCameraClass, "zombie/iso/IsoCamera");
  SafeCacheClass(g_cachedGameClientClass, "zombie/network/GameClient");
  SafeCacheClass(g_cachedSetClass, "java/util/Set");

  env->ExceptionClear();
  CacheCheatVars();

  JNI_LOG("[CacheClasses] Completed");
  return true;
}

bool JNIHelper::CacheCheatVars() {
  JNIEnv *env = GetEnv();
  if (!env)
    return false;

  JNI_LOG("[ENI] Caching Cheat Variables...");

  // 1. Field IDs for IsoGameCharacter
  jclass igc = g_cachedIsoGameCharacterClass;
  if (igc) {
    g_fidInvincible = env->GetFieldID(igc, "invincible", "Z");
    if (!g_fidInvincible)
      env->ExceptionClear();

    g_fidAvoidDamage = env->GetFieldID(igc, "avoidDamage", "Z");
    if (!g_fidAvoidDamage)
      env->ExceptionClear();

    g_fidGhostMode = env->GetFieldID(igc, "ghostMode", "Z");
    if (!g_fidGhostMode)
      env->ExceptionClear();

    // [RDH FIX] En B42: cheats is a PlayerCheats field; search full hierarchy
    g_fidCheats =
        env->GetFieldID(igc, "cheats", "Lzombie/characters/PlayerCheats;");
    if (!g_fidCheats)
      env->ExceptionClear();
    // Try IsoSurvivor (sits between IsoPlayer and IsoGameCharacter)
    if (!g_fidCheats) {
      jclass isoSurvivorCls = env->FindClass("zombie/characters/IsoSurvivor");
      if (isoSurvivorCls) {
        g_fidCheats = env->GetFieldID(isoSurvivorCls, "cheats",
                                      "Lzombie/characters/PlayerCheats;");
        if (!g_fidCheats)
          env->ExceptionClear();
        env->DeleteLocalRef(isoSurvivorCls);
      } else
        env->ExceptionClear();
    }
    // Try IsoPlayer directly (most derived — lastCheatEnable/Toggle live there)
    if (!g_fidCheats && g_cachedIsoPlayerClass) {
      g_fidCheats = env->GetFieldID(g_cachedIsoPlayerClass, "cheats",
                                    "Lzombie/characters/PlayerCheats;");
      if (!g_fidCheats)
        env->ExceptionClear();
    }
    // B41 fallback: raw EnumSet
    if (!g_fidCheats) {
      g_fidCheats = env->GetFieldID(igc, "cheats", "Ljava/util/EnumSet;");
      if (!g_fidCheats)
        env->ExceptionClear();
    }

    g_fidBodyDamage = env->GetFieldID(
        igc, "bodyDamage", "Lzombie/characters/BodyDamage/BodyDamage;");
    if (!g_fidBodyDamage)
      env->ExceptionClear();

    g_fidXP =
        env->GetFieldID(igc, "xp", "Lzombie/characters/IsoGameCharacter$XP;");
    if (!g_fidXP)
      env->ExceptionClear();

    g_fidStats = env->GetFieldID(igc, "stats", "Lzombie/characters/Stats;");
    if (!g_fidStats)
      env->ExceptionClear();

    g_midGetStats =
        env->GetMethodID(igc, "getStats", "()Lzombie/characters/Stats;");
    if (!g_midGetStats)
      env->ExceptionClear();

    g_midGetCurrentSquare = env->GetMethodID(igc, "getCurrentSquare",
                                             "()Lzombie/iso/IsoGridSquare;");
    if (!g_midGetCurrentSquare)
      env->ExceptionClear();

    // Zombie Kills & Survived Time JNI Caching
    g_midGetZombieKills = env->GetMethodID(igc, "getZombieKills", "()I");
    if (!g_midGetZombieKills) env->ExceptionClear();
    g_midSetZombieKills = env->GetMethodID(igc, "setZombieKills", "(I)V");
    if (!g_midSetZombieKills) env->ExceptionClear();
    g_midGetLastZombieKills = env->GetMethodID(igc, "getLastZombieKills", "()I");
    if (!g_midGetLastZombieKills) env->ExceptionClear();
    g_midSetLastZombieKills = env->GetMethodID(igc, "setLastZombieKills", "(I)V");
    if (!g_midSetLastZombieKills) env->ExceptionClear();
    g_midGetHoursSurvived = env->GetMethodID(igc, "getHoursSurvived", "()D");
    if (!g_midGetHoursSurvived) env->ExceptionClear();
    if (g_cachedIsoPlayerClass) {
      g_midSetHoursSurvived = env->GetMethodID(g_cachedIsoPlayerClass, "setHoursSurvived", "(D)V");
      if (!g_midSetHoursSurvived) env->ExceptionClear();
      g_midSavePlayer = env->GetMethodID(g_cachedIsoPlayerClass, "save", "()V");
      if (!g_midSavePlayer) env->ExceptionClear();
    }
  }

  // 3. SandboxOptions caching
  if (g_cachedSandboxOptionsClass) {
    g_fidSandboxInstance = env->GetStaticFieldID(
        g_cachedSandboxOptionsClass, "instance", "Lzombie/SandboxOptions;");
    if (!g_fidSandboxInstance) env->ExceptionClear();
    g_fidCharacterFreePoints =
        env->GetFieldID(g_cachedSandboxOptionsClass, "characterFreePoints",
                        "Lzombie/SandboxOptions$IntegerSandboxOption;");
    if (!g_fidCharacterFreePoints) env->ExceptionClear();
    g_midInitSandboxVars =
        env->GetMethodID(g_cachedSandboxOptionsClass, "initSandboxVars", "()V");
    if (!g_midInitSandboxVars) env->ExceptionClear();
  }

  // [ENI] Optimized Caching
  auto CacheClassGlobal = [&](jclass &globalRef, const char *name) {
    if (globalRef)
      return;
    jclass local = env->FindClass(name);
    if (local) {
      globalRef = (jclass)env->NewGlobalRef(local);
      env->DeleteLocalRef(local);
    } else
      env->ExceptionClear();
  };

  CacheClassGlobal(g_cachedIntegerConfigOptionClass,
                   "zombie/config/IntegerConfigOption");
  if (g_cachedIntegerConfigOptionClass) {
    g_midSetIntOptionValue =
        env->GetMethodID(g_cachedIntegerConfigOptionClass, "setValue", "(I)V");
    if (!g_midSetIntOptionValue) env->ExceptionClear();
    g_fidIntOptionValue =
        env->GetFieldID(g_cachedIntegerConfigOptionClass, "value", "I");
    if (!g_fidIntOptionValue) env->ExceptionClear();
  }

  if (g_cachedLuaManagerClass) {
    g_midRunLua =
        env->GetStaticMethodID(g_cachedLuaManagerClass, "RunLua",
                               "(Ljava/lang/String;)Ljava/lang/Object;");
    if (!g_midRunLua) env->ExceptionClear();
    g_fidLuaEnv = env->GetStaticFieldID(g_cachedLuaManagerClass, "env",
                                        "Lse/krka/kahlua/vm/KahluaTable;");
    if (!g_fidLuaEnv) env->ExceptionClear();
  }

  // [ENI] ESP Field/Method IDs
  if (g_cachedIsoWorldClass) {
    g_fidWorldInstance = env->GetStaticFieldID(
        g_cachedIsoWorldClass, "instance", "Lzombie/iso/IsoWorld;");
    if (!g_fidWorldInstance) {
      env->ExceptionClear();
      g_fidWorldInstance = env->GetStaticFieldID(
          g_cachedIsoWorldClass, "Instance", "Lzombie/iso/IsoWorld;");
    }

    g_fidCurrentCell = env->GetFieldID(g_cachedIsoWorldClass, "CurrentCell",
                                       "Lzombie/iso/IsoCell;");
    if (!g_fidCurrentCell) {
      env->ExceptionClear();
      g_fidCurrentCell = env->GetFieldID(g_cachedIsoWorldClass, "currentCell",
                                         "Lzombie/iso/IsoCell;");
    }
  }

  if (g_cachedIsoCellClass) {
    // Try 'ZombieList' (Capitalized is common in PZ)
    g_fidZombieList = env->GetFieldID(g_cachedIsoCellClass, "ZombieList",
                                      "Ljava/util/ArrayList;");
    if (!g_fidZombieList) {
      env->ExceptionClear();
      g_fidZombieList = env->GetFieldID(g_cachedIsoCellClass, "zombieList",
                                        "Ljava/util/ArrayList;");
    }

    // [ENI] Player List (PVP/Coop) - B42 uses getter method
    // Old way (doesn't work):
    // g_fidRemoteSurvivorList = env->GetFieldID(g_cachedIsoCellClass,
    // "remoteSurvivorList", "Ljava/util/ArrayList;"); New way (correct for
    // B42):
    g_midGetRemoteSurvivorList =
        env->GetMethodID(g_cachedIsoCellClass, "getRemoteSurvivorList",
                         "()Ljava/util/ArrayList;");
    if (!g_midGetRemoteSurvivorList) {
      env->ExceptionClear();
      JNI_LOG("[ESP] ERROR: Could not find getRemoteSurvivorList() method in "
              "IsoCell");
    } else {
      JNI_LOG("[ESP] ✓ Found getRemoteSurvivorList() method in IsoCell");
    }

    // [ENI] Vehicle List - B42 changed to Set and method getVehicles
    g_midGetVehicles = env->GetMethodID(g_cachedIsoCellClass, "getVehicles",
                                        "()Ljava/util/Set;");
    if (!g_midGetVehicles) {
      env->ExceptionClear();
      g_fidVehicleList =
          env->GetFieldID(g_cachedIsoCellClass, "vehicles", "Ljava/util/Set;");
      if (!g_fidVehicleList) {
        env->ExceptionClear();
        g_fidVehicleList = env->GetFieldID(g_cachedIsoCellClass, "VehicleList",
                                           "Ljava/util/ArrayList;");
      }
    }

    // [ENI] Animal List
    g_midGetAnimals = env->GetMethodID(g_cachedIsoCellClass, "getAnimals",
                                       "()Ljava/util/List;");
    if (!g_midGetAnimals)
      env->ExceptionClear();
  }

  if (g_cachedGameClientClass) {
    g_fidGameClientInstance = env->GetStaticFieldID(
        g_cachedGameClientClass, "instance", "Lzombie/network/GameClient;");
    g_midGetGameClientPlayers = env->GetMethodID(
        g_cachedGameClientClass, "getPlayers", "()Ljava/util/ArrayList;");
  }

  if (g_cachedSetClass) {
    g_midSetToArray =
        env->GetMethodID(g_cachedSetClass, "toArray", "()[Ljava/lang/Object;");
  }

  if (g_cachedCoreClass) {
    g_midCoreInstance = env->GetStaticMethodID(g_cachedCoreClass, "getInstance",
                                               "()Lzombie/core/Core;");
    g_midGetOffX = env->GetMethodID(g_cachedCoreClass, "getOffX", "()F");
    g_midGetOffY = env->GetMethodID(g_cachedCoreClass, "getOffY", "()F");
    g_midGetZoom = env->GetMethodID(g_cachedCoreClass, "getZoom", "(I)F");
  }

  if (g_cachedIsoCameraClass) {
    g_midIsoCameraGetOffX =
        env->GetStaticMethodID(g_cachedIsoCameraClass, "getOffX", "()F");
    g_midIsoCameraGetOffY =
        env->GetStaticMethodID(g_cachedIsoCameraClass, "getOffY", "()F");
  }

  // Shared coords (IsoGameCharacter inherits IsoObject)
  if (igc) {
    g_fidX = env->GetFieldID(igc, "x", "F");
    g_fidY = env->GetFieldID(igc, "y", "F");
    g_fidZ = env->GetFieldID(igc, "z", "F");
    g_midIsDead = env->GetMethodID(igc, "isDead", "()Z");
  }

  // [ENI] Getters for coordinate security (replaces private field access errors on Linux/MP)
  jclass imoCls = env->FindClass("zombie/iso/IsoMovingObject");
  if (imoCls) {
    g_midGetX = env->GetMethodID(imoCls, "getX", "()F");
    g_midGetY = env->GetMethodID(imoCls, "getY", "()F");
    g_midGetZ = env->GetMethodID(imoCls, "getZ", "()F");
    env->DeleteLocalRef(imoCls);
    JNI_LOG("[COORDS CACHE] getX=%s getY=%s getZ=%s",
            g_midGetX ? "OK":"NULL", g_midGetY ? "OK":"NULL", g_midGetZ ? "OK":"NULL");
  } else {
    env->ExceptionClear();
    JNI_LOG("[COORDS CACHE] ERROR: IsoMovingObject not found");
  }

  if (g_cachedArrayListClass) {
    g_midArrayListGet = env->GetMethodID(g_cachedArrayListClass, "get",
                                         "(I)Ljava/lang/Object;");
    g_midArrayListSize =
        env->GetMethodID(g_cachedArrayListClass, "size", "()I");
  }

  CacheClassGlobal(g_cachedKahluaTableClass, "se/krka/kahlua/vm/KahluaTable");
  if (g_cachedKahluaTableClass) {
    g_midRawGet = env->GetMethodID(g_cachedKahluaTableClass, "rawget",
                                   "(Ljava/lang/Object;)Ljava/lang/Object;");
    g_midRawSet = env->GetMethodID(g_cachedKahluaTableClass, "rawset",
                                   "(Ljava/lang/Object;Ljava/lang/Object;)V");
  }

  CacheClassGlobal(g_cachedDoubleClass, "java/lang/Double");
  if (g_cachedDoubleClass) {
    g_midDoubleInit = env->GetMethodID(g_cachedDoubleClass, "<init>", "(D)V");
  }

  env->ExceptionClear();

  // [B42] Stats API: Stats.get(CharacterStat) / Stats.set(CharacterStat, float)
  jclass charStatCls = env->FindClass("zombie/characters/CharacterStat");
  if (charStatCls) {
    if (g_cachedStatsClass) {
      g_midStatsGet = env->GetMethodID(g_cachedStatsClass, "get",
                                       "(Lzombie/characters/CharacterStat;)F");
      if (!g_midStatsGet)
        env->ExceptionClear();
      g_midStatsSet = env->GetMethodID(g_cachedStatsClass, "set",
                                       "(Lzombie/characters/CharacterStat;F)Z");
      if (!g_midStatsSet)
        env->ExceptionClear();
    }
    const char *statSig = "Lzombie/characters/CharacterStat;";
    auto CacheStat = [&](const char *name, jobject &out) {
      jfieldID fid = env->GetStaticFieldID(charStatCls, name, statSig);
      if (fid) {
        jobject v = env->GetStaticObjectField(charStatCls, fid);
        if (v)
          out = env->NewGlobalRef(v);
      } else
        env->ExceptionClear();
    };
    CacheStat("HUNGER", g_valStatHunger);
    CacheStat("THIRST", g_valStatThirst);
    CacheStat("FATIGUE", g_valStatFatigue);
    CacheStat("PAIN", g_valStatPain);
    CacheStat("TEMPERATURE", g_valStatTemperature);
    CacheStat("ENDURANCE", g_valStatEndurance);
    CacheStat("PANIC", g_valStatPanic);
    env->DeleteLocalRef(charStatCls);
    JNI_LOG("[STATS-CACHE] get=%s set=%s HUN=%s THI=%s FAT=%s PAIN=%s TEMP=%s "
            "END=%s",
            g_midStatsGet ? "OK" : "NULL", g_midStatsSet ? "OK" : "NULL",
            g_valStatHunger ? "OK" : "NULL", g_valStatThirst ? "OK" : "NULL",
            g_valStatFatigue ? "OK" : "NULL", g_valStatPain ? "OK" : "NULL",
            g_valStatTemperature ? "OK" : "NULL",
            g_valStatEndurance ? "OK" : "NULL");
  } else {
    env->ExceptionClear();
    JNI_LOG("[STATS-CACHE] ERROR: CharacterStat class not found");
  }

  JNI_LOG("[CacheCheatVars] Moodle/Stats caching completed");

  // [ENI] Player Info ESP - Cache methods for extended player info
  jclass playerEspCls = g_cachedIsoPlayerClass;
  if (playerEspCls) {
    JNI_LOG("[PLAYER_ESP] Caching player info methods...");

    g_midGetUsername =
        env->GetMethodID(playerEspCls, "getUsername", "()Ljava/lang/String;");
    if (g_midGetUsername) {
      JNI_LOG("[PLAYER_ESP] SUCCESS: Found 'getUsername()' method");
    } else {
      env->ExceptionClear();
      g_midGetUsername = env->GetMethodID(playerEspCls, "getDisplayName",
                                          "()Ljava/lang/String;");
      JNI_LOG("[PLAYER_ESP] Using 'getDisplayName()' as username fallback: %s",
              g_midGetUsername ? "OK" : "FAIL");
      if (!g_midGetUsername)
        env->ExceptionClear();
    }

    g_midGetHealth = env->GetMethodID(playerEspCls, "getHealth", "()F");
    if (!g_midGetHealth) {
      env->ExceptionClear();
      JNI_LOG("[PLAYER_ESP] FAIL: 'getHealth()' not found");
    } else
      JNI_LOG("[PLAYER_ESP] SUCCESS: Found 'getHealth()'");

    // getPrimaryHandItem is on IsoGameCharacter, not IsoPlayer
    jclass igcEsp = g_cachedIsoGameCharacterClass;
    if (igcEsp) {
      g_midGetPrimaryHandItem = env->GetMethodID(
          igcEsp, "getPrimaryHandItem", "()Lzombie/inventory/InventoryItem;");
      if (!g_midGetPrimaryHandItem) {
        env->ExceptionClear();
        JNI_LOG("[PLAYER_ESP] FAIL: 'getPrimaryHandItem()' not found");
      } else
        JNI_LOG("[PLAYER_ESP] SUCCESS: Found 'getPrimaryHandItem()'");
    }
  }

  env->ExceptionClear();

  JNI_LOG("[CacheCheatVars] Player Info ESP caching completed");

  // 3. ScriptManager Methods
  jclass smCls = g_cachedScriptManagerClass;
  if (smCls) {
    g_midGetAllItems =
        env->GetMethodID(smCls, "getAllItems", "()Ljava/util/ArrayList;");
    if (!g_midGetAllItems)
      env->ExceptionClear();
  }

  // 4. Item Methods
  jclass itemCls = g_cachedItemClass;
  if (itemCls) {
    g_midGetFullName =
        env->GetMethodID(itemCls, "getFullName", "()Ljava/lang/String;");
    if (!g_midGetFullName)
      env->ExceptionClear();

    g_midGetDisplayName =
        env->GetMethodID(itemCls, "getDisplayName", "()Ljava/lang/String;");
    if (!g_midGetDisplayName)
      env->ExceptionClear();
  }

  // [Restored from Backup]
  // 3. Item Factory & Square
  jclass clsFactory = env->FindClass("zombie/inventory/InventoryItemFactory");
  if (clsFactory) {
    g_midCreateItem = env->GetStaticMethodID(
        clsFactory, "CreateItem",
        "(Ljava/lang/String;)Lzombie/inventory/InventoryItem;");
    env->DeleteLocalRef(clsFactory);
  } else
    env->ExceptionClear();

  jclass clsSquare = env->FindClass("zombie/iso/IsoGridSquare");
  if (clsSquare) {
    g_midAddWorldInventoryItem =
        env->GetMethodID(clsSquare, "AddWorldInventoryItem",
                         "(Lzombie/inventory/InventoryItem;FFF)Lzombie/"
                         "inventory/InventoryItem;");
    if (!g_midAddWorldInventoryItem) {
      env->ExceptionClear();
      g_midAddWorldInventoryItem =
          env->GetMethodID(clsSquare, "AddWorldInventoryItem",
                           "(Lzombie/inventory/InventoryItem;FFF)V");
    }
    env->DeleteLocalRef(clsSquare);
  } else
    env->ExceptionClear();

  jclass clsItem = env->FindClass("zombie/inventory/InventoryItem");
  if (clsItem) {
    g_midTransmitCompleteItemToServer =
        env->GetMethodID(clsItem, "transmitCompleteItemToServer", "()V");
    env->DeleteLocalRef(clsItem);
  } else
    env->ExceptionClear();

  // 4. Enum Set & Values
  const char *enumCandidates[] = {"zombie/characters/PlayerCheats$Type",
                                  "zombie/characters/CheatType"};

  jclass enumClass = nullptr;
  const char *foundEnumName = nullptr;

  for (const char *candidate : enumCandidates) {
    jclass c = env->FindClass(candidate);
    if (c) {
      enumClass = (jclass)env->NewGlobalRef(c);
      foundEnumName = candidate;
      JNI_LOG("[ENI] Found Cheat Enum: %s", candidate);
      break;
    }
    env->ExceptionClear();
  }

  if (enumClass) {
    std::string sig = "L" + std::string(foundEnumName) + ";";

    // Use AbstractCollection (concrete class), NOT Set (interface).
    // GetMethodID on an interface cannot be used with CallBooleanMethod —
    // vtable vs itable.
    jclass setClass = env->FindClass("java/util/AbstractCollection");
    if (setClass) {
      g_midSetAdd = env->GetMethodID(setClass, "add", "(Ljava/lang/Object;)Z");
      g_midSetRemove =
          env->GetMethodID(setClass, "remove", "(Ljava/lang/Object;)Z");
      if (!g_midSetAdd)
        env->ExceptionClear();
      if (!g_midSetRemove)
        env->ExceptionClear();

      auto CacheVal = [&](const char *name, jobject &outVal) {
        jfieldID fid = env->GetStaticFieldID(enumClass, name, sig.c_str());
        if (fid) {
          jobject val = env->GetStaticObjectField(enumClass, fid);
          if (val)
            outVal = env->NewGlobalRef(val);
        } else {
          env->ExceptionClear();
        }
      };

      CacheVal("GOD_MODE",            g_valGodMode);
      CacheVal("INVISIBLE",           g_valInvisible);
      CacheVal("ZOMBIES_DONT_ATTACK", g_valZombiesDontAttack);
      CacheVal("GHOST_MODE",          g_valGhostMode);
      CacheVal("NO_CLIP",             g_valNoClip);
      CacheVal("UNLIMITED_AMMO",      g_valUnlimitedAmmo);
      CacheVal("UNLIMITED_CARRY",     g_valUnlimitedCarry);
      CacheVal("ANIMAL",              g_valAnimal);            // [ENI] Animal Cheat
      CacheVal("ANIMAL_EXTRA_VALUES", g_valAnimalExtraValues); // [ENI] Animal Cheat extras
      CacheVal("KNOW_ALL_RECIPES",    g_valKnowAllRecipes);    // [ENI] Know All Recipes

      env->DeleteLocalRef(setClass);
    }

    jclass pcClass = env->FindClass("zombie/characters/PlayerCheats");
    if (pcClass) {
      g_fidEnumSet = env->GetFieldID(pcClass, "cheats", "Ljava/util/EnumSet;");
      g_midPlayerCheatsSet =
          env->GetMethodID(pcClass, "set", "(Lzombie/characters/CheatType;Z)V");
      g_midPlayerCheatsIsSet = env->GetMethodID(
          pcClass, "isSet", "(Lzombie/characters/CheatType;)Z");
      if (!g_midPlayerCheatsIsSet)
        env->ExceptionClear();
      // JNI bypasses Java access control — cache private set/unset directly
      g_midPlayerCheatsSetPrivate =
          env->GetMethodID(pcClass, "set", "(Lzombie/characters/CheatType;)V");
      g_midPlayerCheatsUnsetPrivate = env->GetMethodID(
          pcClass, "unset", "(Lzombie/characters/CheatType;)V");
      if (!g_midPlayerCheatsSetPrivate)
        env->ExceptionClear();
      if (!g_midPlayerCheatsUnsetPrivate)
        env->ExceptionClear();
      JNI_LOG("[CHEAT-CACHE] private set=%s  private unset=%s",
              g_midPlayerCheatsSetPrivate ? "OK" : "NULL",
              g_midPlayerCheatsUnsetPrivate ? "OK" : "NULL");
      env->DeleteLocalRef(pcClass);
    }
    // Unlimited ammo per-frame reset
    if (igc) {
      g_fidRightHandItem = env->GetFieldID(igc, "rightHandItem",
                                           "Lzombie/inventory/InventoryItem;");
      if (!g_fidRightHandItem)
        env->ExceptionClear();
    }
    jclass itemCls = env->FindClass("zombie/inventory/InventoryItem");
    if (itemCls) {
      g_midGetMaxAmmo = env->GetMethodID(itemCls, "getMaxAmmo", "()I");
      g_midSetCurrentAmmo =
          env->GetMethodID(itemCls, "setCurrentAmmoCount", "(I)V");
      if (!g_midGetMaxAmmo)
        env->ExceptionClear();
      if (!g_midSetCurrentAmmo)
        env->ExceptionClear();
      env->DeleteLocalRef(itemCls);
    }

    // [ENI] Soft Aim — HandWeapon field cache
    // JNI bypasses Java access control: los campos private son accesibles.
    // Nunca llamar FindClass/GetFieldID dentro de un tick (R-LOWSPEC).
    jclass hwCls = env->FindClass("zombie/inventory/types/HandWeapon");
    if (hwCls) {
      g_cachedHandWeaponClass = (jclass)env->NewGlobalRef(hwCls);
      env->DeleteLocalRef(hwCls);

      g_fidHWHitChance = env->GetFieldID(g_cachedHandWeaponClass, "hitChance", "I");
      if (!g_fidHWHitChance) env->ExceptionClear();

      g_fidHWMinAngle = env->GetFieldID(g_cachedHandWeaponClass, "minAngle", "F");
      if (!g_fidHWMinAngle) env->ExceptionClear();

      g_fidHWMaxAngle = env->GetFieldID(g_cachedHandWeaponClass, "maxAngle", "F");
      if (!g_fidHWMaxAngle) env->ExceptionClear();

      g_fidHWProjectileSpread = env->GetFieldID(g_cachedHandWeaponClass, "projectileSpread", "F");
      if (!g_fidHWProjectileSpread) env->ExceptionClear();

      g_fidHWJamGunChance = env->GetFieldID(g_cachedHandWeaponClass, "jamGunChance", "F");
      if (!g_fidHWJamGunChance) env->ExceptionClear();

      g_midHWIsRanged = env->GetMethodID(g_cachedHandWeaponClass, "isRanged", "()Z");
      if (!g_midHWIsRanged) env->ExceptionClear();

      // [ENI] Zombie type detection (usa g_cachedIsoZombieClass que ya fue cacheado en SafeCacheClass)
      g_midIsDead = env->GetMethodID(g_cachedIsoZombieClass, "isDead", "()Z");
      if (!g_midIsDead) env->ExceptionClear();
      g_fidZombieSpeedType = env->GetFieldID(g_cachedIsoZombieClass, "speedType", "I");
      if (!g_fidZombieSpeedType) env->ExceptionClear();
      g_fidZombieCrawling  = env->GetFieldID(g_cachedIsoZombieClass, "crawling",  "Z");
      if (!g_fidZombieCrawling)  env->ExceptionClear();
      JNI_LOG("[ESP-CACHE] speedType=%s crawling=%s",
              g_fidZombieSpeedType ? "OK" : "NULL", g_fidZombieCrawling ? "OK" : "NULL");

      g_midHWIsAimedFirearm = env->GetMethodID(g_cachedHandWeaponClass, "isAimedFirearm", "()Z");
      if (!g_midHWIsAimedFirearm) env->ExceptionClear();

      // [ENI] Critical Override — campos de golpe crítico (headshot mecánico)
      g_fidHWCriticalChance = env->GetFieldID(g_cachedHandWeaponClass, "criticalChance", "F");
      if (!g_fidHWCriticalChance) env->ExceptionClear();

      g_fidHWCriticalDmgMult = env->GetFieldID(g_cachedHandWeaponClass, "criticalDamageMultiplier", "F");
      if (!g_fidHWCriticalDmgMult) env->ExceptionClear();

      g_fidHWAimingPerkCritMod = env->GetFieldID(g_cachedHandWeaponClass, "aimingPerkCritModifier", "I");
      if (!g_fidHWAimingPerkCritMod) env->ExceptionClear();

      // [ENI] Combat Supremacy — HandWeapon timing fields
      g_fidHWSwingTime    = env->GetFieldID(g_cachedHandWeaponClass, "swingTime",       "F");
      if (!g_fidHWSwingTime)    env->ExceptionClear();
      g_fidHWMinSwingTime = env->GetFieldID(g_cachedHandWeaponClass, "minimumSwingTime", "F");
      if (!g_fidHWMinSwingTime) env->ExceptionClear();
      g_fidHWRecoilDelayHW = env->GetFieldID(g_cachedHandWeaponClass, "recoilDelay",    "I");
      if (!g_fidHWRecoilDelayHW) env->ExceptionClear();

      JNI_LOG("[SOFT-AIM CACHE] hitChance=%s minAngle=%s maxAngle=%s spread=%s jam=%s isRanged=%s isAimedFirearm=%s",
              g_fidHWHitChance        ? "OK" : "NULL",
              g_fidHWMinAngle         ? "OK" : "NULL",
              g_fidHWMaxAngle         ? "OK" : "NULL",
              g_fidHWProjectileSpread ? "OK" : "NULL",
              g_fidHWJamGunChance     ? "OK" : "NULL",
              g_midHWIsRanged         ? "OK" : "NULL",
              g_midHWIsAimedFirearm   ? "OK" : "NULL");
    } else {
      env->ExceptionClear();
      JNI_LOG("[SOFT-AIM CACHE] ERROR: HandWeapon class not found");
    }

    // [ENI] Combat Supremacy — setters públicos de IsoGameCharacter
    // Usar CallVoidMethod en lugar de SetBooleanField para que el engine acepte el valor en su propio flujo
    if (igc) {
      g_midSetIgnoreStaggerBack = env->GetMethodID(igc, "setIgnoreStaggerBack", "(Z)V");
      if (!g_midSetIgnoreStaggerBack) env->ExceptionClear();
      g_midSetStaggerTimeMod    = env->GetMethodID(igc, "setStaggerTimeMod",    "(F)V");
      if (!g_midSetStaggerTimeMod)    env->ExceptionClear();
      g_midSetRecoilDelay       = env->GetMethodID(igc, "setRecoilDelay",       "(F)V");
      if (!g_midSetRecoilDelay)       env->ExceptionClear();
      // blurFactor no tiene setter público — usamos field directo
      g_fidBlurFactor       = env->GetFieldID(igc, "blurFactor",       "F");
      if (!g_fidBlurFactor)       env->ExceptionClear();
      g_fidBlurFactorTarget = env->GetFieldID(igc, "blurFactorTarget", "F");
      if (!g_fidBlurFactorTarget) env->ExceptionClear();
      // isSeatedInVehicle() y getVehicle() para Vehicle Tools
      g_midIsSeatedInVehicle    = env->GetMethodID(igc, "isSeatedInVehicle", "()Z");
      if (!g_midIsSeatedInVehicle)    env->ExceptionClear();
      g_midGetVehicleFromPlayer = env->GetMethodID(igc, "getVehicle", "()Lzombie/vehicles/BaseVehicle;");
      if (!g_midGetVehicleFromPlayer) env->ExceptionClear();
      JNI_LOG("[COMBAT-SUP CACHE] setIgnoreStagger=%s setStaggerMod=%s setRecoil=%s blur=%s blurTarget=%s",
              g_midSetIgnoreStaggerBack ? "OK":"NULL", g_midSetStaggerTimeMod ? "OK":"NULL",
              g_midSetRecoilDelay ? "OK":"NULL", g_fidBlurFactor ? "OK":"NULL",
              g_fidBlurFactorTarget ? "OK":"NULL");
    }

    // Nutrition JNI cached variables
    if (g_cachedIsoPlayerClass) {
      g_midGetNutrition = env->GetMethodID(g_cachedIsoPlayerClass, "getNutrition", "()Lzombie/characters/BodyDamage/Nutrition;");
      if (!g_midGetNutrition) env->ExceptionClear();
    }
    if (g_cachedNutritionClass) {
      g_midGetWeight = env->GetMethodID(g_cachedNutritionClass, "getWeight", "()F");
      if (!g_midGetWeight) env->ExceptionClear();
      g_midSetWeight = env->GetMethodID(g_cachedNutritionClass, "setWeight", "(F)V");
      if (!g_midSetWeight) env->ExceptionClear();

      g_midGetCalories = env->GetMethodID(g_cachedNutritionClass, "getCalories", "()F");
      if (!g_midGetCalories) env->ExceptionClear();
      g_midSetCalories = env->GetMethodID(g_cachedNutritionClass, "setCalories", "(F)V");
      if (!g_midSetCalories) env->ExceptionClear();

      g_midGetProteins = env->GetMethodID(g_cachedNutritionClass, "getProteins", "()F");
      if (!g_midGetProteins) env->ExceptionClear();
      g_midSetProteins = env->GetMethodID(g_cachedNutritionClass, "setProteins", "(F)V");
      if (!g_midSetProteins) env->ExceptionClear();

      g_midGetCarbohydrates = env->GetMethodID(g_cachedNutritionClass, "getCarbohydrates", "()F");
      if (!g_midGetCarbohydrates) env->ExceptionClear();
      g_midSetCarbohydrates = env->GetMethodID(g_cachedNutritionClass, "setCarbohydrates", "(F)V");
      if (!g_midSetCarbohydrates) env->ExceptionClear();

      g_midGetLipids = env->GetMethodID(g_cachedNutritionClass, "getLipids", "()F");
      if (!g_midGetLipids) env->ExceptionClear();
      g_midSetLipids = env->GetMethodID(g_cachedNutritionClass, "setLipids", "(F)V");
      if (!g_midSetLipids) env->ExceptionClear();
    }

    // [ENI] Vehicle Tools — BaseVehicle cache
    jclass bvClass = env->FindClass("zombie/vehicles/BaseVehicle");
    if (bvClass) {
      g_cachedBaseVehicleClass   = (jclass)env->NewGlobalRef(bvClass);
      g_fidVehicleHotwired       = env->GetFieldID(bvClass, "hotwired",       "Z");
      if (!g_fidVehicleHotwired)       env->ExceptionClear();
      g_fidVehicleAlarmed        = env->GetFieldID(bvClass, "alarmed",        "Z");
      if (!g_fidVehicleAlarmed)        env->ExceptionClear();
      g_fidVehicleKeysInIgnition = env->GetFieldID(bvClass, "keysInIgnition", "Z");
      if (!g_fidVehicleKeysInIgnition) env->ExceptionClear();
      g_fidVehicleIgnitionSwitch = env->GetFieldID(bvClass, "ignitionSwitch", "Lzombie/inventory/ItemContainer;");
      if (!g_fidVehicleIgnitionSwitch) env->ExceptionClear();
      g_midSetHotwired           = env->GetMethodID(bvClass, "setHotwired",        "(Z)V");
      if (!g_midSetHotwired)           env->ExceptionClear();
      g_midSetAlarmed            = env->GetMethodID(bvClass, "setAlarmed",          "(Z)V");
      if (!g_midSetAlarmed)            env->ExceptionClear();
      g_midSetKeysInIgnition     = env->GetMethodID(bvClass, "setKeysInIgnition",   "(Z)V");
      if (!g_midSetKeysInIgnition)     env->ExceptionClear();
      g_midCreateVehicleKey      = env->GetMethodID(bvClass, "createVehicleKey",    "()Lzombie/inventory/InventoryItem;");
      if (!g_midCreateVehicleKey)      env->ExceptionClear();
      g_midIsHotwired            = env->GetMethodID(bvClass, "isHotwired",          "()Z");
      if (!g_midIsHotwired)            env->ExceptionClear();
      g_midGetScriptName         = env->GetMethodID(bvClass, "getScriptName",       "()Ljava/lang/String;");
      if (!g_midGetScriptName)         env->ExceptionClear();
      g_midCheatHotwire          = env->GetMethodID(bvClass, "cheatHotwire",         "(ZZ)V");
      if (!g_midCheatHotwire)          env->ExceptionClear();
      env->DeleteLocalRef(bvClass);
      // ItemContainer.AddItem — necesario para poner la llave en ignitionSwitch
      jclass icClass = env->FindClass("zombie/inventory/ItemContainer");
      if (icClass) {
        g_midItemContainerAddItem = env->GetMethodID(icClass, "AddItem", "(Lzombie/inventory/InventoryItem;)Lzombie/inventory/InventoryItem;");
        if (!g_midItemContainerAddItem) env->ExceptionClear();
        env->DeleteLocalRef(icClass);
      }
      JNI_LOG("[VEHICLE-TOOLS CACHE] hotwiredFID=%s alarmedFID=%s ignitionFID=%s setHotwired=%s cheatHotwire=%s getScriptName=%s",
              g_fidVehicleHotwired ? "OK":"NULL", g_fidVehicleAlarmed ? "OK":"NULL",
              g_fidVehicleIgnitionSwitch ? "OK":"NULL", g_midSetHotwired ? "OK":"NULL",
              g_midCheatHotwire ? "OK":"NULL", g_midGetScriptName ? "OK":"NULL");
    } else {
      env->ExceptionClear();
      JNI_LOG("[VEHICLE-TOOLS CACHE] ERROR: BaseVehicle class not found");
    }

    env->DeleteGlobalRef(enumClass);
  }

  // [ENI] Night Vision — IsoPlayer.setWearingNightVisionGoggles(Z)V
  // IsoPlayer extiende IGC, usamos g_cachedIsoPlayerClass si existe, si no buscamos
  {
    jclass ipClass = env->FindClass("zombie/characters/IsoPlayer");
    if (ipClass) {
      g_midSetNightVision = env->GetMethodID(ipClass, "setWearingNightVisionGoggles", "(Z)V");
      if (!g_midSetNightVision) env->ExceptionClear();
      env->DeleteLocalRef(ipClass);
      JNI_LOG("[NIGHT-VIS CACHE] setWearingNightVisionGoggles=%s", g_midSetNightVision ? "OK" : "NULL");
    } else {
      env->ExceptionClear();
    }
  }

  // [ENI] Lightfoot (stealth) — IGC.wornItemsHearingModifier (private float)
  if (g_cachedIsoGameCharacterClass) {
    g_fidWornItemsHearing = env->GetFieldID(g_cachedIsoGameCharacterClass, "wornItemsHearingModifier", "F");
    if (!g_fidWornItemsHearing) env->ExceptionClear();
    JNI_LOG("[LIGHTFOOT CACHE] wornItemsHearingModifier=%s", g_fidWornItemsHearing ? "OK" : "NULL");
  }

  // [ENI] Loot ESP — IsoObject.container + ItemContainer.getItems + InventoryItem
  {
    jclass ioClass = env->FindClass("zombie/iso/IsoObject");
    if (ioClass) {
      g_fidIsoObjectContainer = env->GetFieldID(ioClass, "container", "Lzombie/inventory/ItemContainer;");
      if (!g_fidIsoObjectContainer) env->ExceptionClear();
      env->DeleteLocalRef(ioClass);
    } else { env->ExceptionClear(); }

    jclass icClass = env->FindClass("zombie/inventory/ItemContainer");
    if (icClass) {
      // getItems() en ItemContainer devuelve ArrayList<InventoryItem>
      g_midItemContainerGetItems = env->GetMethodID(icClass, "getItems", "()Lzombie/util/list/PZArrayList;");
      if (!g_midItemContainerGetItems) {
          env->ExceptionClear();
          g_midItemContainerGetItems = env->GetMethodID(icClass, "getItems", "()Ljava/util/ArrayList;");
          if (!g_midItemContainerGetItems) env->ExceptionClear();
      }
      env->DeleteLocalRef(icClass);
    } else { env->ExceptionClear(); }

    jclass iiClass = env->FindClass("zombie/inventory/InventoryItem");
    if (iiClass) {
      g_midItemGetType = env->GetMethodID(iiClass, "getType", "()Ljava/lang/String;");
      if (!g_midItemGetType) env->ExceptionClear();
      g_midItemGetName = env->GetMethodID(iiClass, "getName", "()Ljava/lang/String;");
      if (!g_midItemGetName) env->ExceptionClear();
      g_midItemGetCategory = env->GetMethodID(iiClass, "getCategory", "()Ljava/lang/String;");
      if (!g_midItemGetCategory) env->ExceptionClear();
      env->DeleteLocalRef(iiClass);
    } else { env->ExceptionClear(); }

    jclass gsClass = env->FindClass("zombie/iso/IsoGridSquare");
    if (gsClass) {
      // getObjects() devuelve ArrayList<IsoObject>
      g_midGridSquareGetObjects = env->GetMethodID(gsClass, "getObjects", "()Lzombie/util/list/PZArrayList;");
      if (!g_midGridSquareGetObjects) {
          env->ExceptionClear();
          // Fallback para B41 por si acaso
          g_midGridSquareGetObjects = env->GetMethodID(gsClass, "getObjects", "()Ljava/util/ArrayList;");
          if (!g_midGridSquareGetObjects) env->ExceptionClear();
      }
      
      g_midGridSquareGetStaticMovingObjects = env->GetMethodID(gsClass, "getStaticMovingObjects", "()Ljava/util/ArrayList;");
      if (!g_midGridSquareGetStaticMovingObjects) env->ExceptionClear();
      env->DeleteLocalRef(gsClass);
    } else { env->ExceptionClear(); }

    JNI_LOG("[LOOT-ESP CACHE] container=%s getItems=%s getType=%s getName=%s getObjects=%s",
            g_fidIsoObjectContainer ? "OK":"NULL", g_midItemContainerGetItems ? "OK":"NULL",
            g_midItemGetType ? "OK":"NULL", g_midItemGetName ? "OK":"NULL",
            g_midGridSquareGetObjects ? "OK":"NULL");
  }

  JNI_LOG(
      "[CHEAT-CACHE] g_fidCheats=%s  g_fidInvincible=%s  g_fidAvoidDamage=%s",
      g_fidCheats ? "OK" : "NULL", g_fidInvincible ? "OK" : "NULL",
      g_fidAvoidDamage ? "OK" : "NULL");
  JNI_LOG("[CHEAT-CACHE] g_midPlayerCheatsSet=%s  g_fidEnumSet=%s  "
          "g_midIsSet=%s  g_midSetAdd=%s",
          g_midPlayerCheatsSet ? "OK" : "NULL", g_fidEnumSet ? "OK" : "NULL",
          g_midPlayerCheatsIsSet ? "OK" : "NULL", g_midSetAdd ? "OK" : "NULL");
  JNI_LOG("[CHEAT-CACHE] GOD=%s  AMMO=%s  NOCLIP=%s  INVISIBLE=%s  CARRY=%s",
          g_valGodMode ? "OK" : "NULL", g_valUnlimitedAmmo ? "OK" : "NULL",
          g_valNoClip ? "OK" : "NULL", g_valInvisible ? "OK" : "NULL",
          g_valUnlimitedCarry ? "OK" : "NULL");
  JNI_LOG("[CHEAT-CACHE] rightHandItem=%s  getMaxAmmo=%s  setCurrentAmmo=%s",
          g_fidRightHandItem ? "OK" : "NULL", g_midGetMaxAmmo ? "OK" : "NULL",
          g_midSetCurrentAmmo ? "OK" : "NULL");

  return true;
}

void JNIHelper::SetCharacterFreePoints(int points) {
  JNIEnv *env = GetEnv();
  if (!env)
    return;

  env->ExceptionClear();

  // 1. Modificación de SandboxOptions (Java) - Autoridad del Servidor/Lógica
  if (g_cachedSandboxOptionsClass && g_fidSandboxInstance &&
      g_fidCharacterFreePoints) {
    jobject sandboxInstance = env->GetStaticObjectField(
        g_cachedSandboxOptionsClass, g_fidSandboxInstance);
    if (sandboxInstance) {
      jobject freePointsObj =
          env->GetObjectField(sandboxInstance, g_fidCharacterFreePoints);
      if (freePointsObj) {
        if (g_midSetIntOptionValue) {
          env->CallVoidMethod(freePointsObj, g_midSetIntOptionValue,
                              (jint)points);
        } else if (g_fidIntOptionValue) {
          env->SetIntField(freePointsObj, g_fidIntOptionValue, (jint)points);
        }

        if (env->ExceptionCheck())
          env->ExceptionClear();

        if (g_midInitSandboxVars) {
          env->CallVoidMethod(sandboxInstance, g_midInitSandboxVars);
          if (env->ExceptionCheck())
            env->ExceptionClear();
        }

        env->DeleteLocalRef(freePointsObj);
      }
      env->DeleteLocalRef(sandboxInstance);
    }
  }

  // 2. Sincronización Directa de Tabla Lua (SandboxVars) - UI Sync
  // [ENI] Bypassing RunLua to avoid FileNotFoundException. Manipulating
  // KahluaTable directly.
  if (g_cachedLuaManagerClass && g_fidLuaEnv && g_cachedKahluaTableClass &&
      g_midRawGet && g_midRawSet) {
    jobject envTable =
        env->GetStaticObjectField(g_cachedLuaManagerClass, g_fidLuaEnv);
    if (envTable) {
      jstring sandboxVarsKey = env->NewStringUTF("SandboxVars");
      jobject sandboxVarsTable =
          env->CallObjectMethod(envTable, g_midRawGet, sandboxVarsKey);

      if (sandboxVarsTable) {
        jstring charFreePointsKey = env->NewStringUTF("CharacterFreePoints");

        // Kahlua representa números como Double
        if (g_cachedDoubleClass && g_midDoubleInit) {
          jobject valObj = env->NewObject(g_cachedDoubleClass, g_midDoubleInit,
                                          (jdouble)points);
          if (valObj) {
            env->CallVoidMethod(sandboxVarsTable, g_midRawSet,
                                charFreePointsKey, valObj);
            env->DeleteLocalRef(valObj);
          }
        }

        env->DeleteLocalRef(charFreePointsKey);
        env->DeleteLocalRef(sandboxVarsTable);
      }

      env->DeleteLocalRef(sandboxVarsKey);
      env->DeleteLocalRef(envTable);
    }
  }

  if (env->ExceptionCheck())
    env->ExceptionClear();
}

void JNIHelper::SyncCharacterFreePoints() {
  // Ported from 2.1: This ensures the points are updated in the UI and the
  // Sandbox concurrently
  int points = GetCharacterFreePoints();
  SetCharacterFreePoints(points);
  JNI_LOG("[POINTS] Synced %d points with Lua/Sandbox", points);
}

void JNIHelper::ResetCharacterFreePoints() {
  SetCharacterFreePoints(0);
  JNI_LOG("[POINTS] Reset to 0");
}

int JNIHelper::GetCharacterFreePoints() {
  JNIEnv *env = GetEnv();
  if (!env)
    return 0;

  if (g_cachedSandboxOptionsClass && g_fidSandboxInstance &&
      g_fidCharacterFreePoints) {
    jobject sandboxInstance = env->GetStaticObjectField(
        g_cachedSandboxOptionsClass, g_fidSandboxInstance);
    if (sandboxInstance) {
      jobject freePointsObj =
          env->GetObjectField(sandboxInstance, g_fidCharacterFreePoints);
      if (freePointsObj) {
        // Assuming IntegerSandboxOption has a getValue() or public field
        // Usually it extends ConfigOption which has getValue
        // Let's try getting field 'value' if we found cached ID for it
        if (g_fidIntOptionValue) {
          int val = env->GetIntField(freePointsObj, g_fidIntOptionValue);
          env->DeleteLocalRef(freePointsObj);
env->DeleteLocalRef(sandboxInstance);
          return val;
        }
        env->DeleteLocalRef(freePointsObj);
      }
      env->DeleteLocalRef(sandboxInstance);
    }
  }
  return 0;
}

void JNIHelper::InjectLootToCorpsesAoE(int radiusTiles) {
  JNIEnv *env = GetEnv();
  if (!env || !g_playerInstance || !g_cachedIsoWorldClass || !g_cachedIsoCellClass ||
      !g_cachedIsoDeadBodyClass || !g_cachedInventoryItemFactoryClass || !g_midCreateItem) {
    JNI_LOG("[InjectLootToCorpsesAoE] ERROR: Missing JNI references or player not hooked.");
    return;
  }

  // Get Player Coordinates using direct method calls
  float px = env->CallFloatMethod(g_playerInstance, g_midGetX);
  float py = env->CallFloatMethod(g_playerInstance, g_midGetY);
  float pz = env->CallFloatMethod(g_playerInstance, g_midGetZ);
  env->ExceptionClear(); // Clear exceptions just in case
  
  if (px == 0.f && py == 0.f) {
     JNI_LOG("[InjectLootToCorpsesAoE] ERROR: Failed to get valid player coordinates.");
     return;
  }

  // Get World and Cell
  jobject world = env->GetStaticObjectField(g_cachedIsoWorldClass, g_fidWorldInstance);
  if (!world) {
     JNI_LOG("[InjectLootToCorpsesAoE] ERROR: world instance is null.");
     return;
  }
  jobject cell = env->GetObjectField(world, g_fidCurrentCell);
  if (!cell) {
     JNI_LOG("[InjectLootToCorpsesAoE] ERROR: cell instance is null.");
     env->DeleteLocalRef(world);
     return;
  }
  
  jmethodID midGetGridSquare = env->GetMethodID(g_cachedIsoCellClass, "getGridSquare", "(DDD)Lzombie/iso/IsoGridSquare;");
  if (!midGetGridSquare) {
      JNI_LOG("[InjectLootToCorpsesAoE] ERROR: getGridSquare method not found.");
      env->ExceptionClear();
      env->DeleteLocalRef(cell);
      env->DeleteLocalRef(world);
      return;
  }
  
  jstring jItemId = env->NewStringUTF(g_targetItemID);
  int injectedCount = 0;

  for (int dx = -radiusTiles; dx <= radiusTiles; dx++) {
    for (int dy = -radiusTiles; dy <= radiusTiles; dy++) {
      double checkX = px + dx;
      double checkY = py + dy;
      
      jobject square = env->CallObjectMethod(cell, midGetGridSquare, checkX, checkY, (double)pz);
      if (square) {
         if (g_midGridSquareGetStaticMovingObjects) {
             jobject objList = env->CallObjectMethod(square, g_midGridSquareGetStaticMovingObjects);
             if (objList) {
                 int size = GetListSize(env, objList);
                 
                 for (int i = 0; i < size; i++) {
                     jobject obj = GetListItem(env, objList, i);
                     if (env->ExceptionCheck()) env->ExceptionClear();
                     
                     if (obj) {
                         if (env->IsInstanceOf(obj, g_cachedIsoDeadBodyClass)) {
                             // Create item
                             jobject newItem = env->CallStaticObjectMethod(g_cachedInventoryItemFactoryClass, g_midCreateItem, jItemId);
                             if (env->ExceptionCheck()) env->ExceptionClear();
                             
                             if (newItem) {
                                 // Get container
                                 jobject container = nullptr;
                                 if (g_fidIsoObjectContainer) {
                                     container = env->GetObjectField(obj, g_fidIsoObjectContainer);
                                 }
                                 if (env->ExceptionCheck()) env->ExceptionClear();

                                 if (container) {
                                     // Add item to container
                                     if (g_midItemContainerAddItem) {
                                         jobject added = env->CallObjectMethod(container, g_midItemContainerAddItem, newItem);
                                         if (env->ExceptionCheck()) env->ExceptionClear();
                                         if (added) env->DeleteLocalRef(added);
                                         injectedCount++;
                                     }
                                     env->DeleteLocalRef(container);
                                 } else {
                                     // Fallback: set primary hand item if no container
                                     jmethodID midSetPrimary = env->GetMethodID(g_cachedIsoDeadBodyClass, "setPrimaryHandItem", "(Lzombie/inventory/InventoryItem;)V");
                                     if (env->ExceptionCheck()) env->ExceptionClear();
                                     if (midSetPrimary) {
                                         env->CallVoidMethod(obj, midSetPrimary, newItem);
                                         if (env->ExceptionCheck()) env->ExceptionClear();
                                         injectedCount++;
                                     }
                                 }
                                 env->DeleteLocalRef(newItem);
                             }
                         }
                         env->DeleteLocalRef(obj);
                     }
                 }
                 env->DeleteLocalRef(objList);
             }
         }
         env->DeleteLocalRef(square);
      }
    }
  }

  env->DeleteLocalRef(jItemId);
  env->DeleteLocalRef(cell);
  env->DeleteLocalRef(world);
  
  JNI_LOG("[InjectLootToCorpsesAoE] Injected %s into %d corpses within %d tiles.", g_targetItemID, injectedCount, radiusTiles);
}

void JNIHelper::InjectLootToContainersAoE(int radiusTiles) {
  JNIEnv *env = GetEnv();
  if (!env || !g_playerInstance || !g_cachedIsoWorldClass || !g_cachedIsoCellClass ||
      !g_cachedInventoryItemFactoryClass || !g_midCreateItem) {
    JNI_LOG("[InjectLootToContainersAoE] ERROR: Missing JNI references or player not hooked.");
    return;
  }

  // Get Player Coordinates using direct method calls
  float px = env->CallFloatMethod(g_playerInstance, g_midGetX);
  float py = env->CallFloatMethod(g_playerInstance, g_midGetY);
  float pz = env->CallFloatMethod(g_playerInstance, g_midGetZ);
  env->ExceptionClear(); // Clear exceptions just in case
  
  if (px == 0.f && py == 0.f) {
     JNI_LOG("[InjectLootToContainersAoE] ERROR: Failed to get valid player coordinates.");
     return;
  }

  // Get World and Cell
  jobject world = env->GetStaticObjectField(g_cachedIsoWorldClass, g_fidWorldInstance);
  if (!world) {
     JNI_LOG("[InjectLootToContainersAoE] ERROR: world instance is null.");
     return;
  }
  jobject cell = env->GetObjectField(world, g_fidCurrentCell);
  if (!cell) {
     JNI_LOG("[InjectLootToContainersAoE] ERROR: cell instance is null.");
     env->DeleteLocalRef(world);
     return;
  }
  
  jmethodID midGetGridSquare = env->GetMethodID(g_cachedIsoCellClass, "getGridSquare", "(DDD)Lzombie/iso/IsoGridSquare;");
  if (!midGetGridSquare) {
      JNI_LOG("[InjectLootToContainersAoE] ERROR: getGridSquare method not found.");
      env->ExceptionClear();
      env->DeleteLocalRef(cell);
      env->DeleteLocalRef(world);
      return;
  }
  
  jstring jItemId = env->NewStringUTF(g_targetItemID);
  int injectedCount = 0;

  for (int dx = -radiusTiles; dx <= radiusTiles; dx++) {
    for (int dy = -radiusTiles; dy <= radiusTiles; dy++) {
      double checkX = px + dx;
      double checkY = py + dy;
      
      jobject square = env->CallObjectMethod(cell, midGetGridSquare, checkX, checkY, (double)pz);
      if (square) {
         if (g_midGridSquareGetObjects) {
             jobject objList = env->CallObjectMethod(square, g_midGridSquareGetObjects);
             if (objList) {
                 int size = GetListSize(env, objList);
                 
                 for (int i = 0; i < size; i++) {
                     jobject obj = GetListItem(env, objList, i);
                     if (env->ExceptionCheck()) env->ExceptionClear();
                     
                     if (obj) {
                         // Solo inyectar si NO es un cadáver (para evitar inyectar doble)
                         if (!env->IsInstanceOf(obj, g_cachedIsoDeadBodyClass)) {
                             // Create item
                             jobject newItem = env->CallStaticObjectMethod(g_cachedInventoryItemFactoryClass, g_midCreateItem, jItemId);
                             if (env->ExceptionCheck()) env->ExceptionClear();
                             
                             if (newItem) {
                                 // Get container
                                 jobject container = nullptr;
                                 if (g_fidIsoObjectContainer) {
                                     container = env->GetObjectField(obj, g_fidIsoObjectContainer);
                                 }
                                 if (env->ExceptionCheck()) env->ExceptionClear();

                                 if (container) {
                                     // Add item to container
                                     if (g_midItemContainerAddItem) {
                                         jobject added = env->CallObjectMethod(container, g_midItemContainerAddItem, newItem);
                                         if (env->ExceptionCheck()) env->ExceptionClear();
                                         if (added) env->DeleteLocalRef(added);
                                         injectedCount++;
                                     }
                                     env->DeleteLocalRef(container);
                                 }
                                 env->DeleteLocalRef(newItem);
                             }
                         }
                         env->DeleteLocalRef(obj);
                     }
                 }
                 env->DeleteLocalRef(objList);
             }
         }
         env->DeleteLocalRef(square);
      }
    }
  }

  env->DeleteLocalRef(jItemId);
  env->DeleteLocalRef(cell);
  env->DeleteLocalRef(world);
  
  JNI_LOG("[InjectLootToContainersAoE] Injected %s into %d containers within %d tiles.", g_targetItemID, injectedCount, radiusTiles);
}

// [ENI] Merged implementations from 2.2

bool JNIHelper::GetLocalPlayer() {
  JNIEnv *env = GetEnv();
  if (!env)
    return false;

  jclass cls = g_cachedIsoPlayerClass;
  if (!cls)
    cls = env->FindClass("zombie/characters/IsoPlayer");
  if (!cls) {
    env->ExceptionClear();
    return false;
  }

  // 1. Try getInstance()
  jmethodID mid = env->GetStaticMethodID(cls, "getInstance",
                                         "()Lzombie/characters/IsoPlayer;");
  if (env->ExceptionCheck())
    env->ExceptionClear();

  if (mid) {
    jobject player = env->CallStaticObjectMethod(cls, mid);
    if (env->ExceptionCheck())
      env->ExceptionClear();
    if (player) {
      if (g_playerInstance)
        env->DeleteGlobalRef(g_playerInstance);
      g_playerInstance = env->NewGlobalRef(player);
      env->DeleteLocalRef(player);
      return true;
    }
  }

  // 2. Fallback for B42: IsoPlayer.players[0]
  jfieldID fidPlayers =
      env->GetStaticFieldID(cls, "players", "[Lzombie/characters/IsoPlayer;");
  if (env->ExceptionCheck())
    env->ExceptionClear();

  if (fidPlayers) {
    jobjectArray arr = (jobjectArray)env->GetStaticObjectField(cls, fidPlayers);
    if (env->ExceptionCheck())
      env->ExceptionClear();
    if (arr) {
      if (env->GetArrayLength(arr) > 0) {
        jobject p0 = env->GetObjectArrayElement(arr, 0);
        if (env->ExceptionCheck())
          env->ExceptionClear();
        if (p0) {
          if (g_playerInstance)
            env->DeleteGlobalRef(g_playerInstance);
          g_playerInstance = env->NewGlobalRef(p0);
          env->DeleteLocalRef(p0);
          env->DeleteLocalRef(arr);
          return true;
        }
      }
      env->DeleteLocalRef(arr);
    }
  }

  return false;
}

std::string JNIHelper::GetPlayerName() {
  JNIEnv *env = GetEnv();
  if (!env || !g_playerInstance)
    return "Unknown";

  jclass cls = env->GetObjectClass(g_playerInstance);
  jmethodID mid = env->GetMethodID(cls, "getUsername", "()Ljava/lang/String;");
  if (mid) {
    jstring str = (jstring)env->CallObjectMethod(g_playerInstance, mid);
    if (str) {
      const char *chars = env->GetStringUTFChars(str, 0);
      std::string name(chars);
      env->ReleaseStringUTFChars(str, chars);
      env->DeleteLocalRef(str);
      env->DeleteLocalRef(cls);
      return name;
    }
  }
  env->DeleteLocalRef(cls);
  return "Unknown";
}

float JNIHelper::GetPlayerHealth() {
  JNIEnv *env = GetEnv();
  if (!env || !g_playerInstance)
    return 0.0f;

  jclass cls = env->GetObjectClass(g_playerInstance);
  jmethodID mid = env->GetMethodID(cls, "getHealth", "()F");
  if (mid) {
    float h = env->CallFloatMethod(g_playerInstance, mid);
    env->DeleteLocalRef(cls);
    return h;
  }
  env->DeleteLocalRef(cls);
  return 0.0f;
}

float JNIHelper::GetPlayerHealthDirect() {
  return GetPlayerHealthDirect_Internal(g_playerInstance);
}

float JNIHelper::GetPlayerHealthDirect_Internal(jobject playerObj) {
  JNIEnv *env = GetEnv();
  if (!env || !playerObj)
    return -1.0f;

  jclass cls = env->GetObjectClass(playerObj);
  jfieldID fid = env->GetFieldID(cls, "health", "F");
  float val = -1.0f;
  if (fid) {
    val = env->GetFloatField(playerObj, fid);
  }
  env->DeleteLocalRef(cls);
  return val;
}

void JNIHelper::RunLua(const char *script) {
  JNIEnv *env = GetEnv();
  if (!env || !g_cachedLuaManagerClass)
    return;

  env->ExceptionClear();

  // 1. Obtener la clase del compilador de Kahlua
  jclass compilerCls = env->FindClass("se/krka/kahlua/luaj/compiler/LuaCompiler");
  if (!compilerCls) {
    env->ExceptionClear();
    JNI_LOG("[RunLua ERROR] No se encontro la clase LuaCompiler");
    return;
  }

  jmethodID midLoadstring = env->GetStaticMethodID(
      compilerCls, "loadstring",
      "(Ljava/lang/String;Ljava/lang/String;Lse/krka/kahlua/vm/KahluaTable;)Lse/krka/kahlua/vm/LuaClosure;");
  if (!midLoadstring) {
    env->ExceptionClear();
    env->DeleteLocalRef(compilerCls);
    JNI_LOG("[RunLua ERROR] No se encontro loadstring() en LuaCompiler");
    return;
  }

  // 2. Obtener campos estaticos en LuaManager
  jfieldID fidThread = env->GetStaticFieldID(g_cachedLuaManagerClass, "thread", "Lse/krka/kahlua/vm/KahluaThread;");
  jfieldID fidCaller = env->GetStaticFieldID(g_cachedLuaManagerClass, "caller", "Lse/krka/kahlua/integration/LuaCaller;");

  if (!g_fidLuaEnv || !fidThread || !fidCaller) {
    env->ExceptionClear();
    env->DeleteLocalRef(compilerCls);
    JNI_LOG("[RunLua ERROR] Campos estaticos thread o caller no encontrados en LuaManager");
    return;
  }

  jobject luaEnv = env->GetStaticObjectField(g_cachedLuaManagerClass, g_fidLuaEnv);
  jobject luaThread = env->GetStaticObjectField(g_cachedLuaManagerClass, fidThread);
  jobject luaCaller = env->GetStaticObjectField(g_cachedLuaManagerClass, fidCaller);

  if (!luaEnv || !luaThread || !luaCaller) {
    env->ExceptionClear();
    env->DeleteLocalRef(compilerCls);
    if (luaEnv) env->DeleteLocalRef(luaEnv);
    if (luaThread) env->DeleteLocalRef(luaThread);
    if (luaCaller) env->DeleteLocalRef(luaCaller);
    JNI_LOG("[RunLua ERROR] Referencias nulas en el entorno global de Lua");
    return;
  }

  // 3. Compilar el string de Lua en caliente
  jstring jScript = env->NewStringUTF(script);
  jstring jName = env->NewStringUTF("RunLua");
  jobject closure = env->CallStaticObjectMethod(compilerCls, midLoadstring, jScript, jName, luaEnv);
  env->DeleteLocalRef(jScript);
  env->DeleteLocalRef(jName);
  env->DeleteLocalRef(compilerCls);

  if (env->ExceptionCheck()) {
    env->ExceptionClear();
    env->DeleteLocalRef(luaEnv);
    env->DeleteLocalRef(luaThread);
    env->DeleteLocalRef(luaCaller);
    JNI_LOG("[RunLua ERROR] Excepcion al compilar script de Lua en memoria");
    return;
  }

  if (!closure) {
    env->DeleteLocalRef(luaEnv);
    env->DeleteLocalRef(luaThread);
    env->DeleteLocalRef(luaCaller);
    JNI_LOG("[RunLua ERROR] LuaCompiler.loadstring devolvio null");
    return;
  }

  // 4. Ejecutar la closure a traves de LuaCaller.pcall
  jclass callerCls = env->GetObjectClass(luaCaller);
  jmethodID midPcall = env->GetMethodID(callerCls, "pcall", 
      "(Lse/krka/kahlua/vm/KahluaThread;Ljava/lang/Object;[Ljava/lang/Object;)[Ljava/lang/Object;");
  env->DeleteLocalRef(callerCls);

  if (!midPcall) {
    env->ExceptionClear();
    env->DeleteLocalRef(closure);
    env->DeleteLocalRef(luaEnv);
    env->DeleteLocalRef(luaThread);
    env->DeleteLocalRef(luaCaller);
    JNI_LOG("[RunLua ERROR] No se encontro pcall() en LuaCaller");
    return;
  }

  // Argumentos vacios para pcall
  jclass objCls = env->FindClass("java/lang/Object");
  jobjectArray emptyArgs = env->NewObjectArray(0, objCls, nullptr);
  env->DeleteLocalRef(objCls);

  jobjectArray pcallResult = (jobjectArray)env->CallObjectMethod(luaCaller, midPcall, luaThread, closure, emptyArgs);
  env->DeleteLocalRef(emptyArgs);
  env->DeleteLocalRef(closure);

  if (env->ExceptionCheck()) {
    env->ExceptionClear();
    JNI_LOG("[RunLua ERROR] Excepcion al invocar pcall() de la closure");
  }

  if (pcallResult) {
    env->DeleteLocalRef(pcallResult);
  }

  env->DeleteLocalRef(luaEnv);
  env->DeleteLocalRef(luaThread);
  env->DeleteLocalRef(luaCaller);
}

void JNIHelper::SetPlayerHealth(float health) {
  JNIEnv *env = GetEnv();
  if (!env || !g_playerInstance)
    return;

  jclass cls = g_cachedIsoGameCharacterClass;
  if (!cls)
    cls = env->GetObjectClass(g_playerInstance);

  jfieldID fidHealth = env->GetFieldID(cls, "health", "F");
  if (fidHealth) {
    env->SetFloatField(g_playerInstance, fidHealth, health);
  }
  env->ExceptionClear();
}

// [ENI-REACTIVE] Helper para eliminar cheats del EnumSet de forma segura
static void SafeRemoveCheat(JNIEnv* env, jobject playerObj, jobject cheatEnumVal) {
  if (!playerObj || !cheatEnumVal || !JNIHelper::g_fidCheats || !JNIHelper::g_fidEnumSet || !JNIHelper::g_midSetRemove) return;
  jobject cheatsFieldObj = env->GetObjectField(playerObj, JNIHelper::g_fidCheats);
  if (cheatsFieldObj) {
    jobject enumSetObj = env->GetObjectField(cheatsFieldObj, JNIHelper::g_fidEnumSet);
    if (enumSetObj) {
      env->CallBooleanMethod(enumSetObj, JNIHelper::g_midSetRemove, cheatEnumVal);
      if (env->ExceptionCheck()) env->ExceptionClear();
      env->DeleteLocalRef(enumSetObj);
    }
    env->DeleteLocalRef(cheatsFieldObj);
  } else {
    env->ExceptionClear();
  }
}

void JNIHelper::ToggleGodMode(bool enable) {
  g_godModeActive = enable;
  JNI_LOG("[CHEAT] God Mode: %s", enable ? "ON" : "OFF");
  if (!enable && g_playerInstance) {
    JNIEnv* env = GetEnv();
    if (env) {
      SafeRemoveCheat(env, g_playerInstance, g_valGodMode);
      env->ExceptionClear();
    }
  }
  if (enable) ApplyActiveCheats();
}
void JNIHelper::ToggleInvisible(bool enable) {
  g_invisibleActive = enable;
  JNI_LOG("[CHEAT] Invisible: %s", enable ? "ON" : "OFF");
  if (!enable && g_playerInstance) {
    JNIEnv* env = GetEnv();
    if (env) {
      SafeRemoveCheat(env, g_playerInstance, g_valInvisible);
      // Quitar zombiesDontAttack si ghost mode no lo requiere
      if (!g_ghostModeActive) SafeRemoveCheat(env, g_playerInstance, g_valZombiesDontAttack);
      env->ExceptionClear();
    }
  }
  if (enable) ApplyActiveCheats();
}
void JNIHelper::ToggleGhostMode(bool enable) {
  g_ghostModeActive = enable;
  JNI_LOG("[CHEAT] Ghost Mode: %s", enable ? "ON" : "OFF");
  if (!enable && g_playerInstance) {
    JNIEnv* env = GetEnv();
    if (env) {
      SafeRemoveCheat(env, g_playerInstance, g_valNoClip);
      if (!g_invisibleActive) SafeRemoveCheat(env, g_playerInstance, g_valZombiesDontAttack);
      env->ExceptionClear();
    }
  }
  if (enable) ApplyActiveCheats();
}
void JNIHelper::ToggleNoClip(bool enable) {
  g_noClipActive = enable;
  JNI_LOG("[CHEAT] NoClip: %s", enable ? "ON" : "OFF");
  if (!enable && g_playerInstance) {
    JNIEnv* env = GetEnv();
    if (env) {
      if (!g_ghostModeActive) SafeRemoveCheat(env, g_playerInstance, g_valNoClip);
      env->ExceptionClear();
    }
  }
  if (enable) ApplyActiveCheats();
}
void JNIHelper::ToggleUnlimitedAmmo(bool enable) {
  g_unlimitedAmmoActive = enable;
  JNI_LOG("[CHEAT] Unlimited Ammo: %s", enable ? "ON" : "OFF");
  if (!enable && g_playerInstance) {
    JNIEnv* env = GetEnv();
    if (env) {
      SafeRemoveCheat(env, g_playerInstance, g_valUnlimitedAmmo);
      env->ExceptionClear();
    }
  }
  if (enable) ApplyActiveCheats();
}
void JNIHelper::ToggleUnlimitedCarry(bool active) {
  g_unlimitedCarryActive = active;
  JNI_LOG("[CHEAT] Unlimited Carry: %s",
          active ? "ON (Packet Bypass Active)" : "OFF");
  if (!active && g_playerInstance) {
    JNIEnv* env = GetEnv();
    if (env) {
      SafeRemoveCheat(env, g_playerInstance, g_valUnlimitedCarry);
      env->ExceptionClear();
    }
  }
  if (active) ApplyActiveCheats();
}

void JNIHelper::ToggleUnlimitedEndurance(bool active) {
  g_unlimitedEnduranceActive = active;
  JNI_LOG("[CHEAT] Unlimited Endurance: %s",
          active ? "ON (Packet Bypass Active)" : "OFF");
  if (!active && g_playerInstance) {
    JNIEnv* env = GetEnv();
    if (env) {
      // Endurance doesn't have a specific Enum flag here usually, but if it did it would go here.
      // Usually it's patched in memory directly.
      env->ExceptionClear();
    }
  }
  if (active) ApplyActiveCheats();
}

// [ENI] Soft Aim — Precision Override para armas de fuego ranged
// Activa/desactiva el override de hitChance/spread/angles en HandWeapon.
// El override se aplica por frame en ApplyActiveCheats(); no requiere estado
// persistente extra porque los valores se escriben cada frame mientras es ON.
void JNIHelper::ToggleSoftAim(bool active) {
  g_softAimActive = active;
  JNI_LOG("[CHEAT] Soft Aim (Ranged Precision Override): %s",
          active ? "ON (hitChance=100, spread=0, noJam)" : "OFF");
}

// [ENI] Critical Override — Toggle limpio con restore inmediato al desactivar.
// ON:  criticalChance=1.0, criticalDmgMult=3.0, aimingPerkCritMod=10 por frame.
// OFF: restaura a baseline safe (10%/x1.5/0) en el mismo frame del toggle.
// [ENI] Animal Cheat — Activa los cheat flags ANIMAL y ANIMAL_EXTRA_VALUES en PlayerCheats.
// Efecto en B42:
//   ANIMAL             → El jugador puede interactuar con animales silvestres sin condiciones
//                        (taming sin necesidad de objetos, aceptación forzada en playerAcceptanceList).
//   ANIMAL_EXTRA_VALUES→ Habilita valores extra de debug: ver stats ocultos del animal,
//                        forzar stress=0, overrides de zona y comportamiento.
// Ambos son toggleables limpiamente vía EnumSet add/remove (mismo mecanismo que GodMode).
void JNIHelper::ToggleAnimalCheat(bool active) {
  g_animalCheatActive = active;
  JNI_LOG("[CHEAT] Animal Cheat (Tame + Extra Values): %s",
          active ? "ON (ANIMAL + ANIMAL_EXTRA_VALUES en PlayerCheats)" : "OFF");
  ApplyActiveCheats(); // Sync inmediato al toggle
}

// [ENI] Combat Supremacy — Toggle con restore inmediato al desactivar.
// Usa setters PÚBLICOS de IGC para que el engine acepte los valores en su propio flujo.
// El servidor NUNCA sincroniza estos campos de vuelta — INVISIBLE AL ADMIN.
// ON:  ignoreStaggerBack=true, blurFactor=0, staggerTimeMod=0, recoilDelay=0
// OFF: restore vía setters a valores baseline del engine
void JNIHelper::ToggleCombatSupremacy(bool active) {
  g_combatSupremacyActive = active;
  JNI_LOG("[CHEAT] Combat Supremacy: %s",
          active ? "ON (stagger=off, blur=0, recoil=0)" : "OFF (restored via setters)");

  // Restore inmediato al desactivar — llamamos setters con valores baseline
  if (!active && g_playerInstance) {
    JNIEnv* env = GetEnv();
    if (env) {
      if (g_midSetIgnoreStaggerBack) env->CallVoidMethod(g_playerInstance, g_midSetIgnoreStaggerBack, (jboolean)JNI_FALSE);
      if (g_midSetStaggerTimeMod)    env->CallVoidMethod(g_playerInstance, g_midSetStaggerTimeMod,    (jfloat)1.0f);  // normal
      if (g_midSetRecoilDelay)       env->CallVoidMethod(g_playerInstance, g_midSetRecoilDelay,       (jfloat)0.15f); // baseline
      if (g_fidBlurFactor)        env->SetFloatField(g_playerInstance, g_fidBlurFactor,       (jfloat)0.0f);
      if (g_fidBlurFactorTarget)  env->SetFloatField(g_playerInstance, g_fidBlurFactorTarget, (jfloat)0.0f);
      env->ExceptionClear();
    }
  }
}

void JNIHelper::ToggleCriticalOverride(bool active) {
  g_criticalOverrideActive = active;
  JNI_LOG("[CHEAT] Critical Override (Headshot Mechanic): %s",
          active ? "ON (critChance=100%, dmgMult=x3, perkCritMod=10)" : "OFF (restored baseline)");

  // Restore inmediato al desactivar: evita que el arma quede con valores override
  // hasta que el jugador cambie de arma o recargue desde script.
  if (!active && g_fidRightHandItem && g_cachedHandWeaponClass) {
    JNIEnv* env = GetEnv();
    if (env && g_playerInstance) {
      jobject weapon = env->GetObjectField(g_playerInstance, g_fidRightHandItem);
      if (weapon && env->IsInstanceOf(weapon, g_cachedHandWeaponClass) == JNI_TRUE) {
        if (g_fidHWCriticalChance)    env->SetFloatField(weapon, g_fidHWCriticalChance,    (jfloat)0.1f);  // baseline ~10%
        if (g_fidHWCriticalDmgMult)  env->SetFloatField(weapon, g_fidHWCriticalDmgMult,   (jfloat)1.5f);  // multiplicador normal
        if (g_fidHWAimingPerkCritMod) env->SetIntField(weapon,  g_fidHWAimingPerkCritMod, (jint)0);       // sin bonus de perk
        env->ExceptionClear();
        env->DeleteLocalRef(weapon);
      } else {
        if (weapon) env->DeleteLocalRef(weapon);
        env->ExceptionClear();
      }
    }
  }
}

void JNIHelper::EnableGodModeReal() {
  g_godModeActive = true;
  JNI_LOG("[CHEAT] God Mode (Real) Forced ON");
  ApplyActiveCheats();
}

void JNIHelper::ApplyActiveCheats() {
  if (!g_playerInstance)
    return;
  JNIEnv *env = GetEnv();
  if (!env)
    return;
  env->ExceptionClear();

  // Protegemos individualmente cada set para que si falla uno, no interrumpa el
  // resto
  if (g_fidInvincible) {
    env->SetBooleanField(g_playerInstance, g_fidInvincible,
                         (jboolean)g_godModeActive);
    env->ExceptionClear();
  }
  if (g_fidAvoidDamage) {
    env->SetBooleanField(g_playerInstance, g_fidAvoidDamage,
                         (jboolean)g_godModeActive);
    env->ExceptionClear();
  }
  if (g_fidGhostMode) {
    env->SetBooleanField(g_playerInstance, g_fidGhostMode,
                         (jboolean)g_ghostModeActive);
    env->ExceptionClear();
  }

  if (g_unlimitedEnduranceActive && g_fidStats && g_midStatsSet) {
    jobject statsObj = env->GetObjectField(g_playerInstance, g_fidStats);
    if (!statsObj) {
      env->ExceptionClear();
      statsObj = env->CallObjectMethod(g_playerInstance, g_midGetStats);
    }
    if (statsObj) {
      if (g_valStatEndurance) {
        env->CallBooleanMethod(statsObj, g_midStatsSet, g_valStatEndurance,
                               (jfloat)1.0f);
        env->ExceptionClear();
      }
      if (g_valStatFatigue) {
        env->CallBooleanMethod(statsObj, g_midStatsSet, g_valStatFatigue,
                               (jfloat)0.0f);
        env->ExceptionClear();
      }
      env->DeleteLocalRef(statsObj);
    } else
      env->ExceptionClear();
  }

  // [ENI] B42 Cheat Sync: Direct EnumSet mutation.
  // This is the reliable method confirmed to work on Windows and matches the
  // B42 docs bypass recommendation.
  if (g_fidCheats && g_fidEnumSet && g_midSetAdd && g_midSetRemove) {
    jobject cheatsFieldObj = env->GetObjectField(g_playerInstance, g_fidCheats);
    if (cheatsFieldObj) {
      jobject enumSetObj = env->GetObjectField(cheatsFieldObj, g_fidEnumSet);
      if (enumSetObj) {
        // [ENI-REACTIVE & NON-FIGHTING]
        // Solo se imponen los valores en true si el toggle de nuestro menú está activo.
        // Si nuestro toggle está inactivo (false), el trainer ignora la variable de PZ,
        // permitiendo que el usuario active GodMode desde el debug menu interno del juego.
        auto SyncCheat = [&](jobject val, bool isActive) {
          if (val && isActive) {
            env->CallBooleanMethod(enumSetObj, g_midSetAdd, val);
            if (env->ExceptionCheck())
              env->ExceptionClear();
          }
        };

        SyncCheat(g_valGodMode, g_godModeActive);
        SyncCheat(g_valInvisible, g_invisibleActive);
        SyncCheat(g_valZombiesDontAttack,
                  g_zombiesDontAttackActive || g_invisibleActive);
        SyncCheat(g_valNoClip, g_noClipActive || g_ghostModeActive);
        SyncCheat(g_valUnlimitedAmmo, g_unlimitedAmmoActive);
        SyncCheat(g_valUnlimitedCarry, g_unlimitedCarryActive);
        SyncCheat(g_valAnimal,            g_animalCheatActive);
        SyncCheat(g_valAnimalExtraValues, g_animalCheatActive);
        SyncCheat(g_valKnowAllRecipes,    g_knowAllRecipesActive); // [ENI] Know All Recipes

        env->DeleteLocalRef(enumSetObj);
      }
      env->DeleteLocalRef(cheatsFieldObj);
    } else
      env->ExceptionClear();
  }

  // [ENI] Night Vision — setWearingNightVisionGoggles() cada frame
  // El campo es private boolean, se resetea al quitarse/ponerse goggles — lo mantenemos via setter
  if (g_nightVisionActive && g_midSetNightVision && g_playerInstance) {
    env->CallVoidMethod(g_playerInstance, g_midSetNightVision, (jboolean)JNI_TRUE);
    env->ExceptionClear();
  }

  // [ENI] Lightfoot — wornItemsHearingModifier = 0.0f cada frame
  // Este campo modifica cuánto sonido emite el jugador según ropa/equipo
  // Valor 0 → el engine aplica 0 modificación de hearing (máximo sigilo de ropa)
  if (g_lightfootActive && g_fidWornItemsHearing && g_playerInstance) {
    env->SetFloatField(g_playerInstance, g_fidWornItemsHearing, 0.0f);
    env->ExceptionClear();
  }

  // [B42] Unlimited ammo: reset rightHandItem.currentAmmoCount = maxAmmo every
  // frame. [R-LOWSPEC] Bloque unificado — ejecutar una sola vez por frame (era
  // duplicado).
  if (g_unlimitedAmmoActive && g_fidRightHandItem && g_midGetMaxAmmo &&
      g_midSetCurrentAmmo) {
    jobject weapon = env->GetObjectField(g_playerInstance, g_fidRightHandItem);
    if (weapon) {
      jint maxAmmo = env->CallIntMethod(weapon, g_midGetMaxAmmo);
      env->ExceptionClear();
      if (maxAmmo > 0) {
        env->CallVoidMethod(weapon, g_midSetCurrentAmmo, maxAmmo);
        env->ExceptionClear();
      }
      env->DeleteLocalRef(weapon);
    } else
      env->ExceptionClear();
  }

  // Periodic verification log (every ~300 frames) — shows whether EnumSet
  // mutation stuck
  static int verifyCounter = 0;
  if (++verifyCounter >= 300 && g_fidCheats && g_midPlayerCheatsIsSet &&
      g_valUnlimitedAmmo) {
    verifyCounter = 0;
    jobject cheatsObj = env->GetObjectField(g_playerInstance, g_fidCheats);
    if (cheatsObj) {
      jboolean ammoSet = env->CallBooleanMethod(
          cheatsObj, g_midPlayerCheatsIsSet, g_valUnlimitedAmmo);
      jboolean godSet =
          g_valGodMode ? env->CallBooleanMethod(
                             cheatsObj, g_midPlayerCheatsIsSet, g_valGodMode)
                       : JNI_FALSE;
      jboolean noClipSet =
          g_valNoClip ? env->CallBooleanMethod(
                            cheatsObj, g_midPlayerCheatsIsSet, g_valNoClip)
                      : JNI_FALSE;
      env->ExceptionClear();
      JNI_LOG("[CHEAT-VERIFY] isSet: AMMO=%s GOD=%s NOCLIP=%s "
              "(unlimitedAmmoActive=%s)",
              ammoSet ? "TRUE" : "FALSE", godSet ? "TRUE" : "FALSE",
              noClipSet ? "TRUE" : "FALSE",
              g_unlimitedAmmoActive ? "ON" : "OFF");
      env->DeleteLocalRef(cheatsObj);
    } else
      env->ExceptionClear();
  }

  // [ENI] Soft Aim + Critical Override — Precision Override para armas ranged
  // Se combinan en un solo GetObjectField para evitar doble lookup por frame.
  // [R-LOWSPEC] IDs ya cacheados en Initialize() — cero FindClass en este bloque.
  bool needWeaponAccess = (g_softAimActive || g_criticalOverrideActive)
                          && g_fidRightHandItem && g_cachedHandWeaponClass;
  if (needWeaponAccess) {
    jobject weapon = env->GetObjectField(g_playerInstance, g_fidRightHandItem);
    if (weapon) {
      jboolean isHW = env->IsInstanceOf(weapon, g_cachedHandWeaponClass);
      if (isHW == JNI_TRUE) {
        jboolean isRanged  = (g_midHWIsRanged      ? env->CallBooleanMethod(weapon, g_midHWIsRanged)      : JNI_FALSE);
        jboolean isAimedFA = (g_midHWIsAimedFirearm? env->CallBooleanMethod(weapon, g_midHWIsAimedFirearm): JNI_FALSE);
        env->ExceptionClear();

        bool isFirearm = (isRanged == JNI_TRUE || isAimedFA == JNI_TRUE);

        // ---- Soft Aim block ----
        if (g_softAimActive && isFirearm) {
          if (g_fidHWHitChance)        env->SetIntField  (weapon, g_fidHWHitChance,        (jint)100);
          if (g_fidHWMinAngle)         env->SetFloatField(weapon, g_fidHWMinAngle,         (jfloat)0.0f);
          if (g_fidHWMaxAngle)         env->SetFloatField(weapon, g_fidHWMaxAngle,         (jfloat)0.0f);
          if (g_fidHWProjectileSpread) env->SetFloatField(weapon, g_fidHWProjectileSpread, (jfloat)0.0f);
          if (g_fidHWJamGunChance)     env->SetFloatField(weapon, g_fidHWJamGunChance,     (jfloat)0.0f);
          env->ExceptionClear();
        }

        // ---- Critical Override block (headshot mecánico) ----
        // criticalChance = 1.0f  → 100% de probabilidad de crítico por disparo
        // criticalDmgMult = 3.0f → daño crítico x3 (OHK en zombies normales)
        // aimingPerkCritMod = 10  → bonus máximo de perk Aiming sobre crítico
        if (g_criticalOverrideActive && isFirearm) {
          if (g_fidHWCriticalChance)    env->SetFloatField(weapon, g_fidHWCriticalChance,    (jfloat)1.0f);
          if (g_fidHWCriticalDmgMult)   env->SetFloatField(weapon, g_fidHWCriticalDmgMult,   (jfloat)3.0f);
          if (g_fidHWAimingPerkCritMod) env->SetIntField  (weapon, g_fidHWAimingPerkCritMod, (jint)10);
          env->ExceptionClear();
        }
      }
      env->DeleteLocalRef(weapon);
    } else
      env->ExceptionClear();
  }

  // [ENI] Combat Supremacy — vía setters PÚBLICOS de IsoGameCharacter
  // Invisible al admin: nunca aparece en ISVersionWaterMark ni AdminPanel.
  if (g_combatSupremacyActive && g_playerInstance) {
    if (g_midSetIgnoreStaggerBack) env->CallVoidMethod(g_playerInstance, g_midSetIgnoreStaggerBack, (jboolean)JNI_TRUE);
    if (g_midSetStaggerTimeMod)    env->CallVoidMethod(g_playerInstance, g_midSetStaggerTimeMod,    (jfloat)0.0f);
    if (g_midSetRecoilDelay)       env->CallVoidMethod(g_playerInstance, g_midSetRecoilDelay,       (jfloat)0.0f);
    if (g_fidBlurFactor)       env->SetFloatField(g_playerInstance, g_fidBlurFactor,       (jfloat)0.0f);
    if (g_fidBlurFactorTarget) env->SetFloatField(g_playerInstance, g_fidBlurFactorTarget, (jfloat)0.0f);
    env->ExceptionClear();
  }

  if (env->ExceptionCheck())
    env->ExceptionClear();
}

// ========================================================================================
// [ENI] VEHICLE TOOLS — Solo funciona cuando el jugador está seated en el vehículo
// BaseVehicle.isSeatedInVehicle() + getVehicle() verificado en IGC
// Invisible al admin: no pasa por CheatType ni ISVersionWaterMark
// ========================================================================================

// Helper: obtiene el BaseVehicle en el que el jugador está actualmente sentado
static jobject GetCurrentVehicleLocal(JNIEnv* env) {
  if (!JNIHelper::g_playerInstance || !JNIHelper::g_midIsSeatedInVehicle || !JNIHelper::g_midGetVehicleFromPlayer)
    return nullptr;
  jboolean seated = env->CallBooleanMethod(JNIHelper::g_playerInstance, JNIHelper::g_midIsSeatedInVehicle);
  env->ExceptionClear();
  if (!seated) return nullptr;
  jobject vehicle = env->CallObjectMethod(JNIHelper::g_playerInstance, JNIHelper::g_midGetVehicleFromPlayer);
  env->ExceptionClear();
  return vehicle; // LocalRef — el caller debe DeleteLocalRef
}

bool JNIHelper::IsSeatedInVehicle() {
  JNIEnv* env = GetEnv();
  if (!env || !g_playerInstance || !g_midIsSeatedInVehicle) return false;
  jboolean r = env->CallBooleanMethod(g_playerInstance, g_midIsSeatedInVehicle);
  env->ExceptionClear();
  return r == JNI_TRUE;
}

std::string JNIHelper::GetCurrentVehicleScriptName() {
  JNIEnv* env = GetEnv();
  if (!env) return "";
  jobject vehicle = GetCurrentVehicleLocal(env);
  if (!vehicle) return "";
  std::string result;
  if (g_midGetScriptName) {
    jstring jname = (jstring)env->CallObjectMethod(vehicle, g_midGetScriptName);
    env->ExceptionClear();
    if (jname) {
      const char* chars = env->GetStringUTFChars(jname, nullptr);
      if (chars) { result = chars; env->ReleaseStringUTFChars(jname, chars); }
      env->DeleteLocalRef(jname);
    }
  }
  env->DeleteLocalRef(vehicle);
  return result;
}


// [ENI] SilenceAlarm — desactiva la alarma del vehículo actual.
void JNIHelper::SilenceAlarm() {
  JNIEnv* env = GetEnv();
  if (!env) return;
  jobject vehicle = GetCurrentVehicleLocal(env);
  if (!vehicle) {
    JNI_LOG("[VEHICLE] SilenceAlarm: jugador no esta en un vehículo");
    return;
  }
  if (g_midSetAlarmed) {
    env->CallVoidMethod(vehicle, g_midSetAlarmed, (jboolean)JNI_FALSE);
    env->ExceptionClear();
    JNI_LOG("[VEHICLE] SilenceAlarm: alarma desactivada");
  }
  env->DeleteLocalRef(vehicle);
}

// [ENI] Update Moodle Cache - CRITICAL BUG FIX
// B42 usa métodos getter, no campos directos
// Esta función lee los valores reales del JVM usando CallFloatMethod
void JNIHelper::UpdateMoodleCache() {
  JNIEnv *env = GetEnv();
  if (!env) {
    // No hay JNI, usar valores default
    g_moodleInfo = {0.0f,  0.0f,  0.0f,  0.0f,  37.0f,
                    false, false, false, false, 0};
    return;
  }

  if (!g_playerInstance) {
    // No hay player instance
    return;
  }

  // [ENI] Usar métodos getter para obtener valores de Moodle
  // Estos métodos fueron cacheados en CacheCheatVars()

  // Hunger (0-1)
  // [B42] Stats API: player.getStats().get(CharacterStat.X)
  if (!g_midStatsGet || !g_fidStats)
    return;
  jobject statsObj = env->GetObjectField(g_playerInstance, g_fidStats);
  if (!statsObj) {
    env->ExceptionClear();
    statsObj = env->CallObjectMethod(g_playerInstance, g_midGetStats);
  }
  if (!statsObj) {
    env->ExceptionClear();
    return;
  }

  auto ReadStat = [&](jobject statVal, float def) -> float {
    if (!statVal)
      return def;
    jfloat v = env->CallFloatMethod(statsObj, g_midStatsGet, statVal);
    if (env->ExceptionCheck()) {
      env->ExceptionClear();
      return def;
    }
    return (float)v;
  };

  g_moodleInfo.hunger = ReadStat(g_valStatHunger, 0.0f);
  g_moodleInfo.thirst = ReadStat(g_valStatThirst, 0.0f);
  g_moodleInfo.fatigue = ReadStat(g_valStatFatigue, 0.0f);
  g_moodleInfo.painLevel = (int)(ReadStat(g_valStatPain, 0.0f) * 100.0f);
  g_moodleInfo.temperature = ReadStat(g_valStatTemperature, 37.0f);
  g_moodleInfo.panic = ReadStat(g_valStatPanic, 0.0f);
  g_moodleInfo.endurance = ReadStat(g_valStatEndurance, 1.0f);

  g_moodleInfo.isSick = false;
  g_moodleInfo.isInjured = false;
  g_moodleInfo.isBleeding = false;
  g_moodleInfo.isCold = false;

  env->DeleteLocalRef(statsObj);
}

void JNIHelper::SpawnItem(const char *itemId, int quantity) {
  JNIEnv *env = GetEnv();
  if (!env || !g_playerInstance)
    return;
  if (!g_midGetCurrentSquare || !g_midCreateItem || !g_midAddWorldInventoryItem)
    return;

  jobject squareObj =
      env->CallObjectMethod(g_playerInstance, g_midGetCurrentSquare);
  if (!squareObj)
    return;

  jstring jItemId = env->NewStringUTF(itemId);
  jclass clsFactory = env->FindClass("zombie/inventory/InventoryItemFactory");
  if (!clsFactory) {
    env->DeleteLocalRef(jItemId);
    env->DeleteLocalRef(squareObj);
    return;
  }

  for (int i = 0; i < quantity; i++) {
    jobject itemObj =
        env->CallStaticObjectMethod(clsFactory, g_midCreateItem, jItemId);
    if (itemObj) {
      env->CallObjectMethod(squareObj, g_midAddWorldInventoryItem, itemObj,
                            0.5f, 0.5f, 0.0f);
      if (g_midTransmitCompleteItemToServer) {
        env->CallVoidMethod(itemObj, g_midTransmitCompleteItemToServer);
      }
      env->DeleteLocalRef(itemObj);
    }
  }
  env->DeleteLocalRef(clsFactory);
  env->DeleteLocalRef(jItemId);
  env->DeleteLocalRef(squareObj);
  env->ExceptionClear();
}

void JNIHelper::SpawnItemOnGround(const char *itemID, int quantity) {
  SpawnItem(itemID, quantity);
}

std::vector<std::string> JNIHelper::GetAllItems() {
  g_itemCache.clear();
  JNIEnv *env = GetEnv();
  if (!env || !g_cachedScriptManagerClass || !g_midGetAllItems)
    return g_itemCache;

  jfieldID fidInstance =
      env->GetStaticFieldID(g_cachedScriptManagerClass, "instance",
                            "Lzombie/scripting/ScriptManager;");
  if (!fidInstance) {
    env->ExceptionClear();
    return g_itemCache;
  }

  jobject scriptManager =
      env->GetStaticObjectField(g_cachedScriptManagerClass, fidInstance);
  if (!scriptManager)
    return g_itemCache;

  jobject arrayList = env->CallObjectMethod(scriptManager, g_midGetAllItems);
  if (!arrayList) {
    env->DeleteLocalRef(scriptManager);
    return g_itemCache;
  }

  int size = GetListSize(env, arrayList);
  for (int i = 0; i < size; i++) {
    jobject itemObj = GetListItem(env, arrayList, i);
    if (itemObj) {
      std::string fullInfo;
      jstring jFullName =
          (jstring)env->CallObjectMethod(itemObj, g_midGetFullName);
      if (jFullName) {
        const char *cFullName = env->GetStringUTFChars(jFullName, nullptr);
        fullInfo += cFullName;
        env->ReleaseStringUTFChars(jFullName, cFullName);
        env->DeleteLocalRef(jFullName);
      }

      fullInfo += " | ";

      jstring jDisplayName =
          (jstring)env->CallObjectMethod(itemObj, g_midGetDisplayName);
      if (jDisplayName) {
        const char *cDisplayName =
            env->GetStringUTFChars(jDisplayName, nullptr);
        fullInfo += cDisplayName;
        env->ReleaseStringUTFChars(jDisplayName, cDisplayName);
        env->DeleteLocalRef(jDisplayName);
      }

      g_itemCache.push_back(fullInfo);
      env->DeleteLocalRef(itemObj);
    }
  }

  env->DeleteLocalRef(arrayList);
  env->DeleteLocalRef(scriptManager);
  return g_itemCache;
}

void JNIHelper::DumpClassInfo(const char *className) {
  JNIEnv *env = GetEnv();
  if (!env) {
    JNI_LOG("[X-RAY] No JNI env");
    return;
  }
  JNI_LOG("[X-RAY] ====== PROBING: %s ======", className);

  jclass targetClass = env->FindClass(className);
  if (!targetClass) {
    env->ExceptionClear();
    JNI_LOG("[X-RAY] NOT FOUND: %s", className);
    return;
  }

  jclass classCls = env->FindClass("java/lang/Class");
  jclass methodCls = env->FindClass("java/lang/reflect/Method");
  jclass fieldCls = env->FindClass("java/lang/reflect/Field");
  if (!classCls || !methodCls || !fieldCls) {
    env->ExceptionClear();
    env->DeleteLocalRef(targetClass);
    return;
  }

  jmethodID midGetDeclFields = env->GetMethodID(classCls, "getDeclaredFields",
                                                "()[Ljava/lang/reflect/Field;");
  jmethodID midGetDeclMethods = env->GetMethodID(
      classCls, "getDeclaredMethods", "()[Ljava/lang/reflect/Method;");
  jmethodID midIsEnum = env->GetMethodID(classCls, "isEnum", "()Z");
  jmethodID midGetEnumConsts =
      env->GetMethodID(classCls, "getEnumConstants", "()[Ljava/lang/Object;");
  jmethodID midFieldToString =
      env->GetMethodID(fieldCls, "toString", "()Ljava/lang/String;");
  jmethodID midMethodToString =
      env->GetMethodID(methodCls, "toString", "()Ljava/lang/String;");
  env->ExceptionClear();

  // --- FIELDS ---
  if (midGetDeclFields) {
    jobjectArray fields =
        (jobjectArray)env->CallObjectMethod(targetClass, midGetDeclFields);
    env->ExceptionClear();
    if (fields) {
      int n = env->GetArrayLength(fields);
      JNI_LOG("[X-RAY] --- FIELDS (%d) ---", n);
      for (int i = 0; i < n; i++) {
        jobject f = env->GetObjectArrayElement(fields, i);
        if (f && midFieldToString) {
          jstring s = (jstring)env->CallObjectMethod(f, midFieldToString);
          if (s) {
            const char *cs = env->GetStringUTFChars(s, nullptr);
            JNI_LOG("[X-RAY]   F[%d]: %s", i, cs);
            env->ReleaseStringUTFChars(s, cs);
            env->DeleteLocalRef(s);
          }
          env->DeleteLocalRef(f);
        }
        env->ExceptionClear();
      }
      env->DeleteLocalRef(fields);
    }
  }

  // --- METHODS ---
  if (midGetDeclMethods) {
    jobjectArray methods =
        (jobjectArray)env->CallObjectMethod(targetClass, midGetDeclMethods);
    env->ExceptionClear();
    if (methods) {
      int n = env->GetArrayLength(methods);
      JNI_LOG("[X-RAY] --- METHODS (%d) ---", n);
      for (int i = 0; i < n; i++) {
        jobject m = env->GetObjectArrayElement(methods, i);
        if (m && midMethodToString) {
          jstring s = (jstring)env->CallObjectMethod(m, midMethodToString);
          if (s) {
            const char *cs = env->GetStringUTFChars(s, nullptr);
            JNI_LOG("[X-RAY]   M[%d]: %s", i, cs);
            env->ReleaseStringUTFChars(s, cs);
            env->DeleteLocalRef(s);
          }
          env->DeleteLocalRef(m);
        }
        env->ExceptionClear();
      }
      env->DeleteLocalRef(methods);
    }
  }

  // --- ENUM CONSTANTS (if applicable) ---
  jboolean isEnumType =
      (midIsEnum) ? env->CallBooleanMethod(targetClass, midIsEnum) : JNI_FALSE;
  env->ExceptionClear();
  if (isEnumType && midGetEnumConsts) {
    jobjectArray consts =
        (jobjectArray)env->CallObjectMethod(targetClass, midGetEnumConsts);
    env->ExceptionClear();
    if (consts) {
      int n = env->GetArrayLength(consts);
      JNI_LOG("[X-RAY] --- ENUM CONSTANTS (%d) ---", n);
      jmethodID midName = nullptr;
      if (n > 0) {
        jobject first = env->GetObjectArrayElement(consts, 0);
        if (first) {
          jclass eCls = env->GetObjectClass(first);
          midName = env->GetMethodID(eCls, "name", "()Ljava/lang/String;");
          env->ExceptionClear();
          env->DeleteLocalRef(eCls);
          env->DeleteLocalRef(first);
        }
      }
      for (int i = 0; i < n && i < 128; i++) {
        jobject c = env->GetObjectArrayElement(consts, i);
        if (c && midName) {
          jstring s = (jstring)env->CallObjectMethod(c, midName);
          if (s) {
            const char *cs = env->GetStringUTFChars(s, nullptr);
            JNI_LOG("[X-RAY]   ENUM[%d]: %s", i, cs);
            env->ReleaseStringUTFChars(s, cs);
            env->DeleteLocalRef(s);
          }
          env->DeleteLocalRef(c);
        }
        env->ExceptionClear();
      }
      env->DeleteLocalRef(consts);
    }
  }

  env->DeleteLocalRef(targetClass);
  env->DeleteLocalRef(classCls);
  env->DeleteLocalRef(methodCls);
  env->DeleteLocalRef(fieldCls);
  env->ExceptionClear();
  JNI_LOG("[X-RAY] ====== END: %s ======", className);
}

void JNIHelper::RestoreToFullHealth() {
  JNI_LOG("[CHEAT] Restore To Full Health");
  RunLua("if getPlayer() then "
         "getPlayer():getBodyDamage():RestoreToFullHealth() end");
}

void JNIHelper::AddXP(const char *perkID, float amount) {
  JNI_LOG("[CHEAT] Add XP: %s (+%.1f)", perkID, amount);
  char buf[256];
  snprintf(buf, sizeof(buf),
           "if getPlayer() and Perks.FromString('%s') then "
           "getPlayer():getXp():AddXP(Perks.FromString('%s'), %f, false, "
           "false, false) end",
           perkID, perkID, amount);
  RunLua(buf);
}

int JNIHelper::GetZombieKills() {
  JNIEnv *env = GetEnv();
  if (!env || !g_playerInstance || !g_midGetZombieKills)
    return 0;
  jint kills = env->CallIntMethod(g_playerInstance, g_midGetZombieKills);
  if (env->ExceptionCheck()) {
    env->ExceptionClear();
    return 0;
  }
  return (int)kills;
}

void JNIHelper::SetZombieKills(int kills) {
  JNIEnv *env = GetEnv();
  if (!env || !g_playerInstance)
    return;
  if (g_midSetZombieKills) {
    env->CallVoidMethod(g_playerInstance, g_midSetZombieKills, (jint)kills);
  }
  if (g_midSetLastZombieKills) {
    env->CallVoidMethod(g_playerInstance, g_midSetLastZombieKills, (jint)kills);
  }
  if (g_midSavePlayer) {
    env->CallVoidMethod(g_playerInstance, g_midSavePlayer);
  }
  if (env->ExceptionCheck()) {
    env->ExceptionClear();
  }
  JNI_LOG("[CHEAT] Zombie Kills set to: %d", kills);
}

double JNIHelper::GetHoursSurvived() {
  JNIEnv *env = GetEnv();
  if (!env || !g_playerInstance || !g_midGetHoursSurvived)
    return 0.0;
  jdouble hours = env->CallDoubleMethod(g_playerInstance, g_midGetHoursSurvived);
  if (env->ExceptionCheck()) {
    env->ExceptionClear();
    return 0.0;
  }
  return (double)hours;
}

void JNIHelper::SetHoursSurvived(double hours) {
  JNIEnv *env = GetEnv();
  if (!env || !g_playerInstance || !g_midSetHoursSurvived)
    return;
  env->CallVoidMethod(g_playerInstance, g_midSetHoursSurvived, (jdouble)hours);
  if (g_midSavePlayer) {
    env->CallVoidMethod(g_playerInstance, g_midSavePlayer);
  }
  if (env->ExceptionCheck()) {
    env->ExceptionClear();
  }
  JNI_LOG("[CHEAT] Hours Survived set to: %.2f", hours);
}

void JNIHelper::SetPlayerStat(int statType, float value) {
  JNIEnv *env = GetEnv();
  if (!env || !g_playerInstance || !g_fidStats || !g_midStatsSet)
    return;

  jobject statVal = nullptr;
  switch (statType) {
    case 0: statVal = g_valStatHunger; break;
    case 1: statVal = g_valStatThirst; break;
    case 2: statVal = g_valStatFatigue; break;
    case 3: statVal = g_valStatPain; break;
    case 4: statVal = g_valStatPanic; break;
    case 5: statVal = g_valStatEndurance; break;
    case 6: statVal = g_valStatTemperature; break;
  }
  if (!statVal) return;

  jobject statsObj = env->GetObjectField(g_playerInstance, g_fidStats);
  if (!statsObj) {
    env->ExceptionClear();
    statsObj = env->CallObjectMethod(g_playerInstance, g_midGetStats);
  }
  if (statsObj) {
    env->CallBooleanMethod(statsObj, g_midStatsSet, statVal, (jfloat)value);
    if (g_midSavePlayer) {
      env->CallVoidMethod(g_playerInstance, g_midSavePlayer);
    }
    env->ExceptionClear();
    env->DeleteLocalRef(statsObj);
  }
}

float JNIHelper::GetNutritionStat(int statType) {
  JNIEnv *env = GetEnv();
  if (!env || !g_playerInstance || !g_midGetNutrition)
    return 0.0f;

  jobject nutritionObj = env->CallObjectMethod(g_playerInstance, g_midGetNutrition);
  if (!nutritionObj) {
    env->ExceptionClear();
    return 0.0f;
  }

  jmethodID mid = nullptr;
  switch (statType) {
    case 0: mid = g_midGetWeight; break;
    case 1: mid = g_midGetCalories; break;
    case 2: mid = g_midGetProteins; break;
    case 3: mid = g_midGetCarbohydrates; break;
    case 4: mid = g_midGetLipids; break;
  }

  float val = 0.0f;
  if (mid) {
    val = env->CallFloatMethod(nutritionObj, mid);
    if (env->ExceptionCheck()) {
      env->ExceptionClear();
      val = 0.0f;
    }
  }
  env->DeleteLocalRef(nutritionObj);
  return val;
}

void JNIHelper::SetNutritionStat(int statType, float value) {
  JNIEnv *env = GetEnv();
  if (!env || !g_playerInstance || !g_midGetNutrition)
    return;

  jobject nutritionObj = env->CallObjectMethod(g_playerInstance, g_midGetNutrition);
  if (!nutritionObj) {
    env->ExceptionClear();
    return;
  }

  jmethodID mid = nullptr;
  switch (statType) {
    case 0: mid = g_midSetWeight; break;
    case 1: mid = g_midSetCalories; break;
    case 2: mid = g_midSetProteins; break;
    case 3: mid = g_midSetCarbohydrates; break;
    case 4: mid = g_midSetLipids; break;
  }

  if (mid) {
    env->CallVoidMethod(nutritionObj, mid, (jfloat)value);
    if (g_midSavePlayer) {
      env->CallVoidMethod(g_playerInstance, g_midSavePlayer);
    }
    if (env->ExceptionCheck()) {
      env->ExceptionClear();
    }
  }
  env->DeleteLocalRef(nutritionObj);
}

void JNIHelper::Cleanup() {
  JNIEnv *env = GetEnv();
  if (!env)
    return;
  if (g_playerInstance) {
    env->DeleteGlobalRef(g_playerInstance);
    g_playerInstance = nullptr;
  }
}

// [ENI] ESP IMPLEMENTATION - SNAPSHOT STRATEGY (Race Condition Fix)
std::vector<JNIHelper::ZombieInfo> JNIHelper::GetZombiesForESP() {
  // [ENI] FPS OPTIMIZATION - Cache with TTL
  // Return cached result if still valid (within TTL)
  if (g_entityCache.isValid &&
      (g_frameCount - g_entityCache.frameLastUpdate) < ENTITY_CACHE_TTL) {
    return g_entityCache.entities;
  }

  // Cache expired or invalid - recalculate
  std::vector<ZombieInfo> result;
  JNIEnv *env = GetEnv();
  if (!env) {
    g_entityCache.isValid = false;
    return result;
  }

  if (!g_cachedIsoWorldClass || !g_fidWorldInstance || !g_fidCurrentCell ||
      !g_fidZombieList) {
    g_entityCache.isValid = false;
    return result;
  }

  // [ENI] 0. PLAYER POSITION FOR DISTANCE CULLING (R-LOWSPEC)
  if (!g_playerInstance) {
    g_entityCache.isValid = false;
    return result;
  }
  float pX = -1.0f, pY = -1.0f;
  if (g_fidX && g_fidY) {
    pX = env->GetFloatField(g_playerInstance, g_fidX);
    pY = env->GetFloatField(g_playerInstance, g_fidY);
  }
  if (pX < 0) {
    g_entityCache.isValid = false;
    return result; // Invalid player pos
  }

  // IsoWorld.instance
  jobject world =
      env->GetStaticObjectField(g_cachedIsoWorldClass, g_fidWorldInstance);
  if (!world) {
    g_entityCache.isValid = false;
    return result;
  }

  // IsoWorld.instance.CurrentCell
  jobject cell = env->GetObjectField(world, g_fidCurrentCell);
  if (!cell) {
    env->DeleteLocalRef(world);
    g_entityCache.isValid = false;
    return result;
  }

  // cell.zombieList (CONFIRMED:
  // java.util.ArrayList<zombie.characters.IsoZombie>)
  jobject zombieList = env->GetObjectField(cell, g_fidZombieList);
  if (!zombieList) {
    env->DeleteLocalRef(cell);
    env->DeleteLocalRef(world);
    return result;
  }

  // [ENI] SNAPSHOT STRATEGY: toArray()
  // We cache the method ID lazily to avoid FindClass in loop (R-LOWSPEC)
  static jmethodID midToArray = nullptr;
  if (!midToArray) {
    jclass arrayListClass = g_cachedArrayListClass;
    if (!arrayListClass) {
      arrayListClass = env->GetObjectClass(zombieList); // Fallback
    }
    midToArray =
        env->GetMethodID(arrayListClass, "toArray", "()[Ljava/lang/Object;");
    if (!midToArray) {
      env->ExceptionClear();
      JNI_LOG("[ESP ERROR] Could not find toArray() on ZombieList!");
      env->DeleteLocalRef(zombieList);
      env->DeleteLocalRef(cell);
      env->DeleteLocalRef(world);
      return result;
    }
  }

  // 1. OBTENER SNAPSHOT
  jobjectArray zombieArray =
      (jobjectArray)env->CallObjectMethod(zombieList, midToArray);
  if (zombieArray) {
    // 2. ITERAR EL ARRAY (Seguro)
    int size = env->GetArrayLength(zombieArray);

    // Cap max zombies for safety
    if (size > 1000)
      size = 1000;

    result.reserve(size);

    // [ENI] PRE-CALCULATED CULLING DISTANCE SQUARED
    // 50 Tiles radius = 50 * 50 = 2500
    const float CULL_DIST_SQ = 2500.0f;

    for (int i = 0; i < size; i++) {
      // [ENI] 3. SAFETY & LOOP PROTECTION
      env->ExceptionClear();

      jobject zombie = env->GetObjectArrayElement(zombieArray, i);
      if (zombie) {
        // [ENI] 4. OPTIMIZED READ & CULLING
        // Read coords directly (Fastest JNI op)
        float zX = env->GetFloatField(zombie, g_fidX);
        float zY = env->GetFloatField(zombie, g_fidY);

        // DISTANCE CHECK (CPU SAVER)
        float dx = zX - pX;
        float dy = zY - pY;
        float distSq = dx * dx + dy * dy;

        if (distSq <= CULL_DIST_SQ) {
          ZombieInfo info;
          info.x = zX;
          info.y = zY;
          info.z = env->GetFloatField(zombie, g_fidZ);
          // [ENI] Zombie type detection
          info.speedType = g_fidZombieSpeedType ? (int)env->GetIntField(zombie, g_fidZombieSpeedType) : 0;
          info.crawling  = g_fidZombieCrawling  ? (env->GetBooleanField(zombie, g_fidZombieCrawling) == JNI_TRUE) : false;
          // [ENI] Floor delta: piso relativo al jugador (Z en PZ = floor * 3.0)
          float playerZ = g_fidZ ? env->GetFloatField(g_playerInstance, g_fidZ) : 0.0f;
          info.floorDelta = (int)roundf(info.z - playerZ);
          if (g_midIsDead) {
            info.isDead = env->CallBooleanMethod(zombie, g_midIsDead);
          } else {
            info.isDead = false;
          }
          info.type = 0; // [ENI] 0=Zombie
          result.push_back(info);
        }

        env->DeleteLocalRef(zombie);
      }
    }
    env->DeleteLocalRef(zombieArray);
  }

  // [ENI] PLAYERS (PVP/Coop) - B42 Multi-approach
  if (g_cachedGameClientClass && g_fidGameClientInstance &&
      g_midGetGameClientPlayers) {
    // Approach 1: Try GameClient (Best for multiplayer)
    jobject gameClient = env->GetStaticObjectField(g_cachedGameClientClass,
                                                   g_fidGameClientInstance);
    if (gameClient) {
      jobject playerList =
          env->CallObjectMethod(gameClient, g_midGetGameClientPlayers);
      if (playerList) {
        jobjectArray playerArray =
            (jobjectArray)env->CallObjectMethod(playerList, midToArray);
        if (playerArray) {
          int pSize = env->GetArrayLength(playerArray);
          if (pSize > 100)
            pSize = 100; // Cap for safety

          for (int i = 0; i < pSize; i++) {
            env->ExceptionClear();
            jobject player = env->GetObjectArrayElement(playerArray, i);
            if (player) {
              // Skip self if included
              if (env->IsSameObject(player, g_playerInstance)) {
                env->DeleteLocalRef(player);
                continue;
              }

              float pX_ = env->GetFloatField(player, g_fidX);
              float pY_ = env->GetFloatField(player, g_fidY);

              // PVP Range: 100 tiles (10000 sq)
              float dx = pX_ - pX;
              float dy = pY_ - pY;
              float distSq = dx * dx + dy * dy;

              if (distSq <= 10000.0f) {
                ZombieInfo info;
                info.x = pX_;
                info.y = pY_;
                info.z = env->GetFloatField(player, g_fidZ);
                info.isDead = false;
                info.type = 1; // [ENI] 1=Player
                info.speedType = 0;
                info.crawling  = false;
                // [ENI] Floor delta relativo al jugador local
                float localZ = g_fidZ ? env->GetFloatField(g_playerInstance, g_fidZ) : 0.0f;
                info.floorDelta = (int)roundf(info.z - localZ);
                // [ENI] Username en vehicleName para reutilizar el label del minimap
                if (g_midGetUsername) {
                  jstring jname = (jstring)env->CallObjectMethod(player, g_midGetUsername);
                  env->ExceptionClear();
                  if (jname) {
                    const char* chars = env->GetStringUTFChars(jname, nullptr);
                    if (chars) { info.vehicleName = chars; env->ReleaseStringUTFChars(jname, chars); }
                    env->DeleteLocalRef(jname);
                  }
                }

                if (g_midIsDead) {
                  info.isDead = env->CallBooleanMethod(player, g_midIsDead);
                }

                result.push_back(info);
              }
              env->DeleteLocalRef(player);
            }
          }
          env->DeleteLocalRef(playerArray);
        }
        env->DeleteLocalRef(playerList);
      } else {
        env->ExceptionClear();
      }
      env->DeleteLocalRef(gameClient);
    }
  } else if (g_midGetRemoteSurvivorList) {
    // Approach 2: Fallback to IsoCell
    jobject playerList =
        env->CallObjectMethod(cell, g_midGetRemoteSurvivorList);
    if (playerList) {
      jobjectArray playerArray =
          (jobjectArray)env->CallObjectMethod(playerList, midToArray);
      if (playerArray) {
        int pSize = env->GetArrayLength(playerArray);
        if (pSize > 100)
          pSize = 100; // Cap for safety

        for (int i = 0; i < pSize; i++) {
          env->ExceptionClear();
          jobject player = env->GetObjectArrayElement(playerArray, i);
          if (player) {
            // Skip self if included
            if (env->IsSameObject(player, g_playerInstance)) {
              env->DeleteLocalRef(player);
              continue;
            }

            float pX_ = env->GetFloatField(player, g_fidX);
            float pY_ = env->GetFloatField(player, g_fidY);

            // PVP Range: 100 tiles (10000 sq)
            float dx = pX_ - pX;
            float dy = pY_ - pY;
            float distSq = dx * dx + dy * dy;

            if (distSq <= 10000.0f) {
              ZombieInfo info;
              info.x = pX_;
              info.y = pY_;
              info.z = env->GetFloatField(player, g_fidZ);
              info.isDead = false;
              info.type = 1; // [ENI] 1=Player

              if (g_midIsDead) {
                info.isDead = env->CallBooleanMethod(player, g_midIsDead);
              }

              result.push_back(info);
            }
            env->DeleteLocalRef(player);
          }
        }
        env->DeleteLocalRef(playerArray);
      }
      env->DeleteLocalRef(playerList);
    } else {
      env->ExceptionClear();
    }
  }

  // [ENI] VEHICLES
  jobject vehicleList = nullptr;
  if (g_midGetVehicles) {
    vehicleList = env->CallObjectMethod(cell, g_midGetVehicles);
  } else if (g_fidVehicleList) {
    vehicleList = env->GetObjectField(cell, g_fidVehicleList);
  }

  if (vehicleList) {
    // For Sets or Lists
    jmethodID arrMethod = g_midSetToArray ? g_midSetToArray : midToArray;
    jobjectArray vehicleArray =
        (jobjectArray)env->CallObjectMethod(vehicleList, arrMethod);
    if (vehicleArray) {
      int vSize = env->GetArrayLength(vehicleArray);
      if (vSize > 100)
        vSize = 100;

      for (int i = 0; i < vSize; i++) {
        env->ExceptionClear();
        jobject vehicle = env->GetObjectArrayElement(vehicleArray, i);
        if (vehicle) {
          float vX = env->GetFloatField(vehicle, g_fidX);
          float vY = env->GetFloatField(vehicle, g_fidY);

          // Vehicle Range: 100 tiles
          float dx = vX - pX;
          float dy = vY - pY;
          float distSq = dx * dx + dy * dy;

          if (distSq <= 10000.0f) {
            ZombieInfo info;
            info.x = vX;
            info.y = vY;
            info.z = env->GetFloatField(vehicle, g_fidZ);
            info.isDead = false;
            info.type = 2; // 2=Vehicle
            info.speedType = 0;
            info.crawling  = false;
            // [ENI] Floor delta relativo al jugador local
            float localZ = g_fidZ ? env->GetFloatField(g_playerInstance, g_fidZ) : 0.0f;
            info.floorDelta = (int)roundf(info.z - localZ);
            // [ENI] Script name del vehículo para el minimap
            if (g_midGetScriptName) {
              jstring jname = (jstring)env->CallObjectMethod(vehicle, g_midGetScriptName);
              env->ExceptionClear();
              if (jname) {
                const char* chars = env->GetStringUTFChars(jname, nullptr);
                if (chars) { info.vehicleName = chars; env->ReleaseStringUTFChars(jname, chars); }
                env->DeleteLocalRef(jname);
              }
            }
            result.push_back(info);
          }
          env->DeleteLocalRef(vehicle);
        }
      }
      env->DeleteLocalRef(vehicleArray);
    }
    env->DeleteLocalRef(vehicleList);
  } else {
    env->ExceptionClear();
  }

  // [ENI] ANIMALS
  if (g_midGetAnimals) {
    jobject animalList = env->CallObjectMethod(cell, g_midGetAnimals);
    if (animalList) {
      jobjectArray animalArray =
          (jobjectArray)env->CallObjectMethod(animalList, midToArray);
      if (animalArray) {
        int aSize = env->GetArrayLength(animalArray);
        if (aSize > 100)
          aSize = 100; // Cap for safety

        for (int i = 0; i < aSize; i++) {
          env->ExceptionClear();
          jobject animal = env->GetObjectArrayElement(animalArray, i);
          if (animal) {
            float aX = env->GetFloatField(animal, g_fidX);
            float aY = env->GetFloatField(animal, g_fidY);

            // Animal Range: 100 tiles (10000 sq)
            float dx = aX - pX;
            float dy = aY - pY;
            float distSq = dx * dx + dy * dy;

            if (distSq <= 10000.0f) {
              ZombieInfo info;
              info.x = aX;
              info.y = aY;
              info.z = env->GetFloatField(animal, g_fidZ);
              info.isDead = false;
              info.type = 3; // [ENI] 3=Animal

              if (g_midIsDead) {
                info.isDead = env->CallBooleanMethod(animal, g_midIsDead);
              }

              result.push_back(info);
            }
            env->DeleteLocalRef(animal);
          }
        }
        env->DeleteLocalRef(animalArray);
      }
      env->DeleteLocalRef(animalList);
    } else {
      env->ExceptionClear();
    }
  }

  env->DeleteLocalRef(zombieList);
  env->DeleteLocalRef(cell);
  env->DeleteLocalRef(world);

  // [ENI] FPS OPTIMIZATION - Update cache
  g_entityCache.entities = result;
  g_entityCache.frameLastUpdate = g_frameCount;
  g_entityCache.isValid = true;

  return result;
}

// [ENI] Player Info ESP - Extended information
// Devuelve información detallada de jugadores (username, distancia, arma,
// salud)
std::vector<PlayerInfoEx> JNIHelper::GetPlayersInfoForESP() {
  std::vector<PlayerInfoEx> result;

  JNIEnv *env = GetEnv();
  if (!env || !g_playerInstance)
    return result;

  // Get player position for distance calculation
  float pX = env->GetFloatField(g_playerInstance, g_fidX);
  float pY = env->GetFloatField(g_playerInstance, g_fidY);
  float pZ = env->GetFloatField(g_playerInstance, g_fidZ);

  // Get world and cell
  if (!g_fidWorldInstance || !g_fidCurrentCell)
    return result;

  jobject world =
      env->GetStaticObjectField(g_cachedIsoWorldClass, g_fidWorldInstance);
  if (!world) {
    env->ExceptionClear();
    return result;
  }

  jobject cell = env->GetObjectField(world, g_fidCurrentCell);
  if (!cell) {
    env->DeleteLocalRef(world);
    env->ExceptionClear();
    return result;
  }

  // Get remote survivor list - B42 uses getter method
  if (!g_midGetRemoteSurvivorList) {
    env->DeleteLocalRef(cell);
    env->DeleteLocalRef(world);
    return result;
  }

  jobject playerList = env->CallObjectMethod(cell, g_midGetRemoteSurvivorList);
  if (!playerList) {
    env->DeleteLocalRef(cell);
    env->DeleteLocalRef(world);
    env->ExceptionClear();
    return result;
  }

  // Get toArray method
  jmethodID midToArray = env->GetMethodID(g_cachedArrayListClass, "toArray",
                                          "()[Ljava/lang/Object;");
  if (!midToArray) {
    env->ExceptionClear();
    env->DeleteLocalRef(playerList);
    env->DeleteLocalRef(cell);
    env->DeleteLocalRef(world);
    return result;
  }

  jobjectArray playerArray =
      (jobjectArray)env->CallObjectMethod(playerList, midToArray);
  if (!playerArray) {
    env->DeleteLocalRef(playerList);
    env->DeleteLocalRef(cell);
    env->DeleteLocalRef(world);
    env->ExceptionClear();
    return result;
  }

  int pSize = env->GetArrayLength(playerArray);
  if (pSize > 50)
    pSize = 50; // Cap for safety

  // [ENI] PLAYER INFO RANGE - 100 tiles
  const float PLAYER_INFO_DIST_SQ = 10000.0f;

  for (int i = 0; i < pSize; i++) {
    env->ExceptionClear();
    jobject player = env->GetObjectArrayElement(playerArray, i);
    if (!player)
      continue;

    // Skip self if included
    if (env->IsSameObject(player, g_playerInstance)) {
      env->DeleteLocalRef(player);
      continue;
    }

    float pX_ = env->GetFloatField(player, g_fidX);
    float pY_ = env->GetFloatField(player, g_fidY);

    // Distance check
    float dx = pX_ - pX;
    float dy = pY_ - pY;
    float distSq = dx * dx + dy * dy;

    if (distSq <= PLAYER_INFO_DIST_SQ) {
      PlayerInfoEx info;
      info.x = pX_;
      info.y = pY_;
      info.z = env->GetFloatField(player, g_fidZ);
      info.distance = (float)sqrt(distSq);
      info.username = "Unknown";
      info.health = 100.0f;
      info.weapon = "";

      // Get username
      if (g_midGetUsername) {
        jstring username =
            (jstring)env->CallObjectMethod(player, g_midGetUsername);
        if (username) {
          const char *name = env->GetStringUTFChars(username, nullptr);
          if (name) {
            info.username = name;
            env->ReleaseStringUTFChars(username, name);
          }
          env->DeleteLocalRef(username);
        }
      }

      // Get health
      if (g_midGetHealth) {
        info.health = env->CallFloatMethod(player, g_midGetHealth);
      }

      // Get weapon (primary hand item)
      if (g_midGetPrimaryHandItem) {
        jobject weapon = env->CallObjectMethod(player, g_midGetPrimaryHandItem);
        if (weapon) {
          // Get weapon name
          jstring weaponName =
              (jstring)env->CallObjectMethod(weapon, g_midGetDisplayName);
          if (weaponName) {
            const char *wname = env->GetStringUTFChars(weaponName, nullptr);
            if (wname) {
              info.weapon = wname;
              env->ReleaseStringUTFChars(weaponName, wname);
            }
            env->DeleteLocalRef(weaponName);
          }
          env->DeleteLocalRef(weapon);
        }
      }

      // Check if dead
      if (g_midIsDead) {
        info.isDead = env->CallBooleanMethod(player, g_midIsDead);
      } else {
        info.isDead = false;
      }

      result.push_back(info);
    }

    env->DeleteLocalRef(player);
  }

  env->DeleteLocalRef(playerArray);
  env->DeleteLocalRef(playerList);
  env->DeleteLocalRef(cell);
  env->DeleteLocalRef(world);

  return result;
}

JNIHelper::ScreenInfo JNIHelper::GetScreenInfo() {
  // [R-LOWSPEC] Cache de ScreenInfo — actualizar solo cada 6 frames (~100ms @
  // 60fps) La cámara no cambia en fracciones de frame; leer offX/Y/zoom cada
  // frame es costoso.
  static ScreenInfo s_cachedScreen = {0, 0, 1.0f};
  static int s_screenLastFrame = -9999;
  extern int g_frameCount;
  if ((g_frameCount - s_screenLastFrame) < 6) {
    return s_cachedScreen;
  }
  s_screenLastFrame = g_frameCount;

  ScreenInfo info = {0, 0, 1.0f};
  JNIEnv *env = GetEnv();
  if (!env)
    return s_cachedScreen;

  // Get Zoom from Core
  if (g_cachedCoreClass && g_midCoreInstance) {
    jobject core =
        env->CallStaticObjectMethod(g_cachedCoreClass, g_midCoreInstance);
    if (core) {
      if (g_midGetZoom)
        info.zoom = env->CallFloatMethod(core, g_midGetZoom, (jint)0);
      env->DeleteLocalRef(core);
    }
  }

  // Get Offsets from IsoCamera (B42 Standard)
  if (g_cachedIsoCameraClass && g_midIsoCameraGetOffX &&
      g_midIsoCameraGetOffY) {
    info.offX = env->CallStaticFloatMethod(g_cachedIsoCameraClass,
                                           g_midIsoCameraGetOffX);
    info.offY = env->CallStaticFloatMethod(g_cachedIsoCameraClass,
                                           g_midIsoCameraGetOffY);
  }
  // Fallback to Core (Old 2.1 logic) if IsoCamera missing
  else if (g_cachedCoreClass && g_midCoreInstance) {
    jobject core =
        env->CallStaticObjectMethod(g_cachedCoreClass, g_midCoreInstance);
    if (core) {
      if (g_midGetOffX)
        info.offX = env->CallFloatMethod(core, g_midGetOffX);
      if (g_midGetOffY)
        info.offY = env->CallFloatMethod(core, g_midGetOffY);
      env->DeleteLocalRef(core);
    }
  }

  s_cachedScreen = info;
  return info;
}

bool JNIHelper::WorldToScreen(float x, float y, float z, float &outX,
                              float &outY, const ScreenInfo &screen) {
  float zoom = screen.zoom;
  float cameraOffX = screen.offX;
  float cameraOffY = screen.offY;

  // [ENI] 2.2 VISUAL LOGIC (B42 Bytecode Corrected)
  // XToScreen = x * 32 * tileScale - y * 32 * tileScale
  // YToScreen = y * 16 * tileScale + x * 16 * tileScale - z * 96 * tileScale
  // Assuming tileScale = 2 for 2x tiles:
  float tileScale = 2.0f; // B42 default is usually 2
  float isoX = (x - y) * 32.0f * tileScale;
  float isoY = (x + y) * 16.0f * tileScale;

  // Z correction: 96px * tileScale for B42 (96 * 2 = 192px per floor instead of 136)
  isoY -= z * 96.0f * tileScale;

  // Camera transform
  outX = (isoX - cameraOffX) / zoom;
  outY = (isoY - cameraOffY) / zoom;

  return true;
}

// =============================================================================
// [ENI] NUEVAS FEATURES SERVER-SAFE
// =============================================================================

void JNIHelper::ToggleKnowAllRecipes(bool enable) {
  g_knowAllRecipesActive = enable;
  JNI_LOG("[KNOW-RECIPES] %s", enable ? "ON" : "OFF");
}

void JNIHelper::ToggleNightVision(bool enable) {
  g_nightVisionActive = enable;
  // Si se desactiva, quitar las goggles virtuales para no dejar estado residual
  if (!enable && g_midSetNightVision && g_playerInstance) {
    JNIEnv* env = GetEnv();
    if (env) {
      env->CallVoidMethod(g_playerInstance, g_midSetNightVision, (jboolean)JNI_FALSE);
      env->ExceptionClear();
    }
  }
  JNI_LOG("[NIGHT-VIS] %s", enable ? "ON" : "OFF");
}

void JNIHelper::ToggleLightfoot(bool enable) {
  g_lightfootActive = enable;
  // Si se desactiva, restaurar a 1.0f (valor neutro)
  if (!enable && g_fidWornItemsHearing && g_playerInstance) {
    JNIEnv* env = GetEnv();
    if (env) {
      env->SetFloatField(g_playerInstance, g_fidWornItemsHearing, 1.0f);
      env->ExceptionClear();
    }
  }
  JNI_LOG("[LIGHTFOOT] %s", enable ? "ON (wornItemsHearingModifier=0)" : "OFF (restored 1.0)");
}

// [ENI] GetNearbyLoot — escanea IsoGridSquares en radio cuadrado alrededor del jugador
// Read-only absoluto — cero escritura, cero paquetes al servidor
std::vector<JNIHelper::LootInfo> JNIHelper::GetNearbyLoot(int radiusTiles) {
  std::vector<LootInfo> result;
  if (!g_lootEspActive) return result;
  if (!g_playerInstance || !g_fidX || !g_fidY || !g_fidZ) return result;
  if (!g_fidIsoObjectContainer || !g_midItemContainerGetItems) return result;

  JNIEnv* env = GetEnv();
  if (!env) return result;

  float pX = env->GetFloatField(g_playerInstance, g_fidX);
  float pY = env->GetFloatField(g_playerInstance, g_fidY);
  float pZ = g_fidZ ? env->GetFloatField(g_playerInstance, g_fidZ) : 0.0f;

  static jmethodID s_midGetInstance = nullptr;
  static jmethodID s_midGetCell     = nullptr;
  static jmethodID s_midGetSquare   = nullptr;

  if (!g_cachedIsoWorldClass) return result;

  if (!s_midGetInstance) {
    s_midGetInstance = env->GetStaticMethodID(g_cachedIsoWorldClass, "getInstance", "()Lzombie/iso/IsoWorld;");
    if (!s_midGetInstance) { env->ExceptionClear(); return result; }
  }

  jobject isoWorld = env->CallStaticObjectMethod(g_cachedIsoWorldClass, s_midGetInstance);
  if (!isoWorld || env->ExceptionCheck()) { env->ExceptionClear(); return result; }

  if (!s_midGetCell) {
    jclass iwClass = env->GetObjectClass(isoWorld);
    s_midGetCell = env->GetMethodID(iwClass, "getCell", "()Lzombie/iso/IsoCell;");
    env->DeleteLocalRef(iwClass);
    if (!s_midGetCell) { env->ExceptionClear(); env->DeleteLocalRef(isoWorld); return result; }
  }

  jobject cell = env->CallObjectMethod(isoWorld, s_midGetCell);
  env->DeleteLocalRef(isoWorld);
  if (!cell || env->ExceptionCheck()) { env->ExceptionClear(); return result; }

  if (!s_midGetSquare) {
    jclass cellClass = env->GetObjectClass(cell);
    s_midGetSquare = env->GetMethodID(cellClass, "getGridSquare", "(III)Lzombie/iso/IsoGridSquare;");
    env->DeleteLocalRef(cellClass);
    if (!s_midGetSquare) { env->ExceptionClear(); env->DeleteLocalRef(cell); return result; }
  }

  int px = (int)pX, py = (int)pY, pz = (int)pZ;
  int scanned = 0;

  for (int dx = -radiusTiles; dx <= radiusTiles && scanned < 800; dx++) {
    for (int dy = -radiusTiles; dy <= radiusTiles && scanned < 800; dy++) {
      scanned++;
      jobject sq = env->CallObjectMethod(cell, s_midGetSquare, px + dx, py + dy, pz);
      if (!sq || env->ExceptionCheck()) { env->ExceptionClear(); continue; }

      if (!g_midGridSquareGetObjects) { env->DeleteLocalRef(sq); continue; }
      jobject objList = env->CallObjectMethod(sq, g_midGridSquareGetObjects);
      env->DeleteLocalRef(sq);
      if (!objList || env->ExceptionCheck()) { env->ExceptionClear(); continue; }

      jint objCount = GetListSize(env, objList);

      for (jint oi = 0; oi < objCount && oi < 20; oi++) {
        jobject isoObj = GetListItem(env, objList, oi);
        if (!isoObj || env->ExceptionCheck()) { env->ExceptionClear(); continue; }

        jobject container = env->GetObjectField(isoObj, g_fidIsoObjectContainer);
        env->DeleteLocalRef(isoObj);
        if (!container || env->ExceptionCheck()) { env->ExceptionClear(); continue; }

        // Leer items del container
        jobject items = env->CallObjectMethod(container, g_midItemContainerGetItems);
        env->DeleteLocalRef(container);
        if (!items || env->ExceptionCheck()) { env->ExceptionClear(); continue; }

        jint itemCount = GetListSize(env, items);
        if (itemCount <= 0) { env->DeleteLocalRef(items); continue; }

        LootInfo info;
        info.x = (float)(px + dx);
        info.y = (float)(py + dy);
        info.z = pZ;
        info.floorDelta = 0; // mismo piso
        info.containerType = "Container";

        for (jint ii = 0; ii < itemCount && ii < 8; ii++) {
          jobject item = GetListItem(env, items, ii);
          if (!item || env->ExceptionCheck()) { env->ExceptionClear(); continue; }

          if (g_midItemGetName && g_midItemGetCategory) {
            jstring jname = (jstring)env->CallObjectMethod(item, g_midItemGetName);
            env->ExceptionClear();
            jstring jcat = (jstring)env->CallObjectMethod(item, g_midItemGetCategory);
            env->ExceptionClear();

            if (jname) {
              const char* chars = env->GetStringUTFChars(jname, nullptr);
              const char* catChars = jcat ? env->GetStringUTFChars(jcat, nullptr) : "Unknown";
              if (chars) { 
                LootItem lItem;
                lItem.name = chars;
                lItem.category = catChars;
                
                // --- APLICAR FILTROS ---
                bool pCategory = false;
                if (g_lootFilterWeapon && lItem.category == "Weapon") pCategory = true;
                else if (g_lootFilterAmmo && (lItem.category == "Ammo" || lItem.category == "WeaponPart")) pCategory = true;
                else if (g_lootFilterMedical && lItem.category == "FirstAid") pCategory = true;
                else if (g_lootFilterFood && lItem.category == "Food") pCategory = true;
                else if (g_lootFilterMods && lItem.category != "Weapon" && lItem.category != "Ammo" && lItem.category != "WeaponPart" && lItem.category != "FirstAid" && lItem.category != "Food") pCategory = true;
                
                bool pCustom = false;
                if (g_lootFilterCustom[0] != '\0') {
                    if (strcasestr(lItem.name.c_str(), g_lootFilterCustom) != nullptr || 
                        strcasestr(lItem.category.c_str(), g_lootFilterCustom) != nullptr) {
                        pCustom = true;
                    }
                    if (!pCustom) pCategory = false; // El custom string anula la categoria si está activo
                }
                
                if (pCategory || pCustom) {
                    info.items.push_back(std::move(lItem)); 
                }
                
                env->ReleaseStringUTFChars(jname, chars); 
              }
              if (jcat) env->ReleaseStringUTFChars(jcat, catChars);
              
              env->DeleteLocalRef(jname);
              if (jcat) env->DeleteLocalRef(jcat);
            }
          }
          env->DeleteLocalRef(item);
        }
        env->DeleteLocalRef(items);

        if (!info.items.empty()) result.push_back(std::move(info));
      }
      env->DeleteLocalRef(objList);
    }
  }
  env->DeleteLocalRef(cell);
  return result;
}

// [ENI-P7] Dynamic Vehicle Key Spawner
std::string JNIHelper::GetNearVehicleScriptName() {
  JNIEnv* env = GetEnv();
  if (!env || !g_playerInstance || !g_cachedIsoWorldClass) return "";
  
  jmethodID midGetInstance = env->GetStaticMethodID(g_cachedIsoWorldClass, "getInstance", "()Lzombie/iso/IsoWorld;");
  if (!midGetInstance) { env->ExceptionClear(); return ""; }
  jobject isoWorld = env->CallStaticObjectMethod(g_cachedIsoWorldClass, midGetInstance);
  if (!isoWorld) { env->ExceptionClear(); return ""; }
  
  jclass worldCls = env->GetObjectClass(isoWorld);
  jmethodID midGetCell = env->GetMethodID(worldCls, "getCell", "()Lzombie/iso/IsoCell;");
  env->DeleteLocalRef(worldCls);
  if (!midGetCell) { env->ExceptionClear(); env->DeleteLocalRef(isoWorld); return ""; }
  
  jobject cell = env->CallObjectMethod(isoWorld, midGetCell);
  env->DeleteLocalRef(isoWorld);
  if (!cell) { env->ExceptionClear(); return ""; }
  
  // [ENI] R-LOWSPEC: Utilizar fallbacks cacheados de ESP
  jobject vehicleList = nullptr;
  if (g_midGetVehicles) {
    vehicleList = env->CallObjectMethod(cell, g_midGetVehicles);
  } else if (g_fidVehicleList) {
    vehicleList = env->GetObjectField(cell, g_fidVehicleList);
  }
  
  std::string nearestScript = "";
  if (vehicleList) {
    jobjectArray vehicleArray = CollectionToArray(env, vehicleList);
    env->DeleteLocalRef(vehicleList);
    
    if (vehicleArray) {
      int vSize = env->GetArrayLength(vehicleArray);
      float px = 0.0f, py = 0.0f;
      if (g_midGetX && g_midGetY) {
        px = env->CallFloatMethod(g_playerInstance, g_midGetX);
        py = env->CallFloatMethod(g_playerInstance, g_midGetY);
        env->ExceptionClear();
      } else {
        px = g_fidX ? env->GetFloatField(g_playerInstance, g_fidX) : 0.0f;
        py = g_fidY ? env->GetFloatField(g_playerInstance, g_fidY) : 0.0f;
      }
      float minDist = 100.0f; // ~10 tiles max
      jobject nearestVehicle = nullptr;
      
      for (int i = 0; i < vSize; i++) {
        env->ExceptionClear();
        jobject v = env->GetObjectArrayElement(vehicleArray, i);
        if (!v) continue;
        
        float vx = 0.0f, vy = 0.0f;
        if (g_midGetX && g_midGetY) {
          vx = env->CallFloatMethod(v, g_midGetX);
          vy = env->CallFloatMethod(v, g_midGetY);
          env->ExceptionClear();
        } else {
          vx = env->GetFloatField(v, g_fidX);
          vy = env->GetFloatField(v, g_fidY);
        }
        
        float distSq = (vx-px)*(vx-px) + (vy-py)*(vy-py);
        if (distSq < minDist) {
          minDist = distSq;
          if (nearestVehicle) env->DeleteLocalRef(nearestVehicle);
          nearestVehicle = env->NewLocalRef(v);
        }
        env->DeleteLocalRef(v);
      }
      
      if (nearestVehicle) {
        nearestScript = "Unknown Vehicle"; // [ENI] Fallback para evitar bloqueo de UI
        if (g_midGetScriptName) {
          jstring nameStr = (jstring)env->CallObjectMethod(nearestVehicle, g_midGetScriptName);
          if (nameStr && !env->ExceptionCheck()) {
            const char* chars = env->GetStringUTFChars(nameStr, nullptr);
            if (chars && strlen(chars) > 0) {
              nearestScript = chars;
            }
            if (chars) env->ReleaseStringUTFChars(nameStr, chars);
            env->DeleteLocalRef(nameStr);
          }
        }
        env->ExceptionClear();
        env->DeleteLocalRef(nearestVehicle);
      }
      env->DeleteLocalRef(vehicleArray);
    }
    env->DeleteLocalRef(vehicleList);
  } else {
    env->ExceptionClear();
  }
  
  env->DeleteLocalRef(cell);
  return nearestScript;
}

void JNIHelper::DumpAllLoadedClasses() {
  std::thread([]() {
    if (!g_javaVm) {
      JNI_LOG("[JVMTI] No JavaVM available.");
      return;
    }

    JNIEnv *env = nullptr;
    jint attachRes = g_javaVm->AttachCurrentThread((void**)&env, nullptr);
    if (attachRes != JNI_OK || !env) {
      JNI_LOG("[JVMTI] Failed to attach thread. (Error: %d)", attachRes);
      return;
    }

    jvmtiEnv *jvmti = nullptr;
    jint res = g_javaVm->GetEnv((void **)&jvmti, JVMTI_VERSION_1_2);
    if (res != JNI_OK || !jvmti) {
      JNI_LOG("[JVMTI] Failed to get JVMTI environment. (Error: %d)", res);
      g_javaVm->DetachCurrentThread();
      return;
    }

    JNI_LOG("[JVMTI] Fetching loaded classes...");

    jint classCount = 0;
    jclass *classes = nullptr;
    if (jvmti->GetLoadedClasses(&classCount, &classes) != JVMTI_ERROR_NONE) {
      JNI_LOG("[JVMTI] Failed to get loaded classes.");
      g_javaVm->DetachCurrentThread();
      return;
    }

    JNI_LOG("[JVMTI] Found %d loaded classes. Starting dump to JSON...", classCount);

    // Guardar en la ruta de logs requerida
    FILE* out = fopen("/home/j4ck/Dev/Prototipo-2.1 (Limpio)/logs/b42_jvmti_dump.json", "w");
    if (!out) {
      JNI_LOG("[JVMTI] Failed to open /home/j4ck/Dev/Prototipo-2.1 (Limpio)/logs/b42_jvmti_dump.json for writing.");
      jvmti->Deallocate((unsigned char *)classes);
      g_javaVm->DetachCurrentThread();
      return;
    }

    fprintf(out, "{\n\"classes\": [\n");

    jclass classCls = env->FindClass("java/lang/Class");
    jclass methodCls = env->FindClass("java/lang/reflect/Method");
    jclass fieldCls = env->FindClass("java/lang/reflect/Field");
    
    jmethodID midGetName = env->GetMethodID(classCls, "getName", "()Ljava/lang/String;");
    jmethodID midGetDeclFields = env->GetMethodID(classCls, "getDeclaredFields", "()[Ljava/lang/reflect/Field;");
    jmethodID midGetDeclMethods = env->GetMethodID(classCls, "getDeclaredMethods", "()[Ljava/lang/reflect/Method;");
    jmethodID midFieldToString = env->GetMethodID(fieldCls, "toString", "()Ljava/lang/String;");
    jmethodID midMethodToString = env->GetMethodID(methodCls, "toString", "()Ljava/lang/String;");

    bool firstClass = true;

    for (int i = 0; i < classCount; ++i) {
      jclass cls = classes[i];
      if (!cls) continue;

      jstring nameStr = (jstring)env->CallObjectMethod(cls, midGetName);
      if (env->ExceptionCheck()) { env->ExceptionClear(); continue; }
      if (!nameStr) continue;
      
      const char* nameCStr = env->GetStringUTFChars(nameStr, nullptr);
      if (!nameCStr) { env->DeleteLocalRef(nameStr); continue; }

      if (strncmp(nameCStr, "zombie", 6) != 0) {
        env->ReleaseStringUTFChars(nameStr, nameCStr);
        env->DeleteLocalRef(nameStr);
        continue;
      }

      if (!firstClass) fprintf(out, ",\n");
      firstClass = false;

      fprintf(out, "  {\n    \"name\": \"%s\"", nameCStr);

      env->ReleaseStringUTFChars(nameStr, nameCStr);
      env->DeleteLocalRef(nameStr);

      fprintf(out, ",\n    \"methods\": [");
      jobjectArray methods = (jobjectArray)env->CallObjectMethod(cls, midGetDeclMethods);
      if (env->ExceptionCheck()) { env->ExceptionClear(); methods = nullptr; }
      if (methods) {
        jsize mCount = env->GetArrayLength(methods);
        for (jsize m = 0; m < mCount; ++m) {
          jobject method = env->GetObjectArrayElement(methods, m);
          if (method) {
            jstring mStr = (jstring)env->CallObjectMethod(method, midMethodToString);
            if (env->ExceptionCheck()) { env->ExceptionClear(); mStr = nullptr; }
            if (mStr) {
              const char* mCStr = env->GetStringUTFChars(mStr, nullptr);
              if (mCStr) {
                fprintf(out, "%s\n      \"%s\"", (m==0 ? "" : ","), mCStr);
                env->ReleaseStringUTFChars(mStr, mCStr);
              }
              env->DeleteLocalRef(mStr);
            }
            env->DeleteLocalRef(method);
          }
        }
        env->DeleteLocalRef(methods);
      }
      fprintf(out, "\n    ]");

      fprintf(out, ",\n    \"fields\": [");
      jobjectArray fields = (jobjectArray)env->CallObjectMethod(cls, midGetDeclFields);
      if (env->ExceptionCheck()) { env->ExceptionClear(); fields = nullptr; }
      if (fields) {
        jsize fCount = env->GetArrayLength(fields);
        for (jsize f = 0; f < fCount; ++f) {
          jobject field = env->GetObjectArrayElement(fields, f);
          if (field) {
            jstring fStr = (jstring)env->CallObjectMethod(field, midFieldToString);
            if (env->ExceptionCheck()) { env->ExceptionClear(); fStr = nullptr; }
            if (fStr) {
              const char* fCStr = env->GetStringUTFChars(fStr, nullptr);
              if (fCStr) {
                fprintf(out, "%s\n      \"%s\"", (f==0 ? "" : ","), fCStr);
                env->ReleaseStringUTFChars(fStr, fCStr);
              }
              env->DeleteLocalRef(fStr);
            }
            env->DeleteLocalRef(field);
          }
        }
        env->DeleteLocalRef(fields);
      }
      fprintf(out, "\n    ]\n  }");
    }

    fprintf(out, "\n]\n}\n");
    fclose(out);
    jvmti->Deallocate((unsigned char *)classes);
    env->DeleteLocalRef(classCls);
    env->DeleteLocalRef(methodCls);
    env->DeleteLocalRef(fieldCls);

    JNI_LOG("[JVMTI] Dump completed successfully to /home/j4ck/Dev/Prototipo-2.1 (Limpio)/logs/b42_jvmti_dump.json");
    g_javaVm->DetachCurrentThread();
  }).detach();
}

void JNIHelper::ToggleDebugMode(bool active) {
  JNIEnv *env = GetEnv();
  if (!env) return;
  jclass coreCls = env->FindClass("zombie/core/Core");
  if (coreCls) {
    jfieldID fidDebug = env->GetStaticFieldID(coreCls, "debug", "Z");
    if (fidDebug) {
      env->SetStaticBooleanField(coreCls, fidDebug, active ? JNI_TRUE : JNI_FALSE);
      JNI_LOG("[DEBUG-MODE] Set zombie.core.Core.debug to %s", active ? "TRUE" : "FALSE");
    } else {
      env->ExceptionClear();
    }
    env->DeleteLocalRef(coreCls);
  }
}

void JNIHelper::AuditPacketTypes() {
  JNIEnv *env = GetEnv();
  if (!env) return;

  static jclass s_clsPacketTypes = nullptr;
  static jfieldID s_fidPacketTypesMap = nullptr;

  if (!s_clsPacketTypes) {
    jclass localClass = env->FindClass("zombie/network/PacketTypes");
    if (localClass) {
      s_clsPacketTypes = (jclass)env->NewGlobalRef(localClass);
      s_fidPacketTypesMap = env->GetStaticFieldID(s_clsPacketTypes, "packetTypes", "Ljava/util/Map;");
      env->DeleteLocalRef(localClass);
    }
  }

  if (!s_clsPacketTypes || !s_fidPacketTypesMap) return;

  jobject mapObj = env->GetStaticObjectField(s_clsPacketTypes, s_fidPacketTypesMap);
  if (!mapObj) return;

  // Logging seguro a ./Prototipo 2.1/logs/packet_audit.log (R-55)
  FILE* out = fopen("./Prototipo 2.1/logs/packet_audit.log", "w");
  if (out) {
    fprintf(out, "[PACKET_TYPES_AUDIT] Auditing zombie.network.PacketTypes.packetTypes map...\n");
    // [R-LOWSPEC] Verificación pasiva realizada
    fprintf(out, "[PACKET_TYPES_AUDIT] Map object initialized successfully.\n");
    fclose(out);
  }
  env->DeleteLocalRef(mapObj);
}


