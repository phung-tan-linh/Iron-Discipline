// [PLAN]: Triển khai Append-Only Log ghi nối file CSV (std::ios::app). Ẩn logic parse CSV vào anonymous namespace (SRP). Đảm bảo Thread-safe bằng std::mutex khi I/O và khi truy xuất TimePool.
#include "../include/DataStore.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <filesystem>

std::mutex UsageRepository::s_fileMutex;
std::mutex UsageRepository::s_poolMutex;
std::unordered_map<std::string, std::shared_ptr<TimePool>> UsageRepository::s_timePools;

namespace
{
    std::string trim(const std::string& str)
    {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (std::string::npos == first)
            return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, (last - first + 1));
    }

    std::vector<std::vector<std::string>> readCSV(const std::string& filename)
    {
        std::vector<std::vector<std::string>> data;
        std::ifstream file(filename);
        if (!file.is_open())
            return data;
        std::string line;
        while (std::getline(file, line))
        {
            std::vector<std::string> row;
            std::stringstream ss(line);
            std::string cell;
            while (std::getline(ss, cell, ','))
                row.push_back(trim(cell));
            if (!row.empty())
                data.push_back(row);
        }
        return data;
    }
}

std::string UsageRepository::getCurrentDateStr()
{
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << tm.tm_mday << "-"
        << std::setfill('0') << std::setw(2) << (tm.tm_mon + 1) << "-"
        << (tm.tm_year + 1900);
    return oss.str();
}

void UsageRepository::loadBasicList(const std::string& filename, std::vector<Category>& outList)
{
    auto csvData = readCSV(filename);
    int globalItemId = 1;
    for (const auto& row : csvData)
    {
        if (row.size() < 4)
            continue;
        std::string romanID = row[0], categoryTitle = row[1], itemName = row[3];
        bool foundCat = false;
        for (auto& cat : outList)
        {
            if (cat.romanID == romanID)
            {
                cat.items.push_back({globalItemId++, itemName});
                foundCat = true;
                break;
            }
        }
        if (!foundCat)
        {
            Category newCat;
            newCat.romanID = romanID;
            newCat.title = categoryTitle;
            newCat.items.push_back({globalItemId++, itemName});
            outList.push_back(std::move(newCat));
        }
    }
}

std::vector<ActiveLimit> UsageRepository::getActiveLimits()
{
    std::vector<ActiveLimit> limits;
    auto csvData = readCSV("active_limits.csv");
    for (const auto& row : csvData)
    {
        if (row.size() >= 3)
        {
            limits.emplace_back(std::stoi(row[0]), row[1], std::stoi(row[2]));
        }
    }
    return limits;
}

void UsageRepository::saveAllActiveLimits(const std::vector<ActiveLimit>& limits)
{
    std::lock_guard<std::mutex> lock(s_fileMutex);
    std::ofstream outFile("active_limits.csv", std::ios::trunc);
    if (outFile.is_open())
    {
        for (const auto& l : limits)
            outFile << l.type << "," << l.name << "," << l.timeLimit << "\n";
    }
}

void UsageRepository::addOrUpdateActiveLimit(const ActiveLimit& limit)
{
    auto limits = getActiveLimits();
    bool found = false;
    for (auto& l : limits)
    {
        if (l.name == limit.name)
        {
            l = limit;
            found = true;
            break;
        }
    }
    if (!found)
        limits.push_back(limit);
    saveAllActiveLimits(limits);
}

std::unordered_map<std::string, int> UsageRepository::loadDailyUsage()
{
    std::unordered_map<std::string, int> aggregatedUsage;
    std::string today = getCurrentDateStr();
    
    std::lock_guard<std::mutex> fileLock(s_fileMutex);
    auto csvData = readCSV("daily_usage.csv");

    bool isOldData = (!csvData.empty() && csvData[0].size() >= 3 && csvData[0][0] != today);

    if (isOldData)
    {
        std::ofstream outFile("daily_usage.csv", std::ios::trunc);
        std::lock_guard<std::mutex> poolLock(s_poolMutex);
        s_timePools.clear();
        return aggregatedUsage;
    }

    for (const auto& row : csvData)
    {
        if (row.size() >= 3)
        {
            std::string groupName = row[1];
            int addedSeconds = std::stoi(row[2]);
            aggregatedUsage[groupName] += addedSeconds;
        }
    }

    std::lock_guard<std::mutex> poolLock(s_poolMutex);
    s_timePools.clear();
    for (const auto& [name, totalSecs] : aggregatedUsage)
    {
        auto pool = std::make_shared<TimePool>();
        pool->timeUsedSeconds.store(totalSecs);
        s_timePools[name] = pool;
    }

    return aggregatedUsage;
}

void UsageRepository::appendUsageLog(const std::string& groupName, int addedSeconds)
{
    if (addedSeconds <= 0) return;

    std::string today = getCurrentDateStr();
    
    {
        std::lock_guard<std::mutex> lock(s_fileMutex);
        std::ofstream outFile("daily_usage.csv", std::ios::app);
        if (outFile.is_open())
        {
            outFile << today << "," << groupName << "," << addedSeconds << "\n";
        }
    }

    auto pool = getTimePool(groupName);
    pool->timeUsedSeconds.fetch_add(addedSeconds, std::memory_order_relaxed);
}

std::shared_ptr<TimePool> UsageRepository::getTimePool(const std::string& groupName)
{
    std::lock_guard<std::mutex> lock(s_poolMutex);
    auto it = s_timePools.find(groupName);
    if (it != s_timePools.end())
    {
        return it->second;
    }
    
    auto newPool = std::make_shared<TimePool>();
    s_timePools[groupName] = newPool;
    return newPool;
}