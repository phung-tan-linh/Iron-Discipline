// [PLAN]: Triển khai UI Console mới không có View Activity, giữ nguyên logic thêm/xóa giới hạn, tránh gọi AnalyticsManager.
#include "../include/ConsoleMenu.h"
#include "../include/FileManager.h"
#include "../include/InputValidator.h"
#include "../include/ProcessManager.h"
#include <iostream>
#include <iomanip>
#include <map>
#include <algorithm>

extern ProcessManager g_ProcMgr;

ConsoleMenu::ConsoleMenu() : currentState(AppState::MAIN_MENU) {}

void ConsoleMenu::init(const std::string &basicListFile) { FileManager::loadBasicList(basicListFile, basicList); }

void ConsoleMenu::clearScreen() { system("cls"); }

void ConsoleMenu::safeWait()
{
    std::cout << "(An Enter de tiep tuc...)\n";
    std::string temp;
    std::getline(std::cin, temp);
}

void ConsoleMenu::handleInvalidState()
{
    InputValidator::clearInputState();
    currentState = AppState::MAIN_MENU;
}

std::string ConsoleMenu::getCurrentDate()
{
    std::time_t t = std::time(nullptr);
    std::tm *now = std::localtime(&t);
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(2) << now->tm_mday << "-" << std::setfill('0') << std::setw(2) << (now->tm_mon + 1) << "-" << (now->tm_year + 1900);
    return ss.str();
}

std::string ConsoleMenu::formatTime(int minutes)
{
    if (minutes > 60)
        return std::to_string(minutes / 60) + " gio " + std::to_string(minutes % 60) + " phut";
    return std::to_string(minutes) + " phut";
}

void ConsoleMenu::printBasicList()
{
    std::cout << "Duoi day la toan bo danh sach co ban:\n";
    for (const auto &cat : basicList)
    {
        std::cout << cat.romanID << ". " << cat.title << ": \n";
        for (const auto &item : cat.items)
            std::cout << item.id << ". " << item.name << ".\n";
        std::cout << "\n";
    }
}

void ConsoleMenu::showMainMenu()
{
    clearScreen();
    std::string choice;
    std::cout << "Ki luat thep\n1. Gioi han ung dung.\n2. Thoat.\nChon? [1/2] : ";
    std::getline(std::cin, choice);
    if (choice == "1")
        currentState = AppState::LIMIT_APP;
    else if (choice == "2")
        currentState = AppState::EXIT;
    else
        handleInvalidState();
}

void ConsoleMenu::showAddBasicLimit()
{
    while (true)
    {
        clearScreen();
        printBasicList();
        std::cout << "\n - Muon them gioi han app/web nao thi liet ke cac so thu tu cua app/web do cach nhau boi dau cach.\n - Muon them gioi han tat ca thi an 'a'.\n - Muon quay lai thi an 'n'.\n-> ";
        
        std::vector<int> selectedIds;
        int failCountInput = 0;
        bool validInput = false, isAll = false;
        
        while (failCountInput < 3)
        {
            std::string input;
            std::getline(std::cin, input);
            
            if (input == "n" || input == "N")
            {
                currentState = AppState::LIMIT_APP;
                return;
            }
            if (input == "a" || input == "A")
            {
                for (const auto &cat : basicList)
                    for (const auto &item : cat.items)
                        selectedIds.push_back(item.id);
                validInput = isAll = true;
                break;
            }
            
            selectedIds = InputValidator::parseSpaceSeparatedIntegers(input);
            if (selectedIds.empty())
            {
                failCountInput++;
                std::cout << "Du lieu nhap vao khong hop le. Yeu cau nhap lai:\n-> ";
                continue;
            }
            
            std::vector<int> validIds;
            for (const auto &cat : basicList)
                for (const auto &item : cat.items)
                    validIds.push_back(item.id);
                    
            bool allValid = true;
            for (int id : selectedIds)
                if (std::find(validIds.begin(), validIds.end(), id) == validIds.end())
                {
                    allValid = false;
                    break;
                }
                
            if (!allValid)
            {
                failCountInput++;
                std::cout << "Du lieu nhap vao khong hop le. Yeu cau nhap lai:\n-> ";
                selectedIds.clear();
            }
            else
            {
                validInput = true;
                break;
            }
        }
        
        if (!validInput)
        {
            currentState = AppState::MAIN_MENU;
            return;
        }
        
        std::string targetStr = isAll ? "tat ca app/web" : (selectedIds.size() == 1 ? "app/web da chon" : "cac app/web da chon");
        std::cout << "\nXac dinh thoi gian su dung toi da cho " << targetStr << ":\n1. 2 gio.\n2. 1 gio.\n3. 30 phut.\nChon? [1/2/3] : ";
        int timeMins = 0;
        std::string level;
        std::getline(std::cin, level);
        
        if (level == "1")
            timeMins = 120;
        else if (level == "2")
            timeMins = 60;
        else if (level == "3")
            timeMins = 30;
        else
        {
            currentState = AppState::MAIN_MENU;
            return;
        }
        
        std::cout << "\nXac nhan them gioi han " << targetStr << "\nVoi thoi gian su dung toi da la " << (timeMins / 60) << " gio " << (timeMins % 60) << " phut:\nChon? [y/n] : ";
        std::string conf;
        std::getline(std::cin, conf);
        
        if (conf == "y" || conf == "Y")
        {
            for (int id : selectedIds)
                for (const auto &cat : basicList)
                    for (const auto &item : cat.items)
                        if (item.id == id)
                        {
                            FileManager::addOrUpdateActiveLimit({1, item.name, timeMins});
                            g_ProcMgr.reloadActiveLimits();
                        }
            std::cout << "\nDa them gioi han thanh cong " << targetStr << ".\n";
            safeWait();
            currentState = AppState::LIMIT_APP;
            return;
        }
        else if (conf == "n" || conf == "N")
            continue;
        else
        {
            currentState = AppState::MAIN_MENU;
            return;
        }
    }
}

