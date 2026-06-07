// [PLAN]: Định nghĩa giao diện TrackableItem, loại bỏ cờ cảnh báo, thêm checkAndEnforce để ủy quyền logic xử lý cho lớp con.
#ifndef TRACKABLE_ITEM_H
#define TRACKABLE_ITEM_H
#include <string>
#include <iostream>
#include <windows.h>

class TrackableItem
{
protected:
    std::string name;
    std::string nameLower;
    int timeLimitMinutes;
    int timeUsedMinutes;
    int timeUsedSeconds;

public:
    TrackableItem(std::string itemName, int limitMinutes);
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
#endif