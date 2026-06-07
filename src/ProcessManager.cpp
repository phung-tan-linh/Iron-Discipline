// [PLAN]: Triển khai MsgWaitForMultipleObjects để tối ưu vòng lặp. Gọi syncDailyUsageToFile mỗi 5 phút. Xóa bỏ Sleep và MessageBox chặn luồng. Tối ưu so sánh chuỗi và đa hình enforceBlock.
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
    TrackableItem *activeItem = nullptr;
    reloadActiveLimits();
    FileManager::loadDailyUsage();

    while (true)
    {
        for (auto it = trackingList.begin(); it != trackingList.end();)
        {
            bool found = false;
            for (const auto &limit : g_currentLimits)
                if (limit.name == (*it)->getName())
                {
                    found = true;
                    break;
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
                ++it;
        }

        for (const auto &limit : g_currentLimits)
        {
            bool found = false;
            for (auto *item : trackingList)
                if (item->getName() == limit.name)
                {
                    found = true;
                    break;
                }
            if (!found)
            {
                TrackableItem *newItem;
                if (limit.name.find(".exe") != std::string::npos)
                    newItem = new AppItem(limit.name, limit.timeLimit);
                else
                    newItem = new WebItem(limit.name, limit.timeLimit);

                int savedSecs = 0;
                {
                    std::lock_guard<std::mutex> lock(FileManager::g_usageMutex);
                    if (FileManager::g_dailyUsageCache.find(newItem->getName()) != FileManager::g_dailyUsageCache.end())
                        savedSecs = FileManager::g_dailyUsageCache[newItem->getName()];
                }
                
                newItem->setTimeUsedSeconds(savedSecs);
                trackingList.push_back(newItem);
            }
        }

        std::string n = FileManager::getCurrentDateStr();
        if (c != n)
        {
            FileManager::loadDailyUsage();
            for (auto *i : trackingList)
            {
                i->setTimeUsedSeconds(0);
                i->setFirstWarningShown(false);
                i->setSecondWarningShown(false);
            }
            c = n;
        }

        activeItem = nullptr;
        HWND h = GetForegroundWindow();
        if (h)
        {
            DWORD p = 0;
            GetWindowThreadProcessId(h, &p);
            std::string w = getActiveWindowTitle(h), r = getActiveProcessName(p);
            
            for (auto *i : trackingList)
            {
                if ((i->getType() == "Application" && isAppMatch(i->getNameLower(), r)) || (i->getType() == "Website" && isWebsiteMatch(i->getNameLower(), w)))
                {
                    i->addTimeUsedSeconds(1);
                    activeItem = i;
                    
                    FileManager::updateDailyUsageItem(i->getName(), i->getTimeUsedSeconds());

                    int m = i->getTimeLimit();
                    for (auto &x : g_currentLimits)
                        if (x.name == i->getName())
                        {
                            m = x.timeLimit;
                            break;
                        }
                        
                    int u = i->getTimeUsedSeconds() / 60;
                    
                    if (u >= m)
                    {
                        if (!i->getIsSecondWarningShown())
                        {
                            i->enforceBlock(h, p);
                            UIManager::ShowWarning(2);
                            i->setSecondWarningShown(true);
                        }
                    }
                    else if (u >= m - 5 && !i->getIsFirstWarningShown())
                    {
                        UIManager::ShowWarning(1);
                        i->setFirstWarningShown(true);
                    }
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