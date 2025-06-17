#pragma once

#include <string>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>
#include <Windows.h>

class DiscordRPC {
public:
    static DiscordRPC& getInstance();
    
    void initialize(const char* applicationId);
    void shutdown();
    void updatePresence(
        const char* state,
        const char* details,
        int64_t startTimestamp,
        const char* largeImageKey,
        const char* largeImageText,
        const char* smallImageKey,
        const char* smallImageText
    );
    
    void runCallbacks();
    
private:
    DiscordRPC();
    ~DiscordRPC();
    
    // Prevent copying
    DiscordRPC(const DiscordRPC&) = delete;
    DiscordRPC& operator=(const DiscordRPC&) = delete;
    
    void connectToDiscord();
    void disconnect();
    void heartbeatThread();
    void sendHandshake();
    void sendFrame(const std::string& data);
    bool receiveFrame(std::string& out);
    
    // WebSocket connection
    SOCKET m_socket = INVALID_SOCKET;
    std::atomic<bool> m_running{false};
    std::thread m_heartbeatThread;
    std::thread m_ioThread;
    std::mutex m_mutex;
    
    // Discord connection info
    std::string m_clientId;
    int m_pid = 0;
    std::string m_transport = "ipc";
    uint32_t m_version = 1;
    
    // Presence data
    struct PresenceData {
        std::string state;
        std::string details;
        int64_t startTimestamp = 0;
        std::string largeImageKey;
        std::string largeImageText;
        std::string smallImageKey;
        std::string smallImageText;
    } m_presence;
    
    std::atomic<bool> m_presenceDirty{false};
    std::mutex m_presenceMutex;
};
