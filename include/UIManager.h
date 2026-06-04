#ifndef UIMANAGER_H
#define UIMANAGER_H
#include <windows.h>
#include <atomic>
extern std::atomic<bool> g_isWarningActive;
class UIManager
{
public:
    static void Init();
    static void ShowWarning(int level);
    static void Cleanup();

private:
    static HWND hwnd;
    static bool initialized;
    static bool CreateDeviceD3D(HWND hWnd);
    static void CleanupDeviceD3D();
    static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
};
#endif