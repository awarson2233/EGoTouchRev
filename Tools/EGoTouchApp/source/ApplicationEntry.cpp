#include <windows.h>
#include <d3d11.h>
#include <tchar.h>
// 使用 C++20 Modules 版本的 ImGui
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include "ServiceProxy.h"
#include "DiagnosticsWorkbench.h"
#include "GuiLogSink.h"
#include "Logger.h"


// Data
static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

int main(int, char**)
{
    // 启动日志框架 (所有日志通过 GuiLogSink 在 Log 面板显示)
    Common::Logger::Init("EGoTouch", "C:/ProgramData/EGoTouchRev/logs/",
                          Common::GuiLogSink::Instance());
    LOG_INFO("App", __func__, "System", "--- EGoTouchApp (DX11) Starts ---");

    // 1. 创建 ServiceProxy（后台自动发现 Service）
    App::ServiceProxy serviceProxy;
    serviceProxy.StartAutoDiscovery();  // 后台线程定期尝试连接

    // 2. 创建 GUI 调试界面对象
    App::DiagnosticsWorkbench diagnosticsWorkbench(&serviceProxy);

    // Create application window
    ImGui_ImplWin32_EnableDpiAwareness(); // 1. Fix DPI Issues on Multi-Monitor
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"EGoTouchApp", nullptr };
    ::RegisterClassExW(&wc);
    
    // 2. Create main window (visible, maximized for DockSpace host)
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    HWND hwnd = ::CreateWindowExW(0, wc.lpszClassName,
        L"EGoTouch Diagnostics", WS_OVERLAPPEDWINDOW,
        0, 0, screenW, screenH,
        nullptr, nullptr, wc.hInstance, nullptr);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show maximized
    ::ShowWindow(hwnd, SW_SHOWMAXIMIZED);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport
    io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleViewports; // Enable secondary viewport DPI scaling
    io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;     // Enable DPI scaled fonts
    
    // Setup Dear ImGui style — Custom Engineering Dark Theme
    ImGui::StyleColorsDark();
    {
        auto& style = ImGui::GetStyle();
        style.WindowRounding    = 4.0f;
        style.FrameRounding     = 3.0f;
        style.GrabRounding      = 2.0f;
        style.TabRounding       = 3.0f;
        style.ScrollbarRounding = 4.0f;
        style.WindowPadding     = ImVec2(8, 8);
        style.FramePadding      = ImVec2(6, 4);
        style.ItemSpacing       = ImVec2(8, 4);
        style.TabBarBorderSize  = 1.0f;

        auto& c = style.Colors;
        // Backgrounds
        c[ImGuiCol_WindowBg]        = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
        c[ImGuiCol_ChildBg]         = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
        c[ImGuiCol_PopupBg]         = ImVec4(0.12f, 0.12f, 0.14f, 0.96f);
        // Borders
        c[ImGuiCol_Border]          = ImVec4(0.25f, 0.25f, 0.28f, 0.80f);
        // Title bar
        c[ImGuiCol_TitleBg]         = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
        c[ImGuiCol_TitleBgActive]   = ImVec4(0.12f, 0.14f, 0.18f, 1.00f);
        // Tabs
        c[ImGuiCol_Tab]             = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);
        c[ImGuiCol_TabHovered]      = ImVec4(0.24f, 0.52f, 0.72f, 0.80f);
        c[ImGuiCol_TabSelected]     = ImVec4(0.18f, 0.40f, 0.58f, 1.00f);
        // Frames / inputs
        c[ImGuiCol_FrameBg]         = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);
        c[ImGuiCol_FrameBgHovered]  = ImVec4(0.22f, 0.22f, 0.26f, 1.00f);
        c[ImGuiCol_FrameBgActive]   = ImVec4(0.18f, 0.36f, 0.52f, 0.67f);
        // Buttons — cyan accent
        c[ImGuiCol_Button]          = ImVec4(0.16f, 0.34f, 0.48f, 1.00f);
        c[ImGuiCol_ButtonHovered]   = ImVec4(0.22f, 0.48f, 0.64f, 1.00f);
        c[ImGuiCol_ButtonActive]    = ImVec4(0.12f, 0.30f, 0.42f, 1.00f);
        // Headers (collapsing headers, menu)
        c[ImGuiCol_Header]          = ImVec4(0.16f, 0.34f, 0.48f, 0.50f);
        c[ImGuiCol_HeaderHovered]   = ImVec4(0.22f, 0.48f, 0.64f, 0.80f);
        c[ImGuiCol_HeaderActive]    = ImVec4(0.18f, 0.40f, 0.58f, 1.00f);
        // Scrollbar
        c[ImGuiCol_ScrollbarBg]     = ImVec4(0.08f, 0.08f, 0.10f, 0.60f);
        c[ImGuiCol_ScrollbarGrab]   = ImVec4(0.30f, 0.30f, 0.34f, 1.00f);
        // Separator
        c[ImGuiCol_Separator]       = ImVec4(0.22f, 0.22f, 0.26f, 1.00f);
        // Checkbox / SliderGrab
        c[ImGuiCol_CheckMark]       = ImVec4(0.30f, 0.70f, 0.90f, 1.00f);
        c[ImGuiCol_SliderGrab]      = ImVec4(0.24f, 0.52f, 0.72f, 1.00f);
        c[ImGuiCol_SliderGrabActive]= ImVec4(0.30f, 0.60f, 0.80f, 1.00f);
        // DockingPreview
        c[ImGuiCol_DockingPreview]  = ImVec4(0.24f, 0.52f, 0.72f, 0.70f);
    }

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Handle window resize (we don't resize directly in the WM_SIZE handler)
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // ======= 调用我们的自定义 UI 渲染 =======
        diagnosticsWorkbench.Render();
        // ========================================

        // Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0); // Present with vsync

        // Update and Render additional Platform Windows
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
    }

    // Cleanup
    serviceProxy.Disconnect();

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    Common::Logger::Shutdown();
    return 0;
}

// Helper functions (Win32/DX11 boilerplate)

bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
