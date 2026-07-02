#pragma once
#include <string>
#include <vector>
#include <optional>
#include "DatabaseConnection.h"
#include "../Models.h"

class FineRepository {
public:
    int              save(int violationId, double amount);
    std::optional<Fine> findById(int id);
    std::optional<Fine> findByViolationId(int violationId);
    std::vector<Fine>   findAll(int limit = 100);
    bool             updateStatus(int id, const std::string& status);
};
