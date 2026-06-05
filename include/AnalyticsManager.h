#ifndef ANALYTICS_MANAGER_H
#define ANALYTICS_MANAGER_H
#include "Models.h"
#include <vector>
#include <string>
#include <map>
#include <ctime>
struct TimeStats
{
    int totalToday = 0;
    int totalYesterday = 0;
    int totalThisWeek = 0;
    int totalLastWeek = 0;
    float avgThisWeek = 0.0f;
    float avgLastWeek = 0.0f;
};
class AnalyticsManager
{
private:
    static std::time_t parseDateToTimeT(const std::string &dateStr);
    static void calculateTimeBoundaries(std::time_t &todayStart, std::time_t &yesterdayStart,
                                        std::time_t &thisWeekStart, std::time_t &lastWeekStart,
                                        int &daysPassedThisWeek);

public:
    static std::map<std::string, std::vector<HistoryLog>> groupLogsByDate(const std::vector<HistoryLog> &logs);
    static TimeStats calculateTimeStatistics(const std::vector<HistoryLog> &logs);
    static std::vector<std::pair<std::string, int>> getTop10Apps(const std::vector<HistoryLog> &filteredLogs);
    static std::vector<HistoryLog> filterLogsByTime(const std::vector<HistoryLog> &allLogs, const std::string &title);
};
#endif