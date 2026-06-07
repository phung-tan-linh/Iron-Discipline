// [PLAN]: Cập nhật ProcessManager để đồng bộ Cache mỗi 5 phút, loại bỏ logic MessageBox/Sleep cũ và tích hợp g_WarningActive. Xóa dead code.
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
    bool isAppMatch(const std::string &nameLower, const std::string &processName);
    bool isWebsiteMatch(const std::string &nameLower, const std::string &windowTitle);
    void monitorAndBlock();
    void forceSaveData();
    void reloadActiveLimits();
};
#endif