// [PLAN]: Triển khai MsgWaitForMultipleObjects để tối ưu vòng lặp. Gọi syncDailyUsageToFile mỗi 5 phút. Xóa bỏ Sleep và MessageBox chặn luồng.
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
        FileManager::updateDailyUsageItem(item->getSharedIdentifier(), item->getTimeUsedSeconds());
    }
    FileManager::syncDailyUsageToFile();
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

void ProcessManager::killAppProcess(DWORD pid)
{
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (hProcess != NULL)
    {
        TerminateProcess(hProcess, 0);
        CloseHandle(hProcess);
        std::cout << "[SYSTEM] Da ep dong App (PID: " << pid << ").\n";
    }
}

void ProcessManager::closeBrowserTab(HWND hwnd)
{
    SetForegroundWindow(hwnd);
    if (hwnd == GetForegroundWindow())
    {
        INPUT inputs[4] = {};
        inputs[0].type = INPUT_KEYBOARD;
        inputs[0].ki.wVk = VK_CONTROL;
        inputs[1].type = INPUT_KEYBOARD;
        inputs[1].ki.wVk = 'W';
        inputs[2].type = INPUT_KEYBOARD;
        inputs[2].ki.wVk = 'W';
        inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;
        inputs[3].type = INPUT_KEYBOARD;
        inputs[3].ki.wVk = VK_CONTROL;
        inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(4, inputs, sizeof(INPUT));
    }
    std::cout << "[SYSTEM] Da dong tab trinh duyet.\n";
}

std::string ProcessManager::toLowerCase(const std::string &str)
{
    std::string lowerStr = str;
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), [](unsigned char c)
                   { return std::tolower(c); });
    return lowerStr;
}

bool ProcessManager::isAppMatch(const std::string &appName, const std::string &processName) { return toLowerCase(processName).find(toLowerCase(appName)) != std::string::npos; }

bool ProcessManager::isWebsiteMatch(const std::string &url, const std::string &windowTitle) { return toLowerCase(windowTitle).find(toLowerCase(url)) != std::string::npos; }

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
                FileManager::updateDailyUsageItem((*it)->getSharedIdentifier(), (*it)->getTimeUsedSeconds());
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
                    if (FileManager::g_dailyUsageCache.find(newItem->getSharedIdentifier()) != FileManager::g_dailyUsageCache.end())
                        savedSecs = FileManager::g_dailyUsageCache[newItem->getSharedIdentifier()];
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
                if ((i->getType() == "Application" && isAppMatch(i->getName(), r)) || (i->getType() == "Website" && isWebsiteMatch(i->getName(), w)))
                {
                    i->addTimeUsedSeconds(1);
                    activeItem = i;
                    
                    FileManager::updateDailyUsageItem(i->getSharedIdentifier(), i->getTimeUsedSeconds());

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
                        if (i->getType() == "Website")
                            closeBrowserTab(h);
                        else
                            killAppProcess(p);
                            
                        i->setSecondWarningShown(true);
                        UIManager::ShowWarning(2);
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