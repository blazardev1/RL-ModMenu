#pragma once

#ifdef _WIN32
    #ifdef DISCORD_RPC_CPP_EXPORTS
        #define DISCORD_RPC_API __declspec(dllexport)
    #else
        #define DISCORD_RPC_API __declspec(dllimport)
    #endif
#else
    #define DISCORD_RPC_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

DISCORD_RPC_API void InitDiscordRPC();
DISCORD_RPC_API void ShutdownDiscordRPC();
DISCORD_RPC_API void UpdateDiscordRPC();

#ifdef __cplusplus
}
#endif
