#include "EmergencyEventRepository.h"
#include <sstream>

static std::string toPgIntArray(const std::vector<int>& ids) {
    std::ostringstream ss;
    ss << "{";
    for (size_t i = 0; i < ids.size(); ++i) {
        if (i) ss << ",";
        ss << ids[i];
    }
    ss << "}";
    return ss.str();
}

static std::vector<int> parsePgIntArray(const std::string& s) {
    std::vector<int> out;
    if (s.size() < 2) return out;
    if (s == "{}") return out;
    std::string inner = s;
    if (inner.front() == '{') inner.erase(inner.begin());
    if (!inner.empty() && inner.back() == '}') inner.pop_back();
    std::stringstream ss(inner);
    std::string item;
    while (std::getline(ss, item, ',')) {
        if (item.empty()) continue;
        try {
            out.push_back(std::stoi(item));
        } catch (...) {
        }
    }
    return out;
}

static EmergencyEvent rowToEvent(const pqxx::row& row) {
    EmergencyEvent e;
    e.id                 = row[0].as<int>();
    e.emergencyVehicleId = row[1].as<int>();
    e.startJunctionId    = row[2].as<int>();
    e.targetJunctionId   = row[3].as<int>();
    e.status             = row[4].as<std::string>();
    e.startedAt          = row[5].as<std::string>();
    e.lastRoute          = parsePgIntArray(row[6].as<std::string>());
    e.estimatedMinutes   = row[7].is_null() ? 0.0 : row[7].as<double>();
    return e;
}

int EmergencyEventRepository::create(int emergencyVehicleId, int startJunctionId, int targetJunctionId) {
    pqxx::work txn(DatabaseConnection::get());
    auto r = txn.exec_params(
        "INSERT INTO emergency_events (emergency_vehicle_id, start_junction_id, target_junction_id) "
        "VALUES ($1, $2, $3) RETURNING id",
        emergencyVehicleId, startJunctionId, targetJunctionId);
    txn.commit();
    return r[0][0].as<int>();
}

void EmergencyEventRepository::setRoute(int eventId, const std::vector<int>& route, double estimatedMinutes) {
    pqxx::work txn(DatabaseConnection::get());
    const std::string arr = toPgIntArray(route);
    txn.exec_params(
        "UPDATE emergency_events SET last_route = $2::int[], estimated_minutes = $3 WHERE id = $1",
        eventId, arr, estimatedMinutes);
    txn.commit();
}

std::optional<EmergencyEvent> EmergencyEventRepository::findById(int eventId) {
    pqxx::work txn(DatabaseConnection::get());
    auto r = txn.exec_params(
        "SELECT id, emergency_vehicle_id, start_junction_id, target_junction_id, status, started_at::text, last_route::text, estimated_minutes "
        "FROM emergency_events WHERE id = $1",
        eventId);
    if (r.empty()) return std::nullopt;
    return rowToEvent(r[0]);
}

std::vector<EmergencyEvent> EmergencyEventRepository::findRecent(int limit) {
    pqxx::work txn(DatabaseConnection::get());
    auto r = txn.exec_params(
        "SELECT id, emergency_vehicle_id, start_junction_id, target_junction_id, status, started_at::text, last_route::text, estimated_minutes "
        "FROM emergency_events ORDER BY started_at DESC LIMIT $1",
        limit);
    std::vector<EmergencyEvent> list;
    list.reserve(r.size());
    for (const auto& row : r) list.push_back(rowToEvent(row));
    return list;
}

bool EmergencyEventRepository::resolve(int eventId) {
    pqxx::work txn(DatabaseConnection::get());
    auto r = txn.exec_params(
        "UPDATE emergency_events SET status = 'RESOLVED' WHERE id = $1 AND status != 'RESOLVED'",
        eventId);
    txn.commit();
    return r.affected_rows() > 0;
}
