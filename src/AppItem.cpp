// [PLAN]: Triển khai AppItem, xử lý ép đóng tiến trình độc lập với cờ cảnh báo, gọi UIManager để hiển thị UI.
#include "../include/AppItem.h"
#include "../include/UIManager.h"
#include <windows.h>

AppItem::AppItem(std::string appName, int limitMinutes, std::string execPath)
    : TrackableItem(appName, limitMinutes), executablePath(std::move(execPath)),
      isFirstWarningShown(false), isSecondWarningShown(false) {}

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

void AppItem::checkAndEnforce(HWND hwnd, DWORD pid, int globalLimitMinutes)
{
    if (timeUsedMinutes >= globalLimitMinutes)
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
        
        if (!isSecondWarningShown)
        {
            UIManager::ShowWarning(2);
            isSecondWarningShown = true;
        }
    }
    else if (globalLimitMinutes - timeUsedMinutes <= 5)
    {
        if (!isFirstWarningShown)
        {
            UIManager::ShowWarning(1);
            isFirstWarningShown = true;
        }
    }
}