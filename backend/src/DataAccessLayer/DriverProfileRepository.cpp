#include "DriverProfileRepository.h"
#include <sstream>

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

static DriverProfile rowToProfile(const pqxx::row& row) {
    DriverProfile p;
    p.id                = row[0].as<int>();
    p.ownerId           = row[1].as<int>();
    p.homeJunctionId    = row[2].is_null() ? 0 : row[2].as<int>();
    p.typicalRoute      = parsePgIntArray(row[3].as<std::string>());
    p.notificationOptIn = row[4].as<bool>();
    p.createdAt         = row[5].as<std::string>();
    return p;
}

std::optional<DriverProfile> DriverProfileRepository::findByOwnerId(int ownerId) {
    pqxx::work txn(DatabaseConnection::get());
    auto r = txn.exec_params(
        "SELECT id, owner_id, home_junction_id, typical_route::text, notification_opt_in, created_at::text "
        "FROM driver_profiles WHERE owner_id = $1",
        ownerId);
    if (r.empty()) return std::nullopt;
    return rowToProfile(r[0]);
}

std::vector<DriverProfile> DriverProfileRepository::findAll(int limit) {
    pqxx::work txn(DatabaseConnection::get());
    auto r = txn.exec_params(
        "SELECT id, owner_id, home_junction_id, typical_route::text, notification_opt_in, created_at::text "
        "FROM driver_profiles ORDER BY id LIMIT $1",
        limit);
    std::vector<DriverProfile> list;
    list.reserve(r.size());
    for (const auto& row : r) list.push_back(rowToProfile(row));
    return list;
}

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

std::vector<DriverProfile> DriverProfileRepository::findOptedInByRouteOverlap(const std::vector<int>& junctionIds, int limit) {
    pqxx::work txn(DatabaseConnection::get());
    const std::string arr = toPgIntArray(junctionIds);
    auto r = txn.exec_params(
        "SELECT id, owner_id, home_junction_id, typical_route::text, notification_opt_in, created_at::text "
        "FROM driver_profiles "
        "WHERE notification_opt_in = TRUE "
        "  AND typical_route && $1::int[] "
        "ORDER BY id LIMIT $2",
        arr, limit);
    std::vector<DriverProfile> list;
    list.reserve(r.size());
    for (const auto& row : r) list.push_back(rowToProfile(row));
    return list;
}

