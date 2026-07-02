#pragma once
#include <optional>
#include <vector>
#include "DatabaseConnection.h"
#include "../Models.h"

class EmergencyVehicleRepository {
public:
    // Insert or update by identifier; plate may be empty (stored as NULL).
    int                          upsert(const std::string& type,
                                        const std::string& identifier,
                                        bool isActive,
                                        const std::string& plate = "");
    std::optional<EmergencyVehicle> findById(int id);
    std::vector<EmergencyVehicle>   findAll(int limit = 200);

    // W15: check whether a plate belongs to a registered emergency vehicle.
    bool existsByPlate(const std::string& plate);
};
