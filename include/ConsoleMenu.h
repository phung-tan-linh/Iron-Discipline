// [PLAN]: Dọn dẹp ConsoleMenu, gỡ bỏ toàn bộ code gọi Analytics, cập nhật Main Menu chỉ còn các tùy chọn quản lý Giới hạn.
#ifndef CONSOLE_MENU_H
#define CONSOLE_MENU_H

#include "DataStore.h"
#include <string>
#include <vector>

class ConsoleMenu
{
private:
    AppState currentState;
    std::vector<Category> basicList;

    void clearScreen();
    void safeWait();
    void handleInvalidState();
    std::string getCurrentDate();
    std::string formatTime(int minutes);
    void printBasicList();

    void showMainMenu();
    void showLimitApp();
    void showAddBasicLimit();
    void showAddCustomLimit();
    void handleCustomOuterList();
    void handleCustomBasicList();
    void handleRemoveLimitLogic();

public:
    ConsoleMenu();
    void init(const std::string &basicListFile);
    void run();
};

#endif