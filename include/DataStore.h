// [PLAN]: Gộp Models và FileManager thành DataStore. Áp dụng Append-Only Log để chống mất dữ liệu khi crash.
// Chuyển giao logic xử lý ID và danh sách từ ConsoleMenu sang UsageRepository (SRP).
// Thêm các hàm addLimitsByBasicIds và removeLimitsByIds để ConsoleMenu chỉ lo UI.
#ifndef DATASTORE_H
#define DATASTORE_H

#include <string>
#include <vector>
#include <unordered_map>
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

class UsageRepository
{
private:
    static std::mutex s_fileMutex;

public:
    static std::string getCurrentDateStr();

    static void loadBasicList(const std::string& filename, std::vector<Category>& outList);
    
    static std::vector<ActiveLimit> getActiveLimits();
    static void saveAllActiveLimits(const std::vector<ActiveLimit>& limits);
    static void addOrUpdateActiveLimit(const ActiveLimit& limit);

    static void addLimitsByBasicIds(const std::vector<int>& ids, const std::vector<Category>& basicList, int timeMins);
    static void removeLimitsByIds(const std::vector<int>& idsToRemove);

    static std::unordered_map<std::string, int> loadDailyUsage();
    static void appendUsageLog(const std::string& appName, int addedSeconds);
};

#endif