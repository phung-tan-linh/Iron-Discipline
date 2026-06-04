#ifndef WEB_ITEM_H
#define WEB_ITEM_H
#include "TrackableItem.h"
class WebItem : public TrackableItem
{
public:
    WebItem(std::string url, int limitMinutes);
    void displayInfo() const override;
    std::string getType() const override;
};
#endif