void ConsoleMenu::handleCustomOuterList()
{
    while (true)
    {
        clearScreen();
        std::cout << " - Muon them gioi han app/web ngoai danh sach co ban thi\nLiet ke cac app(.exe)/url/ten mien cach nhau boi dau cach.\nVi du: osu!.exe https://github.com/phung-tan-linh facebook.com\nAn 'n' de quay lai.\n-> ";
        std::string inputStr;
        std::getline(std::cin, inputStr);
        
        if (inputStr == "n" || inputStr == "N")
            return;
            
        std::vector<std::string> customNames;
        std::stringstream ss(inputStr);
        std::string temp;
        while (ss >> temp)
            customNames.push_back(temp);
            
        if (customNames.empty())
        {
            handleInvalidState();
            return;
        }
        
        std::string targetStr = (customNames.size() == 1) ? "app/web da chon" : "cac app/web da chon";
        int timeMins;
        if (!InputValidator::getValidTimeInput(timeMins, "\nXac dinh thoi gian su dung toi da cho " + targetStr + ":\n"))
        {
            currentState = AppState::MAIN_MENU;
            return;
        }
        
        std::cout << "\nXac nhan them " << targetStr << "\nVoi thoi gian su dung toi da la " << (timeMins / 60) << " gio " << (timeMins % 60) << " phut:\nChon? [y/n] : ";
        std::string conf;
        std::getline(std::cin, conf);
        
        if (conf == "y" || conf == "Y")
        {
            for (const auto &name : customNames)
            {
                FileManager::addOrUpdateActiveLimit({2, name, timeMins});
                g_ProcMgr.reloadActiveLimits();
            }
            std::cout << "\nDa them gioi han thanh cong " << targetStr << ".\n";
            safeWait();
            currentState = AppState::LIMIT_APP;
            return;
        }
        else if (conf == "n" || conf == "N")
            continue;
        else
        {
            currentState = AppState::MAIN_MENU;
            return;
        }
    }
}

