#include "RPCDialog.h"
#include "Components/Includes.hpp"
#include <Windows.h>
#include <string>
#include <thread>
#include <chrono>
#include <cstring>

// Forward declarations from Discord RPC implementation
bool LoadDiscordRPC();
void InitDiscordRPC_Impl();

// Global variables for Discord RPC dialog
bool g_discordRpcInitialized = false;
bool g_showUsernameDialog = true;
char g_rpcUsername[256] = "";

// Discord RPC typedefs
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

// Discord RPC global variables
static HMODULE g_hDiscordDll = nullptr;
static Discord_Initialize_t g_pDiscord_Initialize = nullptr;
static Discord_Shutdown_t g_pDiscord_Shutdown = nullptr;
static Discord_RunCallbacks_t g_pDiscord_RunCallbacks = nullptr;
static Discord_UpdatePresence_t g_pDiscord_UpdatePresence = nullptr;
static std::thread g_discordCallbackThread;
static bool g_discordThreadRunning = false;

// Callback handlers
static void HandleDiscordReady(const DiscordUser* request) {
    Console.Write("[Discord RPC] Connected to Discord!");
    if (request && request->username) {
        Console.Write(std::string("[Discord RPC] Connected as: ") + request->username);
    }
}

static void HandleDiscordDisconnected(int errorCode, const char* message) {
    Console.Error(std::string("[Discord RPC] Disconnected: ") + (message ? message : "Unknown error") + 
                  " (Code: " + std::to_string(errorCode) + ")");
}

static void HandleDiscordError(int errorCode, const char* message) {
    Console.Error(std::string("[Discord RPC] Error: ") + (message ? message : "Unknown error") + 
                  " (Code: " + std::to_string(errorCode) + ")");
}

