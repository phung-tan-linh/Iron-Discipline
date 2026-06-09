#ifndef FILEIO_H
#define FILEIO_H

#include <string>
#include <vector>

class CsvEngine
{
public:
    static std::vector<std::vector<std::string>> readCSV(const std::string& filename);
    static bool writeLines(const std::string& filename, const std::vector<std::string>& lines, bool append = false);
    static bool appendLine(const std::string& filename, const std::string& line);
    static bool clearFile(const std::string& filename);
    static void ensureFilesExist(const std::vector<std::string>& filenames);
};

#endif