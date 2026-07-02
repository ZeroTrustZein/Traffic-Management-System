#pragma once
#include <string>
#include <optional>
#include <vector>

// =============================================================================
// Enums
// =============================================================================

enum class VehicleType { CAR, TRUCK, MOTORCYCLE, BUS, EMERGENCY };
enum class LogStatus   { REGISTERED, UNREGISTERED };
enum class EventType   { PASSAGE, SPEEDING, RED_LIGHT, PARKING };
enum class Severity    { LOW, MEDIUM, HIGH, CRITICAL };
enum class FineStatus  { PENDING, PAID, CANCELLED, OVERDUE };
enum class SignalState { NORMAL, GREEN_PRIORITY };

// --- VehicleType helpers ---
inline std::string toString(VehicleType t) {
    switch (t) {
        case VehicleType::TRUCK:      return "TRUCK";
        case VehicleType::MOTORCYCLE: return "MOTORCYCLE";
        case VehicleType::BUS:        return "BUS";
        case VehicleType::EMERGENCY:  return "EMERGENCY";
        default:                      return "CAR";
    }
}
inline VehicleType vehicleTypeFrom(const std::string& s) {
    if (s == "TRUCK")      return VehicleType::TRUCK;
    if (s == "MOTORCYCLE") return VehicleType::MOTORCYCLE;
    if (s == "BUS")        return VehicleType::BUS;
    if (s == "EMERGENCY")  return VehicleType::EMERGENCY;
    return VehicleType::CAR;
}

// --- LogStatus helpers ---
inline std::string toString(LogStatus s) {
    return s == LogStatus::UNREGISTERED ? "UNREGISTERED" : "REGISTERED";
}
inline LogStatus logStatusFrom(const std::string& s) {
    return s == "UNREGISTERED" ? LogStatus::UNREGISTERED : LogStatus::REGISTERED;
}

// --- EventType helpers ---
inline std::string toString(EventType e) {
    switch (e) {
        case EventType::SPEEDING:  return "SPEEDING";
        case EventType::RED_LIGHT: return "RED_LIGHT";
        case EventType::PARKING:   return "PARKING";
        default:                   return "PASSAGE";
    }
}
inline EventType eventTypeFrom(const std::string& s) {
    if (s == "SPEEDING")  return EventType::SPEEDING;
    if (s == "RED_LIGHT") return EventType::RED_LIGHT;
    if (s == "PARKING")   return EventType::PARKING;
    return EventType::PASSAGE;
}

// --- Severity helpers ---
inline std::string toString(Severity s) {
    switch (s) {
        case Severity::LOW:      return "LOW";
        case Severity::HIGH:     return "HIGH";
        case Severity::CRITICAL: return "CRITICAL";
        default:                 return "MEDIUM";
    }
}
inline Severity severityFrom(const std::string& s) {
    if (s == "LOW")      return Severity::LOW;
    if (s == "HIGH")     return Severity::HIGH;
    if (s == "CRITICAL") return Severity::CRITICAL;
    return Severity::MEDIUM;
}

// --- FineStatus helpers ---
inline std::string toString(FineStatus s) {
    switch (s) {
        case FineStatus::PAID:      return "PAID";
        case FineStatus::CANCELLED: return "CANCELLED";
        case FineStatus::OVERDUE:   return "OVERDUE";
        default:                    return "PENDING";
    }
}
inline FineStatus fineStatusFrom(const std::string& s) {
    if (s == "PAID")      return FineStatus::PAID;
    if (s == "CANCELLED") return FineStatus::CANCELLED;
    if (s == "OVERDUE")   return FineStatus::OVERDUE;
    return FineStatus::PENDING;
}

// --- SignalState helpers ---
inline std::string toString(SignalState s) {
    return s == SignalState::GREEN_PRIORITY ? "GREEN_PRIORITY" : "NORMAL";
}
inline SignalState signalStateFrom(const std::string& s) {
    return s == "GREEN_PRIORITY" ? SignalState::GREEN_PRIORITY : SignalState::NORMAL;
}

// =============================================================================
// Data structs
// =============================================================================

struct Owner {
    int         id        = 0;
    std::string fullName;
    std::string email;
    std::string phone;
    std::string createdAt;
};

