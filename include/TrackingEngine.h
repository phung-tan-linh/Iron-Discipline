#ifndef TRACKING_ENGINE_H
#define TRACKING_ENGINE_H

#include <windows.h>
#include <string>
#include <unordered_map>
#include <memory>
#include "TrackingTargets.h"

class SystemScanner
{
public:
    static std::string getActiveWindowTitle(HWND hwnd);
    static std::string getActiveProcessName(DWORD pid);
};

class TimeEnforcer
{
private:
    std::unordered_map<std::string, std::shared_ptr<TrackableItem>> activeTargets;
    std::string currentDate;

public:
    TimeEnforcer() = default;
    ~TimeEnforcer() = default;

    void reloadLimits();
    void monitorAndBlock();
};

#endif