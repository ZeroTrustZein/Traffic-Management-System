#pragma once
#include <optional>
#include <vector>
#include "DatabaseConnection.h"
#include "../Models.h"

class EmergencyEventRepository {
public:
    int                          create(int emergencyVehicleId, int startJunctionId, int targetJunctionId);
    void                         setRoute(int eventId, const std::vector<int>& route, double estimatedMinutes);
    std::optional<EmergencyEvent> findById(int eventId);
    std::vector<EmergencyEvent>   findRecent(int limit = 50);

    // W15: mark event as RESOLVED.
    bool resolve(int eventId);
};
