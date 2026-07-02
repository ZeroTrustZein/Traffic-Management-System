#pragma once
#include <string>
#include <vector>
#include "DatabaseConnection.h"
#include "../Models.h"

class CongestionRepository {
public:
    int                          save(int junctionId,
                                     const std::string& windowStart,
                                     const std::string& windowEnd,
                                     int vehicleCount,
                                     const std::string& congestionLevel);
    std::vector<CongestionRecord> findAll(int limit = 100);
    std::vector<CongestionRecord> findByJunctionId(int junctionId, int limit = 50);
    double averageRecentCount(int junctionId, int windowCount);

    // W14: average vehicle_count for a specific hour-of-day (0-23) from history.
    double averageCountForHour(int junctionId, int hourOfDay);

    struct JunctionSummary { int junctionId; std::string name; double avgCount; std::string worstLevel; };
    std::vector<JunctionSummary> congestionProneSummary();

    int countLogsInWindow(int junctionId, int secondsBack);

    // Hour-of-day traffic profile — average vehicle count per hour (0–23) per junction
    struct HourlyBucket   { int hour; double avgCount; };
    struct HourlyJunction { int junctionId; std::string name; std::vector<HourlyBucket> hours; };
    std::vector<HourlyJunction> hourlyProfile();
};
