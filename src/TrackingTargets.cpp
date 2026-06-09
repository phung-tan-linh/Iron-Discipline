/*
 * ============================================================================
 * FILE: TrackingTargets.cpp
 * VAI TRÒ: Định nghĩa các thực thể mục tiêu (App và Web) và biện pháp trừng phạt.
 * * ĐIỂM NHẤN HỌC THUẬT:
 * 1. Thiết kế Hướng đối tượng Mức 4 (Polymorphism - Đa hình): 
 * Lớp cha TrackableItem chứa hàm ảo (virtual), cho phép các lớp con AppItem 
 * và WebItem ghi đè (override) để tự quyết định hình phạt riêng.
 * 2. Tương tác WinAPI Cấp thấp:
 * - AppItem: Sử dụng TerminateProcess để tiêu diệt tiến trình.
 * - WebItem: Sử dụng kỹ thuật SendInput (giả lập phím Ctrl+W) kết hợp 
 * thuật toán Debounce (chống dội phím 3000ms) để đóng tab an toàn.
 * ============================================================================
 */

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
        
        // [WINAPI CẤP THẤP] Mở handle với quyền TERMINATE để cưỡng chế đóng ứng dụng.
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
            // [THUẬT TOÁN DEBOUNCE] So sánh GetTickCount() với lastClosedTime (3000ms).
            // Ngăn chặn lỗi Chain-Reaction (đóng hàng loạt tab khác) khi OS bị dồn ứ phím.
            if (GetTickCount() - lastClosedTime >= 3000)
            {
                SetForegroundWindow(hwnd);
                
                // [WINAPI FAKE INPUT] Giả lập tổ hợp phím Ctrl + W để đóng duyên dáng tab hiện tại
                // thay vì giết toàn bộ trình duyệt, bảo vệ an toàn cho dữ liệu người dùng.
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