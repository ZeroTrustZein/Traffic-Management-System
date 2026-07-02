#include "CongestionRepository.h"
#include <map>

namespace {
CongestionRecord rowToRecord(const pqxx::row& row) {
    CongestionRecord cr;
    cr.id              = row[0].as<int>();
    cr.junctionId      = row[1].as<int>();
    cr.timeWindowStart = row[2].as<std::string>();
    cr.timeWindowEnd   = row[3].as<std::string>();
    cr.vehicleCount    = row[4].as<int>();
    cr.congestionLevel = row[5].as<std::string>();
    cr.recordedAt      = row[6].as<std::string>();
    return cr;
}
const char* kSelect =
    "SELECT id, junction_id, time_window_start::text, time_window_end::text, "
    "       vehicle_count, congestion_level, recorded_at::text FROM congestion_records ";
}

int CongestionRepository::save(int junctionId,
                                 const std::string& windowStart,
                                 const std::string& windowEnd,
                                 int vehicleCount,
                                 const std::string& congestionLevel) {
    pqxx::work txn(DatabaseConnection::get());
    auto r = txn.exec_params(
        "INSERT INTO congestion_records "
        "(junction_id, time_window_start, time_window_end, vehicle_count, congestion_level) "
        "VALUES ($1, $2::timestamp, $3::timestamp, $4, $5) RETURNING id",
        junctionId, windowStart, windowEnd, vehicleCount, congestionLevel);
    txn.commit();
    return r[0][0].as<int>();
}

std::vector<CongestionRecord> CongestionRepository::findAll(int limit) {
    pqxx::work txn(DatabaseConnection::get());
    auto r = txn.exec_params(
        std::string(kSelect) + "ORDER BY time_window_start DESC LIMIT $1", limit);
    std::vector<CongestionRecord> list;
    list.reserve(r.size());
    for (const auto& row : r) list.push_back(rowToRecord(row));
    return list;
}

std::vector<CongestionRecord> CongestionRepository::findByJunctionId(int junctionId, int limit) {
    pqxx::work txn(DatabaseConnection::get());
    auto r = txn.exec_params(
        std::string(kSelect) + "WHERE junction_id=$1 ORDER BY time_window_start DESC LIMIT $2",
        junctionId, limit);
    std::vector<CongestionRecord> list;
    list.reserve(r.size());
    for (const auto& row : r) list.push_back(rowToRecord(row));
    return list;
}

double CongestionRepository::averageRecentCount(int junctionId, int windowCount) {
    pqxx::work txn(DatabaseConnection::get());
    auto r = txn.exec_params(
        "SELECT COALESCE(AVG(vehicle_count), 0) FROM ("
        "  SELECT vehicle_count FROM congestion_records "
        "  WHERE junction_id = $1 ORDER BY time_window_start DESC LIMIT $2"
        ") sub",
        junctionId, windowCount);
    return r[0][0].as<double>();
}

double CongestionRepository::averageCountForHour(int junctionId, int hourOfDay) {
    pqxx::work txn(DatabaseConnection::get());
    auto r = txn.exec_params(
        "SELECT COALESCE(AVG(vehicle_count), -1) "
        "FROM congestion_records "
        "WHERE junction_id = $1 "
        "  AND EXTRACT(HOUR FROM time_window_start) = $2",
        junctionId, hourOfDay);
    return r[0][0].as<double>();
}

std::vector<CongestionRepository::JunctionSummary>
CongestionRepository::congestionProneSummary() {
    pqxx::work txn(DatabaseConnection::get());
    auto r = txn.exec(
        "SELECT j.id, j.name, "
        "       COALESCE(AVG(cr.vehicle_count), 0) AS avg_count, "
        "       COALESCE(MAX(cr.congestion_level), 'LOW') AS worst "
        "FROM junctions j "
        "LEFT JOIN congestion_records cr ON cr.junction_id = j.id "
        "GROUP BY j.id, j.name "
        "ORDER BY avg_count DESC");

    std::vector<JunctionSummary> list;
    list.reserve(r.size());
    for (const auto& row : r) {
        JunctionSummary s;
        s.junctionId  = row[0].as<int>();
        s.name        = row[1].as<std::string>();
        s.avgCount    = row[2].as<double>();
        s.worstLevel  = row[3].as<std::string>();
        list.push_back(std::move(s));
    }
    return list;
}

int CongestionRepository::countLogsInWindow(int junctionId, int secondsBack) {
    pqxx::work txn(DatabaseConnection::get());
    auto r = txn.exec_params(
        "SELECT COUNT(*) FROM plate_logs "
        "WHERE junction_id = $1 "
        "  AND detected_at >= NOW() - ($2 || ' seconds')::interval",
        junctionId, secondsBack);
    return r[0][0].as<int>();
}

std::vector<CongestionRepository::HourlyJunction> CongestionRepository::hourlyProfile() {
    pqxx::work txn(DatabaseConnection::get());
    auto r = txn.exec(
        "SELECT cr.junction_id, j.name, "
        "       EXTRACT(HOUR FROM cr.time_window_start)::int AS hour_of_day, "
        "       ROUND(AVG(cr.vehicle_count)::numeric, 1)::float AS avg_count "
        "FROM congestion_records cr "
        "JOIN junctions j ON j.id = cr.junction_id "
        "GROUP BY cr.junction_id, j.name, "
        "         EXTRACT(HOUR FROM cr.time_window_start) "
        "ORDER BY cr.junction_id, hour_of_day");

    std::map<int, HourlyJunction> byJunction;
    for (const auto& row : r) {
        int jid = row[0].as<int>();
        auto& hj = byJunction[jid];
        hj.junctionId = jid;
        hj.name       = row[1].as<std::string>();
        hj.hours.push_back({ row[2].as<int>(), row[3].as<double>() });
    }

    std::vector<HourlyJunction> list;
    list.reserve(byJunction.size());
    for (auto& [id, hj] : byJunction)
        list.push_back(std::move(hj));
    return list;
}
