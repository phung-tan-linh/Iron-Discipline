// [PLAN]: Khai báo các hàm tĩnh xử lý Validation. Tách biệt hoàn toàn logic kiểm tra tính hợp lệ của dữ liệu đầu vào khỏi ConsoleMenu.
#ifndef INPUT_VALIDATOR_H
#define INPUT_VALIDATOR_H

#include <string>
#include <vector>
#include <iostream>

class InputValidator
{
public:
    static std::string toRoman(int num);
    static bool getValidTimeInput(int &outMinutes, const std::string &promptText = "");
    static std::vector<int> parseSpaceSeparatedIntegers(const std::string &input);
    static void clearInputState();
    
    static std::vector<int> getValidSelection(const std::vector<int>& validIds, bool& isAll, bool& isCancel);
    static std::vector<std::string> getValidStringsInput(bool& isCancel);
};

#endif