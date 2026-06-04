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
};
#endif