#define DISCORD_RPC_CPP_IMPLEMENTATION
#define DISCORD_RPC_CPP_EXPORTS

#include "DiscordRPC.h"
#include "DiscordRPCWrapper.h"
#include "DiscordRPC_Embedded.h"
#include <Windows.h>
#include <string>
#include <cstring>
#include <memory>

// Global instance of our embedded Discord RPC
static DiscordRPC* g_discordRpc = nullptr;
static std::string g_username;
static bool g_initialized = false;

// The actual implementation
static void InitDiscordRPC_Impl() {
    if (g_initialized) {
        return;
    }

    // Initialize our embedded Discord RPC
    g_discordRpc = &DiscordRPC::getInstance();
    g_discordRpc->initialize("1372916197951143986");
    
    // Get username from system
    char username[256] = { 0 };
    DWORD username_len = 256;
    GetUserNameA(username, &username_len);
    g_username = username;
    
    // Define version
    const char* version = "1.2.0";
    
    // Create state with build version
    char state[128] = { 0 };
    snprintf(state, sizeof(state), "build: %s", version);
    
    // Create details with username
    char details[128] = { 0 };
    snprintf(details, sizeof(details), "user: %s", username);
    
    // Set the initial rich presence
    g_discordRpc->updatePresence(
        state,                   // state
        details,                 // details
        time(0),                 // startTimestamp
        "icon",                  // largeImageKey
        "KingMod",               // largeImageText
        "ssl",                   // smallImageKey
        "KingMod SSL"            // smallImageText
    );
    
    g_initialized = true;
}

static void ShutdownDiscordRPC_Impl() {
    if (g_initialized && g_discordRpc) {
        g_discordRpc->shutdown();
        g_discordRpc = nullptr;
        g_initialized = false;
    }
}

static void UpdateDiscordRPC_Impl() {
    if (g_initialized && g_discordRpc) {
        g_discordRpc->runCallbacks();
    }
}

// Exported functions - these maintain the same interface as before
DISCORD_RPC_API void InitDiscordRPC() {
    InitDiscordRPC_Impl();
}

DISCORD_RPC_API void ShutdownDiscordRPC() {
    ShutdownDiscordRPC_Impl();
}

DISCORD_RPC_API void UpdateDiscordRPC() {
    UpdateDiscordRPC_Impl();
}
