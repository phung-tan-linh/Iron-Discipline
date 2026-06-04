#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H
#include <windows.h>
#include <vector>
#include <string>
#include <atomic>
#include <algorithm>
#include "TrackableItem.h"
class ProcessManager
{
private:
    std::vector<TrackableItem *> trackingList;

public:
    ProcessManager() = default;
    ~ProcessManager();
    void addItem(TrackableItem *item);
    std::string getActiveWindowTitle(HWND hwnd);
    std::string getActiveProcessName(DWORD pid);
    void killAppProcess(DWORD pid);
    void closeBrowserTab(HWND hwnd);
    void showPersistentWarning(const std::string &message, const std::string &title);
    std::string toLowerCase(const std::string &str);
    bool isAppMatch(const std::string &appName, const std::string &processName);
    bool isWebsiteMatch(const std::string &url, const std::string &windowTitle);
    void monitorAndBlock();
    void forceSaveData();
    void reloadActiveLimits();
};
#endif