// [PLAN]: Gộp AppItem, WebItem và TrackableItem. Áp dụng OOP Đa hình (Polymorphism) với hàm ảo checkAndEnforce. Loại bỏ TimePool, sử dụng biến timeUsedSeconds độc lập. Chuyển cờ cảnh báo lên lớp cơ sở và đồng bộ ngay khi khởi tạo để chống lỗi reset cảnh báo khi restart app.
#ifndef TRACKING_TARGETS_H
#define TRACKING_TARGETS_H

#include <string>
#include <windows.h>

class TrackableItem
{
protected:
    std::string name;
    std::string nameLower;
    int timeLimitMinutes;
    int timeUsedSeconds;
    bool isFirstWarningShown;
    bool isSecondWarningShown;

public:
    TrackableItem(std::string itemName, int limitMinutes, int initialUsedSeconds = 0);
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
    
    void syncWarningState(int currentUsedMinutes);

    virtual void displayInfo() const = 0;
    virtual std::string getType() const = 0;
    virtual void checkAndEnforce(HWND hwnd, DWORD pid, int globalLimitMinutes) = 0;
};

class AppItem : public TrackableItem
{
private:
    std::string executablePath;

public:
    AppItem(std::string appName, int limitMinutes, int initialUsedSeconds = 0, std::string execPath = "");
    void displayInfo() const override;
    std::string getType() const override;
    void checkAndEnforce(HWND hwnd, DWORD pid, int globalLimitMinutes) override;
};

class WebItem : public TrackableItem
{
private:
    std::string browserType;

public:
    WebItem(std::string url, int limitMinutes, int initialUsedSeconds = 0, std::string browser = "");
    void displayInfo() const override;
    std::string getType() const override;
    void checkAndEnforce(HWND hwnd, DWORD pid, int globalLimitMinutes) override;
};

#endif