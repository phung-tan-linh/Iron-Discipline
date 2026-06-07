// [PLAN]: Gộp AppItem, WebItem và TrackableItem. Áp dụng OOP Đa hình (Polymorphism) với hàm ảo checkAndEnforce. Tích hợp std::shared_ptr<TimePool> để chia sẻ quỹ thời gian chung giữa các tiến trình cùng nhóm, đảm bảo an toàn đa luồng bằng std::atomic.
#ifndef TRACKING_TARGETS_H
#define TRACKING_TARGETS_H

#include <string>
#include <memory>
#include <windows.h>
#include "DataStore.h"

class TrackableItem
{
protected:
    std::string name;
    std::string nameLower;
    int timeLimitMinutes;
    std::shared_ptr<TimePool> sharedPool;

public:
    TrackableItem(std::string itemName, int limitMinutes, std::shared_ptr<TimePool> pool);
    virtual ~TrackableItem() = default;
    
    std::string getName() const;
    std::string getNameLower() const;
    int getTimeLimit() const;
    int getTimeUsed() const;
    
    void addTimeUsed(int minutes);
    void addTimeUsedSeconds(int seconds);
    int getTimeUsedSeconds() const;
    void setTimeUsedSeconds(int seconds);
    bool isTimeUp() const;
    
    virtual void displayInfo() const = 0;
    virtual std::string getType() const = 0;
    virtual void checkAndEnforce(HWND hwnd, DWORD pid, int globalLimitMinutes) = 0;
};

class AppItem : public TrackableItem
{
private:
    std::string executablePath;
    bool isFirstWarningShown;
    bool isSecondWarningShown;

public:
    AppItem(std::string appName, int limitMinutes, std::shared_ptr<TimePool> pool, std::string execPath = "");
    void displayInfo() const override;
    std::string getType() const override;
    void checkAndEnforce(HWND hwnd, DWORD pid, int globalLimitMinutes) override;
};

class WebItem : public TrackableItem
{
private:
    std::string browserType;
    bool isFirstWarningShown;
    bool isSecondWarningShown;

public:
    WebItem(std::string url, int limitMinutes, std::shared_ptr<TimePool> pool, std::string browser = "");
    void displayInfo() const override;
    std::string getType() const override;
    void checkAndEnforce(HWND hwnd, DWORD pid, int globalLimitMinutes) override;
};

#endif