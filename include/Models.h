// [PLAN]: Tối ưu Models, loại bỏ hoàn toàn các cấu trúc dữ liệu liên quan đến Analytics (VIEW_ACTIVITY, HistoryLog).
#ifndef MODELS_H
#define MODELS_H
#include <string>
#include <vector>

enum class AppState
{
    MAIN_MENU,
    LIMIT_APP,
    EXIT
};

struct BasicItem
{
    int id;
    std::string name;
};

struct Category
{
    std::string romanID;
    std::string title;
    std::vector<BasicItem> items;
};

struct ActiveLimit
{
    std::string date;
    int type;
    std::string name;
    int timeLimit;
};

#endif