// [PLAN]: Triển khai WebItem, xử lý đóng tab bằng phím tắt độc lập với cờ cảnh báo, gọi UIManager để hiển thị UI.
#include "../include/WebItem.h"
#include "../include/UIManager.h"
#include <windows.h>

WebItem::WebItem(std::string url, int limitMinutes, std::string browser)
    : TrackableItem(url, limitMinutes), browserType(std::move(browser)),
      isFirstWarningShown(false), isSecondWarningShown(false) {}

void WebItem::displayInfo() const
{
    std::cout << "[WEB - URL] " << name
              << " | Da dung: " << timeUsedMinutes
              << "/" << timeLimitMinutes << " phut." << std::endl;
}

std::string WebItem::getType() const
{
    return "Website";
}

void WebItem::checkAndEnforce(HWND hwnd, DWORD pid, int globalLimitMinutes)
{
    if (timeUsedMinutes >= globalLimitMinutes)
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
    else if (globalLimitMinutes - timeUsedMinutes <= 5)
    {
        if (!isFirstWarningShown)
        {
            UIManager::ShowWarning(1);
            isFirstWarningShown = true;
        }
    }
}