// =============================================================================
// Road / Junction class model
// -----------------------------------------------------------------------------
// A Road is a stretch of street with a name and a posted speed limit.
// A Junction is a special kind of road segment (an intersection point) that
// adds a geographic location and a free-text address. Junction inherits from
// Road so any code that consumes a Road (e.g. speed-limit lookups) also works
// for junctions.
// =============================================================================

struct Road {
    int         id            = 0;
    std::string name;
    int         speedLimitKmh = 60;   // legal speed limit on this road
    bool        isActive      = true;
};

struct Junction : public Road {
    // Inherits id, name, speedLimitKmh, isActive from Road.
    std::string location;             // human-readable cross street / address
    double      latitude  = 0.0;      // 0 = unset
    double      longitude = 0.0;      // 0 = unset
};

// A directed edge between two junctions, used by the route-guidance algorithm.
struct RoadSegment {
    int         id              = 0;
    int         fromJunctionId  = 0;
    int         toJunctionId    = 0;
    double      distanceKm      = 0.0;
    int         speedLimitKmh   = 50;
    std::string name;                 // e.g. "Main St"
};

struct Vehicle {
    int         id          = 0;
    std::string numberPlate;
    VehicleType vehicleType = VehicleType::CAR;
    int         ownerId     = 0;
    bool        isActive    = true;
    std::string createdAt;
};

struct DriverProfile {
    int              id                = 0;
    int              ownerId           = 0;
    int              homeJunctionId    = 0;
    std::vector<int> typicalRoute;
    bool             notificationOptIn = true;
    std::string      createdAt;
};

// W15: plate field added for automatic junction detection
struct EmergencyVehicle {
    int         id         = 0;
    std::string type;
    std::string identifier;
    std::string plate;      // nullable — empty string means not set
    bool        isActive   = true;
    std::string createdAt;
};

struct EmergencyEvent {
    int              id                = 0;
    int              emergencyVehicleId = 0;
    int              startJunctionId   = 0;
    int              targetJunctionId  = 0;
    std::string      status;
    std::string      startedAt;
    std::vector<int> lastRoute;
    double           estimatedMinutes  = 0.0;
};

// W15: per-junction signal state for traffic-signal control
struct JunctionSignal {
    int         junctionId       = 0;
    std::string state;            // NORMAL | GREEN_PRIORITY
    int         emergencyEventId = 0;  // 0 = NULL
    std::string updatedAt;
};

struct PlateLog {
    int         id           = 0;
    std::string numberPlate;
    int         vehicleId    = 0;
    int         junctionId   = 0;
    double      speedKmh     = 0.0;
    bool        hasSpeed     = false;
    std::string detectedAt;
    LogStatus   status       = LogStatus::REGISTERED;
    EventType   eventType    = EventType::PASSAGE;
};

// W13: auto_notify and requires_review added for per-type configurable actions
struct ViolationType {
    int         id            = 0;
    std::string name;
    std::string code;
    double      baseFine      = 0.0;
    std::string description;
    bool        isActive      = true;
    double      multLow       = 1.0;
    double      multMedium    = 1.5;
    double      multHigh      = 2.0;
    double      multCritical  = 3.0;
    bool        autoNotify    = true;   // W13: whether to auto-send notification
    bool        requiresReview = false;  // W13: whether manual review is required before fine
};

struct Violation {
    int         id              = 0;
    int         vehicleId       = 0;
    int         junctionId      = 0;
    int         violationTypeId = 0;
    int         plateLogId      = 0;
    std::string detectedAt;
    Severity    severity        = Severity::MEDIUM;
    std::string evidenceNote;
    std::string status;  // OPEN | REVIEW | PAID | CANCELLED | CONTESTED
};

struct Fine {
    int         id          = 0;
    int         violationId = 0;
    double      amount      = 0.0;
    FineStatus  status      = FineStatus::PENDING;
    std::string issuedAt;
    std::string dueDate;
    std::string paidAt;
};

struct CongestionRecord {
    int         id               = 0;
    int         junctionId       = 0;
    std::string timeWindowStart;
    std::string timeWindowEnd;
    int         vehicleCount     = 0;
    std::string congestionLevel;  // LOW | MODERATE | HIGH | SEVERE
    std::string recordedAt;
};

// W13/W14/W15: auditable notification record
struct NotificationLog {
    int         id       = 0;
    int         ownerId  = 0;  // 0 = NULL (junction-level or system alert)
    std::string category;
    std::string subject;
    std::string message;
    std::string sentAt;
    std::string status;
};
