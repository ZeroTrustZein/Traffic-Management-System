#include "ViolationRules.h"

namespace ViolationRules {
namespace {
constexpr int kRepeatOffenderThreshold = 3;
constexpr double kRepeatOffenderSurcharge = 1.5;
}

std::string severityForSpeed(double speedKmh, int speedLimitKmh) {
    const double over = speedKmh - static_cast<double>(speedLimitKmh);
    if (over > 30) return "CRITICAL";
    if (over > 20) return "HIGH";
    if (over > 10) return "MEDIUM";
    return "LOW";
}

std::string severityForEvent(const std::string& eventType, double speedKmh, bool hasSpeed, int speedLimitKmh) {
    if (eventType == "RED_LIGHT") return "CRITICAL";
    if (eventType == "SPEEDING") {
        if (!hasSpeed) return "MEDIUM";
        return severityForSpeed(speedKmh, speedLimitKmh);
    }
    if (eventType == "PARKING") return "MEDIUM";
    return "LOW";
}

double multiplierForSeverity(const std::string& severity, double multLow, double multMedium, double multHigh, double multCritical) {
    if (severity == "CRITICAL") return multCritical;
    if (severity == "HIGH") return multHigh;
    if (severity == "LOW") return multLow;
    return multMedium;
}

bool isRepeatOffender(int priorViolationCount) {
    return priorViolationCount >= kRepeatOffenderThreshold;
}

double repeatOffenderMultiplier() {
    return kRepeatOffenderSurcharge;
}
}
