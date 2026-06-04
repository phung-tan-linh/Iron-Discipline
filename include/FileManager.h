#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H
#include "Models.h"
#include <vector>
#include <string>
#include <map>
class FileManager {
private:
    static bool isOlderThan14Days(const std::string &dateStr);
    static std::string trim(const std::string &str);
    static std::vector<std::vector<std::string>> readCSV(const std::string &filename);
    static std::string standardizeDate(const std::string &dateStr);
public:
    static void loadBasicList(const std::string &filename, std::vector<Category> &outList);
    static std::vector<HistoryLog> readHistoryLog();
    static std::vector<ActiveLimit> getActiveLimits();
    static void saveAllActiveLimits(const std::vector<ActiveLimit> &limits);
    static void addOrUpdateActiveLimit(const ActiveLimit &limit);
    static std::string getCurrentDateStr();
    static std::map<std::string, int> loadDailyUsage();
    static void saveAllDailyUsage(const std::map<std::string, int>& usageMap);
    static void migrateDailyToHistory(const std::string&,const std::map<std::string,int>&);
    static void clearDailyUsage();
};
#endif