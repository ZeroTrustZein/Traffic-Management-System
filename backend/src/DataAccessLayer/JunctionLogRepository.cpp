#include "JunctionLogRepository.h"

int JunctionLogRepository::save(int junctionId, int vehicleId,
                                  const std::string& numberPlate,
                                  const std::string& eventType,
                                  const std::string& status,
                                  double speedKmh, bool hasSpeed) {
    pqxx::work txn(DatabaseConnection::get());
    pqxx::result r;
    if (hasSpeed) {
        r = txn.exec_params(
            "INSERT INTO plate_logs (junction_id, vehicle_id, number_plate, event_type, status, speed_kmh) "
            "VALUES ($1, $2, $3, $4, $5, $6) RETURNING id",
            junctionId, vehicleId, numberPlate, eventType, status, speedKmh);
    } else {
        r = txn.exec_params(
            "INSERT INTO plate_logs (junction_id, vehicle_id, number_plate, event_type, status) "
            "VALUES ($1, $2, $3, $4, $5) RETURNING id",
            junctionId, vehicleId, numberPlate, eventType, status);
    }
    txn.commit();
    return r[0][0].as<int>();
}

int JunctionLogRepository::saveUnregistered(int junctionId,
                                              const std::string& numberPlate) {
    pqxx::work txn(DatabaseConnection::get());
    auto r = txn.exec_params(
        "INSERT INTO plate_logs (junction_id, vehicle_id, number_plate, event_type, status) "
        "VALUES ($1, NULL, $2, 'PASSAGE', 'UNREGISTERED') RETURNING id",
        junctionId, numberPlate);
    txn.commit();
    return r[0][0].as<int>();
}

std::vector<PlateLog> JunctionLogRepository::findByJunctionId(int junctionId, int limit) {
    pqxx::work txn(DatabaseConnection::get());
    auto r = txn.exec_params(
        "SELECT id, COALESCE(vehicle_id,0), junction_id, number_plate, "
        "       COALESCE(speed_kmh::text,''), "
        "       detected_at::text, status, event_type "
        "FROM plate_logs WHERE junction_id = $1 "
        "ORDER BY detected_at DESC LIMIT $2",
        junctionId, limit);

    std::vector<PlateLog> logs;
    logs.reserve(r.size());
    for (const auto& row : r) {
        PlateLog pl;
        pl.id          = row[0].as<int>();
        pl.vehicleId   = row[1].as<int>();
        pl.junctionId  = row[2].as<int>();
        pl.numberPlate = row[3].as<std::string>();
        const std::string spd = row[4].as<std::string>();
        if (!spd.empty()) { pl.speedKmh = std::stod(spd); pl.hasSpeed = true; }
        pl.detectedAt  = row[5].as<std::string>();
        pl.status      = logStatusFrom(row[6].as<std::string>());
        pl.eventType   = eventTypeFrom(row[7].as<std::string>());
        logs.push_back(std::move(pl));
    }
    return logs;
}

std::vector<PlateLog> JunctionLogRepository::findAll(int limit) {
    pqxx::work txn(DatabaseConnection::get());
    auto r = txn.exec_params(
        "SELECT id, COALESCE(vehicle_id,0), junction_id, number_plate, "
        "       COALESCE(speed_kmh::text,''), "
        "       detected_at::text, status, event_type "
        "FROM plate_logs ORDER BY detected_at DESC LIMIT $1",
        limit);

    std::vector<PlateLog> logs;
    logs.reserve(r.size());
    for (const auto& row : r) {
        PlateLog pl;
        pl.id          = row[0].as<int>();
        pl.vehicleId   = row[1].as<int>();
        pl.junctionId  = row[2].as<int>();
        pl.numberPlate = row[3].as<std::string>();
        const std::string spd = row[4].as<std::string>();
        if (!spd.empty()) { pl.speedKmh = std::stod(spd); pl.hasSpeed = true; }
        pl.detectedAt  = row[5].as<std::string>();
        pl.status      = logStatusFrom(row[6].as<std::string>());
        pl.eventType   = eventTypeFrom(row[7].as<std::string>());
        logs.push_back(std::move(pl));
    }
    return logs;
}

std::vector<PlateLog> JunctionLogRepository::findUnprocessedViolationLogs(int junctionId,
                                                                             int hoursBack) {
    pqxx::work txn(DatabaseConnection::get());
    // Violation-triggering logs that have no violations record yet
    auto r = txn.exec_params(
        "SELECT pl.id, COALESCE(pl.vehicle_id,0), pl.junction_id, pl.number_plate, "
        "       COALESCE(pl.speed_kmh::text,''), "
        "       pl.detected_at::text, pl.status, pl.event_type "
        "FROM plate_logs pl "
        "LEFT JOIN violations v ON v.plate_log_id = pl.id "
        "WHERE pl.junction_id = $1 "
        // UNREGISTERED is a status, never an event_type, so it is not listed here.
        "  AND pl.event_type IN ('SPEEDING','RED_LIGHT','PARKING') "
        "  AND pl.detected_at >= NOW() - ($2 || ' hours')::interval "
        "  AND v.id IS NULL "
        "ORDER BY pl.detected_at",
        junctionId, hoursBack);

    std::vector<PlateLog> logs;
    logs.reserve(r.size());
    for (const auto& row : r) {
        PlateLog pl;
        pl.id          = row[0].as<int>();
        pl.vehicleId   = row[1].as<int>();
        pl.junctionId  = row[2].as<int>();
        pl.numberPlate = row[3].as<std::string>();
        const std::string spd = row[4].as<std::string>();
        if (!spd.empty()) { pl.speedKmh = std::stod(spd); pl.hasSpeed = true; }
        pl.detectedAt  = row[5].as<std::string>();
        pl.status      = logStatusFrom(row[6].as<std::string>());
        pl.eventType   = eventTypeFrom(row[7].as<std::string>());
        logs.push_back(std::move(pl));
    }
    return logs;
}
