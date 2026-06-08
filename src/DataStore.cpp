#include "../include/DataStore.h"
#include "../include/FileIO.h"
#include <sstream>
#include <iomanip>
#include <ctime>
#include <algorithm>
#include <unordered_set>

std::vector<Category> UsageRepository::s_cachedBasicList;

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
    auto csvData = CsvEngine::readCSV(filename);
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
    std::vector<ActiveLimit> limits;
    auto csvData = CsvEngine::readCSV("active_limits.csv");
    
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
    std::vector<std::string> lines;
    lines.reserve(limits.size());
    
    for (const auto& l : limits)
    {
        lines.push_back(std::to_string(l.type) + "," + l.name + "," + std::to_string(l.timeLimit));
    }
    
    return CsvEngine::writeLines("active_limits.csv", lines, false);
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
    {
        limits.push_back(limit);
    }
        
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
    { 
        return a.name < b.name; 
    };
    
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
    
    auto csvData = CsvEngine::readCSV("daily_usage.csv");

    bool isOldData = (!csvData.empty() && csvData[0].size() >= 3 && csvData[0][0] != today);

    if (isOldData)
    {
        CsvEngine::clearFile("daily_usage.csv");
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
    std::string line = today + "," + appName + "," + std::to_string(addedSeconds);
    
    CsvEngine::appendLine("daily_usage.csv", line);
}