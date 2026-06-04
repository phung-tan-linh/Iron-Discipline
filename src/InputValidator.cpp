#include "../include/InputValidator.h"
#include <sstream>
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