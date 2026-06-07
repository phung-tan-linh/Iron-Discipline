// [PLAN]: Khai báo AppItem kế thừa TrackableItem, thêm executablePath và cờ cảnh báo, ghi đè checkAndEnforce.
#ifndef APP_ITEM_H
#define APP_ITEM_H
#include "TrackableItem.h"

class AppItem : public TrackableItem
{
private:
    std::string executablePath;
    bool isFirstWarningShown;
    bool isSecondWarningShown;

public:
    AppItem(std::string appName, int limitMinutes, std::string execPath = "");
    void displayInfo() const override;
    std::string getType() const override;
    void checkAndEnforce(HWND hwnd, DWORD pid, int globalLimitMinutes) override;
};
#endif