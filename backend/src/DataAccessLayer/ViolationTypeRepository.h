#pragma once
#include <string>
#include <optional>
#include <vector>
#include "DatabaseConnection.h"
#include "../Models.h"

class ViolationTypeRepository {
public:
    std::optional<ViolationType> findByCode(const std::string& code);
    std::optional<ViolationType> findById(int id);
    std::vector<ViolationType>   findAll();
};
