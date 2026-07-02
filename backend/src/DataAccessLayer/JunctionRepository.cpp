#include "JunctionRepository.h"

bool JunctionRepository::existsById(int junctionId) {
    pqxx::work txn(DatabaseConnection::get());
    auto r = txn.exec_params(
        "SELECT 1 FROM junctions WHERE id = $1 AND is_active = TRUE", junctionId);
    return !r.empty();
}

std::optional<Junction> JunctionRepository::findById(int junctionId) {
    pqxx::work txn(DatabaseConnection::get());
    auto r = txn.exec_params(
        "SELECT id, name, location, speed_limit_kmh, is_active, "
        "       COALESCE(latitude, 0), COALESCE(longitude, 0) "
        "FROM junctions WHERE id = $1",
        junctionId);
    if (r.empty()) return std::nullopt;
    Junction j;
    j.id            = r[0][0].as<int>();
    j.name          = r[0][1].as<std::string>();
    j.location      = r[0][2].as<std::string>();
    j.speedLimitKmh = r[0][3].as<int>();
    j.isActive      = r[0][4].as<bool>();
    j.latitude      = r[0][5].as<double>();
    j.longitude     = r[0][6].as<double>();
    return j;
}

std::vector<Junction> JunctionRepository::findAll() {
    pqxx::work txn(DatabaseConnection::get());
    auto r = txn.exec(
        "SELECT id, name, location, speed_limit_kmh, is_active, "
        "       COALESCE(latitude, 0), COALESCE(longitude, 0) "
        "FROM junctions ORDER BY id");
    std::vector<Junction> list;
    list.reserve(r.size());
    for (const auto& row : r) {
        Junction j;
        j.id            = row[0].as<int>();
        j.name          = row[1].as<std::string>();
        j.location      = row[2].as<std::string>();
        j.speedLimitKmh = row[3].as<int>();
        j.isActive      = row[4].as<bool>();
        j.latitude      = row[5].as<double>();
        j.longitude     = row[6].as<double>();
        list.push_back(std::move(j));
    }
    return list;
}

int JunctionRepository::save(const std::string& name,
                               const std::string& location,
                               int speedLimitKmh,
                               double latitude,
                               double longitude) {
    pqxx::work txn(DatabaseConnection::get());
    auto r = txn.exec_params(
        "INSERT INTO junctions (name, location, speed_limit_kmh, latitude, longitude) "
        "VALUES ($1, $2, $3, $4, $5) RETURNING id",
        name, location, speedLimitKmh, latitude, longitude);
    txn.commit();
    return r[0][0].as<int>();
}
