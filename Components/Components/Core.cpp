#include "Core.hpp"
#include "../Includes.hpp"

// Discord RPC Direct Implementation
#include <Windows.h>
#include <string>
#include <thread>
#include <chrono>
#include <cstring>

// Discord RPC Declarations
typedef struct DiscordRichPresence {
    const char* state;   /* max 128 bytes */
    const char* details; /* max 128 bytes */
    int64_t startTimestamp;
    int64_t endTimestamp;
    const char* largeImageKey;  /* max 32 bytes */
    const char* largeImageText; /* max 128 bytes */
    const char* smallImageKey;  /* max 32 bytes */
    const char* smallImageText; /* max 128 bytes */
    const char* partyId;        /* max 128 bytes */
    int partySize;
    int partyMax;
    const char* matchSecret;    /* max 128 bytes */
    const char* joinSecret;     /* max 128 bytes */
    const char* spectateSecret; /* max 128 bytes */
    int8_t instance;
} DiscordRichPresence;

typedef struct DiscordUser {
    const char* userId;
    const char* username;
    const char* discriminator;
    const char* avatar;
} DiscordUser;

typedef struct DiscordEventHandlers {
    void (*ready)(const DiscordUser* request);
    void (*disconnected)(int errorCode, const char* message);
    void (*errored)(int errorCode, const char* message);
    void (*joinGame)(const char* joinSecret);
    void (*spectateGame)(const char* spectateSecret);
    void (*joinRequest)(const DiscordUser* request);
} DiscordEventHandlers;

// Discord RPC Function Pointers
typedef void(*Discord_Initialize_t)(const char* applicationId, DiscordEventHandlers* handlers, int autoRegister, const char* optionalSteamId);
typedef void(*Discord_Shutdown_t)(void);
typedef void(*Discord_RunCallbacks_t)(void);
typedef void(*Discord_UpdatePresence_t)(const DiscordRichPresence* presence);

// Global variables for Discord RPC
static HMODULE hDiscordDll = nullptr;
static Discord_Initialize_t pDiscord_Initialize = nullptr;
static Discord_Shutdown_t pDiscord_Shutdown = nullptr;
static Discord_RunCallbacks_t pDiscord_RunCallbacks = nullptr;
static Discord_UpdatePresence_t pDiscord_UpdatePresence = nullptr;


CoreComponent::CoreComponent() : Component("Core", "Initializes globals, components, and modules.") { OnCreate(); }

CoreComponent::~CoreComponent() { OnDestroy(); }

void CoreComponent::OnCreate()
{
	MainThread = nullptr;
}

void CoreComponent::OnDestroy() {
	DestroyThread();
}

void CoreComponent::InitializeThread()
{
	MainThread = CreateThread(nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(InitializeGlobals), nullptr, 0, nullptr);
}

void CoreComponent::DestroyThread()
{
	CloseHandle(MainThread);
}

uintptr_t CoreComponent::GetGObjects() {
	// pattern scan can go here
	return NULL;
}

uintptr_t CoreComponent::GetGNames() {
	// pattern scan can go here
	return NULL;
}

// Discord RPC Implementation
static bool LoadDiscordRPC() {
    Console.Write("[Discord RPC] Attempting to load Discord RPC DLL...");
    
    // Try to load the Discord RPC DLL from the specified directory
    hDiscordDll = LoadLibraryA("F:\\RL mod cheats\\KingMod\\DiscordRPC-for-Visual-Studio\\discord-rpc.dll");
    if (hDiscordDll) {
        Console.Write("[Discord RPC] Loaded DLL from DiscordRPC-for-Visual-Studio directory");
    }
    else {
        // If not found in specified directory, try standard locations
        hDiscordDll = LoadLibraryA("discord-rpc.dll");
        if (hDiscordDll) {
            Console.Write("[Discord RPC] Loaded DLL from system path");
        }
        else {
            // Try game directory as last resort
            hDiscordDll = LoadLibraryA(".\\discord-rpc.dll");
            if (hDiscordDll) {
                Console.Write("[Discord RPC] Loaded DLL from current directory");
            }
            else {
                Console.Error("[Discord RPC] Failed to load Discord RPC DLL from any location!");
                return false;
            }
        }
    }

    // Get function pointers
    pDiscord_Initialize = (Discord_Initialize_t)GetProcAddress(hDiscordDll, "Discord_Initialize");
    pDiscord_Shutdown = (Discord_Shutdown_t)GetProcAddress(hDiscordDll, "Discord_Shutdown");
    pDiscord_RunCallbacks = (Discord_RunCallbacks_t)GetProcAddress(hDiscordDll, "Discord_RunCallbacks");
    pDiscord_UpdatePresence = (Discord_UpdatePresence_t)GetProcAddress(hDiscordDll, "Discord_UpdatePresence");

    if (pDiscord_Initialize && pDiscord_Shutdown && pDiscord_RunCallbacks && pDiscord_UpdatePresence) {
        Console.Write("[Discord RPC] Successfully loaded all Discord RPC functions");
        return true;
    }
    else {
        Console.Error("[Discord RPC] Failed to get all required function pointers!");
        if (!pDiscord_Initialize) Console.Error("[Discord RPC] Missing Discord_Initialize");
        if (!pDiscord_Shutdown) Console.Error("[Discord RPC] Missing Discord_Shutdown");
        if (!pDiscord_RunCallbacks) Console.Error("[Discord RPC] Missing Discord_RunCallbacks");
        if (!pDiscord_UpdatePresence) Console.Error("[Discord RPC] Missing Discord_UpdatePresence");
        return false;
    }
}

