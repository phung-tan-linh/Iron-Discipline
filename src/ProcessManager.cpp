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
#include <atomic>
static HHOOK g_hKeyboardHook = NULL;
static std::vector<ActiveLimit> g_currentLimits;
static LRESULT CALLBACK AntiBypassKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION && g_isWarningActive)
    {
        KBDLLHOOKSTRUCT *p = (KBDLLHOOKSTRUCT *)lParam;
        if (p->vkCode == VK_LWIN || p->vkCode == VK_RWIN || p->vkCode == VK_TAB || p->vkCode == VK_ESCAPE || p->vkCode == VK_LMENU || p->vkCode == VK_RMENU || p->vkCode == VK_LCONTROL || p->vkCode == VK_RCONTROL)
            return 1;
    }
    return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
}
ProcessManager::~ProcessManager()
{
    for (TrackableItem *item : trackingList)
        delete item;
    trackingList.clear();
}
void ProcessManager::addItem(TrackableItem *item) { trackingList.push_back(item); }
void ProcessManager::forceSaveData()
{
    auto usageMap = FileManager::loadDailyUsage();
    for (TrackableItem *item : trackingList)
        if (item->getTimeUsedSeconds() > 0)
            usageMap[item->getSharedIdentifier()] = item->getTimeUsedSeconds();
    FileManager::saveAllDailyUsage(usageMap);
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
        std::cout << "\n[SYSTEM] Da ep dong App (PID: " << pid << ").";
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
    std::cout << "\n[SYSTEM] Da dong tab trinh duyet.";
}
void ProcessManager::showPersistentWarning(const std::string &message, const std::string &title) { MessageBoxA(NULL, message.c_str(), title.c_str(), MB_OK | MB_ICONWARNING | MB_TOPMOST); }
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
    TrackableItem *prevActiveItem = nullptr;
    reloadActiveLimits();
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
                auto usageMap = FileManager::loadDailyUsage();
                usageMap[(*it)->getSharedIdentifier()] = (*it)->getTimeUsedSeconds();
                FileManager::saveAllDailyUsage(usageMap);
                delete *it;
                it = trackingList.erase(it);
            }
            else
            {
                ++it;
            }
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
                std::map<std::string, int> usage = FileManager::loadDailyUsage();
                if (usage.count(limit.name))
                    newItem->setTimeUsedSeconds(usage[limit.name]);
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
            }
            c = n;
        }
        prevActiveItem = activeItem;
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
                    int m = i->getTimeLimit();
                    for (auto &x : g_currentLimits)
                        if (x.name == i->getName())
                        {
                            m = x.timeLimit;
                            break;
                        }
                    int u = i->getTimeUsedSeconds() / 60;
                    if (u >= m - 5 && !i->getIsFirstWarningShown())
                    {
                        std::thread([]()
                                    { UIManager::ShowWarning(1); })
                            .detach();
                        i->setFirstWarningShown(true);
                    }
                    if (u >= m)
                    {
                        if (i->getType() == "Website")
                        {
                            closeBrowserTab(h);
                        }
                        else
                        {
                            if (h)
                                PostMessage(h, WM_CLOSE, 0, 0);
                            Sleep(1000);
                            killAppProcess(p);
                        }
                        if (!g_isWarningActive)
                        {
                            std::thread([]()
                                        { UIManager::ShowWarning(2); })
                                .detach();
                        }
                    }
                }
            }
        }
        if (activeItem != prevActiveItem)
            tc = 0;
        tc++;
        if (tc >= 300)
        {
            forceSaveData();
            tc = 0;
        }
        Sleep(1000);
    }
}