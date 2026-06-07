// [PLAN]: Triển khai TrackingEngine. SystemScanner chuyên trách WinAPI. TimeEnforcer dùng .find() O(1) cho App và duyệt nhẹ cho Web. Ghi Append-Log mỗi khi chạm mốc 60 giây.
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
    
    // Đảm bảo TimePools được nạp từ file log trước khi gán vào TrackableItem
    UsageRepository::loadDailyUsage();
    auto limits = UsageRepository::getActiveLimits();

    for (const auto& limit : limits)
    {
        std::string keyLower = limit.name;
        std::transform(keyLower.begin(), keyLower.end(), keyLower.begin(),
                       [](unsigned char c) { return std::tolower(c); });

        auto pool = UsageRepository::getTimePool(limit.name);
        std::shared_ptr<TrackableItem> item;

        if (limit.name.find(".exe") != std::string::npos)
        {
            item = std::make_shared<AppItem>(limit.name, limit.timeLimit, pool);
        }
        else
        {
            item = std::make_shared<WebItem>(limit.name, limit.timeLimit, pool);
        }

        activeTargets[keyLower] = item;
    }
}

void TimeEnforcer::monitorAndBlock()
{
    currentDate = UsageRepository::getCurrentDateStr();
    reloadLimits();

    while (true)
    {
        std::string newDate = UsageRepository::getCurrentDateStr();
        if (newDate != currentDate)
        {
            currentDate = newDate;
            reloadLimits();
        }

        HWND hwnd = GetForegroundWindow();
        if (hwnd)
        {
            DWORD pid = 0;
            GetWindowThreadProcessId(hwnd, &pid);

            std::string processName = SystemScanner::getActiveProcessName(pid);
            std::string windowTitle = SystemScanner::getActiveWindowTitle(hwnd);

            // Tối ưu: Chỉ transform chữ thường 1 lần duy nhất mỗi giây cho tiến trình hiện tại
            std::transform(processName.begin(), processName.end(), processName.begin(),
                           [](unsigned char c) { return std::tolower(c); });
            std::transform(windowTitle.begin(), windowTitle.end(), windowTitle.begin(),
                           [](unsigned char c) { return std::tolower(c); });

            std::shared_ptr<TrackableItem> matchedItem = nullptr;

            // Tra cứu O(1) cho Application bằng Hash Map
            auto it = activeTargets.find(processName);
            if (it != activeTargets.end() && it->second->getType() == "Application")
            {
                matchedItem = it->second;
            }
            else
            {
                // Fallback tìm kiếm chuỗi cho Website (chỉ duyệt các WebItem)
                for (const auto& [key, item] : activeTargets)
                {
                    if (item->getType() == "Website")
                    {
                        if (windowTitle.find(key) != std::string::npos)
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
                matchedItem->checkAndEnforce(hwnd, pid, matchedItem->getTimeLimit());

                // Append-Only Log: Ghi xuống đĩa mỗi khi tích lũy đủ 60 giây (1 phút)
                int currentSecs = matchedItem->getTimeUsedSeconds();
                if (currentSecs > 0 && currentSecs % 60 == 0)
                {
                    UsageRepository::appendUsageLog(matchedItem->getName(), 60);
                }
            }
        }

        // Ngủ 1 giây nhưng vẫn giữ cho Message Loop của Windows hoạt động mượt mà
        MsgWaitForMultipleObjects(0, NULL, FALSE, 1000, QS_ALLEVENTS);
    }
}