// Callback thread and control variables
static std::thread discordCallbackThread;
static bool discordThreadRunning = false;

// Callback handlers with logging
static void handleDiscordReady(const DiscordUser* request) {
    Console.Write("[Discord RPC] Connected to Discord!");
    if (request && request->username) {
        Console.Write(std::string("[Discord RPC] Connected as: ") + request->username);
    }
}

static void handleDiscordDisconnected(int errorCode, const char* message) {
    Console.Error(std::string("[Discord RPC] Disconnected: ") + (message ? message : "Unknown error") + 
                  " (Code: " + std::to_string(errorCode) + ")");
}

static void handleDiscordError(int errorCode, const char* message) {
    Console.Error(std::string("[Discord RPC] Error: ") + (message ? message : "Unknown error") + 
                  " (Code: " + std::to_string(errorCode) + ")");
}

// Discord callback thread function
static void DiscordCallbackLoop() {
    Console.Write("[Discord RPC] Starting callback thread...");
    while (discordThreadRunning && pDiscord_RunCallbacks) {
        pDiscord_RunCallbacks();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    Console.Write("[Discord RPC] Callback thread ended.");
}

// Initialize Discord Rich Presence
static void InitializeDiscordRPC() {
    if (!LoadDiscordRPC()) {
        Console.Error("[Discord RPC] Initialization failed - couldn't load Discord RPC DLL.");
        return;
    }

    Console.Write("[Discord RPC] Setting up handlers...");
    
    // Set up the event handlers
    DiscordEventHandlers handlers;
    memset(&handlers, 0, sizeof(handlers));
    handlers.ready = handleDiscordReady;
    handlers.disconnected = handleDiscordDisconnected;
    handlers.errored = handleDiscordError;

    // Initialize Discord RPC with the specified application ID
    Console.Write("[Discord RPC] Initializing with application ID: 1372916197951143986");
    pDiscord_Initialize("1372916197951143986", &handlers, 1, nullptr);

    // Get username from system
    char username[256] = { 0 };
    DWORD username_len = 256;
    GetUserNameA(username, &username_len);
    
    // Define version
    const char* version = "1.2.0";
    
    // Create state with build version
    char state[128] = { 0 };
    snprintf(state, sizeof(state), "build: %s", version);
    
    // Create details with username
    char details[128] = { 0 };
    snprintf(details, sizeof(details), "user: %s", username);
    
    // Set the initial rich presence
    Console.Write("[Discord RPC] Setting rich presence data...");
    DiscordRichPresence discordPresence;
    memset(&discordPresence, 0, sizeof(discordPresence));
    discordPresence.state = state;
    discordPresence.details = details;
    discordPresence.startTimestamp = time(0);
    
    // Try multiple image key variations that might work with the app
    // The Discord Developer Portal needs to have these assets uploaded
    discordPresence.largeImageKey = "kingmod"; // Try app name
    discordPresence.largeImageText = "KingMod for Rocket League";
    discordPresence.smallImageKey = "ssl"; // Use ssl.png as requested
    discordPresence.smallImageText = "KingMod SSL";
    
    pDiscord_UpdatePresence(&discordPresence);
    Console.Write("[Discord RPC] Presence data sent to Discord");

    // Start a callback thread to keep the presence updated
    discordThreadRunning = true;
    discordCallbackThread = std::thread(DiscordCallbackLoop);
    discordCallbackThread.detach(); // Let it run independently

    // Also run initial callbacks
    pDiscord_RunCallbacks();
}

// Shutdown Discord RPC
static void ShutdownDiscordRPC() {
    // Stop the callback thread
    discordThreadRunning = false;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // Give it time to finish
    
    if (pDiscord_Shutdown) {
        Console.Write("[Discord RPC] Shutting down Discord RPC...");
        pDiscord_Shutdown();
    }
    
    if (hDiscordDll) {
        Console.Write("[Discord RPC] Unloading Discord RPC DLL...");
        FreeLibrary(hDiscordDll);
        hDiscordDll = nullptr;
    }
}

// Update Discord Rich Presence
static void UpdateDiscordRPC() {
    if (pDiscord_RunCallbacks) {
        pDiscord_RunCallbacks();
    }
}

void CoreComponent::InitializeGlobals(HMODULE hModule)
{
	Console.Initialize(std::filesystem::current_path(), "RLMod.log");

	//GObjects = reinterpret_cast<TArray<UObject*>*>(GetGObjects());  for pattern scanning
	//GNames = reinterpret_cast<TArray<FNameEntry*>*>(GetGNames());  for pattern scanning
	uintptr_t BaseAddress = reinterpret_cast<uintptr_t>(GetModuleHandle(NULL));
	GObjects = reinterpret_cast<TArray<UObject*>*>(BaseAddress + 0x2348660);
	GNames = reinterpret_cast<TArray<FNameEntry*>*>(BaseAddress + 0x2348618);

	if (AreGlobalsValid())
	{
		Console.Notify("[Core Module] Entry Point " + Format::ToHex(reinterpret_cast<void*>(GetModuleHandle(NULL))));
		Console.Notify("[Core Module] Global Objects: " + Format::ToHex(GObjects));
		Console.Notify("[Core Module] Global Names: " + Format::ToHex(GNames));
		Console.Write("[Core Module] Initialized!");

		void** UnrealVTable = reinterpret_cast<void**>(UObject::StaticClass()->VfTableObject.Dummy);
		EventsComponent::AttachDetour(reinterpret_cast<ProcessEventType>(UnrealVTable[67]));

		Instances.Initialize();
		Events.Initialize();
		GUI.Initialize();
		Main.Initialize();
		
		// Initialize Discord Rich Presence with the custom icon and application ID
		Console.Write("[Discord RPC] Initializing Discord Rich Presence...");
		
		// First check if discord-rpc.dll exists in the target directory
		WIN32_FIND_DATAA findData;
		HANDLE hFind = FindFirstFileA("F:\\RL mod cheats\\KingMod\\DiscordRPC-for-Visual-Studio\\discord-rpc.dll", &findData);
		if (hFind == INVALID_HANDLE_VALUE) {
			Console.Error("[Discord RPC] Could not find discord-rpc.dll in the specified path!");
			Console.Error("[Discord RPC] Please make sure discord-rpc.dll exists in F:\\RL mod cheats\\KingMod\\DiscordRPC-for-Visual-Studio\\");
			// Try to find discord-rpc.dll in the project directory
			hFind = FindFirstFileA("discord-rpc.dll", &findData);
			if (hFind == INVALID_HANDLE_VALUE) {
				Console.Error("[Discord RPC] Also could not find discord-rpc.dll in the current directory!");
				Console.Error("[Discord RPC] Download discord-rpc.dll and place it in your project directory");
			} else {
				Console.Write("[Discord RPC] Found discord-rpc.dll in current directory, will try to use it");
				FindClose(hFind);
			}
		} else {
			Console.Write("[Discord RPC] Found discord-rpc.dll in the specified path");
			FindClose(hFind);
		}
		
		// Try to initialize Discord RPC
		bool discordInitialized = false;
		try {
			InitializeDiscordRPC(); // Call the local implementation directly
			
			// Check if the DLL was successfully loaded (this variable is set in LoadDiscordRPC)
			if (hDiscordDll != nullptr) {
				Console.Write("[Discord RPC] Discord Rich Presence initialized successfully!");
				discordInitialized = true;
			} else {
				Console.Error("[Discord RPC] Failed to initialize Discord Rich Presence - DLL could not be loaded");
			}
		} catch (...) {
			Console.Error("[Discord RPC] Exception occurred during Discord RPC initialization");
		}
	}
	else
	{
		Console.Error("[Core Module] GObject and GNames are not valid, wrong address detected!");
	}
}

bool CoreComponent::AreGlobalsValid()
{
	return (AreGObjectsValid() && AreGNamesValid());
}

bool CoreComponent::AreGObjectsValid()
{
	if (GObjects
		&& UObject::GObjObjects()->size() > 0
		&& UObject::GObjObjects()->capacity() > UObject::GObjObjects()->size())
	{
		return true;
	}

	return false;
}

bool CoreComponent::AreGNamesValid()
{
	if (GNames
		&& FName::Names()->size() > 0
		&& FName::Names()->capacity() > FName::Names()->size())
	{
		return true;
	}

	return false;
}

class CoreComponent Core {};