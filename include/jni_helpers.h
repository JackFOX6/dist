#pragma once

#ifdef _WIN32
  #include <windows.h>
#else
  #include <dlfcn.h>
  #include <unistd.h>
  // Windows type shims for cross-platform JNI code
  typedef void* HMODULE;
  inline HMODULE GetModuleHandleA(const char*) { return nullptr; }
  inline void* GetProcAddress(HMODULE, const char* sym) { return dlsym(RTLD_DEFAULT, sym); }
#endif

#include <jni.h>
#include <jvmti.h>
#include <string>
#include <vector>
#include <cstdio>
#include <thread>

// External log file handle from main.cpp
extern FILE* fLog;

// [ENI] FPS Optimization - Frame counter (declared in main.cpp)
extern int g_frameCount;

// Helper macro for logging from JNI helpers
#define JNI_LOG(fmt, ...) \
    do { \
        if (fLog) { \
            fprintf(fLog, fmt "\n", ##__VA_ARGS__); \
            fflush(fLog); \
        } \
        printf(fmt "\n", ##__VA_ARGS__); \
        fflush(stdout); \
    } while(0)

// =========================================================================================
// ESP Info Structures
// =========================================================================================

struct ZombieInfo {
    float x, y, z;
    bool isDead;
    int type; // 0=Zombie, 1=Player, 2=Vehicle
    int speedType;   // [ENI] 0=shambler, 1=fast_shambler, 2=sprinter
    bool crawling;   // [ENI] true si es crawler
    int floorDelta;  // [ENI] piso relativo al jugador (0=mismo, +1=arriba, -1=abajo)
    std::string vehicleName; // [ENI] nombre script del vehículo (solo si type==2)
};

struct PlayerInfoEx {
    float x, y, z;
    float distance;
    std::string username;
    float health;
    std::string weapon;
    bool isDead;
    int floorDelta;  // [ENI] piso relativo al jugador local
};

// =========================================================================================
// JNI HELPER FUNCTIONS - Project Zomboid B42
// =========================================================================================

class JNIHelper {
public:
    static JavaVM* g_javaVm;
    static JNIEnv* g_jniEnv;
    static jobject g_playerInstance;

    // ClassLoader and class reference caching
    static jobject g_classLoader;
    static jclass g_classLoaderClass;
    static jclass g_cachedIsoPlayerClass;
    static jclass g_cachedIsoGameCharacterClass;
    static jclass g_cachedPlayerCheatsClass;
    static jclass g_cachedStatsClass;
    static jclass g_cachedCharacterStatClass;
    static jclass g_cachedBodyDamageClass;
    static jclass g_cachedScriptManagerClass;
    static jclass g_cachedItemClass;
    static jclass g_cachedArrayListClass;
    static jclass g_cachedSandboxOptionsClass;
    static jclass g_cachedIntegerConfigOptionClass;
    static jclass g_cachedLuaManagerClass;
    static jclass g_cachedKahluaTableClass;
    static jclass g_cachedDoubleClass;
    static jclass g_cachedNutritionClass;

    // Game State classes caching
    static jclass g_cachedGameWindowClass;
    static jclass g_cachedGameStateMachineClass;
    static jclass g_cachedGameStateClass;
    static jclass g_cachedMainScreenStateClass;

    // Game State field IDs
    static jfieldID g_statesFieldId;
    static jfieldID g_currentFieldId;
    static jfieldID g_mainScreenStateInstanceFieldId;
    
    // [ENI] Cached IDs for 60Hz loop optimization
    static jfieldID g_fidInvincible;
    static jfieldID g_fidAvoidDamage;
    static jfieldID g_fidGhostMode;
    static jfieldID g_fidCheats;
    static jfieldID g_fidEnumSet;
    static jmethodID g_midPlayerCheatsSet;
    static jmethodID g_midPlayerCheatsIsSet;        // isSet(CheatType): (Lzombie/characters/CheatType;)Z
    static jmethodID g_midPlayerCheatsSetPrivate;   // private set(CheatType): no isCheatAllowed() gate
    static jmethodID g_midPlayerCheatsUnsetPrivate; // private unset(CheatType)
    // [B42] Unlimited ammo per-frame reset via InventoryItem
    static jfieldID  g_fidRightHandItem;      // IsoGameCharacter.rightHandItem
    static jmethodID g_midGetMaxAmmo;         // InventoryItem.getMaxAmmo(): ()I
    static jmethodID g_midSetCurrentAmmo;     // InventoryItem.setCurrentAmmoCount(int): (I)V

    // [ENI] Soft Aim — HandWeapon field/method cache (armas de fuego ranged)
    static jclass    g_cachedHandWeaponClass;         // zombie/inventory/types/HandWeapon
    static jfieldID  g_fidHWHitChance;               // HandWeapon.hitChance (int, private)
    static jfieldID  g_fidHWMinAngle;                // HandWeapon.minAngle (float, private)
    static jfieldID  g_fidHWMaxAngle;                // HandWeapon.maxAngle (float, private)
    static jfieldID  g_fidHWProjectileSpread;        // HandWeapon.projectileSpread (float, private)
    static jfieldID  g_fidHWJamGunChance;            // HandWeapon.jamGunChance (float, private)
    static jmethodID g_midHWIsRanged;                // HandWeapon.isRanged(): ()Z
    static jmethodID g_midHWIsAimedFirearm;          // HandWeapon.isAimedFirearm(): ()Z
    // [ENI] Critical Override — headshot mecánico vía criticalChance/Multiplier
    static jfieldID  g_fidHWCriticalChance;          // HandWeapon.criticalChance (float, private)
    static jfieldID  g_fidHWCriticalDmgMult;         // HandWeapon.criticalDamageMultiplier (float, private)
    static jfieldID  g_fidHWAimingPerkCritMod;       // HandWeapon.aimingPerkCritModifier (int, private)
    // [ENI] Combat Supremacy — HandWeapon timing fields
    static jfieldID  g_fidHWSwingTime;               // HandWeapon.swingTime (float, private)
    static jfieldID  g_fidHWMinSwingTime;            // HandWeapon.minimumSwingTime (float, private)
    static jfieldID  g_fidHWRecoilDelayHW;           // HandWeapon.recoilDelay (int, private)
    static jfieldID g_fidBodyDamage;
    static jfieldID g_fidXP;
    static jfieldID g_fidEndurance;
    static jfieldID g_fidFatigue;
    // [ENI] Combat Supremacy — setters públicos (persisten correctamente en el engine)
    static jmethodID g_midSetIgnoreStaggerBack; // setIgnoreStaggerBack(boolean)
    static jmethodID g_midSetStaggerTimeMod;    // setStaggerTimeMod(float)
    static jmethodID g_midSetRecoilDelay;       // setRecoilDelay(float)
    // blurFactor se setea via field directo (no tiene setter público en IGC)
    static jfieldID  g_fidBlurFactor;           // IsoGameCharacter.blurFactor (float)
    static jfieldID  g_fidBlurFactorTarget;     // IsoGameCharacter.blurFactorTarget (float)
    static jfieldID g_fidStats;
    static jfieldID g_fidSandboxInstance;
    static jfieldID g_fidCharacterFreePoints;
    static jfieldID g_fidIntOptionValue;
    static jfieldID g_fidLuaEnv;
    static jmethodID g_midInitSandboxVars;
    static jmethodID g_midRunLua;
    static jmethodID g_midRawGet;
    static jmethodID g_midRawSet;
    static jmethodID g_midDoubleInit;
    
    // [ENI] ESP - Zombie Scanner & Core
    static jclass g_cachedIsoWorldClass;
    static jclass g_cachedIsoCellClass;
    static jclass g_cachedIsoZombieClass;
    static jclass g_cachedIsoVehicleClass;
    static jclass g_cachedCoreClass;
    static jclass g_cachedIsoCameraClass;
    static jclass g_cachedGameClientClass;
    static jclass g_cachedSetClass;
    static jclass g_cachedIsoDeadBodyClass;
    static jclass g_cachedInventoryItemFactoryClass;
    // [ENI] Vehicle Tools — BaseVehicle
    static jclass    g_cachedBaseVehicleClass;       // zombie/vehicles/BaseVehicle
    static jfieldID  g_fidVehicleHotwired;           // boolean hotwired
    static jfieldID  g_fidVehicleAlarmed;            // boolean alarmed
    static jfieldID  g_fidVehicleKeysInIgnition;     // boolean keysInIgnition
    static jfieldID  g_fidVehicleIgnitionSwitch;     // ItemContainer ignitionSwitch
    static jmethodID g_midIsSeatedInVehicle;         // isSeatedInVehicle()
    static jmethodID g_midGetVehicleFromPlayer;      // getVehicle() en IGC
    static jmethodID g_midSetHotwired;               // setHotwired(boolean)
    static jmethodID g_midSetAlarmed;                // setAlarmed(boolean)
    static jmethodID g_midSetKeysInIgnition;         // setKeysInIgnition(boolean)
    static jmethodID g_midCreateVehicleKey;          // createVehicleKey() -> InventoryItem
    static jmethodID g_midIsHotwired;                // isHotwired() -> boolean
    static jmethodID g_midGetScriptName;             // getScriptName() -> String
    static jmethodID g_midItemContainerAddItem;      // ItemContainer.AddItem(InventoryItem)
    static jmethodID g_midCheatHotwire;              // cheatHotwire(boolean, boolean) -> void

    static jfieldID g_fidWorldInstance;
    static jfieldID g_fidCurrentCell;
    static jfieldID g_fidZombieList;
    static jfieldID g_fidRemoteSurvivorList; // [ENI] Player List
    static jfieldID g_fidVehicleList;
    static jfieldID g_fidGameClientInstance;

    static jmethodID g_midGetRemoteSurvivorList;
    static jmethodID g_midGetGameClientPlayers;
    static jmethodID g_midGetVehicles;
    static jmethodID g_midGetAnimals;
    static jmethodID g_midSetToArray;
    static jfieldID g_fidX;
    static jfieldID g_fidY;
    static jfieldID g_fidZ;
    static jmethodID g_midGetX;
    static jmethodID g_midGetY;
    static jmethodID g_midGetZ;
    
    // [B42] Stats API — Stats.get(CharacterStat) / Stats.set(CharacterStat, float)
    static jmethodID g_midStatsGet;    // Stats.get(CharacterStat): (Lzombie/characters/CharacterStat;)F
    static jmethodID g_midStatsSet;    // Stats.set(CharacterStat, float): (Lzombie/characters/CharacterStat;F)Z
    // CharacterStat enum values (cached jobjects)
    static jobject g_valStatHunger;
    static jobject g_valStatThirst;
    static jobject g_valStatFatigue;
    static jobject g_valStatPain;
    static jobject g_valStatTemperature;
    static jobject g_valStatEndurance;
    static jobject g_valStatPanic;
    
    // [ENI] Player Info ESP Method IDs
    static jmethodID g_midGetUsername;
    static jmethodID g_midGetHealth;
    static jmethodID g_midGetPrimaryHandItem;
    // g_midGetDisplayName está definido en jni_helpers.cpp línea 119
    
    static jmethodID g_midCoreInstance;
    static jmethodID g_midGetOffX;
    static jmethodID g_midGetOffY;
    static jmethodID g_midGetZoom;
    static jmethodID g_midIsDead;
    static jmethodID g_midIsoCameraGetOffX;
    static jmethodID g_midIsoCameraGetOffY;
    // [ENI] Zombie type detection — floor delta
    static jfieldID  g_fidZombieSpeedType;   // IsoZombie.speedType (int, public)
    static jfieldID  g_fidZombieCrawling;    // IsoZombie.crawling (boolean, public)
    // [ENI] Night Vision + Lightfoot (stealth)
    static jmethodID g_midSetNightVision;        // IsoPlayer.setWearingNightVisionGoggles(Z)V
    static jfieldID  g_fidWornItemsHearing;      // IGC.wornItemsHearingModifier (float)
    // [ENI] KNOW_ALL_RECIPES CheatType
    static jobject   g_valKnowAllRecipes;        // CheatType::KNOW_ALL_RECIPES
    // [ENI] Loot ESP
    static jfieldID  g_fidIsoObjectContainer;    // IsoObject.container (public ItemContainer)
    static jmethodID g_midItemContainerGetItems; // ItemContainer.getItems() -> ArrayList
    static jmethodID g_midItemGetType;           // InventoryItem.getType() -> String
    static jmethodID g_midItemGetName;           // InventoryItem.getName() -> String
    static jmethodID g_midItemGetCategory;       // InventoryItem.getCategory() -> String
    static jmethodID g_midGridSquareGetObjects;  // IsoGridSquare.getObjects() or similar
    static jmethodID g_midGridSquareGetStaticMovingObjects; // IsoGridSquare.getStaticMovingObjects()
    static jmethodID g_midArrayListGet;
    static jmethodID g_midArrayListSize;

    struct ZombieInfo {
        float x, y, z;
        bool isDead;
        int type; // 0=Zombie, 1=Player, 2=Vehicle, 3=Animal
        int speedType;          // [ENI] 0=shambler, 1=fast_shambler, 2=sprinter
        bool crawling;          // [ENI] true si es crawler
        int floorDelta;         // [ENI] piso relativo al jugador (0=mismo, +1=arriba, -1=abajo)
        std::string vehicleName;// [ENI] script name vehículo / username jugador
    };
    static std::vector<ZombieInfo> GetZombiesForESP();
    static std::vector<ZombieInfo> GetPlayersForESP();
    static std::vector<ZombieInfo> GetVehiclesForESP();
    
    // [ENI] Player Info ESP - Extended information
    static std::vector<PlayerInfoEx> GetPlayersInfoForESP();
    
    struct ScreenInfo {
        float offX, offY, zoom;
    };
    static ScreenInfo GetScreenInfo();
    static bool WorldToScreen(float x, float y, float z, float& outX, float& outY, const ScreenInfo& screen);

    // Cached Enum Values (Global Refs)
    static jobject g_valGodMode;
    static jobject g_valInvisible;
    static jobject g_valZombiesDontAttack;
    static jobject g_valGhostMode;
    static jobject g_valNoClip;
    static jobject g_valUnlimitedAmmo;
    static jobject g_valUnlimitedCarry;
    static jobject g_valAnimal;             // [ENI] CheatType::ANIMAL
    static jobject g_valAnimalExtraValues;  // [ENI] CheatType::ANIMAL_EXTRA_VALUES
    
    // Cached Set methods
    static jmethodID g_midSetAdd;
    static jmethodID g_midSetRemove;
    
    // Zombie Kills & Survived Time method IDs
    static jmethodID g_midGetZombieKills;
    static jmethodID g_midSetZombieKills;
    static jmethodID g_midGetLastZombieKills;
    static jmethodID g_midSetLastZombieKills;
    static jmethodID g_midGetHoursSurvived;
    static jmethodID g_midSetHoursSurvived;
    static jmethodID g_midSavePlayer;

    // Nutrition JNI cached variables
    static jmethodID g_midGetNutrition;
    static jmethodID g_midGetWeight;
    static jmethodID g_midSetWeight;
    static jmethodID g_midGetCalories;
    static jmethodID g_midSetCalories;
    static jmethodID g_midGetProteins;
    static jmethodID g_midSetProteins;
    static jmethodID g_midGetCarbohydrates;
    static jmethodID g_midSetCarbohydrates;
    static jmethodID g_midGetLipids;
    static jmethodID g_midSetLipids;

    // Cached method IDs
    static jmethodID g_hasInstanceMethodId;
    static jmethodID g_getInstanceMethodId;
    static jmethodID g_midGetStats;
    static jmethodID g_midRestoreToFullHealth;
    static jmethodID g_midAddXP;
    static jmethodID g_midGetPerk;
    static jmethodID g_midGetCurrentSquare;
    static jmethodID g_midCreateItem;
    static jmethodID g_midAddWorldInventoryItem;
    static jmethodID g_midTransmitCompleteItemToServer;
    static jmethodID g_midGetAllItems;
    static jmethodID g_midGetFullName;
    static jmethodID g_midGetDisplayName;
    static jmethodID g_midSetIntOptionValue;

    // Cheat States
    static bool g_godModeActive;
    static bool g_invisibleActive;
    static bool g_zombiesDontAttackActive;
    static bool g_ghostModeActive;
    static bool g_noClipActive;
    static bool g_unlimitedAmmoActive;
    static bool g_unlimitedCarryActive;
    static bool g_unlimitedEnduranceActive;
    static bool g_softAimActive;             // [ENI] Soft Aim
    static bool g_criticalOverrideActive;    // [ENI] Critical Override
    static bool g_animalCheatActive;         // [ENI] Animal Cheat
    static bool g_combatSupremacyActive;     // [ENI] Combat Supremacy
    static bool g_knowAllRecipesActive;      // [ENI] KNOW_ALL_RECIPES CheatType
    static bool g_nightVisionActive;         // [ENI] Fake night vision goggles
    static bool g_lightfootActive;           // [ENI] wornItemsHearingModifier = 0 (sigilo)
    static bool g_lootEspActive;             // [ENI] Loot ESP
    static bool g_claimBypassActive;         // [ENI] Vehicle Claim Bypass
    // ESP States
    static bool g_playerEspActive;
    static bool g_vehicleEspActive;
    static bool g_animalEspActive;

    // [ENI] Entity Cache for FPS Optimization (Throttling)
    // Evita llamadas JNI cada frame - actualiza cada N frames
    struct EntityCache {
        std::vector<ZombieInfo> entities;
        int frameLastUpdate;
        bool isValid;
    };
    static EntityCache g_entityCache;
    // [R-LOWSPEC] TTL=0 => scan JNI cada frame para evitar stuttering y dar 60 FPS reales en ESP
    static constexpr int ENTITY_CACHE_TTL = 0;

    // [ENI] Minimap Configuration
    struct MinimapConfig {
        bool enabled;
        float scale;           // 50.0f - 300.0f
        float alpha;          // 0.3f - 1.0f
        int position;         // 0=Bottom-Right, 1=Bottom-Left, 2=Floating
        bool showGrid;
        bool showZombies;
        bool showPlayers;
        bool showVehicles;
        bool showAnimals;
        bool showContainers;
    };
    static MinimapConfig g_minimapConfig;

    // [ENI] Minimap Rendering
    static void RenderMinimap();

    // [ENI] Moodle Display Configuration
    struct MoodleDisplayConfig {
        bool enabled;
        int position;      // 0=Top-Left, 1=Top-Right, 2=Bottom-Left, 3=Bottom-Right, 4=Floating
        float alpha;       // 0.3f - 1.0f
        bool showHunger;
        bool showThirst;
        bool showFatigue;
        bool showSick;
        bool showInjured;
        bool showPanic;
    };
    static MoodleDisplayConfig g_moodleConfig;

    // [ENI] Moodle Info (cached data)
    struct MoodleInfo {
        float hunger;     // 0-1
        float thirst;     // 0-1
        float fatigue;   // 0-1
        float panic;     // 0-1
        float temperature; // 36-42
        float endurance;   // 0-1
        bool isSick;
        bool isInjured;
        bool isBleeding;
        bool isCold;
        int painLevel;   // 0-100
    };
    static MoodleInfo g_moodleInfo;

    // [ENI] Moodle Rendering
    static void RenderMoodleDisplay();
    
    // [ENI] Update Moodle Cache - CRITICAL BUG FIX
    // Los campos Moodle están cacheados pero nunca se actualizaban
    static void UpdateMoodleCache();

    // Autocomplete cache
    static std::vector<std::string> g_itemCache;
    static std::string g_selectedItem;
    static char g_targetItemID[128];

    // Initialize JNI
    static bool Initialize();
    
    static bool CacheClasses();
    static bool CacheCheatVars();

    // Get local player instance
    static bool GetLocalPlayer();
    
    // Get JNIEnv for current thread
    static JNIEnv* GetEnv();

    // Enhanced class finder using ClassLoader
    static jclass FindClassEx(const char* className);
    
    // Restore player to full health (via BodyDamage)
    static void RestoreToFullHealth();

    // Spawn item on ground at player's location
    static void SpawnItemOnGround(const char* itemID, int quantity = 1);

    // Corpse & Container AoE Injection
    static int corpseAoERadius;
    static void InjectLootToCorpsesAoE(int radiusTiles);
    static void InjectLootToContainersAoE(int radiusTiles);

    // Add XP (fractional XP where supported)
    // Adds `amount` XP to the specified perk and attempts to sync with server.
    static void AddXP(const char* perkID, float amount);

    // Get player info
    static std::string GetPlayerName();
    static float GetPlayerHealth();
    static float GetPlayerHealthDirect();  // Read health field directly (bypass method)
    static void EnableGodModeReal();       // [ENI] Force Invincible + EnumSet Injection
    
    // [ENI] Granular Persistent Toggles
    static void ToggleGodMode(bool enable);
    static void ToggleInvisible(bool enable);
    static void ToggleGhostMode(bool enable);
    static void ToggleNoClip(bool enable);
    static void ToggleUnlimitedAmmo(bool enable);
    static void ToggleUnlimitedCarry(bool enable);
    static void ToggleUnlimitedEndurance(bool enable);
    static void ToggleSoftAim(bool enable);
    static void ToggleCriticalOverride(bool enable);
    static void ToggleAnimalCheat(bool enable);
    static void ToggleCombatSupremacy(bool enable);
    // [ENI] Vehicle Tools backend (sin UI — server-authoritative en MP)
    static void ToggleHotwire();
    static void CreateKeyInIgnition();
    static void SilenceAlarm();
    static bool IsSeatedInVehicle();
    static std::string GetCurrentVehicleScriptName();
    // [ENI] Nuevas features server-safe
    static void ToggleKnowAllRecipes(bool enable);
    static void ToggleNightVision(bool enable);
    static void ToggleLightfoot(bool enable);
    static void ToggleClaimBypass(bool enable);
    static void ApplyActiveCheats();

    // [ENI-P7] Dynamic Vehicle Key Spawner
    static std::string GetNearVehicleScriptName();
    static void SpawnKeyForNearVehicle();

    // [ENI] Loot ESP — leer contenido de containers cercanos (read-only, zero packets)
    struct LootItem {
        std::string name;
        std::string category;
    };
    struct LootInfo {
        float x, y, z;
        int floorDelta;
        std::string containerType;  // tipo de sprite/objeto (ej. "Fridge", "Crate")
        std::vector<LootItem> items; // ítems filtrados
    };
    static std::vector<LootInfo> GetNearbyLoot(int radiusTiles = 15);
    
    // Filtros de Loot
    static bool g_lootFilterWeapon;
    static bool g_lootFilterAmmo;
    static bool g_lootFilterMedical;
    static bool g_lootFilterFood;
    static bool g_lootFilterMods; // Mostrar cualquier categoría que no sea estándar
    static char g_lootFilterCustom[128];
    static std::vector<LootInfo> g_lootCache;

    // [ENI] Proximity alarm — removido (redundante con ESP)
    // static float GetNearestPlayerDistance();

    static void SpawnItem(const char* itemId, int quantity = 1); // [ENI] World Spawner (Foraging Method)
    static std::vector<std::string> GetAllItems();
    static void SetPlayerHealth(float health);
    
    // Zombie Kills & Survived Time Editor
    static int GetZombieKills();
    static void SetZombieKills(int kills);
    static double GetHoursSurvived();
    static void SetHoursSurvived(double hours);
    
    // Moodles & Nutrition Editor
    static void SetPlayerStat(int statType, float value);
    static float GetNutritionStat(int statType);
    static void SetNutritionStat(int statType, float value);
    
    // Sandbox Options
    static void SetCharacterFreePoints(int points);
    static int GetCharacterFreePoints();
    static void SyncCharacterFreePoints();
    static void ResetCharacterFreePoints();

    static void RunLua(const char* script);

    // X-RAY: Class Analysis (Scraper) & Network Protocol Audit
    static void DumpClassInfo(const char* className);
    static void DumpAllLoadedClasses(); // [ENI] Full JVMTI JSON Dump
    static void ToggleDebugMode(bool active); // [ENI] Toggle native debug mode
    static void AuditPacketTypes(); // [ENI] Packet Inspection Module (Read-Only)

    // Cleanup
    static void Cleanup();

    // Generic JNI Collection Helpers (Type-Safe for ArrayList, PZArrayList, Set)
    static jint GetListSize(JNIEnv *env, jobject listObj);
    static jobject GetListItem(JNIEnv *env, jobject listObj, jint index);
    static jobjectArray CollectionToArray(JNIEnv *env, jobject collObj);

private:
    // Internal helper to validate player objects (checks if health > 0)
    static float GetPlayerHealthDirect_Internal(jobject playerObj);
};