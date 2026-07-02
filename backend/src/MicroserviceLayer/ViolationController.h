#pragma once
#include <crow.h>
#include "../DataAccessLayer/ViolationRepository.h"
#include "../DataAccessLayer/ViolationTypeRepository.h"
#include "../DataAccessLayer/FineRepository.h"
#include "../DataAccessLayer/JunctionLogRepository.h"
#include "../DataAccessLayer/JunctionRepository.h"
#include "../DataAccessLayer/VehicleRepository.h"
#include "../DataAccessLayer/NotificationRepository.h"

class ViolationController {
public:
    crow::response detectViolations(const crow::request& req);
    crow::response listViolations(const crow::request& req);
    crow::response getViolation(const crow::request& req, int id);
    crow::response listViolationTypes(const crow::request& req);
    crow::response issueFine(const crow::request& req, int violationId);

private:
    ViolationRepository     violationRepo_;
    ViolationTypeRepository typeRepo_;
    FineRepository          fineRepo_;
    JunctionLogRepository   logRepo_;
    JunctionRepository      junctionRepo_;
    VehicleRepository       vehicleRepo_;
    NotificationRepository  notifRepo_;  // W13: notification audit trail
};
