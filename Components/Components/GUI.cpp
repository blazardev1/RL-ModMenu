#include "GUI.hpp"
#include "../Includes.hpp"

#include <d3d11.h>
#include <winuser.h>
#include <dxgi.h>
#include <windows.h>
#include <shellapi.h>

#include "../ImGui/Kiero/kiero.h"
#include "../ImGui/imgui.h"
#include "../ImGui/imgui_impl_win32.h"
#include "../ImGui/imgui_impl_dx11.h"
#include "../ImGui/imgui_stdlib.h"

typedef HRESULT(__stdcall* Present) (IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
typedef LRESULT(CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);
typedef uintptr_t PTR;

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
// Global variables for rendering
Present oPresent;
HWND window = NULL;
WNDPROC oWndProc;
ID3D11Device* pDevice = NULL;
ID3D11DeviceContext* pContext = NULL;
ID3D11RenderTargetView* mainRenderTargetView;

// Global variable for Alt+F4 functionality
bool g_enableAltF4 = false;

GUIComponent::GUIComponent() : Component("UserInterface", "Displays an interface") { OnCreate(); }

GUIComponent::~GUIComponent() { OnDestroy(); }

bool init = false;

HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
    if (!init)
    {
        if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&pDevice)))
        {
            pDevice->GetImmediateContext(&pContext);
            DXGI_SWAP_CHAIN_DESC sd;
            pSwapChain->GetDesc(&sd);
            window = sd.OutputWindow;
            ID3D11Texture2D* pBackBuffer;
            pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
            pDevice->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetView);
            pBackBuffer->Release();
            oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)GUIComponent::WndProc);
            GUIComponent::InitImGui();
            init = true;
        }

        else
            return oPresent(pSwapChain, SyncInterval, Flags);
    }

    GUIComponent::Render();

    pContext->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    return oPresent(pSwapChain, SyncInterval, Flags);
}

DWORD WINAPI MainThread()
{
    bool init_hook = false;
    do
    {
        if (kiero::init(kiero::RenderType::D3D11) == kiero::Status::Success)
        {
            kiero::bind(8, (void**)&oPresent, hkPresent);
            init_hook = true;
        }
    } while (!init_hook);
    return TRUE;
}

void GUIComponent::OnCreate() {}

void GUIComponent::OnDestroy() {}

void GUIComponent::Unload()
{

    if (init)
    {
        kiero::shutdown();
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        SetWindowLongPtrW(window, GWLP_WNDPROC, (LONG_PTR)oWndProc);
    }

    CloseHandle(InterfaceThread);
}

HANDLE GUIComponent::InterfaceThread = NULL;

bool GUIComponent::IsOpen = true;

void GUIComponent::Initialize() {
    InterfaceThread = CreateThread(nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(MainThread), nullptr, 0, nullptr);
}

void GUIComponent::InitImGui()
{
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;
    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX11_Init(pDevice, pContext);
    //io.Fonts->AddFontFromFileTTF("Helvetica.ttf", 13);

}

LRESULT __stdcall GUIComponent::WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);
    if (io.WantCaptureMouse && (uMsg == WM_LBUTTONDOWN || uMsg == WM_LBUTTONUP || uMsg == WM_RBUTTONDOWN || uMsg == WM_RBUTTONUP || uMsg == WM_MBUTTONDOWN || uMsg == WM_MBUTTONUP || uMsg == WM_MOUSEWHEEL || uMsg == WM_MOUSEMOVE))
    {
        return TRUE;
    }

    return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}

void DefaultImGuiStyle()
{
    ImGuiStyle& style = ImGui::GetStyle();

    // Modern rounded style
    style.WindowRounding = 8.0f;
    style.ChildRounding = 8.0f;
    style.FrameRounding = 8.0f;
    style.GrabRounding = 8.0f;
    style.TabRounding = 8.0f;
    style.PopupRounding = 8.0f;
    style.ScrollbarRounding = 8.0f;
    style.GrabMinSize = 12.0f;

    // Padding and spacing
    style.WindowPadding = ImVec2(12.0f, 12.0f);
    style.FramePadding = ImVec2(8.0f, 6.0f);
    style.ItemSpacing = ImVec2(12.0f, 8.0f);
    style.ItemInnerSpacing = ImVec2(8.0f, 6.0f);
    style.IndentSpacing = 24.0f;
    style.ScrollbarSize = 14.0f;
    style.WindowBorderSize = 0.0f;
    style.ChildBorderSize = 0.0f;
    style.FrameBorderSize = 0.0f;
    style.PopupBorderSize = 0.0f;
    style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
    style.WindowMenuButtonPosition = ImGuiDir_Left;
    style.ColorButtonPosition = ImGuiDir_Right;
    style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

    // Colors
    style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.30f, 0.60f, 0.99f, 0.78f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.30f, 0.60f, 0.99f, 1.00f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    style.Colors[ImGuiCol_Separator] = ImVec4(0.28f, 0.28f, 0.28f, 0.78f);
    style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.78f);
    style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.67f, 0.67f, 0.67f, 0.78f);
    style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    style.Colors[ImGuiCol_Tab] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    style.Colors[ImGuiCol_TabHovered] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
    style.Colors[ImGuiCol_TabActive] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
    style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    style.Colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}

void GUIComponent::InitMainTab() {
    // Game Titles section
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.0f, 1.0f));
    ImGui::Text("Game Titles");
    ImGui::PopStyleColor();
    ImGui::Separator();

    ImGui::TextWrapped("This box shows common Rocket League titles with their colors.");

    // Define a simple structure for titles
    struct TitleInfo {
        const char* name;
        const char* color;
        int id;
    };

    // Static list of titles that persists between frames
    static std::vector<TitleInfo> gameTitles;
    static bool titlesLoaded = false;

    // Only load titles once
    if (!titlesLoaded) {
        // Season Titles
        gameTitles.push_back({ "Season 1 Grand Champion", "#FF0000", 101 });
        gameTitles.push_back({ "Season 2 Grand Champion", "#FF4500", 102 });
        gameTitles.push_back({ "Season 3 Grand Champion", "#FF6347", 103 });

        // Special Titles
        gameTitles.push_back({ "RLCS World Champion", "#FFD700", 201 });
        gameTitles.push_back({ "RLCS Elite", "#C0C0C0", 202 });
        gameTitles.push_back({ "Verified", "#00FF00", 209 });
        gameTitles.push_back({ "Developer", "#FF00FF", 210 });

        // Rank Titles
        gameTitles.push_back({ "Bronze Champion", "#CD7F32", 301 });
        gameTitles.push_back({ "Silver Champion", "#C0C0C0", 302 });
        gameTitles.push_back({ "Gold Champion", "#FFD700", 303 });
        gameTitles.push_back({ "Platinum Champion", "#7FFFD4", 304 });
        gameTitles.push_back({ "Diamond Champion", "#1E90FF", 305 });
        gameTitles.push_back({ "Champion Elite", "#9400D3", 306 });
        gameTitles.push_back({ "Grand Champion", "#FF0000", 307 });
        gameTitles.push_back({ "Supersonic Legend", "#FFFFFF", 308 });

        // Achievement Titles
        gameTitles.push_back({ "MVP", "#9400D3", 408 });
        gameTitles.push_back({ "The Magnificent", "#FF1493", 405 });

        titlesLoaded = true;
    }

    // Refresh button
    if (ImGui::Button("Refresh Titles", ImVec2(120, 30))) {
        // Could be expanded to reload titles from the game
        Main.SpawnNotification("Titles", "Refreshed titles list", 3);
    }

    // Filter input for title search
    static char searchBuffer[128] = "";
    ImGui::SameLine();
    ImGui::PushItemWidth(200);
    ImGui::InputText("Filter", searchBuffer, IM_ARRAYSIZE(searchBuffer));
    ImGui::PopItemWidth();
    std::string searchLower = searchBuffer;
    std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(), ::tolower);

    // Title count
    ImGui::SameLine();
    ImGui::TextDisabled("(%d titles)", (int)gameTitles.size());

    // Create a box to show all titles
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
    if (ImGui::BeginChild("TitlesBox", ImVec2(0, 300), true, ImGuiWindowFlags_AlwaysVerticalScrollbar)) {
        // Table to display titles
        if (ImGui::BeginTable("TitlesTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            // Table headers
            ImGui::TableSetupColumn("Title Name", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Color", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 50.0f);
            ImGui::TableHeadersRow();

            // Display titles with colors
            for (const auto& title : gameTitles) {
                // Apply filter if search text is not empty
                std::string titleLower = title.name;
                std::transform(titleLower.begin(), titleLower.end(), titleLower.begin(), ::tolower);
                if (!searchLower.empty() && titleLower.find(searchLower) == std::string::npos) {
                    continue; // Skip this title if it doesn't match the search
                }

                ImGui::TableNextRow();

                // Title Name
                ImGui::TableSetColumnIndex(0);

                // Parse color and apply it
                ImVec4 titleColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // Default white
                if (strlen(title.color) >= 7) { // #RRGGBB format
                    // Parse hex color manually
                    std::string hexColor(title.color); // Convert const char* to string
                    hexColor = hexColor.substr(1); // Skip the '#'
                    int r = 255, g = 255, b = 255; // Default white

                    if (hexColor.length() >= 6) {
                        char* endPtr = nullptr;
                        // Get red component (first 2 chars)
                        r = (int)strtol(hexColor.substr(0, 2).c_str(), &endPtr, 16);
                        // Get green component (middle 2 chars)
                        g = (int)strtol(hexColor.substr(2, 2).c_str(), &endPtr, 16);
                        // Get blue component (last 2 chars)
                        b = (int)strtol(hexColor.substr(4, 2).c_str(), &endPtr, 16);
                    }

                    titleColor = ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
                }

                ImGui::PushStyleColor(ImGuiCol_Text, titleColor);
                ImGui::Text("%s", title.name);
                ImGui::PopStyleColor();

                // Color value
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%s", title.color);

                // ID
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%d", title.id);
            }

            ImGui::EndTable();
        }

        ImGui::EndChild();
    }
    ImGui::PopStyleVar();

    // Information about titles
    ImGui::TextWrapped("Titles are displayed in their colors. Use the filter to search for specific titles.");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Game Actions section
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.8f, 1.0f));
    ImGui::Text("Game Actions");
    ImGui::PopStyleColor();
    ImGui::Separator();


    ImGui::SameLine();


    ImGui::Spacing();
    ImGui::Separator();
}


// RGB color cycling effect
static ImVec4 GetRainbowColor(float time, float speed = 1.0f) {
    float r = (sinf(time * speed) + 1.0f) * 0.5f;
    float g = (sinf(time * speed + 2.0f) + 1.0f) * 0.5f;
    float b = (sinf(time * speed + 4.0f) + 1.0f) * 0.5f;
    return ImVec4(r, g, b, 1.0f);
}

