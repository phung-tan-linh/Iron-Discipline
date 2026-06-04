#include "UIManager.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#pragma comment(lib, "d3d11.lib")
std::atomic<bool> g_isWarningActive{false};
HWND UIManager::hwnd = NULL;
bool UIManager::initialized = false;
static ID3D11Device *g_pd3dDevice = NULL;
static ID3D11DeviceContext *g_pd3dDeviceContext = NULL;
static IDXGISwapChain *g_pSwapChain = NULL;
static ID3D11RenderTargetView *g_mainRenderTargetView = NULL;
static ImFont *fBig = nullptr;
static ImFont *fSmall = nullptr;
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI UIManager::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;
    switch (msg)
    {
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
    int w = GetSystemMetrics(SM_CXSCREEN), h = GetSystemMetrics(SM_CYSCREEN), x = 0, y = 0;
    hwnd = CreateWindowExA(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, "UIMgr", "", WS_POPUP | WS_VISIBLE, x, y, w, h, NULL, NULL, wc.hInstance, NULL);
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
    fBig = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 60.0f, NULL, io.Fonts->GetGlyphRangesVietnamese());
    fSmall = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 30.0f, NULL, io.Fonts->GetGlyphRangesVietnamese());
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
    initialized = true;
}
void UIManager::ShowWarning(int level)
{
    g_isWarningActive = true;
    if (!initialized)
        Init();
    UpdateWindow(hwnd);
    bool done = false;
    while (!done)
    {
        MSG msg;
        while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;
        if (GetForegroundWindow() != hwnd)
            SetForegroundWindow(hwnd);
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 10.0f);
        ImGui::PushStyleColor(ImGuiCol_Border, level == 1 ? ImVec4(1, 1, 0, 1) : ImVec4(1, 0, 0, 1));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.85f));
        ImGui::Begin("W", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground);
        ImVec2 ws = ImGui::GetWindowSize();
        ImGui::PushFont(fBig);
        const char *tB = level == 1 ? u8"NHẮC NHỞ!" : u8"HẾT GIỜ!";
        ImGui::PushStyleColor(ImGuiCol_Text, level == 1 ? ImVec4(1, 1, 0, 1) : ImVec4(1, 0, 0, 1));
        ImVec2 tsB = ImGui::CalcTextSize(tB);
        ImGui::SetCursorPos(ImVec2((ws.x - tsB.x) * 0.5f, ws.y * 0.2f));
        ImGui::Text(tB);
        ImGui::PopStyleColor();
        ImGui::PopFont();
        ImGui::PushFont(fSmall);
        const char *tS = level == 1 ? u8"Còn 5 phút nữa thôi. Hãy lưu lại tiến trình ngay đi!" : u8"Thì phải gì ạ? Thì phải... Phải chịu, đừng có kêu!";
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
        ImVec2 tsS = ImGui::CalcTextSize(tS);
        ImGui::SetCursorPos(ImVec2((ws.x - tsS.x) * 0.5f, ws.y * 0.5f));
        ImGui::Text(tS);
        ImGui::PopStyleColor();
        ImGui::PopFont();
        ImGui::PushFont(fSmall);
        const char *btn = level == 1 ? u8"BIẾT RỒI" : u8"ĐÃ HIỂU";
        ImVec2 bs(200, 60);
        ImGui::SetCursorPos(ImVec2((ws.x - bs.x) * 0.5f, ws.y * 0.75f));
        if (ImGui::Button(btn, bs))
            done = true;
        ImGui::PopFont();
        ImGui::End();
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar();
        ImGui::Render();
        const float cb[4] = {0.f, 0.f, 0.f, 0.f};
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, cb);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pSwapChain->Present(1, 0);
    }
    g_isWarningActive = false;
    ShowWindow(hwnd, SW_HIDE);
}
void UIManager::Cleanup()
{
    if (!initialized)
        return;
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceD3D();
    DestroyWindow(hwnd);
    UnregisterClassA("UIMgr", GetModuleHandle(NULL));
    initialized = false;
}