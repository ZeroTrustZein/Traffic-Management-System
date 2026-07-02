#pragma once
#include <crow.h>
#include "../DataAccessLayer/EmergencyVehicleRepository.h"
#include "../DataAccessLayer/EmergencyEventRepository.h"
#include "../DataAccessLayer/RoadSegmentRepository.h"
#include "../DataAccessLayer/JunctionRepository.h"
#include "../DataAccessLayer/DriverProfileRepository.h"
#include "../DataAccessLayer/OwnerRepository.h"
#include "../DataAccessLayer/SignalRepository.h"
#include "../DataAccessLayer/NotificationRepository.h"

class EmergencyController {
public:
    crow::response upsertEmergencyVehicle(const crow::request& req);
    crow::response listEmergencyVehicles(const crow::request& req);

    crow::response createEmergencyEvent(const crow::request& req);
    crow::response getEmergencyEvent(const crow::request& req, int eventId);
    crow::response listAffectedDrivers(const crow::request& req, int eventId);

    // W15: resolve an active emergency event and release its signal corridor.
    crow::response resolveEmergencyEvent(const crow::request& req, int eventId);

    // W15: list all junction signal states.
    crow::response listSignals(const crow::request& req);

private:
    EmergencyVehicleRepository vehicleRepo_;
    EmergencyEventRepository   eventRepo_;
    RoadSegmentRepository      roadRepo_;
    JunctionRepository         junctionRepo_;
    DriverProfileRepository    profileRepo_;
    OwnerRepository            ownerRepo_;
    SignalRepository           signalRepo_;
    NotificationRepository     notifRepo_;
};
