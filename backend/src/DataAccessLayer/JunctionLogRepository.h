#pragma once
#include <string>
#include <vector>
#include "DatabaseConnection.h"
#include "../Models.h"

// Renamed conceptually to PlateLogRepository; file name retained for compatibility.
class JunctionLogRepository {
public:
    int                   save(int junctionId, int vehicleId,
                               const std::string& numberPlate,
                               const std::string& eventType,
                               const std::string& status,
                               double speedKmh, bool hasSpeed);
    int                   saveUnregistered(int junctionId,
                                           const std::string& numberPlate);
    std::vector<PlateLog> findByJunctionId(int junctionId, int limit = 50);
    std::vector<PlateLog> findAll(int limit = 200);
    // Returns logs that triggered a violation event but have no violation record yet
    std::vector<PlateLog> findUnprocessedViolationLogs(int junctionId, int hoursBack);
};
