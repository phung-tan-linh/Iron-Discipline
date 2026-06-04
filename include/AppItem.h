#ifndef APP_ITEM_H
#define APP_ITEM_H
#include "TrackableItem.h"
class AppItem : public TrackableItem
{
public:
    AppItem(std::string appName, int limitMinutes);
    void displayInfo() const override;
    std::string getType() const override;
};
#endif