void GUIComponent::Render()
{
    static float time = 0.0f;
    time += 0.01f;  // Animation speed

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGuiIO& io = ImGui::GetIO();
    GUI.DisplayX = io.DisplaySize.x;
    GUI.DisplayY = io.DisplaySize.y;
    
    // Alt+F4 functionality - works even when menu is hidden
    if (g_enableAltF4)
    {
        SHORT altState = GetAsyncKeyState(VK_MENU); // ALT
        SHORT f4State = GetAsyncKeyState(VK_F4);   // F4

        // Check if both keys are pressed this frame
        if ((altState & 0x8000) && (f4State & 0x8000))
        {
            HWND hwnd = FindWindowA(nullptr, "Rocket League (64-bit, DX11, Cooked)");
            if (!hwnd) hwnd = FindWindowA(nullptr, "Rocket League");
            if (!hwnd) hwnd = FindWindowA(nullptr, "RocketLeague.exe");

            if (hwnd)
            {
                PostMessage(hwnd, WM_CLOSE, 0, 0);
            }
        }
    }

    // Toggle menu with F1 key
    static bool f1WasPressed = false;
    bool f1IsPressed = (GetAsyncKeyState(VK_F1) & 0x8000) != 0;

    // Toggle menu state when F1 is pressed
    if (f1IsPressed && !f1WasPressed) {
        IsOpen = !IsOpen;
    }
    f1WasPressed = f1IsPressed;

    io.MouseDrawCursor = IsOpen; // Only show cursor when menu is open
    ImGui::SetNextWindowSize(ImVec2(840, 450));

    static int currentStyle = 0;
    static bool isFirstFrame = true;
    if (isFirstFrame) {
        DefaultImGuiStyle();
        currentStyle = 0; // Set to Dark theme
        isFirstFrame = false;
    }

    // Only render the menu if it's open
    if (!IsOpen) {
        ImGui::Render();
        return;
    }

    // Main window with no title bar (we'll draw our own)
    if (ImGui::Begin("##KingMod", &IsOpen,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_AlwaysAutoResize))
    {
        // Draw custom title bar with centered text
        float windowWidth = ImGui::GetWindowWidth();

        // KingMod title with version
        std::string titleText = "KingMod v1.0.0 - https://discord.gg/Dsx9BdFKxH";
        ImGui::SetCursorPosX((windowWidth - ImGui::CalcTextSize(titleText.c_str()).x) * 0.5f);

        // Draw "King" in white and "Mod" in red
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "King");
        ImGui::SameLine(0, 0);
        ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "Mod");

        // Version
        ImGui::SameLine(0, 0);
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), " v1.0.0");

        // Separator
        ImGui::SameLine(0, 0);
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), " - ");

        // Make the link clickable
        ImGui::SameLine(0, 0);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.5f, 1.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImGui::GetColorU32(ImGuiCol_HeaderHovered));
        ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.0f, 0.5f));
        if (ImGui::Selectable("https://discord.gg/Dsx9BdFKxH", false, ImGuiSelectableFlags_AllowDoubleClick | ImGuiSelectableFlags_DontClosePopups)) {
            ShellExecuteA(0, "open", "https://discord.gg/Dsx9BdFKxH", 0, 0, SW_SHOW);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);

        // FPS counter in top-right
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 80);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetTextLineHeight() - 4);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
        ImGui::Text("FPS: %.0f", io.Framerate);
        ImGui::PopStyleColor();

        ImGui::Separator();

        // Apply RGB border effect to the window
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
        ImVec4 borderColor = GetRainbowColor(time);
        ImGui::PushStyleColor(ImGuiCol_Border, borderColor);

        if (ImGui::BeginTabBar("Tab bar", ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_FittingPolicyScroll | ImGuiTabBarFlags_NoTooltip))
        {

           

            // Regular tabs without RGB effect
            if (ImGui::BeginTabItem("Items"))
            {
                ImGui::Spacing();

                // Items section header
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.8f, 1.0f, 1.0f));
                ImGui::Text("Rocket League Items");
                ImGui::PopStyleColor();
                ImGui::Separator();

                // Setup tabs for item categories
                ImGui::BeginTabBar("ItemCategories", ImGuiTabBarFlags_FittingPolicyScroll);

                // Black Market Items Tab
                if (ImGui::BeginTabItem("Black Market"))
                {
                    // Define item structure
                    struct ItemInfo {
                        const char* name;
                        const char* type;
                        const char* color;
                        int id;
                    };

                    // Static list of items
                    static std::vector<ItemInfo> blackMarketItems;
                    static bool blackMarketLoaded = false;

                    if (!blackMarketLoaded) {
                        // Black Market Decals
                        blackMarketItems.push_back({ "20XX", "Decal", "#FF00FF", 1001 });
                        blackMarketItems.push_back({ "Biomass", "Decal", "#00FF00", 1002 });
                        blackMarketItems.push_back({ "Bubbly", "Decal", "#87CEEB", 1003 });
                        blackMarketItems.push_back({ "Chameleon", "Decal", "#FFA500", 1004 });
                        blackMarketItems.push_back({ "Dissolver", "Decal", "#40E0D0", 1005 });
                        blackMarketItems.push_back({ "Fire God", "Decal", "#FF0000", 1006 });
                        blackMarketItems.push_back({ "Heatwave", "Decal", "#FF4500", 1007 });
                        blackMarketItems.push_back({ "Hexed", "Decal", "#800080", 1008 });
                        blackMarketItems.push_back({ "Interstellar", "Decal", "#6A5ACD", 1009 });
                        blackMarketItems.push_back({ "Labyrinth", "Decal", "#663399", 1010 });
                        blackMarketItems.push_back({ "Mainframe", "Decal", "#FFFFFF", 1011 });
                        blackMarketItems.push_back({ "Parallax", "Decal", "#1E90FF", 1012 });
                        blackMarketItems.push_back({ "Slipstream", "Decal", "#00BFFF", 1013 });
                        blackMarketItems.push_back({ "Spectre", "Decal", "#7B68EE", 1014 });
                        blackMarketItems.push_back({ "Stipple Gait", "Decal", "#B8860B", 1015 });
                        blackMarketItems.push_back({ "Storm Watch", "Decal", "#4169E1", 1016 });
                        blackMarketItems.push_back({ "Streamline", "Decal", "#20B2AA", 1017 });
                        blackMarketItems.push_back({ "Tidal Stream", "Decal", "#5F9EA0", 1018 });
                        blackMarketItems.push_back({ "Trigon", "Decal", "#BA55D3", 1019 });
                        blackMarketItems.push_back({ "Wet Paint", "Decal", "#008080", 1020 });

                        // Black Market Goal Explosions
                        blackMarketItems.push_back({ "Atomizer", "Goal Explosion", "#FFD700", 1101 });
                        blackMarketItems.push_back({ "Dueling Dragons", "Goal Explosion", "#4169E1", 1102 });
                        blackMarketItems.push_back({ "Electroshock", "Goal Explosion", "#00FFFF", 1103 });
                        blackMarketItems.push_back({ "Fireworks", "Goal Explosion", "#FF0000", 1104 });
                        blackMarketItems.push_back({ "Gravity Bomb", "Goal Explosion", "#8A2BE2", 1105 });
                        blackMarketItems.push_back({ "Hellfire", "Goal Explosion", "#FF4500", 1106 });
                        blackMarketItems.push_back({ "Juiced", "Goal Explosion", "#ADFF2F", 1107 });
                        blackMarketItems.push_back({ "Meteor Storm", "Goal Explosion", "#B22222", 1108 });
                        blackMarketItems.push_back({ "Neuro-Agitator", "Goal Explosion", "#800080", 1109 });
                        blackMarketItems.push_back({ "Party Time", "Goal Explosion", "#00FF00", 1110 });
                        blackMarketItems.push_back({ "Poly Pop", "Goal Explosion", "#1E90FF", 1111 });
                        blackMarketItems.push_back({ "Popcorn", "Goal Explosion", "#FFA500", 1112 });
                        blackMarketItems.push_back({ "Shattered", "Goal Explosion", "#87CEEB", 1113 });
                        blackMarketItems.push_back({ "Singularity", "Goal Explosion", "#8B008B", 1114 });
                        blackMarketItems.push_back({ "Solar Flare", "Goal Explosion", "#FFD700", 1115 });
                        blackMarketItems.push_back({ "Sub-Zero", "Goal Explosion", "#00BFFF", 1116 });
                        blackMarketItems.push_back({ "Toon", "Goal Explosion", "#FFFFFF", 1117 });
                        blackMarketItems.push_back({ "Voxel", "Goal Explosion", "#696969", 1118 });

                        blackMarketLoaded = true;
                    }

                    // Filter input for item search
                    static char bmSearchBuffer[128] = "";
                    ImGui::PushItemWidth(200);
                    ImGui::InputText("Filter Black Market Items", bmSearchBuffer, IM_ARRAYSIZE(bmSearchBuffer));
                    ImGui::PopItemWidth();
                    std::string bmSearchLower = bmSearchBuffer;
                    std::transform(bmSearchLower.begin(), bmSearchLower.end(), bmSearchLower.begin(), ::tolower);

                    // Item count
                    ImGui::SameLine();
                    ImGui::TextDisabled("(%d items)", (int)blackMarketItems.size());

                    // Display items in a table
                    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
                    if (ImGui::BeginChild("BlackMarketTable", ImVec2(0, 280), true, ImGuiWindowFlags_AlwaysVerticalScrollbar)) {
                        // Table to display items
                        if (ImGui::BeginTable("BMItemsTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                            // Table headers
                            ImGui::TableSetupColumn("Item Name", ImGuiTableColumnFlags_WidthStretch);
                            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 120.0f);
                            ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 50.0f);
                            ImGui::TableHeadersRow();

                            // Display items with colors
                            for (const auto& item : blackMarketItems) {
                                // Apply filter if search text is not empty
                                std::string itemLower = item.name;
                                std::transform(itemLower.begin(), itemLower.end(), itemLower.begin(), ::tolower);
                                if (!bmSearchLower.empty() &&
                                    itemLower.find(bmSearchLower) == std::string::npos) {
                                    continue; // Skip if doesn't match search
                                }

                                ImGui::TableNextRow();

                                // Item Name
                                ImGui::TableSetColumnIndex(0);

                                // Parse color and apply it
                                ImVec4 itemColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // Default white
                                if (strlen(item.color) >= 7) { // #RRGGBB format
                                    std::string hexColor(item.color);
                                    hexColor = hexColor.substr(1); // Skip the '#'

                                    if (hexColor.length() >= 6) {
                                        char* endPtr = nullptr;
                                        int r = (int)strtol(hexColor.substr(0, 2).c_str(), &endPtr, 16);
                                        int g = (int)strtol(hexColor.substr(2, 2).c_str(), &endPtr, 16);
                                        int b = (int)strtol(hexColor.substr(4, 2).c_str(), &endPtr, 16);
                                        itemColor = ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
                                    }
                                }

                                ImGui::PushStyleColor(ImGuiCol_Text, itemColor);
                                ImGui::Text("%s", item.name);
                                ImGui::PopStyleColor();

                                // Type
                                ImGui::TableSetColumnIndex(1);
                                ImGui::Text("%s", item.type);

                                // ID
                                ImGui::TableSetColumnIndex(2);
                                ImGui::Text("%d", item.id);
                            }

                            ImGui::EndTable();
                        }
                        ImGui::EndChild();
                    }
                    ImGui::PopStyleVar();

                    // Info about black market items
                    ImGui::TextWrapped("Black Market items are the rarest and most valuable items in the game. They include universal decals and goal explosions.");

                    ImGui::EndTabItem();
                }

                // Exotic Items Tab
                if (ImGui::BeginTabItem("Exotic"))
                {
                    ImGui::TextWrapped("Exotic wheels are rare and visually impressive wheels that stand out from other items.");

                    // Define item structure (similar to above)
                    struct ItemInfo {
                        const char* name;
                        const char* type;
                        const char* color;
                        int id;
                    };

                    // Static list of items
                    static std::vector<ItemInfo> exoticItems;
                    static bool exoticLoaded = false;

                    if (!exoticLoaded) {
                        // Exotic Wheels
                        exoticItems.push_back({ "ARA-51", "Wheel", "#FFA500", 2001 });
                        exoticItems.push_back({ "Balla-Carrà", "Wheel", "#00BFFF", 2002 });
                        exoticItems.push_back({ "Centro", "Wheel", "#4169E1", 2003 });
                        exoticItems.push_back({ "Chrono", "Wheel", "#8A2BE2", 2004 });
                        exoticItems.push_back({ "Clockwork", "Wheel", "#FF4500", 2005 });
                        exoticItems.push_back({ "Cruxe", "Wheel", "#FF0000", 2006 });
                        exoticItems.push_back({ "Discotheque", "Wheel", "#FF00FF", 2007 });
                        exoticItems.push_back({ "Draco", "Wheel", "#FFD700", 2008 });
                        exoticItems.push_back({ "Dynamo", "Wheel", "#FF6347", 2009 });
                        exoticItems.push_back({ "Equalizer", "Wheel", "#7B68EE", 2010 });
                        exoticItems.push_back({ "FGSP", "Wheel", "#20B2AA", 2011 });
                        exoticItems.push_back({ "Gernot", "Wheel", "#C0C0C0", 2012 });
                        exoticItems.push_back({ "Hikari P5", "Wheel", "#00FFFF", 2013 });
                        exoticItems.push_back({ "Hypnotik", "Wheel", "#00FF00", 2014 });
                        exoticItems.push_back({ "Infinium", "Wheel", "#40E0D0", 2015 });
                        exoticItems.push_back({ "K2", "Wheel", "#FFFAFA", 2016 });
                        exoticItems.push_back({ "Kalos", "Wheel", "#8B008B", 2017 });
                        exoticItems.push_back({ "Lobo", "Wheel", "#A9A9A9", 2018 });
                        exoticItems.push_back({ "Looper", "Wheel", "#FF1493", 2019 });
                        exoticItems.push_back({ "Photon", "Wheel", "#FFFF00", 2020 });
                        exoticItems.push_back({ "Pulsus", "Wheel", "#9370DB", 2021 });
                        exoticItems.push_back({ "Wonderment", "Wheel", "#FF0000", 2022 });
                        exoticItems.push_back({ "Zomba", "Wheel", "#FFFFFF", 2023 });

                        exoticLoaded = true;
                    }

                    // Filter input
                    static char exoticSearchBuffer[128] = "";
                    ImGui::PushItemWidth(200);
                    ImGui::InputText("Filter Exotic Items", exoticSearchBuffer, IM_ARRAYSIZE(exoticSearchBuffer));
                    ImGui::PopItemWidth();
                    std::string exoticSearchLower = exoticSearchBuffer;
                    std::transform(exoticSearchLower.begin(), exoticSearchLower.end(), exoticSearchLower.begin(), ::tolower);

                    // Item count
                    ImGui::SameLine();
                    ImGui::TextDisabled("(%d wheels)", (int)exoticItems.size());

                    // Display items in a table (similar to black market)
                    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
                    if (ImGui::BeginChild("ExoticTable", ImVec2(0, 280), true, ImGuiWindowFlags_AlwaysVerticalScrollbar)) {
                        // Table display code (similar to above)
                        if (ImGui::BeginTable("ExoticItemsTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                            ImGui::TableSetupColumn("Wheel Name", ImGuiTableColumnFlags_WidthStretch);
                            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 120.0f);
                            ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 50.0f);
                            ImGui::TableHeadersRow();

                            // Display items
                            for (const auto& item : exoticItems) {
                                // Apply filter
                                std::string itemLower = item.name;
                                std::transform(itemLower.begin(), itemLower.end(), itemLower.begin(), ::tolower);
                                if (!exoticSearchLower.empty() &&
                                    itemLower.find(exoticSearchLower) == std::string::npos) {
                                    continue;
                                }

                                ImGui::TableNextRow();

                                // Item Name with color
                                ImGui::TableSetColumnIndex(0);

                                // Parse color and apply it
                                ImVec4 itemColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                                if (strlen(item.color) >= 7) {
                                    std::string hexColor(item.color);
                                    hexColor = hexColor.substr(1);

                                    if (hexColor.length() >= 6) {
                                        char* endPtr = nullptr;
                                        int r = (int)strtol(hexColor.substr(0, 2).c_str(), &endPtr, 16);
                                        int g = (int)strtol(hexColor.substr(2, 2).c_str(), &endPtr, 16);
                                        int b = (int)strtol(hexColor.substr(4, 2).c_str(), &endPtr, 16);
                                        itemColor = ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
                                    }
                                }

                                ImGui::PushStyleColor(ImGuiCol_Text, itemColor);
                                ImGui::Text("%s", item.name);
                                ImGui::PopStyleColor();

                                // Type
                                ImGui::TableSetColumnIndex(1);
                                ImGui::Text("%s", item.type);

                                // ID
                                ImGui::TableSetColumnIndex(2);
                                ImGui::Text("%d", item.id);
                            }

                            ImGui::EndTable();
                        }
                        ImGui::EndChild();
                    }
                    ImGui::PopStyleVar();

                    ImGui::EndTabItem();
                }

                // Import Tab
                if (ImGui::BeginTabItem("Import"))
                {
                    ImGui::TextWrapped("Import quality items are popular cars and other items that come from crates and blueprints.");

                    // List of popular import cars
                    ImGui::Spacing();
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.5f, 0.0f, 1.0f));
                    ImGui::Text("Popular Import Cars");
                    ImGui::PopStyleColor();

                    ImGui::Columns(2, "ImportCars", false);
                    ImGui::SetColumnWidth(0, 200);

                    ImGui::Text("Fennec"); ImGui::NextColumn();
                    ImGui::Text("Most popular competitive car"); ImGui::NextColumn();

                    ImGui::Text("Octane ZSR"); ImGui::NextColumn();
                    ImGui::Text("Variant of the default Octane"); ImGui::NextColumn();

                    ImGui::Text("Dominus GT"); ImGui::NextColumn();
                    ImGui::Text("Variant of the Dominus"); ImGui::NextColumn();

                    ImGui::Text("Breakout Type-S"); ImGui::NextColumn();
                    ImGui::Text("Variant of the Breakout"); ImGui::NextColumn();

                    ImGui::Text("Endo"); ImGui::NextColumn();
                    ImGui::Text("Sleek hybrid car"); ImGui::NextColumn();

                    ImGui::Text("Mantis"); ImGui::NextColumn();
                    ImGui::Text("Flat battle-car"); ImGui::NextColumn();

                    ImGui::Text("Takumi RX-T"); ImGui::NextColumn();
                    ImGui::Text("Variant of the Takumi"); ImGui::NextColumn();

                    ImGui::Text("Jäger 619"); ImGui::NextColumn();
                    ImGui::Text("Sports car design"); ImGui::NextColumn();

                    ImGui::Text("Imperator DT5"); ImGui::NextColumn();
                    ImGui::Text("Race car design"); ImGui::NextColumn();

                    ImGui::Text("Centio V17"); ImGui::NextColumn();
                    ImGui::Text("Formula 1 inspired"); ImGui::NextColumn();

                    ImGui::Columns(1);

                    ImGui::Spacing();
                    ImGui::TextWrapped("Other import items include boosts, wheels, and animated decals that are rarer than Very Rare items but not as rare as Exotic or Black Market items.");

                    ImGui::EndTabItem();
                }

                // Controls for working with items
                if (ImGui::BeginTabItem("Tools"))
                {
                    ImGui::Spacing();
                    ImGui::TextWrapped("Use these tools to retrieve information about items and customize your loadout.");
                    ImGui::Spacing();

                    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);

                    if (ImGui::Button("Refresh Item Database", ImVec2(200, 30))) {
                        Main.SpawnNotification("Items", "Refreshed item database", 3);
                    }
                    ImGui::SameLine();
                    ImGui::TextDisabled("(Updates local item catalog)");

                    ImGui::Spacing();

                    ImGui::Separator();
                    ImGui::Text("Item Stats");
                    ImGui::Spacing();

                    static int totalItems = 783; // Sample data
                    static int blackMarketCount = 38;
                    static int exoticCount = 23;
                    static int importCount = 76;
                    static int veryRareCount = 212;
                    static int rareCount = 301;
                    static int uncommonCount = 133;

                    ImGui::Text("Total items in database: %d", totalItems);
                    ImGui::Spacing();

                    ImGui::Text("Items by rarity:");
                    ImGui::Indent(10.0f);
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 1.0f, 1.0f), "Black Market: %d", blackMarketCount);
                    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Exotic: %d", exoticCount);
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Import: %d", importCount);
                    ImGui::TextColored(ImVec4(0.5f, 0.0f, 1.0f, 1.0f), "Very Rare: %d", veryRareCount);
                    ImGui::TextColored(ImVec4(0.0f, 0.5f, 1.0f, 1.0f), "Rare: %d", rareCount);
                    ImGui::TextColored(ImVec4(0.0f, 0.8f, 0.0f, 1.0f), "Uncommon: %d", uncommonCount);
                    ImGui::Unindent(10.0f);

                    ImGui::PopStyleVar();

                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Titles"))
            {
                ImGui::Spacing();

                // Game Titles section header
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.0f, 1.0f));
                ImGui::Text("Game Titles");
                ImGui::PopStyleColor();
                ImGui::Separator();

                ImGui::TextWrapped("Browse and search through Rocket League titles with their colors.");

                // Define a simple structure for titles
                struct TitleInfo {
                    const char* name;
                    const char* color;
                    int id;
                };

                // Static list of titles that persists between frames
                static std::vector<TitleInfo> gameTitles;
                static bool titlesLoaded = false;

                // Only load titles once
                if (!titlesLoaded) {
                    // Season Titles (1-14)
                    gameTitles.push_back({ "Season 1 Grand Champion", "#FF0000", 1001 });
                    gameTitles.push_back({ "Season 2 Grand Champion", "#FF4500", 1002 });
                    gameTitles.push_back({ "Season 3 Grand Champion", "#FF6347", 1003 });
                    gameTitles.push_back({ "Season 4 Grand Champion", "#FF7F50", 1004 });
                    gameTitles.push_back({ "Season 5 Grand Champion", "#FF8C00", 1005 });
                    gameTitles.push_back({ "Season 6 Grand Champion", "#FFA500", 1006 });
                    gameTitles.push_back({ "Season 7 Grand Champion", "#FFD700", 1007 });
                    gameTitles.push_back({ "Season 8 Grand Champion", "#FFFF00", 1008 });
                    gameTitles.push_back({ "Season 9 Grand Champion", "#FFD700", 1009 });
                    gameTitles.push_back({ "Season 10 Grand Champion", "#FF0000", 1010 });
                    gameTitles.push_back({ "Season 11 Grand Champion", "#FFA500", 1011 });
                    gameTitles.push_back({ "Season 12 Grand Champion", "#FF7F50", 1012 });
                    gameTitles.push_back({ "Season 13 Grand Champion", "#FF6347", 1013 });
                    gameTitles.push_back({ "Season 14 Grand Champion", "#FF4500", 1014 });

                    // F2P Season Titles
                    gameTitles.push_back({ "S1 Grand Champion", "#FF0000", 1101 });
                    gameTitles.push_back({ "S1 Supersonic Legend", "#FFFFFF", 1102 });
                    gameTitles.push_back({ "S2 Grand Champion", "#FF0000", 1103 });
                    gameTitles.push_back({ "S2 Supersonic Legend", "#FFFFFF", 1104 });
                    gameTitles.push_back({ "S3 Grand Champion", "#FF0000", 1105 });
                    gameTitles.push_back({ "S3 Supersonic Legend", "#FFFFFF", 1106 });
                    gameTitles.push_back({ "S4 Grand Champion", "#FF0000", 1107 });
                    gameTitles.push_back({ "S4 Supersonic Legend", "#FFFFFF", 1108 });
                    gameTitles.push_back({ "S5 Grand Champion", "#FF0000", 1109 });
                    gameTitles.push_back({ "S5 Supersonic Legend", "#FFFFFF", 1110 });
                    gameTitles.push_back({ "S6 Grand Champion", "#FF0000", 1111 });
                    gameTitles.push_back({ "S6 Supersonic Legend", "#FFFFFF", 1112 });
                    gameTitles.push_back({ "S7 Grand Champion", "#FF0000", 1113 });
                    gameTitles.push_back({ "S7 Supersonic Legend", "#FFFFFF", 1114 });
                    gameTitles.push_back({ "S8 Grand Champion", "#FF0000", 1115 });
                    gameTitles.push_back({ "S8 Supersonic Legend", "#FFFFFF", 1116 });
                    gameTitles.push_back({ "S9 Grand Champion", "#FF0000", 1117 });
                    gameTitles.push_back({ "S9 Supersonic Legend", "#FFFFFF", 1118 });
                    gameTitles.push_back({ "S10 Grand Champion", "#FF0000", 1119 });
                    gameTitles.push_back({ "S10 Supersonic Legend", "#FFFFFF", 1120 });
                    gameTitles.push_back({ "S11 Grand Champion", "#FF0000", 1121 });
                    gameTitles.push_back({ "S11 Supersonic Legend", "#FFFFFF", 1122 });

                    // Tournament Titles
                    gameTitles.push_back({ "S1 Tournament Champion", "#00CC00", 1201 });
                    gameTitles.push_back({ "S1 Grand Tournament Champion", "#27E800", 1202 });
                    gameTitles.push_back({ "S2 Tournament Champion", "#00CC00", 1203 });
                    gameTitles.push_back({ "S2 Grand Tournament Champion", "#27E800", 1204 });
                    gameTitles.push_back({ "S3 Tournament Champion", "#00CC00", 1205 });
                    gameTitles.push_back({ "S3 Grand Tournament Champion", "#27E800", 1206 });
                    gameTitles.push_back({ "S4 Tournament Champion", "#00CC00", 1207 });
                    gameTitles.push_back({ "S4 Grand Tournament Champion", "#27E800", 1208 });
                    gameTitles.push_back({ "S5 Tournament Champion", "#00CC00", 1209 });
                    gameTitles.push_back({ "S5 Grand Tournament Champion", "#27E800", 1210 });

                    // RLCS Titles
                    gameTitles.push_back({ "RLCS World Champion", "#FFD700", 2001 });
                    gameTitles.push_back({ "RLCS Elite", "#C0C0C0", 2002 });
                    gameTitles.push_back({ "RLCS Grand Finalist", "#FFA500", 2003 });
                    gameTitles.push_back({ "RLCS Contender", "#87CEEB", 2004 });
                    gameTitles.push_back({ "RLCS Regional Champion", "#00BFFF", 2005 });
                    gameTitles.push_back({ "RLCS - X Champion", "#FFD700", 2006 });
                    gameTitles.push_back({ "RLCS Season 8 World Champion", "#FFD700", 2007 });
                    gameTitles.push_back({ "RLCS Season 7 World Champion", "#FFD700", 2008 });
                    gameTitles.push_back({ "RLCS Season 6 World Champion", "#FFD700", 2009 });
                    gameTitles.push_back({ "RLCS Season 5 World Champion", "#FFD700", 2010 });
                    gameTitles.push_back({ "RLCS Season 4 World Champion", "#FFD700", 2011 });
                    gameTitles.push_back({ "RLCS Season 3 World Champion", "#FFD700", 2012 });
                    gameTitles.push_back({ "RLCS Season 2 World Champion", "#FFD700", 2013 });
                    gameTitles.push_back({ "RLCS Season 1 World Champion", "#FFD700", 2014 });

                    // ESL and MLG Titles
                    gameTitles.push_back({ "ESL Monthly Champion", "#1E90FF", 3001 });
                    gameTitles.push_back({ "ESL Elite", "#4169E1", 3002 });
                    gameTitles.push_back({ "ESL Contender", "#87CEFA", 3003 });
                    gameTitles.push_back({ "MLG Champion", "#FF0000", 3004 });
                    gameTitles.push_back({ "MLG Elite", "#DC143C", 3005 });

                    // Official Titles
                    gameTitles.push_back({ "Verified", "#00FF00", 4001 });
                    gameTitles.push_back({ "Moderator", "#00FFFF", 4002 });
                    gameTitles.push_back({ "Developer", "#FF00FF", 4003 });
                    gameTitles.push_back({ "Creator", "#ADFF2F", 4004 });
                    gameTitles.push_back({ "Streamer", "#8A2BE2", 4005 });
                    gameTitles.push_back({ "Content Creator", "#9370DB", 4006 });
                    gameTitles.push_back({ "World Champion", "#FFD700", 4007 });
                    gameTitles.push_back({ "Tester", "#7B68EE", 4008 });
                    gameTitles.push_back({ "Community Manager", "#4682B4", 4009 });
                    gameTitles.push_back({ "Nexus Gaming", "#B22222", 4010 });
                    gameTitles.push_back({ "Pigeon Man", "#808080", 4011 });
                    gameTitles.push_back({ "THE PIGEON MAN", "#696969", 4012 });
                    gameTitles.push_back({ "Rocket Sledge", "#B22222", 4013 });
                    gameTitles.push_back({ "Mustache Man", "#8B4513", 4014 });
                    gameTitles.push_back({ "The Nightmare", "#000000", 4015 });

                    // Rank Titles
                    gameTitles.push_back({ "Rookie", "#FFFFFF", 5001 });
                    gameTitles.push_back({ "Pro", "#FFFFFF", 5002 });
                    gameTitles.push_back({ "Veteran", "#FFFFFF", 5003 });
                    gameTitles.push_back({ "Expert", "#FFFFFF", 5004 });
                    gameTitles.push_back({ "Master", "#FFFFFF", 5005 });
                    gameTitles.push_back({ "Legend", "#8A2BE2", 5006 });
                    gameTitles.push_back({ "Rocketeer", "#FF4500", 5007 });
                    gameTitles.push_back({ "Elite Challenger", "#D2B48C", 5008 });
                    gameTitles.push_back({ "All-Star", "#0000FF", 5009 });
                    gameTitles.push_back({ "Superstar", "#00BFFF", 5010 });
                    gameTitles.push_back({ "Champion", "#9400D3", 5011 });
                    gameTitles.push_back({ "Bronze Champion", "#CD7F32", 5012 });
                    gameTitles.push_back({ "Silver Champion", "#C0C0C0", 5013 });
                    gameTitles.push_back({ "Gold Champion", "#FFD700", 5014 });
                    gameTitles.push_back({ "Platinum Champion", "#7FFFD4", 5015 });
                    gameTitles.push_back({ "Diamond Champion", "#1E90FF", 5016 });
                    gameTitles.push_back({ "Champion Elite", "#9400D3", 5017 });
                    gameTitles.push_back({ "Grand Champion", "#FF0000", 5018 });
                    gameTitles.push_back({ "Supersonic Legend", "#FFFFFF", 5019 });

                    // Achievement Titles
                    gameTitles.push_back({ "Sherpa", "#98FB98", 6001 });
                    gameTitles.push_back({ "Diplomat", "#87CEEB", 6002 });
                    gameTitles.push_back({ "Collector", "#FFFF00", 6003 });
                    gameTitles.push_back({ "Rocket Demigod", "#FFA500", 6004 });
                    gameTitles.push_back({ "The Magnificent", "#FF1493", 6005 });
                    gameTitles.push_back({ "The Calculating", "#7CFC00", 6006 });
                    gameTitles.push_back({ "The Exterminator", "#FF4500", 6007 });
                    gameTitles.push_back({ "MVP", "#9400D3", 6008 });
                    gameTitles.push_back({ "Aerial Ace", "#00BFFF", 6009 });
                    gameTitles.push_back({ "Wall Crawler", "#FF8C00", 6010 });
                    gameTitles.push_back({ "Tactician", "#008000", 6011 });
                    gameTitles.push_back({ "Showoff", "#FF69B4", 6012 });
                    gameTitles.push_back({ "Striker", "#FF6347", 6013 });
                    gameTitles.push_back({ "Playmaker", "#4169E1", 6014 });
                    gameTitles.push_back({ "Guardian", "#FF8C00", 6015 });
                    gameTitles.push_back({ "The Streak", "#FF0000", 6016 });
                    gameTitles.push_back({ "The Friendly", "#FF69B4", 6017 });
                    gameTitles.push_back({ "The Warrior", "#8B0000", 6018 });

                    // Extra Mode Titles
                    gameTitles.push_back({ "Snowday Specialist", "#B0E0E6", 7001 });
                    gameTitles.push_back({ "Hoops Champion", "#FFA500", 7002 });
                    gameTitles.push_back({ "Dropshot Destroyer", "#9932CC", 7003 });
                    gameTitles.push_back({ "Rumble Royalty", "#FF1493", 7004 });
                    gameTitles.push_back({ "RNG Champ", "#DA70D6", 7005 });
                    gameTitles.push_back({ "Floor Destroyer", "#8A2BE2", 7006 });
                    gameTitles.push_back({ "Slam Dunk Wizard", "#FFA500", 7007 });
                    gameTitles.push_back({ "Ice King", "#87CEFA", 7008 });

                    // Miscellaneous Titles
                    gameTitles.push_back({ "Battle-Car Master", "#C0C0C0", 8001 });
                    gameTitles.push_back({ "Rocket Scientist", "#4682B4", 8002 });
                    gameTitles.push_back({ "Boosting Maniac", "#00FF00", 8003 });
                    gameTitles.push_back({ "Drift King", "#800080", 8004 });
                    gameTitles.push_back({ "Demo Demon", "#FF0000", 8005 });
                    gameTitles.push_back({ "Custom Training Creator", "#20B2AA", 8006 });
                    gameTitles.push_back({ "Animator", "#FF8C00", 8007 });
                    gameTitles.push_back({ "Designer", "#9370DB", 8008 });
                    gameTitles.push_back({ "Workshop Creator", "#008080", 8009 });
                    gameTitles.push_back({ "Freestyler", "#FF00FF", 8010 });
                    gameTitles.push_back({ "The Virtuoso", "#6A5ACD", 8011 });
                    gameTitles.push_back({ "The Tactical Whiffer", "#696969", 8012 });
                    gameTitles.push_back({ "The Merc Main", "#556B2F", 8013 });
                    gameTitles.push_back({ "Quick Chat Champion", "#FFD700", 8014 });
                    gameTitles.push_back({ "Tilt Proof", "#2E8B57", 8015 });
                    gameTitles.push_back({ "Fennec Fanatic", "#FF7F50", 8016 });
                    gameTitles.push_back({ "Octane Operator", "#3CB371", 8017 });
                    gameTitles.push_back({ "Dominus Dominator", "#8B008B", 8018 });

                    // Established Year Titles
                    gameTitles.push_back({ "Est. 2015", "#FFFFFF", 9001 });
                    gameTitles.push_back({ "Est. 2016", "#FFFFFF", 9002 });
                    gameTitles.push_back({ "Est. 2017", "#FFFFFF", 9003 });
                    gameTitles.push_back({ "Est. 2018", "#FFFFFF", 9004 });
                    gameTitles.push_back({ "Est. 2019", "#FFFFFF", 9005 });
                    gameTitles.push_back({ "Est. 2020", "#FFFFFF", 9006 });
                    gameTitles.push_back({ "Est. 2021", "#FFFFFF", 9007 });
                    gameTitles.push_back({ "Est. 2022", "#FFFFFF", 9008 });
                    gameTitles.push_back({ "Est. 2023", "#FFFFFF", 9009 });
                    gameTitles.push_back({ "Est. 2024", "#FFFFFF", 9010 });
                    gameTitles.push_back({ "Est. 2025", "#FFFFFF", 9011 });

                    titlesLoaded = true;
                }

                // Refresh button
                if (ImGui::Button("Refresh Titles", ImVec2(120, 30))) {
                    Main.SpawnNotification("Titles", "Refreshed titles list", 3);
                }

                // Filter input for title search
                static char searchBuffer[128] = "";
                ImGui::SameLine();
                ImGui::PushItemWidth(200);
                ImGui::InputText("Filter", searchBuffer, IM_ARRAYSIZE(searchBuffer));
                ImGui::PopItemWidth();
                std::string searchLower = searchBuffer;
                std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(), ::tolower);

                // Title count
                ImGui::SameLine();
                ImGui::TextDisabled("(%d titles)", (int)gameTitles.size());

                // Create a box to show all titles
                ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
                if (ImGui::BeginChild("TitlesBox", ImVec2(0, 300), true, ImGuiWindowFlags_AlwaysVerticalScrollbar)) {
                    // Table to display titles
                    if (ImGui::BeginTable("TitlesTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                        // Table headers
                        ImGui::TableSetupColumn("Title Name", ImGuiTableColumnFlags_WidthStretch);
                        ImGui::TableSetupColumn("Color", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                        ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 50.0f);
                        ImGui::TableHeadersRow();

                        // Display titles with colors
                        for (const auto& title : gameTitles) {
                            // Apply filter if search text is not empty
                            std::string titleLower = title.name;
                            std::transform(titleLower.begin(), titleLower.end(), titleLower.begin(), ::tolower);
                            if (!searchLower.empty() && titleLower.find(searchLower) == std::string::npos) {
                                continue; // Skip this title if it doesn't match the search
                            }

                            ImGui::TableNextRow();

                            // Title Name
                            ImGui::TableSetColumnIndex(0);

                            // Parse color and apply it
                            ImVec4 titleColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // Default white
                            if (strlen(title.color) >= 7) { // #RRGGBB format
                                // Parse hex color manually
                                std::string hexColor(title.color); // Convert const char* to string
                                hexColor = hexColor.substr(1); // Skip the '#'
                                int r = 255, g = 255, b = 255; // Default white

                                if (hexColor.length() >= 6) {
                                    char* endPtr = nullptr;
                                    // Get red component (first 2 chars)
                                    r = (int)strtol(hexColor.substr(0, 2).c_str(), &endPtr, 16);
                                    // Get green component (middle 2 chars)
                                    g = (int)strtol(hexColor.substr(2, 2).c_str(), &endPtr, 16);
                                    // Get blue component (last 2 chars)
                                    b = (int)strtol(hexColor.substr(4, 2).c_str(), &endPtr, 16);
                                }

                                titleColor = ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
                            }

                            ImGui::PushStyleColor(ImGuiCol_Text, titleColor);
                            ImGui::Text("%s", title.name);
                            ImGui::PopStyleColor();

                            // Color value
                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text("%s", title.color);

                            // ID
                            ImGui::TableSetColumnIndex(2);
                            ImGui::Text("%d", title.id);
                        }

                        ImGui::EndTable();
                    }

                    ImGui::EndChild();
                }
                ImGui::PopStyleVar();

                // Information about titles
                ImGui::TextWrapped("Titles are displayed in their actual colors. Use the filter to search for specific titles.");

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Colors"))
            {
                ImGui::Spacing();

                // Colors & Paint section header
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.5f, 0.5f, 1.0f));
                ImGui::Text("Colors & Paint Finishes");
                ImGui::PopStyleColor();
                ImGui::Separator();
                ImGui::TextWrapped("Explore different paint colors and finish types for your vehicles.");
                ImGui::Spacing();

                // Setup tabs for color categories
                ImGui::BeginTabBar("ColorCategories", ImGuiTabBarFlags_FittingPolicyScroll);

                // Paint Colors Tab
                if (ImGui::BeginTabItem("Paint Colors"))
                {
                    ImGui::Spacing();

                    // Define paint color structure
                    struct PaintColor {
                        const char* name;
                        ImVec4 color;
                        int id;
                    };

                    // Static list of paint colors
                    static std::vector<PaintColor> paintColors;
                    static bool colorsLoaded = false;

                    if (!colorsLoaded) {
                        // Standard colors
                        paintColors.push_back({ "Titanium White", ImVec4(1.0f, 1.0f, 1.0f, 1.0f), 1 });
                        paintColors.push_back({ "Black", ImVec4(0.1f, 0.1f, 0.1f, 1.0f), 2 });
                        paintColors.push_back({ "Grey", ImVec4(0.5f, 0.5f, 0.5f, 1.0f), 3 });
                        paintColors.push_back({ "Crimson", ImVec4(0.9f, 0.1f, 0.1f, 1.0f), 4 });
                        paintColors.push_back({ "Pink", ImVec4(1.0f, 0.4f, 0.7f, 1.0f), 5 });
                        paintColors.push_back({ "Cobalt", ImVec4(0.0f, 0.4f, 0.8f, 1.0f), 6 });
                        paintColors.push_back({ "Sky Blue", ImVec4(0.4f, 0.7f, 1.0f, 1.0f), 7 });
                        paintColors.push_back({ "Burnt Sienna", ImVec4(0.55f, 0.25f, 0.1f, 1.0f), 8 });
                        paintColors.push_back({ "Saffron", ImVec4(1.0f, 0.85f, 0.0f, 1.0f), 9 });
                        paintColors.push_back({ "Lime", ImVec4(0.4f, 0.9f, 0.0f, 1.0f), 10 });
                        paintColors.push_back({ "Forest Green", ImVec4(0.0f, 0.5f, 0.1f, 1.0f), 11 });
                        paintColors.push_back({ "Orange", ImVec4(1.0f, 0.5f, 0.0f, 1.0f), 12 });
                        paintColors.push_back({ "Purple", ImVec4(0.5f, 0.0f, 0.8f, 1.0f), 13 });

                        // Special colors
                        paintColors.push_back({ "Gold", ImVec4(0.85f, 0.7f, 0.0f, 1.0f), 14 });
                        paintColors.push_back({ "Rose Gold", ImVec4(0.85f, 0.55f, 0.55f, 1.0f), 15 });
                        paintColors.push_back({ "Platinum", ImVec4(0.8f, 0.8f, 0.85f, 1.0f), 16 });

                        colorsLoaded = true;
                    }

                    ImGui::Text("Available Paint Colors (%d)", (int)paintColors.size());
                    ImGui::Spacing();

                    // Grid layout for color swatches
                    int numColumns = 4;
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 10));

                    float swatchSize = 60.0f;
                    float windowWidth = ImGui::GetContentRegionAvail().x;
                    int columnsToUse = (int)(windowWidth / (swatchSize + 10.0f));
                    if (columnsToUse > 0) numColumns = columnsToUse;

                    // Adjust for different window sizes
                    if (numColumns < 2) numColumns = 2;
                    if (numColumns > 6) numColumns = 6;

                    // Display color swatches in a grid
                    for (int i = 0; i < paintColors.size(); i++) {
                        if (i % numColumns != 0) ImGui::SameLine();

                        // Color box
                        ImGui::BeginGroup();
                        ImGui::PushID(i);

                        // Color swatch with border
                        ImGui::PushStyleColor(ImGuiCol_Button, paintColors[i].color);
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, paintColors[i].color);
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, paintColors[i].color);
                        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

                        if (ImGui::Button("", ImVec2(swatchSize, swatchSize))) {
                            // Could implement selection functionality here
                            Main.SpawnNotification("Paint Color",
                                std::string("Selected ") + paintColors[i].name, 2);
                        }

                        ImGui::PopStyleVar();
                        ImGui::PopStyleColor(3);

                        // Color name
                        float textWidth = ImGui::CalcTextSize(paintColors[i].name).x;
                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (swatchSize - textWidth) * 0.5f);
                        ImGui::TextUnformatted(paintColors[i].name);

                        ImGui::PopID();
                        ImGui::EndGroup();
                    }

                    ImGui::PopStyleVar();

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    // RGB Color Picker
                    ImGui::Text("Custom Color Creator");
                    static ImVec4 pickedColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
                    static float color[4] = { 1.0f, 0.0f, 0.0f, 1.0f };

                    ImGui::ColorEdit3("Custom Color", color);
                    pickedColor = ImVec4(color[0], color[1], color[2], color[3]);

                    ImGui::Spacing();
                    ImGui::Text("Preview:");

                    ImGui::PushStyleColor(ImGuiCol_Button, pickedColor);
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, pickedColor);
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, pickedColor);
                    ImGui::Button("##colorpreview", ImVec2(200, 50));
                    ImGui::PopStyleColor(3);

                    ImGui::SameLine();
                    if (ImGui::Button("Use This Color", ImVec2(120, 30))) {
                        Main.SpawnNotification("Custom Color", "Custom color applied", 2);
                    }

                    ImGui::EndTabItem();
                }

                // Paint Finishes Tab
                if (ImGui::BeginTabItem("Paint Finishes"))
                {
                    ImGui::Spacing();
                    ImGui::TextWrapped("Paint finishes determine how your vehicle's surface will look. They can be matte, glossy, metallic, or have special effects.");
                    ImGui::Spacing();

                    // Define paint finish structure
                    struct PaintFinish {
                        const char* name;
                        const char* description;
                        const char* rarity;
                    };

                    // Static list of paint finishes
                    static std::vector<PaintFinish> paintFinishes;
                    static bool finishesLoaded = false;

                    if (!finishesLoaded) {
                        // Common finishes
                        paintFinishes.push_back({ "Glossy", "Basic glossy finish", "Common" });
                        paintFinishes.push_back({ "Matte", "Non-reflective finish", "Common" });
                        paintFinishes.push_back({ "Semi-Gloss", "Moderate shine", "Common" });
                        paintFinishes.push_back({ "Metallic", "Basic metallic finish", "Common" });
                        paintFinishes.push_back({ "Metallic Pearl", "Shimmering effect", "Common" });
                        paintFinishes.push_back({ "Pearlescent", "Color-shifting effect", "Common" });

                        // Rare finishes
                        paintFinishes.push_back({ "Circuit Board", "Electronic pattern", "Rare" });
                        paintFinishes.push_back({ "Furry", "Textured, fuzzy surface", "Rare" });
                        paintFinishes.push_back({ "Zebra", "Black and white stripes", "Rare" });

                        // Very Rare finishes
                        paintFinishes.push_back({ "Anodized", "Metallic polish", "Very Rare" });
                        paintFinishes.push_back({ "Burlap", "Rough fabric texture", "Very Rare" });
                        paintFinishes.push_back({ "Dino", "Scale-like pattern", "Very Rare" });
                        paintFinishes.push_back({ "Metal Flake", "Metallic with flakes", "Very Rare" });
                        paintFinishes.push_back({ "Toon Glossy", "Cartoon-like shine", "Very Rare" });
                        paintFinishes.push_back({ "Toon Matte", "Cartoon-like flat", "Very Rare" });
                        paintFinishes.push_back({ "Moon Rock", "Textured rocky surface", "Very Rare" });
                        paintFinishes.push_back({ "Straight-Line", "Linear pattern", "Very Rare" });

                        // Import finishes
                        paintFinishes.push_back({ "Brushed Metal", "Brushed metal texture", "Import" });
                        paintFinishes.push_back({ "Anodized Pearl", "Pearlescent with metallic polish", "Import" });

                        finishesLoaded = true;
                    }

                    // Filter input for finish search
                    static char finishSearchBuffer[128] = "";
                    ImGui::PushItemWidth(200);
                    ImGui::InputText("Filter Finishes", finishSearchBuffer, IM_ARRAYSIZE(finishSearchBuffer));
                    ImGui::PopItemWidth();
                    std::string finishSearchLower = finishSearchBuffer;
                    std::transform(finishSearchLower.begin(), finishSearchLower.end(), finishSearchLower.begin(), ::tolower);

                    // Display finishes in a table
                    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
                    if (ImGui::BeginChild("FinishesTable", ImVec2(0, 280), true, ImGuiWindowFlags_AlwaysVerticalScrollbar)) {
                        // Table to display finishes
                        if (ImGui::BeginTable("PaintFinishesTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                            // Table headers
                            ImGui::TableSetupColumn("Finish Name", ImGuiTableColumnFlags_WidthStretch);
                            ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_WidthStretch);
                            ImGui::TableSetupColumn("Rarity", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                            ImGui::TableHeadersRow();

                            // Display finishes
                            for (const auto& finish : paintFinishes) {
                                // Apply filter if search text is not empty
                                std::string finishLower = finish.name;
                                std::transform(finishLower.begin(), finishLower.end(), finishLower.begin(), ::tolower);
                                if (!finishSearchLower.empty() &&
                                    finishLower.find(finishSearchLower) == std::string::npos) {
                                    continue; // Skip if doesn't match search
                                }

                                ImGui::TableNextRow();

                                // Finish Name
                                ImGui::TableSetColumnIndex(0);
                                ImGui::TextUnformatted(finish.name);

                                // Description
                                ImGui::TableSetColumnIndex(1);
                                ImGui::TextUnformatted(finish.description);

                                // Rarity with color
                                ImGui::TableSetColumnIndex(2);
                                ImVec4 rarityColor;
                                if (strcmp(finish.rarity, "Common") == 0) {
                                    rarityColor = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
                                }
                                else if (strcmp(finish.rarity, "Rare") == 0) {
                                    rarityColor = ImVec4(0.0f, 0.5f, 1.0f, 1.0f);
                                }
                                else if (strcmp(finish.rarity, "Very Rare") == 0) {
                                    rarityColor = ImVec4(0.5f, 0.0f, 1.0f, 1.0f);
                                }
                                else if (strcmp(finish.rarity, "Import") == 0) {
                                    rarityColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
                                }
                                else {
                                    rarityColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                                }

                                ImGui::PushStyleColor(ImGuiCol_Text, rarityColor);
                                ImGui::TextUnformatted(finish.rarity);
                                ImGui::PopStyleColor();
                            }

                            ImGui::EndTable();
                        }
                        ImGui::EndChild();
                    }
                    ImGui::PopStyleVar();

                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Rank"))
            {
                ImGui::Spacing();

                // Rank section header
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.6f, 1.0f, 1.0f));
                ImGui::Text("Competitive Ranks & Rewards");
                ImGui::PopStyleColor();
                ImGui::Separator();
                ImGui::TextWrapped("View and browse Rocket League competitive ranks and season rewards.");
                ImGui::Spacing();

                // Setup tabs for rank categories
                ImGui::BeginTabBar("RankCategories", ImGuiTabBarFlags_FittingPolicyScroll);

                // Competitive Ranks Tab
                if (ImGui::BeginTabItem("Competitive Ranks"))
                {
                    ImGui::Spacing();

                    // Define rank structure
                    struct RankInfo {
                        const char* name;
                        const char* division;
                        const char* color;
                        int mmr;
                    };

                    // Define a set of ranks
                    static std::vector<RankInfo> ranks;
                    static bool ranksLoaded = false;

                    if (!ranksLoaded) {
                        // Bronze Ranks
                        ranks.push_back({ "Bronze I", "Division I", "#CD7F32", 100 });
                        ranks.push_back({ "Bronze I", "Division II", "#CD7F32", 116 });
                        ranks.push_back({ "Bronze I", "Division III", "#CD7F32", 132 });
                        ranks.push_back({ "Bronze I", "Division IV", "#CD7F32", 148 });
                        ranks.push_back({ "Bronze II", "Division I", "#CD7F32", 164 });
                        ranks.push_back({ "Bronze II", "Division II", "#CD7F32", 180 });
                        ranks.push_back({ "Bronze II", "Division III", "#CD7F32", 196 });
                        ranks.push_back({ "Bronze II", "Division IV", "#CD7F32", 212 });
                        ranks.push_back({ "Bronze III", "Division I", "#CD7F32", 228 });
                        ranks.push_back({ "Bronze III", "Division II", "#CD7F32", 244 });
                        ranks.push_back({ "Bronze III", "Division III", "#CD7F32", 260 });
                        ranks.push_back({ "Bronze III", "Division IV", "#CD7F32", 276 });

                        // Silver Ranks
                        ranks.push_back({ "Silver I", "Division I", "#C0C0C0", 292 });
                        ranks.push_back({ "Silver I", "Division II", "#C0C0C0", 308 });
                        ranks.push_back({ "Silver I", "Division III", "#C0C0C0", 324 });
                        ranks.push_back({ "Silver I", "Division IV", "#C0C0C0", 340 });
                        ranks.push_back({ "Silver II", "Division I", "#C0C0C0", 356 });
                        ranks.push_back({ "Silver II", "Division II", "#C0C0C0", 372 });
                        ranks.push_back({ "Silver II", "Division III", "#C0C0C0", 388 });
                        ranks.push_back({ "Silver II", "Division IV", "#C0C0C0", 404 });
                        ranks.push_back({ "Silver III", "Division I", "#C0C0C0", 420 });
                        ranks.push_back({ "Silver III", "Division II", "#C0C0C0", 436 });
                        ranks.push_back({ "Silver III", "Division III", "#C0C0C0", 452 });
                        ranks.push_back({ "Silver III", "Division IV", "#C0C0C0", 468 });

                        // Gold Ranks
                        ranks.push_back({ "Gold I", "Division I", "#FFD700", 484 });
                        ranks.push_back({ "Gold I", "Division II", "#FFD700", 500 });
                        ranks.push_back({ "Gold I", "Division III", "#FFD700", 516 });
                        ranks.push_back({ "Gold I", "Division IV", "#FFD700", 532 });
                        ranks.push_back({ "Gold II", "Division I", "#FFD700", 548 });
                        ranks.push_back({ "Gold II", "Division II", "#FFD700", 564 });
                        ranks.push_back({ "Gold II", "Division III", "#FFD700", 580 });
                        ranks.push_back({ "Gold II", "Division IV", "#FFD700", 596 });
                        ranks.push_back({ "Gold III", "Division I", "#FFD700", 612 });
                        ranks.push_back({ "Gold III", "Division II", "#FFD700", 628 });
                        ranks.push_back({ "Gold III", "Division III", "#FFD700", 644 });
                        ranks.push_back({ "Gold III", "Division IV", "#FFD700", 660 });

                        // Platinum Ranks
                        ranks.push_back({ "Platinum I", "Division I", "#7FFFD4", 676 });
                        ranks.push_back({ "Platinum I", "Division II", "#7FFFD4", 692 });
                        ranks.push_back({ "Platinum I", "Division III", "#7FFFD4", 708 });
                        ranks.push_back({ "Platinum I", "Division IV", "#7FFFD4", 724 });
                        ranks.push_back({ "Platinum II", "Division I", "#7FFFD4", 740 });
                        ranks.push_back({ "Platinum II", "Division II", "#7FFFD4", 756 });
                        ranks.push_back({ "Platinum II", "Division III", "#7FFFD4", 772 });
                        ranks.push_back({ "Platinum II", "Division IV", "#7FFFD4", 788 });
                        ranks.push_back({ "Platinum III", "Division I", "#7FFFD4", 804 });
                        ranks.push_back({ "Platinum III", "Division II", "#7FFFD4", 820 });
                        ranks.push_back({ "Platinum III", "Division III", "#7FFFD4", 836 });
                        ranks.push_back({ "Platinum III", "Division IV", "#7FFFD4", 852 });

                        // Diamond Ranks
                        ranks.push_back({ "Diamond I", "Division I", "#1E90FF", 868 });
                        ranks.push_back({ "Diamond I", "Division II", "#1E90FF", 884 });
                        ranks.push_back({ "Diamond I", "Division III", "#1E90FF", 900 });
                        ranks.push_back({ "Diamond I", "Division IV", "#1E90FF", 916 });
                        ranks.push_back({ "Diamond II", "Division I", "#1E90FF", 932 });
                        ranks.push_back({ "Diamond II", "Division II", "#1E90FF", 948 });
                        ranks.push_back({ "Diamond II", "Division III", "#1E90FF", 964 });
                        ranks.push_back({ "Diamond II", "Division IV", "#1E90FF", 980 });
                        ranks.push_back({ "Diamond III", "Division I", "#1E90FF", 996 });
                        ranks.push_back({ "Diamond III", "Division II", "#1E90FF", 1012 });
                        ranks.push_back({ "Diamond III", "Division III", "#1E90FF", 1028 });
                        ranks.push_back({ "Diamond III", "Division IV", "#1E90FF", 1044 });

                        // Champion Ranks
                        ranks.push_back({ "Champion I", "Division I", "#9400D3", 1060 });
                        ranks.push_back({ "Champion I", "Division II", "#9400D3", 1076 });
                        ranks.push_back({ "Champion I", "Division III", "#9400D3", 1092 });
                        ranks.push_back({ "Champion I", "Division IV", "#9400D3", 1108 });
                        ranks.push_back({ "Champion II", "Division I", "#9400D3", 1124 });
                        ranks.push_back({ "Champion II", "Division II", "#9400D3", 1140 });
                        ranks.push_back({ "Champion II", "Division III", "#9400D3", 1156 });
                        ranks.push_back({ "Champion II", "Division IV", "#9400D3", 1172 });
                        ranks.push_back({ "Champion III", "Division I", "#9400D3", 1188 });
                        ranks.push_back({ "Champion III", "Division II", "#9400D3", 1204 });
                        ranks.push_back({ "Champion III", "Division III", "#9400D3", 1220 });
                        ranks.push_back({ "Champion III", "Division IV", "#9400D3", 1236 });

                        // Grand Champion Ranks
                        ranks.push_back({ "Grand Champion I", "Division I", "#FF0000", 1252 });
                        ranks.push_back({ "Grand Champion I", "Division II", "#FF0000", 1277 });
                        ranks.push_back({ "Grand Champion I", "Division III", "#FF0000", 1302 });
                        ranks.push_back({ "Grand Champion I", "Division IV", "#FF0000", 1327 });
                        ranks.push_back({ "Grand Champion II", "Division I", "#FF0000", 1352 });
                        ranks.push_back({ "Grand Champion II", "Division II", "#FF0000", 1377 });
                        ranks.push_back({ "Grand Champion II", "Division III", "#FF0000", 1402 });
                        ranks.push_back({ "Grand Champion II", "Division IV", "#FF0000", 1427 });
                        ranks.push_back({ "Grand Champion III", "Division I", "#FF0000", 1452 });
                        ranks.push_back({ "Grand Champion III", "Division II", "#FF0000", 1487 });
                        ranks.push_back({ "Grand Champion III", "Division III", "#FF0000", 1522 });
                        ranks.push_back({ "Grand Champion III", "Division IV", "#FF0000", 1557 });

                        // Supersonic Legend Rank
                        ranks.push_back({ "Supersonic Legend", "", "#FFFFFF", 1592 });

                        ranksLoaded = true;
                    }

                    // Filter ranks
                    static char rankSearchBuffer[128] = "";
                    ImGui::PushItemWidth(200);
                    ImGui::InputText("Filter Ranks", rankSearchBuffer, IM_ARRAYSIZE(rankSearchBuffer));
                    ImGui::PopItemWidth();
                    std::string rankSearchLower = rankSearchBuffer;
                    std::transform(rankSearchLower.begin(), rankSearchLower.end(), rankSearchLower.begin(), ::tolower);

                    // Rank selection combobox
                    static int currentRankCategory = 0;
                    ImGui::SameLine();
                    const char* rankCategories[] = { "All Ranks", "Bronze", "Silver", "Gold", "Platinum", "Diamond", "Champion", "Grand Champion", "Supersonic Legend" };
                    ImGui::PushItemWidth(150);
                    ImGui::Combo("Rank Tier", &currentRankCategory, rankCategories, IM_ARRAYSIZE(rankCategories));
                    ImGui::PopItemWidth();

                    // Display ranks in a table
                    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
                    if (ImGui::BeginChild("RanksTable", ImVec2(0, 280), true, ImGuiWindowFlags_AlwaysVerticalScrollbar)) {
                        if (ImGui::BeginTable("CompetitiveRanksTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                            // Table headers
                            ImGui::TableSetupColumn("Rank", ImGuiTableColumnFlags_WidthStretch);
                            ImGui::TableSetupColumn("Division", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                            ImGui::TableSetupColumn("MMR", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                            ImGui::TableHeadersRow();

                            // Display ranks
                            for (const auto& rank : ranks) {
                                // Filter by rank category if not "All Ranks"
                                bool showRank = true;
                                if (currentRankCategory > 0) {
                                    if (currentRankCategory == 1 && strncmp(rank.name, "Bronze", 6) != 0) showRank = false;
                                    else if (currentRankCategory == 2 && strncmp(rank.name, "Silver", 6) != 0) showRank = false;
                                    else if (currentRankCategory == 3 && strncmp(rank.name, "Gold", 4) != 0) showRank = false;
                                    else if (currentRankCategory == 4 && strncmp(rank.name, "Platinum", 8) != 0) showRank = false;
                                    else if (currentRankCategory == 5 && strncmp(rank.name, "Diamond", 7) != 0) showRank = false;
                                    else if (currentRankCategory == 6 && strncmp(rank.name, "Champion", 8) != 0) showRank = false;
                                    else if (currentRankCategory == 7 && strncmp(rank.name, "Grand Champion", 14) != 0) showRank = false;
                                    else if (currentRankCategory == 8 && strncmp(rank.name, "Supersonic Legend", 17) != 0) showRank = false;
                                }

                                // Apply text filter if not empty
                                if (!rankSearchLower.empty()) {
                                    std::string rankNameLower = rank.name;
                                    std::transform(rankNameLower.begin(), rankNameLower.end(), rankNameLower.begin(), ::tolower);
                                    if (rankNameLower.find(rankSearchLower) == std::string::npos) {
                                        showRank = false;
                                    }
                                }

                                if (!showRank) continue;

                                ImGui::TableNextRow();

                                // Rank Name with color
                                ImGui::TableSetColumnIndex(0);

                                // Parse color and apply it
                                ImVec4 rankColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                                if (strlen(rank.color) >= 7) {
                                    std::string hexColor(rank.color);
                                    hexColor = hexColor.substr(1);

                                    if (hexColor.length() >= 6) {
                                        char* endPtr = nullptr;
                                        int r = (int)strtol(hexColor.substr(0, 2).c_str(), &endPtr, 16);
                                        int g = (int)strtol(hexColor.substr(2, 2).c_str(), &endPtr, 16);
                                        int b = (int)strtol(hexColor.substr(4, 2).c_str(), &endPtr, 16);
                                        rankColor = ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
                                    }
                                }

                                ImGui::PushStyleColor(ImGuiCol_Text, rankColor);
                                ImGui::TextUnformatted(rank.name);
                                ImGui::PopStyleColor();

                                // Division
                                ImGui::TableSetColumnIndex(1);
                                ImGui::TextUnformatted(rank.division);

                                // MMR
                                ImGui::TableSetColumnIndex(2);
                                ImGui::Text("%d", rank.mmr);
                            }

                            ImGui::EndTable();
                        }
                        ImGui::EndChild();
                    }
                    ImGui::PopStyleVar();

                    ImGui::EndTabItem();
                }

                // Season Rewards Tab
                if (ImGui::BeginTabItem("Season Rewards"))
                {
                    ImGui::Spacing();
                    ImGui::TextWrapped("Season rewards are special items given to players based on their highest rank achieved during a competitive season.");
                    ImGui::Spacing();

                    // Define reward structure
                    struct RewardInfo {
                        const char* season;
                        const char* rewardType;
                        const char* description;
                    };

                    // Static list of rewards
                    static std::vector<RewardInfo> rewards;
                    static bool rewardsLoaded = false;

                    if (!rewardsLoaded) {
                        // Season rewards over time
                        rewards.push_back({ "Season 1", "Crown Toppers", "Bronze, Silver, Gold, and Platinum crowns" });
                        rewards.push_back({ "Season 2", "Rocket Boost", "Prospect, Challenger, and Star-themed rocket boosts" });
                        rewards.push_back({ "Season 3", "Wheels", "Star, Champion, and Prospect-themed wheels" });
                        rewards.push_back({ "Season 4", "Trails", "Bronze to Champion-tiered trails" });
                        rewards.push_back({ "Season 5", "Player Banners", "Bronze to Champion-tiered player banners" });
                        rewards.push_back({ "Season 6", "Wheels", "Bronze to Champion-tiered wheels" });
                        rewards.push_back({ "Season 7", "Goal Explosions", "Bronze to Champion-tiered goal explosions" });
                        rewards.push_back({ "Season 8", "Rocket Boost", "Bronze to Grand Champion-tiered boosts" });
                        rewards.push_back({ "Season 9", "Wheels", "Bronze to Grand Champion-tiered wheels" });
                        rewards.push_back({ "Season 10", "Goal Explosions", "Bronze to Grand Champion-tiered goal explosions" });
                        rewards.push_back({ "Season 11", "Universal Decals", "Bronze to Grand Champion-tiered animated decals" });
                        rewards.push_back({ "Season 12", "Wheels", "Bronze to Grand Champion-tiered wheels" });
                        rewards.push_back({ "Season 13", "Universal Decals", "Bronze to Grand Champion-tiered decals" });
                        rewards.push_back({ "Season 14", "Universal Decals", "Bronze to Grand Champion-tiered decals" });

                        // Free to Play Seasons
                        rewards.push_back({ "F2P Season 1", "Decals & Titles", "Bronze to Supersonic Legend-tiered decals and titles" });
                        rewards.push_back({ "F2P Season 2", "Wheels & Titles", "Bronze to Supersonic Legend-tiered wheels and titles" });
                        rewards.push_back({ "F2P Season 3", "Rocket Boost & Titles", "Bronze to Supersonic Legend-tiered boosts and titles" });
                        rewards.push_back({ "F2P Season 4", "Goal Explosions & Titles", "Bronze to Supersonic Legend-tiered explosions and titles" });
                        rewards.push_back({ "F2P Season 5", "Dragon Decals & Titles", "Bronze to Supersonic Legend-tiered dragon decals and titles" });
                        rewards.push_back({ "F2P Season 6", "Animated Decals & Titles", "Bronze to Supersonic Legend-tiered animated decals and titles" });
                        rewards.push_back({ "F2P Season 7", "Wheels & Titles", "Bronze to Supersonic Legend-tiered wheels and titles" });
                        rewards.push_back({ "F2P Season 8", "Goal Explosions & Titles", "Bronze to Supersonic Legend-tiered explosions and titles" });
                        rewards.push_back({ "F2P Season 9", "Rocket Boosts & Titles", "Bronze to Supersonic Legend-tiered boosts and titles" });
                        rewards.push_back({ "F2P Season 10", "Trails & Titles", "Bronze to Supersonic Legend-tiered trails and titles" });

                        rewardsLoaded = true;
                    }

                    // Display rewards in a table
                    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
                    if (ImGui::BeginChild("RewardsTable", ImVec2(0, 280), true, ImGuiWindowFlags_AlwaysVerticalScrollbar)) {
                        if (ImGui::BeginTable("SeasonRewardsTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                            // Table headers
                            ImGui::TableSetupColumn("Season", ImGuiTableColumnFlags_WidthFixed, 120.0f);
                            ImGui::TableSetupColumn("Reward Type", ImGuiTableColumnFlags_WidthFixed, 150.0f);
                            ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_WidthStretch);
                            ImGui::TableHeadersRow();

                            // Different color for F2P seasons vs original seasons
                            for (const auto& reward : rewards) {
                                ImGui::TableNextRow();

                                // Season
                                ImGui::TableSetColumnIndex(0);

                                // Color F2P seasons differently
                                if (strncmp(reward.season, "F2P", 3) == 0) {
                                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
                                }
                                else {
                                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.7f, 0.0f, 1.0f));
                                }

                                ImGui::TextUnformatted(reward.season);
                                ImGui::PopStyleColor();

                                // Reward Type
                                ImGui::TableSetColumnIndex(1);
                                ImGui::TextUnformatted(reward.rewardType);

                                // Description
                                ImGui::TableSetColumnIndex(2);
                                ImGui::TextUnformatted(reward.description);
                            }

                            ImGui::EndTable();
                        }
                        ImGui::EndChild();
                    }
                    ImGui::PopStyleVar();

                    ImGui::Spacing();
                    ImGui::TextWrapped("Season rewards require winning a certain number of matches at each rank level to unlock. The current season rewards are not shown until officially announced.");

                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();

                ImGui::EndTabItem();
            }
            // Misc tab
            if (ImGui::BeginTabItem("Misc"))
            {
                // Template Button
                if (ImGui::Button("Template Button")) {
                    Main.Execute([]() {
                        Main.SpawnNotification("Template", "Template button has been pressed!", 10);
                        });
                }

                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Only works in Training mode");
                }

                // Alt+F4 checkbox - now referencing the global variable
                extern bool g_enableAltF4;
                ImGui::Checkbox("Alt+F4 Close Game", &g_enableAltF4);

                ImGui::EndTabItem();
            }

            // Alt+F4 functionality is now moved outside the menu visibility check

            // Options tab
            if (ImGui::BeginTabItem("Options"))
            {
                ImGui::Columns(2);
                ImGui::SetColumnWidth(0, 150);

                // Style Presets with Alpha Control
                ImGui::Text("Style:");
                ImGui::NextColumn();
                ImGui::PushItemWidth(150);
                const char* styles[] = { "Dark", "Midnight", "Cyberpunk", "Ocean", "Forest", "Purple Haze" };
                if (ImGui::BeginCombo("##preset", styles[currentStyle], ImGuiComboFlags_NoArrowButton))
                {
                    for (int i = 0; i < IM_ARRAYSIZE(styles); i++) {
                        if (ImGui::Selectable(styles[i], currentStyle == i)) {
                            currentStyle = i;
                            ImGuiStyle& style = ImGui::GetStyle();

                            // Reset to default first
                            DefaultImGuiStyle();

                            // Apply selected style
                            switch (i) {
                            case 0: // Dark (default)
                                break;

                            case 1: // Midnight
                                style.Colors[ImGuiCol_WindowBg] = ImVec4(0.04f, 0.04f, 0.06f, 0.95f);
                                style.Colors[ImGuiCol_ChildBg] = ImVec4(0.07f, 0.07f, 0.09f, 0.95f);
                                style.Colors[ImGuiCol_Button] = ImVec4(0.16f, 0.16f, 0.20f, 1.00f);
                                style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.24f, 0.24f, 0.30f, 1.00f);
                                style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.30f, 0.30f, 0.38f, 1.00f);
                                break;

                            case 2: // Cyberpunk
                                style.Colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.03f, 0.06f, 0.95f);
                                style.Colors[ImGuiCol_ChildBg] = ImVec4(0.10f, 0.05f, 0.10f, 0.95f);
                                style.Colors[ImGuiCol_Button] = ImVec4(0.90f, 0.10f, 0.50f, 0.70f);
                                style.Colors[ImGuiCol_ButtonHovered] = ImVec4(1.00f, 0.20f, 0.60f, 1.00f);
                                style.Colors[ImGuiCol_ButtonActive] = ImVec4(1.00f, 0.10f, 0.40f, 1.00f);
                                style.Colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
                                break;

                            case 3: // Ocean
                                style.Colors[ImGuiCol_WindowBg] = ImVec4(0.00f, 0.10f, 0.20f, 0.95f);
                                style.Colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.15f, 0.25f, 0.95f);
                                style.Colors[ImGuiCol_Button] = ImVec4(0.00f, 0.47f, 0.80f, 0.60f);
                                style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.00f, 0.57f, 0.90f, 1.00f);
                                style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.00f, 0.37f, 0.70f, 1.00f);
                                break;

                            case 4: // Forest
                                style.Colors[ImGuiCol_WindowBg] = ImVec4(0.05f, 0.15f, 0.08f, 0.95f);
                                style.Colors[ImGuiCol_ChildBg] = ImVec4(0.08f, 0.20f, 0.12f, 0.95f);
                                style.Colors[ImGuiCol_Button] = ImVec4(0.20f, 0.60f, 0.30f, 0.70f);
                                style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.70f, 0.35f, 1.00f);
                                style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.15f, 0.50f, 0.25f, 1.00f);
                                break;

                            case 5: // Purple Haze
                                style.Colors[ImGuiCol_WindowBg] = ImVec4(0.15f, 0.05f, 0.30f, 0.95f);
                                style.Colors[ImGuiCol_ChildBg] = ImVec4(0.20f, 0.08f, 0.35f, 0.95f);
                                style.Colors[ImGuiCol_Button] = ImVec4(0.50f, 0.20f, 0.80f, 0.70f);
                                style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.60f, 0.30f, 0.90f, 1.00f);
                                style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.40f, 0.15f, 0.70f, 1.00f);
                                break;
                            }
                        }
                    }
                    ImGui::EndCombo();
                }
                ImGui::SameLine();
                ImGui::Text("Alpha:");
                ImGui::SameLine();
                ImGui::PushItemWidth(100);
                ImGui::SliderFloat("##alpha", &ImGui::GetStyle().Alpha, 0.2f, 1.0f, "%.2f");
                ImGui::PopItemWidth();
                ImGui::NextColumn();

                ImGui::Columns(1);
                ImGui::EndTabItem();
            }
            // Info tab with enhanced information
            if (ImGui::BeginTabItem("Info"))
            {
                ImGui::Spacing();

                // Header with RGB effect on developer name
                ImGui::PushStyleColor(ImGuiCol_Text, GetRainbowColor(time, 1.0f));
                ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("Developer: XKing").x) * 0.5f);
                ImGui::Text("Developer: XKing");
                ImGui::PopStyleColor();

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // Project Information
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "KingMod");
                ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Version: ");
                ImGui::SameLine();
                ImGui::Text("1.0.0 (Stable)");

                ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Status: ");
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Online");

                ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Last Update: ");
                ImGui::SameLine();
                ImGui::Text("20 May 2024");

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // Contact Information
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Contact Information:");
                ImGui::Text("Discord: ");
                ImGui::SameLine();
                if (ImGui::Button("XKing#0001")) {
                    // Copy to clipboard or open Discord
                    ImGui::SetClipboardText("XKing#0001");
                }

                ImGui::Text("Server: ");
                ImGui::SameLine();
                if (ImGui::Button("Join Discord")) {
                    // Open Discord invite
                    ShellExecuteA(0, 0, "https://discord.gg/Dsx9BdFKxH", 0, 0, SW_SHOW);
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // Info tab content ends here
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();

            // Remove the RGB border style
            ImGui::PopStyleVar();
            ImGui::PopStyleColor();
        }
        ImGui::End();
    }
    ImGui::Render();
}

GUIComponent GUI;