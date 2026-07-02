#pragma once
#include <optional>
#include <vector>
#include "DatabaseConnection.h"
#include "../Models.h"

class DriverProfileRepository {
public:
    std::optional<DriverProfile> findByOwnerId(int ownerId);
    std::vector<DriverProfile>   findAll(int limit = 200);
    std::vector<DriverProfile>   findOptedInByRouteOverlap(const std::vector<int>& junctionIds, int limit = 200);
};

