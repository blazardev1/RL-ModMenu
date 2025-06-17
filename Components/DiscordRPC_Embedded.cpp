#include "DiscordRPC_Embedded.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <sstream>
#include <random>
#include <codecvt>
#include <locale>
#include <fstream>
#include <nlohmann/json.hpp>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "shlwapi.lib")

using json = nlohmann::json;

namespace {
    constexpr const char* DISCORD_IPC_PATHS[] = {
        "/tmp/discord-ipc-0",
        "/tmp/discord-ipc-1",
        "/tmp/discord-ipc-2",
        "/tmp/discord-ipc-3",
        "/tmp/discord-ipc-4",
        "/tmp/discord-ipc-5",
        "/tmp/discord-ipc-6",
        "/tmp/discord-ipc-7",
        "/tmp/discord-ipc-8",
        "/tmp/discord-ipc-9"
    };

    constexpr size_t DISCORD_IPC_PATH_COUNT = sizeof(DISCORD_IPC_PATHS) / sizeof(DISCORD_IPC_PATHS[0]);
    constexpr uint32_t DISCORD_IPC_HEADER_SIZE = 8;
    constexpr uint32_t DISCORD_IPC_MAX_MESSAGE_SIZE = 1024 * 16;
    constexpr uint32_t OP_HANDSHAKE = 0;
    constexpr uint32_t OP_FRAME = 1;
    constexpr uint32_t OP_CLOSE = 2;
    constexpr uint32_t OP_PING = 3;
    constexpr uint32_t OP_PONG = 4;

    struct DiscordIPCMessage {
        uint32_t opcode;
        uint32_t length;
        std::string message;
    };

    std::string getTempPathForDiscordIPC() {
        char tempPath[MAX_PATH];
        if (GetTempPathA(MAX_PATH, tempPath) == 0) {
            return "";
        }
        return std::string(tempPath) + "discord-ipc-0";
    }

    std::string generateNonce() {
        static const char charset[] =
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";
        std::string result;
        result.resize(32);
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, sizeof(charset) - 2);
        
        for (size_t i = 0; i < 32; ++i) {
            result[i] = charset[dis(gen)];
        }
        
        return result;
    }
}

DiscordRPC& DiscordRPC::getInstance() {
    static DiscordRPC instance;
    return instance;
}

DiscordRPC::DiscordRPC() {
    // Initialize Winsock
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    
    // Get current process ID
    m_pid = GetCurrentProcessId();
}

DiscordRPC::~DiscordRPC() {
    shutdown();
    WSACleanup();
}

void DiscordRPC::initialize(const char* applicationId) {
    if (m_running) return;
    
    m_clientId = applicationId;
    m_running = true;
    
    // Start the connection thread
    m_ioThread = std::thread([this]() {
        while (m_running) {
            connectToDiscord();
            
            if (!m_running) break;
            
            // Wait before attempting to reconnect
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    });
    
    // Start the heartbeat thread
    m_heartbeatThread = std::thread(&DiscordRPC::heartbeatThread, this);
}

void DiscordRPC::shutdown() {
    if (!m_running) return;
    
    m_running = false;
    
    // Close the socket to unblock any pending operations
    if (m_socket != INVALID_SOCKET) {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }
    
    // Wait for threads to finish
    if (m_heartbeatThread.joinable()) {
        m_heartbeatThread.join();
    }
    
    if (m_ioThread.joinable()) {
        m_ioThread.join();
    }
}

void DiscordRPC::updatePresence(
    const char* state,
    const char* details,
    int64_t startTimestamp,
    const char* largeImageKey,
    const char* largeImageText,
    const char* smallImageKey,
    const char* smallImageText
) {
    std::lock_guard<std::mutex> lock(m_presenceMutex);
    
    if (state) m_presence.state = state;
    if (details) m_presence.details = details;
    m_presence.startTimestamp = startTimestamp;
    if (largeImageKey) m_presence.largeImageKey = largeImageKey;
    if (largeImageText) m_presence.largeImageText = largeImageText;
    if (smallImageKey) m_presence.smallImageKey = smallImageKey;
    if (smallImageText) m_presence.smallImageText = smallImageText;
    
    m_presenceDirty = true;
}

void DiscordRPC::runCallbacks() {
    if (!m_running || m_socket == INVALID_SOCKET) {
        return;
    }
    
    // Check if presence needs to be updated
    bool shouldUpdate = false;
    {
        std::lock_guard<std::mutex> lock(m_presenceMutex);
        shouldUpdate = m_presenceDirty;
        m_presenceDirty = false;
    }
    
    if (shouldUpdate) {
        json presence = {
            {"cmd", "SET_ACTIVITY"},
            {"args", {
                {"pid", m_pid},
                {"activity", {
                    {"state", m_presence.state},
                    {"details", m_presence.details},
                    {"timestamps", {
                        {"start", m_presence.startTimestamp}
                    }},
                    {"assets", {
                        {"large_image", m_presence.largeImageKey},
                        {"large_text", m_presence.largeImageText},
                        {"small_image", m_presence.smallImageKey.empty() ? nullptr : m_presence.smallImageKey},
                        {"small_text", m_presence.smallImageText.empty() ? nullptr : m_presence.smallImageText}
                    }}
                }}
            }},
            {"nonce", generateNonce()}
        };
        
        json args = presence["args"];
        if (m_presence.smallImageKey.empty()) {
            args["activity"]["assets"].erase("small_image");
            args["activity"]["assets"].erase("small_text");
        }
        
        std::string message = presence.dump();
        
        // Send the frame
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_socket != INVALID_SOCKET) {
            sendFrame(message);
        }
    }
    
    // Process incoming messages
    std::string message;
    while (receiveFrame(message)) {
        try {
            json data = json::parse(message);
            uint32_t op = data["op"].get<uint32_t>();
            
            switch (op) {
                case OP_PING:
                {
                    // Respond to ping with pong
                    json pong = {
                        {"op", OP_PONG},
                        {"data", data["data"]}
                    };
                    sendFrame(pong.dump());
                    break;
                }
                case OP_CLOSE:
                    // Server requested disconnect
                    disconnect();
                    break;
                default:
                    // Ignore other opcodes
                    break;
            }
        } catch (const std::exception& e) {
            // Invalid JSON or missing fields
        }
    }
}

