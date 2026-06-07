// [PLAN]: Khai báo WebItem kế thừa TrackableItem, thêm browserType và cờ cảnh báo, ghi đè checkAndEnforce.
#ifndef WEB_ITEM_H
#define WEB_ITEM_H
#include "TrackableItem.h"

class WebItem : public TrackableItem
{
private:
    std::string browserType;
    bool isFirstWarningShown;
    bool isSecondWarningShown;

public:
    WebItem(std::string url, int limitMinutes, std::string browser = "");
    void displayInfo() const override;
    std::string getType() const override;
    void checkAndEnforce(HWND hwnd, DWORD pid, int globalLimitMinutes) override;
};
#endif