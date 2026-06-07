// [PLAN]: Gộp Models và FileManager thành DataStore. Áp dụng Append-Only Log để chống mất dữ liệu khi crash. Quản lý thời gian bằng Shared Time Pool (std::atomic) kết hợp std::shared_ptr để tra cứu O(1) và an toàn đa luồng.
#ifndef DATASTORE_H
#define DATASTORE_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <atomic>
#include <mutex>

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

struct TimePool
{
    std::atomic<int> timeUsedSeconds{0};
};

class UsageRepository
{
private:
    static std::mutex s_fileMutex;
    static std::mutex s_poolMutex;
    static std::unordered_map<std::string, std::shared_ptr<TimePool>> s_timePools;

public:
    static std::string getCurrentDateStr();

    static void loadBasicList(const std::string& filename, std::vector<Category>& outList);
    
    static std::vector<ActiveLimit> getActiveLimits();
    static void saveAllActiveLimits(const std::vector<ActiveLimit>& limits);
    static void addOrUpdateActiveLimit(const ActiveLimit& limit);

    static std::unordered_map<std::string, int> loadDailyUsage();
    static void appendUsageLog(const std::string& groupName, int addedSeconds);
    
    static std::shared_ptr<TimePool> getTimePool(const std::string& groupName);
};

#endif