// [PLAN]: Triển khai WebItem, ghi đè enforceBlock để đóng tab trình duyệt bằng cách giả lập phím Ctrl+W.
#include "../include/WebItem.h"
#include <windows.h>

WebItem::WebItem(std::string url, int limitMinutes)
    : TrackableItem(url, limitMinutes) {}

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

void WebItem::enforceBlock(HWND hwnd, DWORD pid)
{
    if (hwnd != NULL)
    {
        SetForegroundWindow(hwnd);
        
        INPUT inputs[4] = {};
        
        // Press Ctrl
        inputs[0].type = INPUT_KEYBOARD;
        inputs[0].ki.wVk = VK_CONTROL;
        
        // Press W
        inputs[1].type = INPUT_KEYBOARD;
        inputs[1].ki.wVk = 'W';
        
        // Release W
        inputs[2].type = INPUT_KEYBOARD;
        inputs[2].ki.wVk = 'W';
        inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;
        
        // Release Ctrl
        inputs[3].type = INPUT_KEYBOARD;
        inputs[3].ki.wVk = VK_CONTROL;
        inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;
        
        SendInput(4, inputs, sizeof(INPUT));
    }
}