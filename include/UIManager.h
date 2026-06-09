#ifndef UIMANAGER_H
#define UIMANAGER_H
#include <windows.h>
#include <atomic>
#include <thread>

extern std::atomic<int> g_WarningActive;

class UIManager
{
public:
    static void Init();
    static void ShowWarning(int level);
    static void RenderOverlayLoop();
    static void Cleanup();

private:
    static HWND hwnd;
    static bool initialized;
    static bool CreateDeviceD3D(HWND hWnd);
    static void CleanupDeviceD3D();
    static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
};
#endif