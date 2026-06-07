// [PLAN]: Áp dụng SOLID chia tách SystemScanner (WinAPI) và TimeEnforcer (Logic). Tối ưu 100% CPU bằng std::unordered_map tra cứu O(1). Chuẩn hóa Key sang chữ thường 1 lần lúc nạp. Chỉ transform tên tiến trình hiện tại 1 lần/giây, loại bỏ vòng lặp O(N) tốn kém.
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
    // Thay vì chỉ lưu TimePool, ta lưu TrackableItem (chứa sẵn TimePool) để gọi đa hình checkAndEnforce O(1)
    std::unordered_map<std::string, std::shared_ptr<TrackableItem>> activeTargets;
    std::string currentDate;

public:
    TimeEnforcer() = default;
    ~TimeEnforcer() = default;

    void reloadLimits();
    void monitorAndBlock();
};

#endif