#pragma once
#include <string>

class PlateValidator {
public:
    // Accepts UK-style 7-char plates: 2 letters + 2 digits + 3 letters (e.g. AB12CDE)
    static bool validate(const std::string& plate);
};
