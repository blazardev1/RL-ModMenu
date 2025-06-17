#define DISCORD_RPC_CPP_EXPORTS
#include "DiscordRPCWrapper.h"
#include "DiscordRPC.h"

// Forward declarations from DiscordRPC.cpp
extern "C" {
    void InitDiscordRPC_Impl();
    void ShutdownDiscordRPC_Impl();
    void UpdateDiscordRPC_Impl();
}

DISCORD_RPC_API void InitDiscordRPC() {
    // Forward to the implementation in DiscordRPC.cpp
    InitDiscordRPC_Impl();
}

DISCORD_RPC_API void ShutdownDiscordRPC() {
    // Forward to the implementation in DiscordRPC.cpp
    ShutdownDiscordRPC_Impl();
}

DISCORD_RPC_API void UpdateDiscordRPC() {
    // Forward to the implementation in DiscordRPC.cpp
    UpdateDiscordRPC_Impl();
}
