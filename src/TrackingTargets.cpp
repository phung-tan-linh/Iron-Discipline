// [PLAN]: Triển khai các lớp TrackingTargets. Quản lý thời gian độc lập trong từng object.
// Sử dụng WinAPI TerminateProcess cho App và SendInput (Ctrl+W) cho Web.
// Áp dụng Debounce (3000ms) trong WebItem::checkAndEnforce để ngăn lỗi dồn ứ hàng đợi SendInput.
#include "../include/TrackingTargets.h"
#include "../include/UIManager.h"
#include <iostream>
#include <algorithm>
#include <cctype>

TrackableItem::TrackableItem(std::string itemName, int limitMinutes, int initialUsedSeconds)
    : name(std::move(itemName)), timeLimitMinutes(limitMinutes), timeUsedSeconds(initialUsedSeconds),
      isFirstWarningShown(false), isSecondWarningShown(false)
{
    nameLower = name;
    std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    syncWarningState(timeUsedSeconds / 60);
}

std::string TrackableItem::getName() const { return name; }
std::string TrackableItem::getNameLower() const { return nameLower; }
int TrackableItem::getTimeLimit() const { return timeLimitMinutes; }
int TrackableItem::getTimeUsed() const { return timeUsedSeconds / 60; }

void TrackableItem::addTimeUsed(int minutes) { timeUsedSeconds += minutes * 60; }
void TrackableItem::addTimeUsedSeconds(int seconds) { timeUsedSeconds += seconds; }
int TrackableItem::getTimeUsedSeconds() const { return timeUsedSeconds; }
void TrackableItem::setTimeUsedSeconds(int seconds) { timeUsedSeconds = seconds; }
bool TrackableItem::isTimeUp() const { return (timeUsedSeconds / 60) >= timeLimitMinutes; }

void TrackableItem::syncWarningState(int currentUsedMinutes)
{
    if (currentUsedMinutes >= timeLimitMinutes)
    {
        isFirstWarningShown = true;
        isSecondWarningShown = true;
    }
    else if (timeLimitMinutes - currentUsedMinutes <= 5)
    {
        isFirstWarningShown = true;
    }
}

// --- AppItem Implementation ---

AppItem::AppItem(std::string appName, int limitMinutes, int initialUsedSeconds, std::string execPath)
    : TrackableItem(std::move(appName), limitMinutes, initialUsedSeconds), executablePath(std::move(execPath))
{
}

void AppItem::displayInfo() const
{
    std::cout << "[App] " << name << " | Limit: " << timeLimitMinutes 
              << "m | Used: " << getTimeUsed() << "m\n";
}

std::string AppItem::getType() const
{
    return "Application";
}

void AppItem::checkAndEnforce(HWND hwnd, DWORD pid, int globalLimitMinutes)
{
    int currentMinutes = timeUsedSeconds / 60;
    
    if (currentMinutes >= timeLimitMinutes)
    {
        if (!isSecondWarningShown)
        {
            UIManager::ShowWarning(2);
            isSecondWarningShown = true;
        }
        
        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
        if (hProcess != NULL)
        {
            TerminateProcess(hProcess, 0);
            CloseHandle(hProcess);
        }
    }
    else if (timeLimitMinutes - currentMinutes <= 5)
    {
        if (!isFirstWarningShown)
        {
            UIManager::ShowWarning(1);
            isFirstWarningShown = true;
        }
    }
}

// --- WebItem Implementation ---

WebItem::WebItem(std::string url, int limitMinutes, int initialUsedSeconds, std::string browser)
    : TrackableItem(std::move(url), limitMinutes, initialUsedSeconds), browserType(std::move(browser)), lastClosedTime(0)
{
}

void WebItem::displayInfo() const
{
    std::cout << "[Web] " << name << " | Limit: " << timeLimitMinutes 
              << "m | Used: " << getTimeUsed() << "m\n";
}

std::string WebItem::getType() const
{
    return "Website";
}

void WebItem::checkAndEnforce(HWND hwnd, DWORD pid, int globalLimitMinutes)
{
    int currentMinutes = timeUsedSeconds / 60;
    
    if (currentMinutes >= timeLimitMinutes)
    {
        if (!isSecondWarningShown)
        {
            UIManager::ShowWarning(2);
            isSecondWarningShown = true;
        }
        
        if (hwnd)
        {
            if (GetTickCount() - lastClosedTime >= 3000)
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
                
                lastClosedTime = GetTickCount();
            }
        }
    }
    else if (timeLimitMinutes - currentMinutes <= 5)
    {
        if (!isFirstWarningShown)
        {
            UIManager::ShowWarning(1);
            isFirstWarningShown = true;
        }
    }
}