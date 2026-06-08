#include "../include/FileIO.h"
#include <fstream>
#include <sstream>
#include <mutex>

// Quản lý Thread-safe nội bộ cho các thao tác I/O ổ cứng
static std::recursive_mutex s_fileMutex;

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
}

std::vector<std::vector<std::string>> CsvEngine::readCSV(const std::string& filename)
{
    std::lock_guard<std::recursive_mutex> lock(s_fileMutex);
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
        {
            row.push_back(trim(cell));
        }
        if (!row.empty())
        {
            data.push_back(row);
        }
    }
    return data;
}

bool CsvEngine::writeLines(const std::string& filename, const std::vector<std::string>& lines, bool append)
{
    std::lock_guard<std::recursive_mutex> lock(s_fileMutex);
    std::ios_base::openmode mode = append ? std::ios::app : std::ios::trunc;
    std::ofstream file(filename, mode);
    
    if (!file.is_open())
        return false;
        
    for (const auto& line : lines)
    {
        file << line << "\n";
    }
    return true;
}

bool CsvEngine::appendLine(const std::string& filename, const std::string& line)
{
    return writeLines(filename, {line}, true);
}

bool CsvEngine::clearFile(const std::string& filename)
{
    std::lock_guard<std::recursive_mutex> lock(s_fileMutex);
    std::ofstream file(filename, std::ios::trunc);
    return file.is_open();
}