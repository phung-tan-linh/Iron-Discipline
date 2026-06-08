// [PLAN]: Triển khai InputValidator. Xử lý các vòng lặp kiểm tra mảng ID và chuỗi hợp lệ, đảm bảo trả về dữ liệu sạch 100% cho ConsoleMenu.
#include "../include/InputValidator.h"
#include <sstream>
#include <algorithm>

std::string InputValidator::toRoman(int num)
{
    const std::string roman[] = {"", "I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX", "X"};
    if (num > 0 && num <= 10)
        return roman[num];
    return std::to_string(num);
}

void InputValidator::clearInputState()
{
    if (std::cin.fail())
    {
        std::cin.clear();
    }
}

bool InputValidator::getValidTimeInput(int &outMinutes, const std::string &promptText)
{
    if (!promptText.empty())
        std::cout << promptText;
    int attempts = 0;
    while (attempts < 3)
    {
        std::string hStr, mStr;
        std::cout << "Gio? [0-23] : ";
        std::getline(std::cin, hStr);
        std::cout << "Phut? [0-59] : ";
        std::getline(std::cin, mStr);
        try
        {
            int h = std::stoi(hStr);
            int m = std::stoi(mStr);
            if (h >= 0 && h <= 23 && m >= 0 && m <= 59)
            {
                int total = h * 60 + m;
                if (total > 9)
                {
                    outMinutes = total;
                    return true;
                }
            }
        }
        catch (...)
        {
        }
        attempts++;
        if (attempts < 3)
        {
            std::cout << "Du lieu nhap vao khong dung hoac khong hop le. Yeu cau nhap lai:\n";
        }
    }
    return false;
}

std::vector<int> InputValidator::parseSpaceSeparatedIntegers(const std::string &input)
{
    std::vector<int> result;
    if (input.empty() || input.find_first_not_of(' ') == std::string::npos)
        return result;
    std::stringstream ss(input);
    std::string temp;
    while (ss >> temp)
    {
        try
        {
            size_t pos;
            int num = std::stoi(temp, &pos);
            if (pos != temp.length())
                return std::vector<int>();
            result.push_back(num);
        }
        catch (...)
        {
            return std::vector<int>();
        }
    }
    return result;
}

std::vector<int> InputValidator::getValidSelection(const std::vector<int>& validIds, bool& isAll, bool& isCancel)
{
    int failCount = 0;
    isAll = false;
    isCancel = false;
    while (failCount < 3)
    {
        std::string input;
        std::getline(std::cin, input);
        
        if (input == "n" || input == "N")
        {
            isCancel = true;
            return {};
        }
        if (input == "a" || input == "A")
        {
            isAll = true;
            return validIds;
        }
        
        std::vector<int> selectedIds = parseSpaceSeparatedIntegers(input);
        if (selectedIds.empty())
        {
            failCount++;
            if (failCount < 3)
                std::cout << "Du lieu nhap vao khong hop le. Yeu cau nhap lai:\n-> ";
            continue;
        }
        
        bool allValid = true;
        for (int id : selectedIds)
        {
            if (std::find(validIds.begin(), validIds.end(), id) == validIds.end())
            {
                allValid = false;
                break;
            }
        }
            
        if (!allValid)
        {
            failCount++;
            if (failCount < 3)
                std::cout << "Du lieu nhap vao khong hop le. Yeu cau nhap lai:\n-> ";
        }
        else
        {
            return selectedIds;
        }
    }
    return {};
}

std::vector<std::string> InputValidator::getValidStringsInput(bool& isCancel)
{
    isCancel = false;
    std::string inputStr;
    std::getline(std::cin, inputStr);
    
    if (inputStr == "n" || inputStr == "N")
    {
        isCancel = true;
        return {};
    }
        
    std::vector<std::string> customNames;
    std::stringstream ss(inputStr);
    std::string temp;
    while (ss >> temp)
    {
        customNames.push_back(temp);
    }
    return customNames;
}