#ifndef DATASTORE_H
#define DATASTORE_H

#include "Models.h"
#include <string>
#include <vector>
#include <unordered_map>

class UsageRepository
{
private:
    static std::vector<Category> s_cachedBasicList;

public:
    static std::string getCurrentDateStr();

    static void loadBasicList(const std::string& filename);
    static const std::vector<Category>& getCachedBasicList();
    static std::vector<int> getAllBasicItemIds();
    
    static std::vector<ActiveLimit> getActiveLimits();
    static bool saveAllActiveLimits(const std::vector<ActiveLimit>& limits);
    static void addOrUpdateActiveLimit(const ActiveLimit& limit);

    static void addLimitsByBasicIds(const std::vector<int>& ids, int timeMins);
    
    static std::vector<DisplayLimit> getSortedDisplayLimits();
    static bool removeLimitsByDisplayIds(const std::vector<int>& displayIds, const std::vector<DisplayLimit>& currentDisplayList);

    static std::unordered_map<std::string, int> loadDailyUsage();
    static void appendUsageLog(const std::string& appName, int addedSeconds);
};

#endif