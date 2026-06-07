// [PLAN]: Tối ưu Models, trang bị Constructor cho ActiveLimit chuẩn OOP, hỗ trợ Move Semantics để tối ưu bộ nhớ.
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

    ActiveLimit() = default;
    ActiveLimit(int t, std::string n, int tl) : type(t), name(std::move(n)), timeLimit(tl) {}
};

#endif