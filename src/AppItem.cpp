// [PLAN]: Triển khai AppItem, ghi đè enforceBlock để ép đóng ứng dụng bằng TerminateProcess.
#include "../include/AppItem.h"
#include <windows.h>

AppItem::AppItem(std::string appName, int limitMinutes)
    : TrackableItem(appName, limitMinutes) {}

void AppItem::displayInfo() const
{
    std::cout << "[APP - .exe] " << name
              << " | Da dung: " << timeUsedMinutes
              << "/" << timeLimitMinutes << " phut." << std::endl;
}

std::string AppItem::getType() const
{
    return "Application";
}

void AppItem::enforceBlock(HWND hwnd, DWORD pid)
{
    if (pid != 0)
    {
        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
        if (hProcess != NULL)
        {
            TerminateProcess(hProcess, 0);
            CloseHandle(hProcess);
        }
    }
}