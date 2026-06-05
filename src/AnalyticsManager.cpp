#include "../include/AnalyticsManager.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <iostream>
std::time_t AnalyticsManager::parseDateToTimeT(const std::string &dateStr)
{
    std::tm tm_info = {};
    std::stringstream ss(dateStr);
    int d, m, y;
    char sep1, sep2;
    if (ss >> d >> sep1 >> m >> sep2 >> y)
    {
        tm_info.tm_mday = d;
        tm_info.tm_mon = m - 1;
        tm_info.tm_year = y - 1900;
        tm_info.tm_hour = 0;
        tm_info.tm_min = 0;
        tm_info.tm_sec = 0;
        return std::mktime(&tm_info);
    }
    return 0;
}
void AnalyticsManager::calculateTimeBoundaries(std::time_t &todayStart, std::time_t &yesterdayStart,
                                               std::time_t &thisWeekStart, std::time_t &lastWeekStart,
                                               int &daysPassedThisWeek)
{
    std::time_t now = std::time(nullptr);
    std::tm *tm_now = std::localtime(&now);
    std::tm tm_today = *tm_now;
    tm_today.tm_hour = 0;
    tm_today.tm_min = 0;
    tm_today.tm_sec = 0;
    todayStart = std::mktime(&tm_today);
    yesterdayStart = todayStart - (24 * 60 * 60);
    int wday = tm_now->tm_wday;
    daysPassedThisWeek = (wday == 0) ? 6 : (wday - 1);
    thisWeekStart = todayStart - (daysPassedThisWeek * 24 * 60 * 60);
    lastWeekStart = thisWeekStart - (7 * 24 * 60 * 60);
}
std::map<std::string, std::vector<HistoryLog>> AnalyticsManager::groupLogsByDate(const std::vector<HistoryLog> &logs)
{
    std::map<std::string, std::vector<HistoryLog>> groupedData;
    for (const auto &log : logs)
    {
        groupedData[log.date].push_back(log);
    }
    return groupedData;
}
TimeStats AnalyticsManager::calculateTimeStatistics(const std::vector<HistoryLog> &logs)
{
    TimeStats stats;
    std::time_t todayStart, yesterdayStart, thisWeekStart, lastWeekStart;
    int daysPassedThisWeek;
    calculateTimeBoundaries(todayStart, yesterdayStart, thisWeekStart, lastWeekStart, daysPassedThisWeek);
    for (const auto &log : logs)
    {
        std::time_t logTime = parseDateToTimeT(log.date);
        if (logTime == todayStart)
            stats.totalToday += log.timeUsed;
        if (logTime == yesterdayStart)
            stats.totalYesterday += log.timeUsed;
        if (logTime >= thisWeekStart)
            stats.totalThisWeek += log.timeUsed;
        if (logTime >= lastWeekStart && logTime < thisWeekStart)
        {
            stats.totalLastWeek += log.timeUsed;
        }
    }
    if (daysPassedThisWeek > 0)
    {
        stats.avgThisWeek = static_cast<float>(stats.totalThisWeek) / daysPassedThisWeek;
    }
    stats.avgLastWeek = static_cast<float>(stats.totalLastWeek) / 7.0f;
    return stats;
}
std::vector<std::pair<std::string, int>> AnalyticsManager::getTop10Apps(const std::vector<HistoryLog> &filteredLogs)
{

    std::map<std::string, int> appUsage;
    for (const auto &log : filteredLogs)
    {
        appUsage[log.name] += log.timeUsed;
    }
    std::vector<std::pair<std::string, int>> sortedApps(appUsage.begin(), appUsage.end());
    std::sort(sortedApps.begin(), sortedApps.end(), [](const auto &a, const auto &b)
              { return a.second > b.second; });
    if (sortedApps.size() > 10)
    {
        sortedApps.resize(10);
    }
    return sortedApps;
}
std::vector<HistoryLog> AnalyticsManager::filterLogsByTime(const std::vector<HistoryLog> &allLogs, const std::string &title)
{
    std::vector<HistoryLog> filteredLogs;
    filteredLogs.reserve(allLogs.size());
    std::time_t todayStart, yesterdayStart, thisWeekStart, lastWeekStart;
    int daysPassedThisWeek;
    calculateTimeBoundaries(todayStart, yesterdayStart, thisWeekStart, lastWeekStart, daysPassedThisWeek);
    for (const auto &log : allLogs)
    {
        std::time_t logTime = parseDateToTimeT(log.date);
        bool keepLog = false;
        if (title == "Hom nay")
        {
            if (logTime == todayStart)
                keepLog = true;
        }
        else if (title == "Hom qua")
        {
            if (logTime == yesterdayStart)
                keepLog = true;
        }
        else if (title == "Tuan nay")
        {
            if (logTime >= thisWeekStart)
                keepLog = true;
        }
        else if (title == "Tuan truoc")
        {
            if (logTime >= lastWeekStart && logTime < thisWeekStart)
                keepLog = true;
        }
        if (keepLog)
        {
            filteredLogs.push_back(log);
        }
    }
    filteredLogs.shrink_to_fit();
    return filteredLogs;
}