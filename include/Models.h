// [PLAN]: Tối ưu Models, loại bỏ thuộc tính date khỏi ActiveLimit, giữ nguyên các cấu trúc dữ liệu cơ bản.
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
    int type;
    std::string name;
    int timeLimit;
};

#endif