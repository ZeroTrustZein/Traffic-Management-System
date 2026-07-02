#pragma once
#include <crow.h>
#include "../DataAccessLayer/VehicleRepository.h"
#include "../DataAccessLayer/OwnerRepository.h"

class VehicleController {
public:
    crow::response registerVehicle(const crow::request& req);
    crow::response getVehicle(const crow::request& req, const std::string& plate);
    crow::response listVehicles(const crow::request& req);

private:
    VehicleRepository vehicleRepo_;
    OwnerRepository   ownerRepo_;
};
