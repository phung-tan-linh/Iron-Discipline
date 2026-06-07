// [PLAN]: Khai báo lớp AppItem kế thừa TrackableItem, ghi đè hàm enforceBlock để xử lý chặn ứng dụng.
#ifndef APP_ITEM_H
#define APP_ITEM_H
#include "TrackableItem.h"

class AppItem : public TrackableItem
{
public:
    AppItem(std::string appName, int limitMinutes);
    void displayInfo() const override;
    std::string getType() const override;
    void enforceBlock(HWND hwnd, DWORD pid) override;
};
#endif