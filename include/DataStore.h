// [PLAN]: Gộp Models và FileManager thành DataStore. Áp dụng Append-Only Log để chống mất dữ liệu khi crash.
// Chuyển giao toàn bộ logic xử lý ID, sắp xếp mảng và trích xuất dữ liệu từ ConsoleMenu sang UsageRepository (SRP).
// Thêm s_cachedBasicList để DataStore tự quản lý dữ liệu gốc, ConsoleMenu chỉ đóng vai trò View.
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

struct DisplayLimit
{
    int displayId;
    std::string type;
    std::string name;
    int limitMinutes;
};

class UsageRepository
{
private:
    static std::mutex s_fileMutex;
    static std::vector<Category> s_cachedBasicList;

public:
    static std::string getCurrentDateStr();

    static void loadBasicList(const std::string& filename);
    static const std::vector<Category>& getCachedBasicList();
    static std::vector<int> getAllBasicItemIds();
    
    static std::vector<ActiveLimit> getActiveLimits();
    static bool saveAllActiveLimits(const std::vector<ActiveLimit>& limits);
    static void addOrUpdateActiveLimit(const ActiveLimit& limit);

    static void addLimitsByBasicIds(const std::vector<int>& ids, int timeMins);
    
    static std::vector<DisplayLimit> getSortedDisplayLimits();
    static bool removeLimitsByDisplayIds(const std::vector<int>& displayIds, const std::vector<DisplayLimit>& currentDisplayList);

    static std::unordered_map<std::string, int> loadDailyUsage();
    static void appendUsageLog(const std::string& appName, int addedSeconds);
};

#endif