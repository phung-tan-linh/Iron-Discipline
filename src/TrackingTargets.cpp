// [PLAN]: Triển khai TrackingTargets. Loại bỏ cờ g_WarningActive để thời gian không bị đóng băng. AppItem dùng TerminateProcess để ép đóng .exe, WebItem dùng SendInput (Ctrl+W) để đóng tab. Cả hai đều gọi UIManager để hiển thị cảnh báo.
#include "../include/TrackingTargets.h"
#include "UIManager.h"
#include <iostream>
#include <algorithm>
#include <cctype>

TrackableItem::TrackableItem(std::string itemName, int limitMinutes, std::shared_ptr<TimePool> pool)
    : name(std::move(itemName)), timeLimitMinutes(limitMinutes), sharedPool(std::move(pool))
{
    nameLower = name;
    std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(),
                   [](unsigned char c) { return std::tolower(c); });
}

std::string TrackableItem::getName() const { return name; }
std::string TrackableItem::getNameLower() const { return nameLower; }
int TrackableItem::getTimeLimit() const { return timeLimitMinutes; }

int TrackableItem::getTimeUsedSeconds() const
{
    if (sharedPool)
    {
        return sharedPool->timeUsedSeconds.load(std::memory_order_relaxed);
    }
    return 0;
}

int TrackableItem::getTimeUsed() const
{
    return getTimeUsedSeconds() / 60;
}

void TrackableItem::addTimeUsed(int minutes)
{
    if (minutes > 0)
    {
        addTimeUsedSeconds(minutes * 60);
    }
}

void TrackableItem::addTimeUsedSeconds(int seconds)
{
    if (seconds > 0 && sharedPool)
    {
        sharedPool->timeUsedSeconds.fetch_add(seconds, std::memory_order_relaxed);
    }
}

void TrackableItem::setTimeUsedSeconds(int seconds)
{
    if (seconds >= 0 && sharedPool)
    {
        sharedPool->timeUsedSeconds.store(seconds, std::memory_order_relaxed);
    }
}

bool TrackableItem::isTimeUp() const
{
    return getTimeUsedSeconds() >= (timeLimitMinutes * 60);
}

// --- AppItem Implementation ---

AppItem::AppItem(std::string appName, int limitMinutes, std::shared_ptr<TimePool> pool, std::string execPath)
    : TrackableItem(std::move(appName), limitMinutes, std::move(pool)),
      executablePath(std::move(execPath)),
      isFirstWarningShown(false), isSecondWarningShown(false) {}

void AppItem::displayInfo() const
{
    std::cout << "[APP - .exe] " << name
              << " | Da dung: " << getTimeUsed()
              << "/" << timeLimitMinutes << " phut." << std::endl;
}

std::string AppItem::getType() const
{
    return "Application";
}

void AppItem::checkAndEnforce(HWND hwnd, DWORD pid, int globalLimitMinutes)
{
    int usedMins = getTimeUsed();
    if (usedMins >= globalLimitMinutes)
    {
        if (pid != 0)
        {
            HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
            if (hProcess != NULL)
            {
                TerminateProcess(hProcess, 0);
                CloseHandle(hProcess);
            }
        }
        
        if (!isSecondWarningShown)
        {
            UIManager::ShowWarning(2);
            isSecondWarningShown = true;
        }
    }
    else if (globalLimitMinutes - usedMins <= 5)
    {
        if (!isFirstWarningShown)
        {
            UIManager::ShowWarning(1);
            isFirstWarningShown = true;
        }
    }
}

// --- WebItem Implementation ---

WebItem::WebItem(std::string url, int limitMinutes, std::shared_ptr<TimePool> pool, std::string browser)
    : TrackableItem(std::move(url), limitMinutes, std::move(pool)),
      browserType(std::move(browser)),
      isFirstWarningShown(false), isSecondWarningShown(false) {}

void WebItem::displayInfo() const
{
    std::cout << "[WEB - URL] " << name
              << " | Da dung: " << getTimeUsed()
              << "/" << timeLimitMinutes << " phut." << std::endl;
}

std::string WebItem::getType() const
{
    return "Website";
}

void WebItem::checkAndEnforce(HWND hwnd, DWORD pid, int globalLimitMinutes)
{
    int usedMins = getTimeUsed();
    if (usedMins >= globalLimitMinutes)
    {
        if (hwnd != NULL)
        {
            SetForegroundWindow(hwnd);
            
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
        
        if (!isSecondWarningShown)
        {
            UIManager::ShowWarning(2);
            isSecondWarningShown = true;
        }
    }
    else if (globalLimitMinutes - usedMins <= 5)
    {
        if (!isFirstWarningShown)
        {
            UIManager::ShowWarning(1);
            isFirstWarningShown = true;
        }
    }
}