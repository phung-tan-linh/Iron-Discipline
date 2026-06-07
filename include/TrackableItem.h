// [PLAN]: Định nghĩa giao diện TrackableItem, loại bỏ sharedIdentifier, thêm nameLower để cache và enforceBlock để thể hiện đa hình.
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
    bool isFirstWarningShown;
    bool isSecondWarningShown;

public:
    TrackableItem(std::string itemName, int limitMinutes);
    virtual ~TrackableItem() = default;
    
    std::string getName() const;
    std::string getNameLower() const;
    int getTimeLimit() const;
    int getTimeUsed() const;
    
    bool getIsFirstWarningShown() const;
    void setFirstWarningShown(bool status);
    bool getIsSecondWarningShown() const;
    void setSecondWarningShown(bool status);
    
    void addTimeUsed(int minutes);
    void addTimeUsedSeconds(int seconds);
    int getTimeUsedSeconds() const;
    void setTimeUsedSeconds(int seconds);
    bool isTimeUp() const;
    
    virtual void displayInfo() const = 0;
    virtual std::string getType() const = 0;
    virtual void enforceBlock(HWND hwnd, DWORD pid) = 0;
};
#endif