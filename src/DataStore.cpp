// [PLAN]: Triển khai Append-Only Log ghi nối file CSV (std::ios::app). Ẩn logic parse CSV vào anonymous namespace (SRP).
// Quản lý s_cachedBasicList nội bộ. Cung cấp các API getCachedBasicList, getAllBasicItemIds, addLimitsByBasicIds độc lập.
// Đảm bảo Thread-safe bằng std::mutex khi I/O và kiểm soát ngoại lệ trả về bool khi thao tác file.
#include "../include/DataStore.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <filesystem>
#include <algorithm>

std::mutex UsageRepository::s_fileMutex;
std::vector<Category> UsageRepository::s_cachedBasicList;

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

void UsageRepository::loadBasicList(const std::string& filename)
{
    s_cachedBasicList.clear();
    auto csvData = readCSV(filename);
    int globalItemId = 1;
    for (const auto& row : csvData)
    {
        if (row.size() < 4)
            continue;
        std::string romanID = row[0], categoryTitle = row[1], itemName = row[3];
        bool foundCat = false;
        for (auto& cat : s_cachedBasicList)
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
            s_cachedBasicList.push_back(std::move(newCat));
        }
    }
}

const std::vector<Category>& UsageRepository::getCachedBasicList()
{
    return s_cachedBasicList;
}

std::vector<int> UsageRepository::getAllBasicItemIds()
{
    std::vector<int> ids;
    for (const auto& cat : s_cachedBasicList)
    {
        for (const auto& item : cat.items)
        {
            ids.push_back(item.id);
        }
    }
    return ids;
}

std::vector<ActiveLimit> UsageRepository::getActiveLimits()
{
    std::lock_guard<std::mutex> lock(s_fileMutex);
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

bool UsageRepository::saveAllActiveLimits(const std::vector<ActiveLimit>& limits)
{
    std::lock_guard<std::mutex> lock(s_fileMutex);
    std::ofstream outFile("active_limits.csv", std::ios::trunc);
    if (outFile.is_open())
    {
        for (const auto& l : limits)
            outFile << l.type << "," << l.name << "," << l.timeLimit << "\n";
        return true;
    }
    return false;
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

void UsageRepository::addLimitsByBasicIds(const std::vector<int>& ids, int timeMins)
{
    if (timeMins <= 0) return;
    for (int id : ids)
    {
        for (const auto& cat : s_cachedBasicList)
        {
            for (const auto& item : cat.items)
            {
                if (item.id == id)
                {
                    addOrUpdateActiveLimit(ActiveLimit(1, item.name, timeMins));
                }
            }
        }
    }
}

std::vector<DisplayLimit> UsageRepository::getSortedDisplayLimits()
{
    auto allLimits = getActiveLimits();
    
    auto sortAlpha = [](const ActiveLimit& a, const ActiveLimit& b)
    { return a.name < b.name; };
    
    std::sort(allLimits.begin(), allLimits.end(), sortAlpha);
    
    std::vector<DisplayLimit> result;
    int displayId = 1;
    
    for (const auto& limit : allLimits)
    {
        std::string typeStr = (limit.type == 1) ? "Co ban" : "Tuy chon";
        result.push_back({displayId++, typeStr, limit.name, limit.timeLimit});
    }
    
    return result;
}

bool UsageRepository::removeLimitsByDisplayIds(const std::vector<int>& displayIds, const std::vector<DisplayLimit>& currentDisplayList)
{
    std::unordered_set<std::string> namesToRemove;
    for (int id : displayIds)
    {
        int index = id - 1;
        if (index >= 0 && index < static_cast<int>(currentDisplayList.size()))
        {
            namesToRemove.insert(currentDisplayList[index].name);
        }
    }

    auto allLimits = getActiveLimits();
    std::vector<ActiveLimit> limitsToKeep;
    for (const auto& limit : allLimits)
    {
        if (namesToRemove.find(limit.name) == namesToRemove.end())
        {
            limitsToKeep.push_back(limit);
        }
    }

    return saveAllActiveLimits(limitsToKeep);
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
        return aggregatedUsage;
    }

    for (const auto& row : csvData)
    {
        if (row.size() >= 3)
        {
            std::string appName = row[1];
            int addedSeconds = std::stoi(row[2]);
            aggregatedUsage[appName] += addedSeconds;
        }
    }

    return aggregatedUsage;
}

void UsageRepository::appendUsageLog(const std::string& appName, int addedSeconds)
{
    if (addedSeconds <= 0) return;

    std::string today = getCurrentDateStr();
    
    std::lock_guard<std::mutex> lock(s_fileMutex);
    std::ofstream outFile("daily_usage.csv", std::ios::app);
    if (outFile.is_open())
    {
        outFile << today << "," << appName << "," << addedSeconds << "\n";
    }
}