void ConsoleMenu::handleCustomBasicList()
{
    while (true)
    {
        clearScreen();
        printBasicList();
        std::cout << " - Muon them gioi han app/web nao thi liet ke cac so thu tu cua app/web do cach nhau boi dau cach.\n - Muon them gioi han tat ca thi an 'a'.\n - Muon quay lai menu Gioi han ung dung thi an 'n'.\n-> ";
        
        int failCountInput = 0;
        std::vector<int> selectedIds;
        bool isAll = false, validSelection = false;
        
        while (failCountInput < 3)
        {
            std::string input;
            std::getline(std::cin, input);
            
            if (input == "n" || input == "N")
                return;
            if (input == "a" || input == "A")
            {
                for (const auto &cat : basicList)
                    for (const auto &item : cat.items)
                        selectedIds.push_back(item.id);
                isAll = validSelection = true;
                break;
            }
            
            selectedIds = InputValidator::parseSpaceSeparatedIntegers(input);
            if (selectedIds.empty())
            {
                failCountInput++;
                std::cout << "Du lieu nhong hop le. Yeu cau nhap lai:\n-> ";
                continue;
            }
            
            std::vector<int> validIds;
            for (const auto &cat : basicList)
                for (const auto &item : cat.items)
                    validIds.push_back(item.id);
                    
            bool allValid = true;
            for (int id : selectedIds)
                if (std::find(validIds.begin(), validIds.end(), id) == validIds.end())
                {
                    allValid = false;
                    break;
                }
                
            if (!allValid)
            {
                failCountInput++;
                std::cout << "Du lieu nhong hop le. Yeu cau nhap lai:\n-> ";
                selectedIds.clear();
            }
            else
            {
                validSelection = true;
                break;
            }
        }
        
        if (!validSelection)
        {
            currentState = AppState::MAIN_MENU;
            return;
        }
        
        std::string targetStr = isAll ? "tat ca app/web" : (selectedIds.size() == 1 ? "app/web da chon" : "cac app/web da chon");
        int timeMins = 0;
        if (!InputValidator::getValidTimeInput(timeMins, "\nXac dinh thoi gian su dung toi da cho " + targetStr + ":\n"))
        {
            currentState = AppState::MAIN_MENU;
            return;
        }
        
        std::cout << "\nXac nhan them " << targetStr << "\nVoi thoi gian su dung toi da la " << (timeMins / 60) << " gio " << (timeMins % 60) << " phut:\nChon? [y/n] : ";
        std::string conf;
        std::getline(std::cin, conf);
        
        if (conf == "y" || conf == "Y")
        {
            for (int id : selectedIds)
                for (const auto &cat : basicList)
                    for (const auto &item : cat.items)
                        if (item.id == id)
                        {
                            FileManager::addOrUpdateActiveLimit({1, item.name, timeMins});
                            g_ProcMgr.reloadActiveLimits();
                        }
                        
            std::cout << "\nDa them gioi han thanh cong " << targetStr << ".\n - Muon them gioi han app/web ngoai danh sach thi an 'y'.\nMuon quay lai menu Gioi han ung dung thi an 'n'.\nChon? [y/n] : ";
            std::string afterConf;
            std::getline(std::cin, afterConf);
            
            if (afterConf == "y" || afterConf == "Y")
            {
                handleCustomOuterList();
                return;
            }
            else if (afterConf == "n" || afterConf == "N")
            {
                currentState = AppState::LIMIT_APP;
                return;
            }
            else
            {
                currentState = AppState::MAIN_MENU;
                return;
            }
        }
        else if (conf == "n" || conf == "N")
            continue;
        else
        {
            currentState = AppState::MAIN_MENU;
            return;
        }
    }
}

void ConsoleMenu::showAddCustomLimit()
{
    clearScreen();
    std::cout << " - Muon them gioi han cac app/web co trong danh sach co ban thi an 'y'.\n - Muon bo qua de them gioi han app/web ngoai danh sach thi an 's'.\n - Muon quay lai thi an 'n'.\nChon? [y/s/n] : ";
    std::string choice;
    std::getline(std::cin, choice);
    
    if (choice == "n" || choice == "N")
        currentState = AppState::LIMIT_APP;
    else if (choice == "s" || choice == "S")
        handleCustomOuterList();
    else if (choice == "y" || choice == "Y")
        handleCustomBasicList();
    else
        handleInvalidState();
}

