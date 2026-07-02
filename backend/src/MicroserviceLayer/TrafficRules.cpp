#include "TrafficRules.h"

namespace TrafficRules {
std::string classifyCongestionLevel(int vehicleCount) {
    if (vehicleCount >= 30) return "SEVERE";
    if (vehicleCount >= 20) return "HIGH";
    if (vehicleCount >= 10) return "MODERATE";
    return "LOW";
}

double congestionPenaltyMultiplier(const std::string& level) {
    if (level == "SEVERE") return 2.5;
    if (level == "HIGH") return 1.8;
    if (level == "MODERATE") return 1.3;
    return 1.0;
}
}

