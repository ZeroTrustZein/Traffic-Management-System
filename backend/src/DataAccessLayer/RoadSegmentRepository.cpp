#include "RoadSegmentRepository.h"

namespace {
RoadSegment rowToSegment(const pqxx::row& row) {
    RoadSegment s;
    s.id             = row[0].as<int>();
    s.fromJunctionId = row[1].as<int>();
    s.toJunctionId   = row[2].as<int>();
    s.name           = row[3].as<std::string>();
    s.distanceKm     = row[4].as<double>();
    s.speedLimitKmh  = row[5].as<int>();
    return s;
}
const char* kSelect =
    "SELECT id, from_junction_id, to_junction_id, name, "
    "       distance_km::float, speed_limit_kmh FROM road_segments ";
}

std::vector<RoadSegment> RoadSegmentRepository::findAll() {
    pqxx::work txn(DatabaseConnection::get());
    auto r = txn.exec(std::string(kSelect) + "ORDER BY id");
    std::vector<RoadSegment> list;
    list.reserve(r.size());
    for (const auto& row : r) list.push_back(rowToSegment(row));
    return list;
}

std::vector<RoadSegment> RoadSegmentRepository::findOutgoing(int fromJunctionId) {
    pqxx::work txn(DatabaseConnection::get());
    auto r = txn.exec_params(
        std::string(kSelect) + "WHERE from_junction_id = $1 ORDER BY id",
        fromJunctionId);
    std::vector<RoadSegment> list;
    list.reserve(r.size());
    for (const auto& row : r) list.push_back(rowToSegment(row));
    return list;
}
