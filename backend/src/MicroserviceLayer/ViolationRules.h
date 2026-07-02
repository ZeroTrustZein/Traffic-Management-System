#pragma once
#include <string>

namespace ViolationRules {
std::string severityForSpeed(double speedKmh, int speedLimitKmh);
std::string severityForEvent(const std::string& eventType, double speedKmh, bool hasSpeed, int speedLimitKmh);
double multiplierForSeverity(const std::string& severity, double multLow, double multMedium, double multHigh, double multCritical);
bool isRepeatOffender(int priorViolationCount);
double repeatOffenderMultiplier();
}
