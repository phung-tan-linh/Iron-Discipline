// [PLAN]: Triển khai an toàn luồng (Thread-safe) cho RAM Buffer bằng std::mutex. Tách biệt hoàn toàn việc update số liệu (trên RAM) và việc đồng bộ file (xuống đĩa).
#include "../include/FileManager.h"
#include "../include/InputValidator.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <ctime>
#include <iomanip>
#include <algorithm>
#include <windows.h>

std::map<std::string, int> FileManager::g_dailyUsageCache;
std::mutex FileManager::g_usageMutex;

std::string FileManager::standardizeDate(const std::string &dateStr)
{
    std::string result = trim(dateStr);
    for (char &c : result)
        if (c == '/' || c == '.')
            c = '-';
    return result;
}

bool FileManager::isOlderThan14Days(const std::string &dateStr)
{
    std::string cleanDate = standardizeDate(dateStr);
    std::stringstream ss(cleanDate);
    int d = 0, m = 0, y = 0;
    char sep1, sep2;
    if (ss >> d >> sep1 >> m >> sep2 >> y)
    {
        std::tm tm_info = {};
        tm_info.tm_mday = d;
        tm_info.tm_mon = m - 1;
        tm_info.tm_year = (y < 100) ? (y + 100) : (y - 1900);
        std::time_t fileTime = std::mktime(&tm_info);
        std::time_t now = std::time(nullptr);
        if (fileTime == -1)
            return false;
        return std::difftime(now, fileTime) > (14 * 24 * 60 * 60);
    }
    return false;
}

std::string FileManager::trim(const std::string &str)
{
    size_t first = str.find_first_not_of(" \t\r\n");
    if (std::string::npos == first)
        return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

std::vector<std::vector<std::string>> FileManager::readCSV(const std::string &filename)
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
    file.close();
    return data;
}

void FileManager::loadBasicList(const std::string &filename, std::vector<Category> &outList)
{
    auto csvData = readCSV("basic_list.csv");
    int globalItemId = 1;
    for (const auto &row : csvData)
    {
        if (row.size() < 4)
            continue;
        std::string romanID = row[0], categoryTitle = row[1], itemName = row[3];
        bool foundCat = false;
        for (auto &cat : outList)
            if (cat.romanID == romanID)
            {
                cat.items.push_back({globalItemId++, itemName});
                foundCat = true;
                break;
            }
        if (!foundCat)
        {
            Category newCat;
            newCat.romanID = romanID;
            newCat.title = categoryTitle;
            newCat.items.push_back({globalItemId++, itemName});
            outList.push_back(newCat);
        }
    }
}

std::vector<ActiveLimit> FileManager::getActiveLimits()
{
    std::vector<ActiveLimit> limits;
    auto csvData = readCSV("active_limits.csv");
    bool needsCleanup = false;
    for (const auto &row : csvData)
        if (row.size() >= 4)
        {
            std::string date = standardizeDate(row[0]);
            if (isOlderThan14Days(date))
                needsCleanup = true;
            else
                limits.push_back({date, std::stoi(row[1]), row[2], std::stoi(row[3])});
        }
    if (needsCleanup)
        saveAllActiveLimits(limits);
    return limits;
}

void FileManager::saveAllActiveLimits(const std::vector<ActiveLimit> &limits)
{
    std::ofstream outFile("active_limits.csv", std::ios::trunc);
    if (outFile.is_open())
    {
        for (const auto &l : limits)
            outFile << standardizeDate(l.date) << "," << l.type << "," << l.name << "," << l.timeLimit << "\n";
        outFile.close();
    }
}

void FileManager::addOrUpdateActiveLimit(const ActiveLimit &limit)
{
    auto limits = getActiveLimits();
    bool found = false;
    for (auto &l : limits)
        if (l.name == limit.name)
        {
            l = limit;
            found = true;
            break;
        }
    if (!found)
        limits.push_back(limit);
    saveAllActiveLimits(limits);
}

std::string FileManager::getCurrentDateStr()
{
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << tm.tm_mday << "-" 
        << std::setfill('0') << std::setw(2) << (tm.tm_mon + 1) << "-" 
        << (tm.tm_year + 1900);
    return oss.str();
}

std::map<std::string, int> FileManager::loadDailyUsage()
{
    std::map<std::string, int> m;
    auto d = readCSV("daily_usage.csv");
    std::string t = getCurrentDateStr();
    
    if (!d.empty() && d[0].size() >= 3 && d[0][0] != t)
    {
        std::ofstream u("daily_usage.csv", std::ios::trunc);
        if (u.is_open()) {
            u.close();
        }
        std::lock_guard<std::mutex> lock(g_usageMutex);
        g_dailyUsageCache.clear();
        return m;
    }
    
    for (auto &r : d)
    {
        if (r.size() >= 3)
            m[r[1]] = std::stoi(r[2]);
    }
            
    std::lock_guard<std::mutex> lock(g_usageMutex);
    g_dailyUsageCache = m;
    return m;
}

void FileManager::saveAllDailyUsage(const std::map<std::string, int> &usageCache)
{
    if (usageCache.empty())
        return;
    std::string t = getCurrentDateStr();
    std::ofstream f("daily_usage_temp.csv", std::ios::trunc);
    if (f.is_open())
    {
        for (const auto &p : usageCache)
            f << t << "," << p.first << "," << p.second << "\n";
        f.close();
        remove("daily_usage.csv");
        rename("daily_usage_temp.csv", "daily_usage.csv");
    }
}

void FileManager::updateDailyUsageItem(const std::string& id, int seconds)
{
    std::lock_guard<std::mutex> lock(g_usageMutex);
    g_dailyUsageCache[id] = seconds;
}

void FileManager::syncDailyUsageToFile()
{
    std::map<std::string, int> cacheCopy;
    {
        std::lock_guard<std::mutex> lock(g_usageMutex);
        cacheCopy = g_dailyUsageCache;
    }
    saveAllDailyUsage(cacheCopy);
}