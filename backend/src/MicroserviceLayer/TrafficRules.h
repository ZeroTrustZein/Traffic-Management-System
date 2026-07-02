#pragma once
#include <string>

namespace TrafficRules {
std::string classifyCongestionLevel(int vehicleCount);
double congestionPenaltyMultiplier(const std::string& level);
}

