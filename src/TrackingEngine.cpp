/*
 * ============================================================================
 * FILE: TrackingEngine.cpp
 * VAI TRÒ: Bộ máy quét hệ thống (System Scanner & Time Enforcer).
 * * ĐIỂM NHẤN HỌC THUẬT:
 * 1. Tối ưu Tra cứu: Sử dụng cấu trúc dữ liệu std::unordered_map mang lại độ
 * phức tạp O(1), giúp tra cứu mục tiêu cực nhanh mà không gây nghẽn CPU.
 * 2. Tối ưu Hiệu năng (Caching): Áp dụng kỹ thuật bộ đệm (Tuple Cache) giữ lại 
 * [HWND, WindowTitle] để tránh việc phải liên tục ép kiểu chuỗi (tolower) 
 * ở tốc độ cao. Giúp ứng dụng chạy ngầm với mức tiêu thụ CPU tiệm cận 0%.
 * ============================================================================
 */

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
    // [WINAPI] Xin quyền đọc thông tin tiến trình từ hệ điều hành.
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
        // Luôn phải dọn dẹp Handle để tránh rò rỉ bộ nhớ (Memory Leak)
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

        // Phân loại tự động giữa App và Web dựa trên phần mở rộng .exe
        if (limit.name.find(".exe") != std::string::npos)
        {
            item = std::make_shared<AppItem>(limit.name, limit.timeLimit, usedSecs);
        }
        else
        {
            item = std::make_shared<WebItem>(limit.name, limit.timeLimit, usedSecs);
        }

        // Đẩy vào unordered_map để sau này tra cứu với chi phí O(1)
        activeTargets[keyLower] = item;
    }
}

void TimeEnforcer::monitorAndBlock()
{
    currentDate = UsageRepository::getCurrentDateStr();
    reloadLimits();
    
    static HWND lastHwnd = NULL;
    static std::string lastWindowTitleRaw = "";
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
            std::string currentTitle = SystemScanner::getActiveWindowTitle(currentHwnd);
            
            // [BẢO VỆ CPU] Tuple Caching: Chỉ thực thi phân tích chuỗi nếu Handle cửa sổ (HWND) 
            // HOẶC Tiêu đề (Title) thực sự thay đổi. Tránh việc gọi WinAPI vô ích.
            if (currentHwnd != lastHwnd || currentTitle != lastWindowTitleRaw)
            {
                lastHwnd = currentHwnd;
                lastWindowTitleRaw = currentTitle;
                
                GetWindowThreadProcessId(currentHwnd, &cachedPid);
                cachedProcessName = SystemScanner::getActiveProcessName(cachedPid);
                cachedWindowTitle = currentTitle;
                
                std::transform(cachedProcessName.begin(), cachedProcessName.end(), cachedProcessName.begin(),
                               [](unsigned char c) { return std::tolower(c); });
                std::transform(cachedWindowTitle.begin(), cachedWindowTitle.end(), cachedWindowTitle.begin(),
                               [](unsigned char c) { return std::tolower(c); });
            }

            std::shared_ptr<TrackableItem> matchedItem = nullptr;

            // Tra cứu O(1) cho Application
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
                
                // [ĐA HÌNH MỨC 4] Gọi hàm ảo. Tự bản thân AppItem hoặc WebItem 
                // sẽ biết phải dùng thuật toán trừng phạt nào mà không cần lệnh if-else cồng kềnh.
                matchedItem->checkAndEnforce(currentHwnd, cachedPid, matchedItem->getTimeLimit());

                int currentSecs = matchedItem->getTimeUsedSeconds();
                if (currentSecs > 0 && currentSecs % 60 == 0)
                {
                    UsageRepository::appendUsageLog(matchedItem->getName(), 60);
                }
            }
        }

        // Nhường CPU cho các tiến trình khác, giảm tải luồng (Thread optimization)
        MsgWaitForMultipleObjects(0, NULL, FALSE, 1000, QS_ALLEVENTS);
    }
}