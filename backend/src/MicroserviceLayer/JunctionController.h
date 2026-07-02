#pragma once
#include <crow.h>
#include "../DataAccessLayer/VehicleRepository.h"
#include "../DataAccessLayer/JunctionRepository.h"
#include "../DataAccessLayer/JunctionLogRepository.h"
#include "../DataAccessLayer/EmergencyVehicleRepository.h"
#include "../DataAccessLayer/SignalRepository.h"
#include "../DataAccessLayer/NotificationRepository.h"

class JunctionController {
public:
    crow::response logVehicle(const crow::request& req, int junctionId);
    crow::response getLogs(const crow::request& req, int junctionId);
    crow::response getAllLogs(const crow::request& req);
    crow::response listJunctions(const crow::request& req);
    crow::response createJunction(const crow::request& req);

private:
    VehicleRepository          vehicleRepo_;
    JunctionRepository         junctionRepo_;
    JunctionLogRepository      logRepo_;
    EmergencyVehicleRepository emVehicleRepo_;  // W15: plate-based emergency detection
    SignalRepository           signalRepo_;     // W15: set GREEN_PRIORITY on detection
    NotificationRepository     notifRepo_;      // W15: audit log for EMERGENCY_DETECTED
};
