// [PLAN]: Tích hợp TrackingEngine, quản lý Single Instance, Tray Icon và Console Worker.
// Thêm tính năng Auto-Start qua Registry (HKCU) để tự động chạy ngầm cùng Windows
// bằng quyền User thường, loại bỏ hoàn toàn sự phụ thuộc vào quyền Admin (UAC).
#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <shellapi.h>
#include <atomic>
#include <thread>
#include <string>
#include "../include/ConsoleMenu.h"
#include "../include/UIManager.h"
#include "../include/TrackingEngine.h"

#define WM_TRAYICON (WM_USER + 1)
#define WM_ACTIVATE_OLD_INSTANCE (WM_USER + 2)

extern std::atomic<int> g_WarningActive;

TimeEnforcer g_Engine;
std::atomic<bool> isConsoleOpen(false);
const wchar_t CLASS_NAME[] = L"IronDisciplineHiddenWindow";
HHOOK g_hKeyboardHook = NULL;

void RegisterAutoStart()
{
    WCHAR exePath[MAX_PATH];
    if (GetModuleFileNameW(NULL, exePath, MAX_PATH) == 0)
    {
        return;
    }

    HKEY hKey;
    LSTATUS status = RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey);
    if (status == ERROR_SUCCESS)
    {
        RegSetValueExW(hKey, L"IronDiscipline", 0, REG_SZ, (const BYTE*)exePath, (lstrlenW(exePath) + 1) * sizeof(WCHAR));
        RegCloseKey(hKey);
    }
}

BOOL WINAPI ConsoleCtrlHandler(DWORD dwCtrlType)
{
    switch (dwCtrlType)
    {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
        return TRUE;
    case CTRL_CLOSE_EVENT:
        return TRUE;
    default:
        return FALSE;
    }
}

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION)
    {
        KBDLLHOOKSTRUCT *pKeyBoard = (KBDLLHOOKSTRUCT *)lParam;
        if (g_WarningActive > 0)
        {
            bool isAlt = (pKeyBoard->flags & LLKHF_ALTDOWN) != 0;
            bool isCtrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
            if ((pKeyBoard->vkCode == VK_TAB && isAlt) || pKeyBoard->vkCode == VK_LWIN || pKeyBoard->vkCode == VK_RWIN || (pKeyBoard->vkCode == VK_ESCAPE && isCtrl) || (pKeyBoard->vkCode == VK_F4 && isAlt))
                return 1;
        }
        if (wParam == WM_SYSKEYDOWN && pKeyBoard->vkCode == VK_F4)
        {
            if (GetAsyncKeyState(VK_MENU) & 0x8000)
            {
                HWND hForeground = GetForegroundWindow();
                HWND hConsole = GetConsoleWindow();
                if (hConsole != NULL && hForeground == hConsole)
                {
                    ShowWindow(hConsole, SW_HIDE);
                    return 1;
                }
            }
        }
    }
    return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
}

void RunConsoleWorker()
{
    if (isConsoleOpen)
        return;
    isConsoleOpen = true;
    AllocConsole();
    HWND hConsole = GetConsoleWindow();
    if (hConsole != NULL)
    {
        HMENU hSysMenu = GetSystemMenu(hConsole, FALSE);
        if (hSysMenu != NULL)
            DeleteMenu(hSysMenu, SC_CLOSE, MF_BYCOMMAND);
        LONG_PTR exStyle = GetWindowLongPtr(hConsole, GWL_EXSTYLE);
        exStyle = (exStyle & ~WS_EX_APPWINDOW) | WS_EX_TOOLWINDOW;
        SetWindowLongPtr(hConsole, GWL_EXSTYLE, exStyle);
        SetForegroundWindow(hConsole);
    }
    SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);
    FILE *fpIn;
    FILE *fpOut;
    FILE *fpErr;
    freopen_s(&fpIn, "CONIN$", "r", stdin);
    freopen_s(&fpOut, "CONOUT$", "w", stdout);
    freopen_s(&fpErr, "CONOUT$", "w", stderr);
    
    ConsoleMenu app;
    app.init("basic_list.csv");
    app.run();
    
    if (fpIn) fclose(fpIn);
    if (fpOut) fclose(fpOut);
    if (fpErr) fclose(fpErr);
    
    SetConsoleCtrlHandler(ConsoleCtrlHandler, FALSE);
    FreeConsole();
    isConsoleOpen = false;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_TRAYICON:
        if (lParam == WM_LBUTTONUP)
        {
            if (!isConsoleOpen)
            {
                std::thread(RunConsoleWorker).detach();
            }
            else
            {
                HWND hConsole = GetConsoleWindow();
                if (hConsole != NULL)
                {
                    ShowWindow(hConsole, SW_RESTORE);
                    SetForegroundWindow(hConsole);
                }
            }
        }
        break;
    case WM_ACTIVATE_OLD_INSTANCE:
        if (!isConsoleOpen)
        {
            std::thread(RunConsoleWorker).detach();
        }
        else
        {
            HWND hConsole = GetConsoleWindow();
            if (hConsole != NULL)
            {
                ShowWindow(hConsole, SW_RESTORE);
                SetForegroundWindow(hConsole);
            }
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

extern "C" int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    HANDLE hMutex = CreateMutexW(NULL, TRUE, L"IronDiscipline_SingleInstance_Mutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        HWND hExistingWnd = FindWindowW(CLASS_NAME, NULL);
        if (hExistingWnd != NULL)
        {
            DWORD pid = 0;
            GetWindowThreadProcessId(hExistingWnd, &pid);
            AllowSetForegroundWindow(pid);
            PostMessageW(hExistingWnd, WM_ACTIVATE_OLD_INSTANCE, 0, 0);
        }
        CloseHandle(hMutex);
        return 0;
    }
    
    RegisterAutoStart();
    
    UIManager::Init();
    
    std::thread([]() { g_Engine.monitorAndBlock(); }).detach();
    
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    if (!RegisterClassW(&wc))
        return 0;
        
    HWND hwnd = CreateWindowExW(0, CLASS_NAME, L"Tray Background Worker", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);
    if (hwnd == NULL)
        return 0;
        
    NOTIFYICONDATAW nid = {0};
    nid.cbSize = sizeof(NOTIFYICONDATAW);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcsncpy_s(nid.szTip, L"Kỉ luật thép - Iron Discipline", _TRUNCATE);
    Shell_NotifyIconW(NIM_ADD, &nid);
    
    g_hKeyboardHook = SetWindowsHookExW(WH_KEYBOARD_LL, LowLevelKeyboardProc, hInstance, 0);
    
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    if (g_hKeyboardHook)
        UnhookWindowsHookEx(g_hKeyboardHook);
        
    Shell_NotifyIconW(NIM_DELETE, &nid);
    
    UIManager::Cleanup();
    
    if (hMutex)
    {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }
    return 0;
}