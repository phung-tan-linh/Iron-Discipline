/*
 * ============================================================================
 * FILE: UIManager.cpp
 * VAI TRÒ: Quản lý luồng hiển thị giao diện đồ họa (Overlay) và thao tác WinAPI.
 *
 * ĐIỂM NHẤN HỌC THUẬT:
 * 1. Thao tác Cửa sổ Hệ thống (WinAPI): Sử dụng SetWindowLongPtr để bóc tách,
 * chuyển đổi linh hoạt giữa cờ WS_EX_NOACTIVATE (tránh mất focus của game)
 * và việc ép Z-Order lên Topmost để khóa màn hình.
 * 2. Quản lý Đa luồng (Multithreading): Tách biệt hoàn toàn Vòng lặp Render
 * DirectX 11 sang một thread độc lập (RenderOverlayLoop) để không làm
 * nghẽn luồng xử lý chính của ứng dụng.
 * 3. Cơ chế Focus Stealing an toàn: Lưu lại HWND của tiến trình hiện tại
 * trước khi bung cảnh báo và trả lại focus ngay sau khi người dùng xác nhận.
 * ============================================================================
 */

#include "../include/UIManager.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <chrono>
#include <thread>

#pragma comment(lib, "d3d11.lib")

std::atomic<int> g_WarningActive{0};
DWORD g_WarningStartTime = 0;

HWND UIManager::hwnd = NULL;
bool UIManager::initialized = false;
static ID3D11Device *g_pd3dDevice = NULL;
static ID3D11DeviceContext *g_pd3dDeviceContext = NULL;
static IDXGISwapChain *g_pSwapChain = NULL;
static ID3D11RenderTargetView *g_mainRenderTargetView = NULL;
static ImFont *fBig = nullptr;
static ImFont *fSmall = nullptr;

static HWND g_AppHwndToRestore = NULL;
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI UIManager::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;
    switch (msg)
    {
    case WM_MOUSEACTIVATE:
        // Ngăn OS cấp Focus khi người dùng vô tình click vào Overlay lúc nó đang là Cảnh báo 1 (Nhắc nhở)
        // Điều này giúp người dùng không bị văng khỏi các game Fullscreen (Exclusive).
        return MA_NOACTIVATE; 
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            if (g_mainRenderTargetView)
            {
                g_mainRenderTargetView->Release();
                g_mainRenderTargetView = NULL;
            }
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            ID3D11Texture2D *pBackBuffer;
            g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
            g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
            pBackBuffer->Release();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

bool UIManager::CreateDeviceD3D(HWND hWnd)
{
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
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};
    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
        return false;
    ID3D11Texture2D *pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    pBackBuffer->Release();
    return true;
}

void UIManager::CleanupDeviceD3D()
{
    if (g_mainRenderTargetView)
    {
        g_mainRenderTargetView->Release();
        g_mainRenderTargetView = NULL;
    }
    if (g_pSwapChain)
    {
        g_pSwapChain->Release();
        g_pSwapChain = NULL;
    }
    if (g_pd3dDeviceContext)
    {
        g_pd3dDeviceContext->Release();
        g_pd3dDeviceContext = NULL;
    }
    if (g_pd3dDevice)
    {
        g_pd3dDevice->Release();
        g_pd3dDevice = NULL;
    }
}

void UIManager::Init()
{
    SetProcessDPIAware();
    if (initialized)
        return;
    WNDCLASSEXA wc = {sizeof(WNDCLASSEXA), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, "UIMgr", NULL};
    RegisterClassExA(&wc);
    int w = GetSystemMetrics(SM_CXSCREEN), h = GetSystemMetrics(SM_CYSCREEN);
    
    // Thêm WS_EX_NOACTIVATE để đảm bảo Overlay sinh ra ngầm, không cướp Focus của User
    hwnd = CreateWindowExA(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE, "UIMgr", "", WS_POPUP, 0, 0, w, h, NULL, NULL, wc.hInstance, NULL);
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        UnregisterClassA("UIMgr", wc.hInstance);
        return;
    }
    
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.IniFilename = NULL;
    fBig = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 64.0f, NULL, io.Fonts->GetGlyphRangesVietnamese());
    fSmall = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 36.0f, NULL, io.Fonts->GetGlyphRangesVietnamese());
    
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
    
    initialized = true;
    
    // Tách vòng lặp Render UI sang luồng (thread) phụ độc lập để giải phóng CPU cho luồng chính
    std::thread(&UIManager::RenderOverlayLoop).detach();
}

void UIManager::ShowWarning(int level)
{
    if (!initialized)
        Init();
        
    g_WarningActive = level;
    g_WarningStartTime = GetTickCount();
}

