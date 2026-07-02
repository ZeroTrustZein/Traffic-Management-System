#pragma once
#include <string>

struct ValidationResult {
    bool        isValid = true;
    std::string fieldName;
    std::string errorMessage;

    static ValidationResult ok() {
        return {true, "", ""};
    }

    static ValidationResult fail(const std::string& field, const std::string& msg) {
        return {false, field, msg};
    }
};
