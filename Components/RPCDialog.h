#pragma once

// Interface for Discord RPC username dialog and initialization

// Global variables for Discord RPC dialog
extern bool g_discordRpcInitialized;
extern bool g_showUsernameDialog;
extern char g_rpcUsername[256];

// Function to start Discord RPC with the given username
void StartDiscordRPC(const char* username);