void ConsoleMenu::handleRemoveLimitLogic()
{
    while (true)
    {
        auto allLimits = FileManager::getActiveLimits();
        std::vector<ActiveLimit> basicLimits, customLimits;
        for (const auto &limit : allLimits)
            if (limit.type == 1)
                basicLimits.push_back(limit);
            else
                customLimits.push_back(limit);
                
        auto sortAlpha = [](const ActiveLimit &a, const ActiveLimit &b)
        { return a.name < b.name; };
        
        std::sort(basicLimits.begin(), basicLimits.end(), sortAlpha);
        std::sort(customLimits.begin(), customLimits.end(), sortAlpha);
        
        clearScreen();
        std::cout << "Duoi day la toan bo app/web dang bi gioi han:\nI. Danh sach co ban:\n";
        std::map<int, ActiveLimit> displayMap;
        int displayId = 1;
        for (const auto &limit : basicLimits)
        {
            std::cout << displayId << ". " << limit.name << " : " << formatTime(limit.timeLimit) << "\n";
            displayMap[displayId++] = limit;
        }
        
        std::cout << "\n\nII. Danh sach ngoai:\n";
        for (const auto &limit : customLimits)
        {
            std::cout << displayId << ". " << limit.name << " : " << formatTime(limit.timeLimit) << "\n";
            displayMap[displayId++] = limit;
        }
        
        std::cout << "\n - Muon bo gioi han app/web nao thi liet ke cac so thu tu cua app/web do cach nhau boi dau cach.\n - Muon bo gioi han tat ca thi an 'a'.\n- Muon quay lai thi an 'n'.\n-> ";
        
        int failCountRemove = 0;
        std::vector<int> idsToRemove;
        bool validRemove = false, isAllRemove = false;
        
        while (failCountRemove < 3)
        {
            std::string input;
            std::getline(std::cin, input);
            
            if (input == "n" || input == "N")
            {
                currentState = AppState::LIMIT_APP;
                return;
            }
            if (input == "a" || input == "A")
            {
                for (const auto &pair : displayMap)
                    idsToRemove.push_back(pair.first);
                isAllRemove = validRemove = true;
                break;
            }
            
            idsToRemove = InputValidator::parseSpaceSeparatedIntegers(input);
            if (idsToRemove.empty())
            {
                failCountRemove++;
                if (failCountRemove < 3)
                    std::cout << "Du lieu nhap vao khong hop le. Yeu cau nhap lai:\n-> ";
                continue;
            }
            
            bool allValid = true;
            for (int id : idsToRemove)
                if (id < 1 || id >= displayId)
                {
                    allValid = false;
                    break;
                }
                
            if (!allValid)
            {
                failCountRemove++;
                if (failCountRemove < 3)
                    std::cout << "Du lieu nhap vao khong hop le. Yeu cau nhap lai:\n-> ";
                idsToRemove.clear();
            }
            else
            {
                validRemove = true;
                break;
            }
        }
        
        if (!validRemove)
        {
            currentState = AppState::MAIN_MENU;
            return;
        }
        
        std::string targetStr = isAllRemove ? "tat ca app/web" : (idsToRemove.size() == 1 ? "app/web da chon" : "cac app/web da chon");
        std::cout << "Xac nhan bo " << targetStr << "\nChon? [y/n] : ";
        std::string conf;
        std::getline(std::cin, conf);
        
        if (conf == "y" || conf == "Y")
        {
            std::vector<ActiveLimit> limitsToKeep;
            for (const auto &pair : displayMap)
                if (std::find(idsToRemove.begin(), idsToRemove.end(), pair.first) == idsToRemove.end())
                    limitsToKeep.push_back(pair.second);
                    
            FileManager::saveAllActiveLimits(limitsToKeep);
            g_ProcMgr.reloadActiveLimits();
            std::cout << "\nDa bo gioi han thanh cong " << targetStr << ".\n";
            safeWait();
            currentState = AppState::LIMIT_APP;
            return;
        }
        else if (conf == "n" || conf == "N")
            continue;
        else
        {
            currentState = AppState::MAIN_MENU;
            return;
        }
    }
}

void ConsoleMenu::showLimitApp()
{
    clearScreen();
    std::cout << "* Gioi han ung dung:\n1. Them gioi han co ban.\n2. Them gioi han tuy chon.\n3. Bo gioi han.\n4. Quay lai.\nChon? [1/2/3/4] : ";
    std::string choice;
    std::getline(std::cin, choice);
    if (choice == "1")
        showAddBasicLimit();
    else if (choice == "2")
        showAddCustomLimit();
    else if (choice == "3")
        handleRemoveLimitLogic();
    else if (choice == "4")
        currentState = AppState::MAIN_MENU;
    else
        handleInvalidState();
}

void ConsoleMenu::run()
{
    while (currentState != AppState::EXIT)
    {
        switch (currentState)
        {
        case AppState::MAIN_MENU:
            showMainMenu();
            break;
        case AppState::LIMIT_APP:
            showLimitApp();
            break;
        default:
            currentState = AppState::MAIN_MENU;
            break;
        }
    }
}