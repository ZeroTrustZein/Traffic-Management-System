#pragma once
#include <string>
#include <vector>
#include <optional>
#include "DatabaseConnection.h"
#include "../Models.h"

class ViolationRepository {
public:
    // status defaults to "OPEN"; pass "REVIEW" when requires_review is true.
    int                      save(int vehicleId, int junctionId, int violationTypeId,
                                  int plateLogId, const std::string& severity,
                                  const std::string& evidenceNote,
                                  const std::string& status = "OPEN");
    std::optional<Violation> findById(int id);
    std::vector<Violation>   findAll(int limit = 100);
    std::vector<Violation>   findByVehicleId(int vehicleId);
    bool                     updateStatus(int id, const std::string& status);

    // W13/F9: count prior violations for a vehicle (for repeat-offender escalation).
    int                      countByVehicle(int vehicleId);
};
