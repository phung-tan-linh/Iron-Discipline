#include "../include/AppItem.h"
AppItem::AppItem(std::string appName, int limitMinutes)
    : TrackableItem(appName, limitMinutes) {}
void AppItem::displayInfo() const
{
    std::cout << "[APP - .exe] " << name
              << " | Da dung: " << timeUsedMinutes
              << "/" << timeLimitMinutes << " phut." << std::endl;
}
std::string AppItem::getType() const
{
    return "Application";
}