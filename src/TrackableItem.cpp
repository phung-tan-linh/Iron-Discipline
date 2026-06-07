// [PLAN]: Triển khai TrackableItem, loại bỏ logic cờ cảnh báo, giữ nguyên logic đếm thời gian và cache chuỗi chữ thường.
#include "../include/TrackableItem.h"
#include <atomic>
#include <algorithm>
#include <cctype>

extern std::atomic<int> g_WarningActive;

TrackableItem::TrackableItem(std::string itemName, int limitMinutes)
{
    this->name = itemName;
    this->nameLower = itemName;
    std::transform(this->nameLower.begin(), this->nameLower.end(), this->nameLower.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    this->timeLimitMinutes = limitMinutes;
    this->timeUsedMinutes = 0;
    this->timeUsedSeconds = 0;
}

std::string TrackableItem::getName() const { return name; }
std::string TrackableItem::getNameLower() const { return nameLower; }
int TrackableItem::getTimeLimit() const { return timeLimitMinutes; }
int TrackableItem::getTimeUsed() const { return timeUsedMinutes; }

void TrackableItem::addTimeUsed(int minutes)
{
    if (minutes > 0)
    {
        addTimeUsedSeconds(minutes * 60);
    }
}

void TrackableItem::addTimeUsedSeconds(int seconds)
{
    if (seconds > 0 && g_WarningActive == 0)
    {
        timeUsedSeconds += seconds;
        timeUsedMinutes = timeUsedSeconds / 60;
    }
}

int TrackableItem::getTimeUsedSeconds() const { return timeUsedSeconds; }

void TrackableItem::setTimeUsedSeconds(int seconds)
{
    if (seconds >= 0)
    {
        timeUsedSeconds = seconds;
        timeUsedMinutes = timeUsedSeconds / 60;
    }
}

bool TrackableItem::isTimeUp() const
{
    return timeUsedSeconds >= (timeLimitMinutes * 60);
}