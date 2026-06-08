// [PLAN]: Triển khai Append-Only Log ghi nối file CSV (std::ios::app). Ẩn logic parse CSV vào anonymous namespace (SRP).
// Bổ sung addLimitsByBasicIds và removeLimitsByIds để xử lý logic tìm kiếm/xóa limit theo ID hiển thị.
// Đảm bảo Thread-safe bằng std::mutex khi I/O. Đã loại bỏ hoàn toàn TimePool theo yêu cầu.
#include "../include/DataStore.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <filesystem>
#include <algorithm>

std::mutex UsageRepository::s_fileMutex;

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

void UsageRepository::addLimitsByBasicIds(const std::vector<int>& ids, const std::vector<Category>& basicList, int timeMins)
{
    if (timeMins <= 0) return;
    for (int id : ids)
    {
        for (const auto& cat : basicList)
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

void UsageRepository::removeLimitsByIds(const std::vector<int>& idsToRemove)
{
    auto allLimits = getActiveLimits();
    std::vector<ActiveLimit> basicLimits, customLimits;
    for (const auto& limit : allLimits)
    {
        if (limit.type == 1)
            basicLimits.push_back(limit);
        else
            customLimits.push_back(limit);
    }
    
    auto sortAlpha = [](const ActiveLimit& a, const ActiveLimit& b)
    { return a.name < b.name; };
    
    std::sort(basicLimits.begin(), basicLimits.end(), sortAlpha);
    std::sort(customLimits.begin(), customLimits.end(), sortAlpha);
    
    std::vector<ActiveLimit> sortedLimits;
    sortedLimits.insert(sortedLimits.end(), basicLimits.begin(), basicLimits.end());
    sortedLimits.insert(sortedLimits.end(), customLimits.begin(), customLimits.end());
    
    std::vector<ActiveLimit> limitsToKeep;
    for (size_t i = 0; i < sortedLimits.size(); ++i)
    {
        int displayId = static_cast<int>(i) + 1;
        if (std::find(idsToRemove.begin(), idsToRemove.end(), displayId) == idsToRemove.end())
        {
            limitsToKeep.push_back(sortedLimits[i]);
        }
    }
    
    saveAllActiveLimits(limitsToKeep);
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