#ifndef MODELS_H
#define MODELS_H
#include <string>
#include <vector>
enum class AppState
{
    MAIN_MENU,
    VIEW_ACTIVITY,
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
struct HistoryLog
{
    std::string date;
    std::string name;
    int timeUsed;
};
#endif