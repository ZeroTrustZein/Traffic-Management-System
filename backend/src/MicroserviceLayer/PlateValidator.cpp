#include "PlateValidator.h"
#include <regex>

bool PlateValidator::validate(const std::string& plate) {
    static const std::regex kPattern("^[A-Z]{2}[0-9]{2}[A-Z]{3}$");
    return std::regex_match(plate, kPattern);
}