void UIManager::RenderOverlayLoop()
{
    int lastLevel = 0;
    while (true)
    {
        int currentLevel = g_WarningActive.load();
        if (lastLevel == 0 && currentLevel > 0)
        {
            // Snapshot lưu lại HWND của app hiện tại đang dùng (để trả lại sau khi đóng UI)
            g_AppHwndToRestore = GetForegroundWindow();
        }
        lastLevel = currentLevel;

        MSG msg;
        while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                return;
        }

        if (currentLevel == 0)
        {
            if (IsWindowVisible(hwnd))
            {
                ShowWindow(hwnd, SW_HIDE);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        if (!IsWindowVisible(hwnd))
        {
            if (currentLevel == 1)
            {
                // Cảnh báo mềm: Hiện lên màn hình nhưng KHÔNG lấy Focus (không làm văng game)
                ShowWindow(hwnd, SW_SHOWNOACTIVATE);
            }
            else if (currentLevel == 2)
            {
                // Cảnh báo cứng (Hết giờ): Dùng toán tử Bitwise (~) tước bỏ cờ NOACTIVATE
                LONG_PTR exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
                exStyle &= ~WS_EX_NOACTIVATE; 
                SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle);
                
                ShowWindow(hwnd, SW_SHOW);
                SetForegroundWindow(hwnd);
            }
        }
        else if (currentLevel == 2)
        {
            // Radar Focus O(1): Nếu phát hiện Cảnh báo 2 bị mất Focus (người dùng cố bấm ra ngoài)
            // Lập tức ép Z-Order (HWND_TOPMOST) và kéo Focus về lại Overlay.
            if (GetForegroundWindow() != hwnd)
            {
                SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                SetForegroundWindow(hwnd);
            }
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        ImGuiIO& io = ImGui::GetIO();
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(640, 360));
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 4.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
        ImGui::PushStyleColor(ImGuiCol_Border, currentLevel == 1 ? ImVec4(1.0f, 0.8f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.08f, 0.08f, 0.95f));
        ImGui::Begin("W", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove);
        ImVec2 ws = ImGui::GetWindowSize();
        
        if (fBig) ImGui::PushFont(fBig);
        const char *tB = currentLevel == 1 ? u8"NHẮC NHỞ!" : u8"HẾT GIỜ!";
        ImGui::PushStyleColor(ImGuiCol_Text, currentLevel == 1 ? ImVec4(1.0f, 0.8f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
        ImVec2 tsB = ImGui::CalcTextSize(tB);
        ImGui::SetCursorPos(ImVec2((ws.x - tsB.x) * 0.5f, ws.y * 0.15f));
        ImGui::Text(tB);
        ImGui::PopStyleColor();
        if (fBig) ImGui::PopFont();
        
        if (fSmall) ImGui::PushFont(fSmall);
        const char *tS = currentLevel == 1 ? u8"Còn 5 phút nữa thôi. Hãy lưu lại tiến trình ngay đi!"
        : u8"Thì phải gì ạ? Thì phải... Phải chịu, đừng có kêu!";
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
        ImVec2 tsS = ImGui::CalcTextSize(tS);
        ImGui::SetCursorPos(ImVec2((ws.x - tsS.x) * 0.5f, ws.y * 0.42f));
        ImGui::Text(tS);
        ImGui::PopStyleColor();
        if (fSmall) ImGui::PopFont();
        
        // Anti-Spam: Buộc người dùng phải nhìn thông báo đủ 3 giây mới hiện nút tắt
        if (GetTickCount() - g_WarningStartTime >= 3000)
        {
            if (fSmall) ImGui::PushFont(fSmall);
            const char *btn = currentLevel == 1 ? u8"BIẾT RỒI" : u8"ĐÃ HIỂU";
            ImVec2 bs(200, 70);
            ImGui::SetCursorPos(ImVec2((ws.x - bs.x) * 0.5f, ws.y * 0.68f));
            
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
            
            ImVec4 btnColor = currentLevel == 1 ?
            ImVec4(0.85f, 0.65f, 0.0f, 1.0f) : ImVec4(0.8f, 0.1f, 0.1f, 1.0f);
            ImVec4 btnHover = currentLevel == 1 ?
            ImVec4(0.95f, 0.75f, 0.1f, 1.0f) : ImVec4(0.9f, 0.2f, 0.2f, 1.0f);
            ImVec4 btnActive = currentLevel == 1 ?
            ImVec4(0.75f, 0.55f, 0.0f, 1.0f) : ImVec4(0.7f, 0.0f, 0.0f, 1.0f);
            
            ImGui::PushStyleColor(ImGuiCol_Button, btnColor);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, btnHover);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, btnActive);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
            if (ImGui::Button(btn, bs))
            {
                g_WarningActive = 0;
                lastLevel = 0;
                ShowWindow(hwnd, SW_HIDE);
                
                // Mặc giáp lại cho Overlay (Bật cờ NOACTIVATE)
                LONG_PTR exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
                exStyle |= WS_EX_NOACTIVATE;
                SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle);
                
                // Kỹ thuật Graceful Return: Hoàn trả lại Focus cho ứng dụng mà user đang dùng dở
                if (g_AppHwndToRestore)
                {
                    SetForegroundWindow(g_AppHwndToRestore);
                    g_AppHwndToRestore = NULL;
                }
            }
            
            ImGui::PopStyleColor(4);
            ImGui::PopStyleVar();
            
            if (fSmall) ImGui::PopFont();
        }
        
        ImGui::End();
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(2);
        ImGui::Render();
        
        const float cb[4] = {0.f, 0.f, 0.f, 0.f};
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, cb);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pSwapChain->Present(1, 0);
    }
}

void UIManager::Cleanup()
{
    if (!initialized)
        return;
        
    g_WarningActive = 0;
    
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceD3D();
    DestroyWindow(hwnd);
    UnregisterClassA("UIMgr", GetModuleHandle(NULL));
    initialized = false;
}