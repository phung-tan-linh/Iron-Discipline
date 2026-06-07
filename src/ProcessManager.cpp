// [PLAN]: Triển khai Lazy Initialization (chỉ tạo TrackableItem khi phát hiện tiến trình), gọi đa hình checkAndEnforce, dọn dẹp bộ nhớ an toàn.
#include "../include/ProcessManager.h"
#include "../include/FileManager.h"
#include "../include/UIManager.h"
#include "../include/AppItem.h"
#include "../include/WebItem.h"
#include "../include/Models.h"
#include <psapi.h>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <thread>
#include <map>
#include <mutex>

static std::vector<ActiveLimit> g_currentLimits;

ProcessManager::~ProcessManager()
{
    for (TrackableItem *item : trackingList)
        delete item;
    trackingList.clear();
}

void ProcessManager::addItem(TrackableItem *item) { trackingList.push_back(item); }

void ProcessManager::forceSaveData()
{
    for (TrackableItem *item : trackingList)
    {
        if (item->getTimeUsedSeconds() > 0)
        {
            FileManager::updateDailyUsageItem(item->getName(), item->getTimeUsedSeconds());
        }
    }
    std::thread([]() { FileManager::syncDailyUsageToFile(); }).detach();
}

std::string ProcessManager::getActiveWindowTitle(HWND hwnd)
{
    char windowTitle[256];
    if (GetWindowTextA(hwnd, windowTitle, sizeof(windowTitle)) > 0)
        return std::string(windowTitle);
    return "";
}

std::string ProcessManager::getActiveProcessName(DWORD pid)
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
                processName = fullPath.substr(pos + 1);
        }
        CloseHandle(hProcess);
    }
    return processName;
}

bool ProcessManager::isAppMatch(const std::string &nameLower, const std::string &processName)
{
    std::string pLower = processName;
    std::transform(pLower.begin(), pLower.end(), pLower.begin(), [](unsigned char c) { return std::tolower(c); });
    return pLower.find(nameLower) != std::string::npos;
}

bool ProcessManager::isWebsiteMatch(const std::string &nameLower, const std::string &windowTitle)
{
    std::string wLower = windowTitle;
    std::transform(wLower.begin(), wLower.end(), wLower.begin(), [](unsigned char c) { return std::tolower(c); });
    return wLower.find(nameLower) != std::string::npos;
}

void ProcessManager::reloadActiveLimits() { g_currentLimits = FileManager::getActiveLimits(); }

void ProcessManager::monitorAndBlock()
{
    std::string c = FileManager::getCurrentDateStr();
    int tc = 0;
    reloadActiveLimits();
    FileManager::loadDailyUsage();

    while (true)
    {
        for (auto it = trackingList.begin(); it != trackingList.end();)
        {
            bool found = false;
            for (const auto &limit : g_currentLimits)
            {
                if (limit.name == (*it)->getName())
                {
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                if ((*it)->getTimeUsedSeconds() > 0)
                {
                    FileManager::updateDailyUsageItem((*it)->getName(), (*it)->getTimeUsedSeconds());
                }
                delete *it;
                it = trackingList.erase(it);
            }
            else
            {
                ++it;
            }
        }

        std::string n = FileManager::getCurrentDateStr();
        if (c != n)
        {
            FileManager::loadDailyUsage();
            for (auto *i : trackingList)
            {
                delete i;
            }
            trackingList.clear();
            c = n;
        }

        HWND h = GetForegroundWindow();
        if (h)
        {
            DWORD p = 0;
            GetWindowThreadProcessId(h, &p);
            std::string w = getActiveWindowTitle(h);
            std::string r = getActiveProcessName(p);
            
            for (const auto &limit : g_currentLimits)
            {
                bool isApp = (limit.name.find(".exe") != std::string::npos);
                std::string limitLower = limit.name;
                std::transform(limitLower.begin(), limitLower.end(), limitLower.begin(), [](unsigned char ch) { return std::tolower(ch); });

                bool matched = false;
                if (isApp && isAppMatch(limitLower, r)) matched = true;
                else if (!isApp && isWebsiteMatch(limitLower, w)) matched = true;

                if (matched)
                {
                    TrackableItem *activeItem = nullptr;
                    for (auto *item : trackingList)
                    {
                        if (item->getName() == limit.name)
                        {
                            activeItem = item;
                            break;
                        }
                    }

                    if (!activeItem)
                    {
                        if (isApp)
                            activeItem = new AppItem(limit.name, limit.timeLimit, r);
                        else
                            activeItem = new WebItem(limit.name, limit.timeLimit, "");

                        int savedSecs = 0;
                        {
                            std::lock_guard<std::mutex> lock(FileManager::g_usageMutex);
                            if (FileManager::g_dailyUsageCache.find(activeItem->getName()) != FileManager::g_dailyUsageCache.end())
                                savedSecs = FileManager::g_dailyUsageCache[activeItem->getName()];
                        }
                        
                        activeItem->setTimeUsedSeconds(savedSecs);
                        trackingList.push_back(activeItem);
                    }

                    activeItem->addTimeUsedSeconds(1);
                    FileManager::updateDailyUsageItem(activeItem->getName(), activeItem->getTimeUsedSeconds());

                    activeItem->checkAndEnforce(h, p, limit.timeLimit);
                    
                    break;
                }
            }
        }

        tc++;
        if (tc >= 300)
        {
            forceSaveData();
            tc = 0;
        }
        
        MsgWaitForMultipleObjects(0, NULL, FALSE, 1000, QS_ALLEVENTS);
    }
}