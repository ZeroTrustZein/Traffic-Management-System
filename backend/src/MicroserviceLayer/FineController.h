#pragma once
#include <crow.h>
#include "../DataAccessLayer/FineRepository.h"
#include "../DataAccessLayer/ViolationRepository.h"
#include "../DataAccessLayer/VehicleRepository.h"
#include "../DataAccessLayer/OwnerRepository.h"

class FineController {
public:
    crow::response listFines(const crow::request& req);
    crow::response getFine(const crow::request& req, int id);
    crow::response payFine(const crow::request& req, int id);
    crow::response cancelFine(const crow::request& req, int id);

private:
    FineRepository      fineRepo_;
    ViolationRepository violationRepo_;
    VehicleRepository   vehicleRepo_;
    OwnerRepository     ownerRepo_;
};
