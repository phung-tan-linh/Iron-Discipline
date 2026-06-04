#include "../include/WebItem.h"
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