#pragma once
#include <string>
#include <optional>
#include <vector>
#include "DatabaseConnection.h"
#include "../Models.h"

class VehicleRepository {
public:
    int                    save(const std::string& numberPlate,
                                const std::string& vehicleType,
                                int ownerId);
    bool                   existsByPlate(const std::string& plate);
    std::optional<Vehicle> findByPlate(const std::string& plate);
    std::optional<Vehicle> findById(int id);
    std::vector<Vehicle>   findAll(int limit = 100);
};