void DiscordRPC::connectToDiscord() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Close existing connection if any
    if (m_socket != INVALID_SOCKET) {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }
    
    // Try to connect to Discord IPC socket
    std::string ipcPath = getTempPathForDiscordIPC();
    if (ipcPath.empty()) {
        return;
    }
    
    // Create a Windows named pipe
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring pipeName = L"\\\\?\\pipe\\discord-ipc-0";
    
    // Try to connect to the pipe
    HANDLE hPipe = CreateFileW(
        pipeName.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );
    
    if (hPipe == INVALID_HANDLE_VALUE) {
        return;
    }
    
    // Convert the Windows pipe handle to a socket
    m_socket = _open_osfhandle(reinterpret_cast<intptr_t>(hPipe), 0);
    if (m_socket == -1) {
        CloseHandle(hPipe);
        return;
    }
    
    // Send handshake
    sendHandshake();
}

void DiscordRPC::disconnect() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_socket != INVALID_SOCKET) {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }
}

void DiscordRPC::heartbeatThread() {
    while (m_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 / 60)); // 60 FPS
        
        if (m_socket != INVALID_SOCKET) {
            // Send a ping every 30 seconds
            static auto lastPing = std::chrono::steady_clock::now();
            auto now = std::chrono::steady_clock::now();
            
            if (std::chrono::duration_cast<std::chrono::seconds>(now - lastPing).count() >= 30) {
                json ping = {
                    {"op", OP_PING},
                    {"data", generateNonce()}
                };
                
                std::lock_guard<std::mutex> lock(m_mutex);
                if (m_socket != INVALID_SOCKET) {
                    sendFrame(ping.dump());
                }
                
                lastPing = now;
            }
        }
    }
}

void DiscordRPC::sendHandshake() {
    json handshake = {
        {"v", 1},
        {"client_id", m_clientId}
    };
    
    std::string message = handshake.dump();
    
    // Send the handshake as a frame with OP_HANDSHAKE
    uint32_t op = OP_HANDSHAKE;
    uint32_t len = static_cast<uint32_t>(message.length());
    
    std::vector<uint8_t> buffer(DISCORD_IPC_HEADER_SIZE + len);
    memcpy(buffer.data(), &op, sizeof(op));
    memcpy(buffer.data() + sizeof(op), &len, sizeof(len));
    memcpy(buffer.data() + DISCORD_IPC_HEADER_SIZE, message.c_str(), len);
    
    send(m_socket, reinterpret_cast<const char*>(buffer.data()), static_cast<int>(buffer.size()), 0);
}

void DiscordRPC::sendFrame(const std::string& data) {
    if (m_socket == INVALID_SOCKET) return;
    
    uint32_t op = OP_FRAME;
    uint32_t len = static_cast<uint32_t>(data.length());
    
    std::vector<uint8_t> buffer(DISCORD_IPC_HEADER_SIZE + len);
    memcpy(buffer.data(), &op, sizeof(op));
    memcpy(buffer.data() + sizeof(op), &len, sizeof(len));
    memcpy(buffer.data() + DISCORD_IPC_HEADER_SIZE, data.c_str(), len);
    
    send(m_socket, reinterpret_cast<const char*>(buffer.data()), static_cast<int>(buffer.size()), 0);
}

bool DiscordRPC::receiveFrame(std::string& out) {
    if (m_socket == INVALID_SOCKET) return false;
    
    // Check if there's data available
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(m_socket, &readSet);
    
    timeval timeout = {0, 0};
    int result = select(0, &readSet, nullptr, nullptr, &timeout);
    
    if (result <= 0 || !FD_ISSET(m_socket, &readSet)) {
        return false;
    }
    
    // Read the header
    uint8_t header[DISCORD_IPC_HEADER_SIZE];
    int bytesRead = recv(m_socket, reinterpret_cast<char*>(header), DISCORD_IPC_HEADER_SIZE, 0);
    
    if (bytesRead != DISCORD_IPC_HEADER_SIZE) {
        if (bytesRead <= 0) {
            // Connection closed or error
            disconnect();
        }
        return false;
    }
    
    // Parse the header
    uint32_t op = *reinterpret_cast<uint32_t*>(header);
    uint32_t length = *reinterpret_cast<uint32_t*>(header + 4);
    
    if (length > DISCORD_IPC_MAX_MESSAGE_SIZE) {
        // Message too large, discard
        return false;
    }
    
    // Read the message
    std::vector<char> buffer(length + 1);
    bytesRead = recv(m_socket, buffer.data(), static_cast<int>(length), 0);
    
    if (bytesRead != static_cast<int>(length)) {
        return false;
    }
    
    buffer[length] = '\0';
    out.assign(buffer.data(), length);
    
    return true;
}
