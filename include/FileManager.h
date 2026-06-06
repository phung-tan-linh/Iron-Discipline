// [PLAN]: Tối ưu I/O File bằng kỹ thuật Caching trên RAM (RAM buffer), loại bỏ hoàn toàn các logic liên quan đến Analytics và History Log.
#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H
#include "Models.h"
#include <vector>
#include <string>
#include <map>
#include <thread>
#include <atomic>
#include <mutex>

class FileManager {
private:
    static bool isOlderThan14Days(const std::string &dateStr);
    static std::string trim(const std::string &str);
    static std::vector<std::vector<std::string>> readCSV(const std::string &filename);
    static std::string standardizeDate(const std::string &dateStr);
    
public:
    static std::map<std::string, int> g_dailyUsageCache;
    static std::mutex g_usageMutex;

    static void loadBasicList(const std::string &filename, std::vector<Category> &outList);
    static std::vector<ActiveLimit> getActiveLimits();
    static void saveAllActiveLimits(const std::vector<ActiveLimit> &limits);
    static void addOrUpdateActiveLimit(const ActiveLimit &limit);
    static std::string getCurrentDateStr();
    static std::map<std::string, int> loadDailyUsage();
    static void saveAllDailyUsage(const std::map<std::string, int>& usageCache);
    
    static void updateDailyUsageItem(const std::string& id, int seconds);
    static void syncDailyUsageToFile();
};
#endif