// Discord callback thread function
static void DiscordCallbackLoop() {
    Console.Write("[Discord RPC] Starting callback thread...");
    while (g_discordThreadRunning && g_pDiscord_RunCallbacks) {
        g_pDiscord_RunCallbacks();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    Console.Write("[Discord RPC] Callback thread ended.");
}

// Load Discord RPC DLL
static bool LoadDiscordRPC() {
    Console.Write("[Discord RPC] Attempting to load Discord RPC DLL...");
    
    // Try to load the Discord RPC DLL from the specified directory
    g_hDiscordDll = LoadLibraryA("F:\\RL mod cheats\\KingMod\\DiscordRPC-for-Visual-Studio\\discord-rpc.dll");
    if (g_hDiscordDll) {
        Console.Write("[Discord RPC] Loaded DLL from DiscordRPC-for-Visual-Studio directory");
    }
    else {
        // If not found in specified directory, try standard locations
        g_hDiscordDll = LoadLibraryA("discord-rpc.dll");
        if (g_hDiscordDll) {
            Console.Write("[Discord RPC] Loaded DLL from system path");
        }
        else {
            // Try game directory as last resort
            g_hDiscordDll = LoadLibraryA(".\\discord-rpc.dll");
            if (g_hDiscordDll) {
                Console.Write("[Discord RPC] Loaded DLL from current directory");
            }
            else {
                Console.Error("[Discord RPC] Failed to load Discord RPC DLL from any location!");
                return false;
            }
        }
    }

    // Get function pointers
    g_pDiscord_Initialize = (Discord_Initialize_t)GetProcAddress(g_hDiscordDll, "Discord_Initialize");
    g_pDiscord_Shutdown = (Discord_Shutdown_t)GetProcAddress(g_hDiscordDll, "Discord_Shutdown");
    g_pDiscord_RunCallbacks = (Discord_RunCallbacks_t)GetProcAddress(g_hDiscordDll, "Discord_RunCallbacks");
    g_pDiscord_UpdatePresence = (Discord_UpdatePresence_t)GetProcAddress(g_hDiscordDll, "Discord_UpdatePresence");

    if (g_pDiscord_Initialize && g_pDiscord_Shutdown && g_pDiscord_RunCallbacks && g_pDiscord_UpdatePresence) {
        Console.Write("[Discord RPC] Successfully loaded all Discord RPC functions");
        return true;
    }
    else {
        Console.Error("[Discord RPC] Failed to get all required function pointers!");
        if (!g_pDiscord_Initialize) Console.Error("[Discord RPC] Missing Discord_Initialize");
        if (!g_pDiscord_Shutdown) Console.Error("[Discord RPC] Missing Discord_Shutdown");
        if (!g_pDiscord_RunCallbacks) Console.Error("[Discord RPC] Missing Discord_RunCallbacks");
        if (!g_pDiscord_UpdatePresence) Console.Error("[Discord RPC] Missing Discord_UpdatePresence");
        return false;
    }
}

// Shutdown Discord RPC
void ShutdownDiscordRPC() {
    // Stop the callback thread
    g_discordThreadRunning = false;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // Give it time to finish
    
    if (g_pDiscord_Shutdown) {
        Console.Write("[Discord RPC] Shutting down Discord RPC...");
        g_pDiscord_Shutdown();
    }
    
    if (g_hDiscordDll) {
        Console.Write("[Discord RPC] Unloading Discord RPC DLL...");
        FreeLibrary(g_hDiscordDll);
        g_hDiscordDll = nullptr;
    }
}

// Start Discord RPC with the given username
void StartDiscordRPC(const char* username) {
    if (g_discordRpcInitialized) {
        Console.Write("[Discord RPC] Already initialized, not starting again");
        return;
    }
    
    // Check if username is empty
    if (!username || strlen(username) == 0) {
        Console.Error("[Discord RPC] Cannot start with empty username");
        return;
    }
    
    std::string rl_username = username;
    
    // Set version string
    const char* kingmod_version = "1.0.0";
    
    // Build the state and details strings
    char user_text[256];
    char build_text[256];
    sprintf_s(user_text, "User: %s", rl_username.c_str());
    sprintf_s(build_text, "Build: %s", kingmod_version);
    
    // Start Discord RPC
    if (!LoadDiscordRPC()) {
        Console.Error("[Discord RPC] Initialization failed - couldn't load Discord RPC DLL.");
        return;
    }

    Console.Write("[Discord RPC] Setting up handlers...");
    
    // Set up the event handlers
    DiscordEventHandlers handlers;
    memset(&handlers, 0, sizeof(handlers));
    handlers.ready = HandleDiscordReady;
    handlers.disconnected = HandleDiscordDisconnected;
    handlers.errored = HandleDiscordError;

    // Initialize Discord RPC with the specified application ID
    Console.Write("[Discord RPC] Initializing with application ID: 1372916197951143986");
    g_pDiscord_Initialize("1372916197951143986", &handlers, 1, nullptr);

    // Set the initial rich presence
    Console.Write("[Discord RPC] Setting rich presence data...");
    DiscordRichPresence discordPresence;
    memset(&discordPresence, 0, sizeof(discordPresence));
    discordPresence.details = user_text; // Username on top line
    discordPresence.state = build_text;  // Build version on bottom line
    discordPresence.startTimestamp = time(0);
    
    // Set up image keys
    discordPresence.largeImageKey = "kingmod"; // Main app icon
    discordPresence.largeImageText = "KingMod for Rocket League";
    discordPresence.smallImageKey = "ssl";     // Using ssl.png
    discordPresence.smallImageText = "Rocket League with KingMod";
    
    Console.Write(std::string("[Discord RPC] User: ") + rl_username);
    Console.Write(std::string("[Discord RPC] Build: ") + kingmod_version);
    
    g_pDiscord_UpdatePresence(&discordPresence);
    Console.Write("[Discord RPC] Presence data sent to Discord");

    // Start a callback thread to keep the presence updated
    g_discordThreadRunning = true;
    g_discordCallbackThread = std::thread(DiscordCallbackLoop);
    g_discordCallbackThread.detach(); // Let it run independently

    // Also run initial callbacks
    g_pDiscord_RunCallbacks();
    
    // Mark as initialized
    g_discordRpcInitialized = true;
    
    // Hide the dialog once initialized
    g_showUsernameDialog = false;
}
