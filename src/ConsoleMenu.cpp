// [PLAN]: Áp dụng triệt để SRP. ConsoleMenu chỉ chịu trách nhiệm hiển thị UI và điều hướng luồng.
// Loại bỏ hoàn toàn việc lưu trữ basicList. Giao tiếp với DataStore thông qua các hàm getCachedBasicList,
// getAllBasicItemIds, addLimitsByBasicIds và removeLimitsByDisplayIds.
#include "../include/ConsoleMenu.h"
#include "../include/DataStore.h"
#include "../include/InputValidator.h"
#include "../include/TrackingEngine.h"
#include <iostream>
#include <iomanip>
#include <algorithm>

extern TimeEnforcer g_Engine;

ConsoleMenu::ConsoleMenu() : currentState(AppState::MAIN_MENU) {}

void ConsoleMenu::init(const std::string &basicListFile) { UsageRepository::loadBasicList(basicListFile); }

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
    for (const auto &cat : UsageRepository::getCachedBasicList())
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
        
        std::vector<int> validIds = UsageRepository::getAllBasicItemIds();
                
        bool isAll = false, isCancel = false;
        std::vector<int> selectedIds = InputValidator::getValidSelection(validIds, isAll, isCancel);
        
        if (isCancel)
        {
            currentState = AppState::LIMIT_APP;
            return;
        }
        if (selectedIds.empty())
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
            if (timeMins > 0)
            {
                UsageRepository::addLimitsByBasicIds(selectedIds, timeMins);
                g_Engine.reloadLimits();
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
        
        bool isCancel = false;
        std::vector<std::string> customNames = InputValidator::getValidStringsInput(isCancel);
        
        if (isCancel)
            return;
            
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
            if (timeMins > 0)
            {
                for (const auto &name : customNames)
                {
                    UsageRepository::addOrUpdateActiveLimit(ActiveLimit(2, name, timeMins));
                }
                g_Engine.reloadLimits();
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
        std::cout << "\n - Muon them gioi han app/web nao thi liet ke cac so thu tu cua app/web do cach nhau boi dau cach.\n - Muon them gioi han tat ca thi an 'a'.\n - Muon quay lai menu Gioi han ung dung thi an 'n'.\n-> ";
        
        std::vector<int> validIds = UsageRepository::getAllBasicItemIds();
                
        bool isAll = false, isCancel = false;
        std::vector<int> selectedIds = InputValidator::getValidSelection(validIds, isAll, isCancel);
        
        if (isCancel)
            return;
            
        if (selectedIds.empty())
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
            if (timeMins > 0)
            {
                UsageRepository::addLimitsByBasicIds(selectedIds, timeMins);
                g_Engine.reloadLimits();
            }
                        
            std::cout << "\nDa them gioi han thanh cong " << targetStr << ".\n - Muon them gioi han app/web ngoai danh sach thi an 'y'.\n - Muon quay lai menu Gioi han ung dung thi an 'n'.\nChon? [y/n] : ";
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
        auto displayList = UsageRepository::getSortedDisplayLimits();
        
        clearScreen();
        std::cout << "Duoi day la toan bo app/web dang bi gioi han:\n";
        
        std::vector<int> validIds;
        
        for (const auto& dl : displayList)
        {
            std::cout << dl.displayId << ". " << dl.name << " : " << formatTime(dl.limitMinutes) << "\n";
            validIds.push_back(dl.displayId);
        }
        
        std::cout << "\n - Muon bo gioi han app/web nao thi liet ke cac so thu tu cua app/web do cach nhau boi dau cach.\n - Muon bo gioi han tat ca thi an 'a'.\n- Muon quay lai thi an 'n'.\n-> ";
        
        bool isAllRemove = false, isCancel = false;
        std::vector<int> idsToRemove = InputValidator::getValidSelection(validIds, isAllRemove, isCancel);
        
        if (isCancel)
        {
            currentState = AppState::LIMIT_APP;
            return;
        }
        if (idsToRemove.empty())
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
            if (UsageRepository::removeLimitsByDisplayIds(idsToRemove, displayList))
            {
                g_Engine.reloadLimits();
                std::cout << "\nDa bo gioi han thanh cong " << targetStr << ".\n";
            }
            else
            {
                std::cout << "\nKhong the luu thay doi vao he thong!\n";
            }
            
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
    std::cout << "* Gioi han ung dung:\n1. Them gioi han co ban.\n2. Them gioi han tuy chon.\n3. Bo gioi han.\nChon? [1/2/3] : ";
    std::string choice;
    std::getline(std::cin, choice);
    if (choice == "1")
        showAddBasicLimit();
    else if (choice == "2")
        showAddCustomLimit();
    else if (choice == "3")
        handleRemoveLimitLogic();
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
