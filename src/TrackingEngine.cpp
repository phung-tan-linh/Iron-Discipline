// [PLAN]: Triển khai TrackingEngine. SystemScanner chuyên trách WinAPI. TimeEnforcer dùng .find() O(1) cho App và duyệt nhẹ cho Web. Tối ưu CPU bằng cách cache HWND, ProcessName và WindowTitle. Ghi Append-Log mỗi khi chạm mốc 60 giây.
#include "../include/TrackingEngine.h"
#include "../include/DataStore.h"
#include <psapi.h>
#include <algorithm>
#include <cctype>

// --- SystemScanner Implementation ---

std::string SystemScanner::getActiveWindowTitle(HWND hwnd)
{
    char windowTitle[256];
    if (GetWindowTextA(hwnd, windowTitle, sizeof(windowTitle)) > 0)
    {
        return std::string(windowTitle);
    }
    return "";
}

std::string SystemScanner::getActiveProcessName(DWORD pid)
{
    std::string processName = "";
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (hProcess != NULL)
    {
        char exePath[MAX_PATH];
        if (GetModuleFileNameExA(hProcess, NULL, exePath, MAX_PATH))
        {
            std::string fullPath(exePath);
            size_t pos = fullPath.find_last_of("\\/");
            if (pos != std::string::npos)
            {
                processName = fullPath.substr(pos + 1);
            }
        }
        CloseHandle(hProcess);
    }
    return processName;
}

// --- TimeEnforcer Implementation ---

void TimeEnforcer::reloadLimits()
{
    activeTargets.clear();
    
    auto dailyUsage = UsageRepository::loadDailyUsage();
    auto limits = UsageRepository::getActiveLimits();

    for (const auto& limit : limits)
    {
        std::string keyLower = limit.name;
        std::transform(keyLower.begin(), keyLower.end(), keyLower.begin(),
                       [](unsigned char c) { return std::tolower(c); });

        int usedSecs = dailyUsage[limit.name];
        std::shared_ptr<TrackableItem> item;

        if (limit.name.find(".exe") != std::string::npos)
        {
            item = std::make_shared<AppItem>(limit.name, limit.timeLimit, usedSecs);
        }
        else
        {
            item = std::make_shared<WebItem>(limit.name, limit.timeLimit, usedSecs);
        }

        activeTargets[keyLower] = item;
    }
}

void TimeEnforcer::monitorAndBlock()
{
    currentDate = UsageRepository::getCurrentDateStr();
    reloadLimits();

    static HWND lastHwnd = NULL;
    static std::string cachedProcessName = "";
    static std::string cachedWindowTitle = "";
    static DWORD cachedPid = 0;

    while (true)
    {
        std::string newDate = UsageRepository::getCurrentDateStr();
        if (newDate != currentDate)
        {
            currentDate = newDate;
            reloadLimits();
        }

        HWND currentHwnd = GetForegroundWindow();
        if (currentHwnd)
        {
            if (currentHwnd != lastHwnd)
            {
                lastHwnd = currentHwnd;
                GetWindowThreadProcessId(currentHwnd, &cachedPid);

                cachedProcessName = SystemScanner::getActiveProcessName(cachedPid);
                cachedWindowTitle = SystemScanner::getActiveWindowTitle(currentHwnd);

                std::transform(cachedProcessName.begin(), cachedProcessName.end(), cachedProcessName.begin(),
                               [](unsigned char c) { return std::tolower(c); });
                std::transform(cachedWindowTitle.begin(), cachedWindowTitle.end(), cachedWindowTitle.begin(),
                               [](unsigned char c) { return std::tolower(c); });
            }

            std::shared_ptr<TrackableItem> matchedItem = nullptr;

            auto it = activeTargets.find(cachedProcessName);
            if (it != activeTargets.end() && it->second->getType() == "Application")
            {
                matchedItem = it->second;
            }
            else
            {
                for (const auto& [key, item] : activeTargets)
                {
                    if (item->getType() == "Website")
                    {
                        if (cachedWindowTitle.find(key) != std::string::npos)
                        {
                            matchedItem = item;
                            break;
                        }
                    }
                }
            }

            if (matchedItem)
            {
                matchedItem->addTimeUsedSeconds(1);
                matchedItem->checkAndEnforce(currentHwnd, cachedPid, matchedItem->getTimeLimit());

                int currentSecs = matchedItem->getTimeUsedSeconds();
                if (currentSecs > 0 && currentSecs % 60 == 0)
                {
                    UsageRepository::appendUsageLog(matchedItem->getName(), 60);
                }
            }
        }

        MsgWaitForMultipleObjects(0, NULL, FALSE, 1000, QS_ALLEVENTS);
    }
}