// [PLAN]: Triển khai TrackableItem với logic Heuristic để tránh lặp state cảnh báo và tạm dừng đếm giờ khi có Overlay thông báo (thông qua g_WarningActive).
#include "../include/TrackableItem.h"
#include <atomic>

extern std::atomic<int> g_WarningActive;

TrackableItem::TrackableItem(std::string itemName, int limitMinutes, std::string identifier)
{
    this->name = itemName;
    this->sharedIdentifier = identifier.empty() ? itemName : identifier;
    this->timeLimitMinutes = limitMinutes;
    this->timeUsedMinutes = 0;
    this->timeUsedSeconds = 0;
    this->isFirstWarningShown = false;
    this->isSecondWarningShown = false;
}

std::string TrackableItem::getName() const { return name; }
std::string TrackableItem::getSharedIdentifier() const { return sharedIdentifier; }
int TrackableItem::getTimeLimit() const { return timeLimitMinutes; }
int TrackableItem::getTimeUsed() const { return timeUsedMinutes; }

bool TrackableItem::getIsFirstWarningShown() const { return isFirstWarningShown; }
void TrackableItem::setFirstWarningShown(bool status) { isFirstWarningShown = status; }

bool TrackableItem::getIsSecondWarningShown() const { return isSecondWarningShown; }
void TrackableItem::setSecondWarningShown(bool status) { isSecondWarningShown = status; }

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
        
        if (timeUsedMinutes >= timeLimitMinutes - 5)
        {
            isFirstWarningShown = true;
        }
    }
}

bool TrackableItem::isTimeUp() const
{
    return timeUsedSeconds >= (timeLimitMinutes * 60);
}