#include "EmergencyVehicleRepository.h"

static EmergencyVehicle rowToVehicle(const pqxx::row& row) {
    EmergencyVehicle v;
    v.id         = row[0].as<int>();
    v.type       = row[1].as<std::string>();
    v.identifier = row[2].as<std::string>();
    v.plate      = row[3].is_null() ? "" : row[3].as<std::string>();
    v.isActive   = row[4].as<bool>();
    v.createdAt  = row[5].as<std::string>();
    return v;
}

int EmergencyVehicleRepository::upsert(const std::string& type,
                                        const std::string& identifier,
                                        bool isActive,
                                        const std::string& plate) {
    pqxx::work txn(DatabaseConnection::get());
    pqxx::result r;
    if (!plate.empty()) {
        r = txn.exec_params(
            "INSERT INTO emergency_vehicles (type, identifier, plate, is_active) "
            "VALUES ($1, $2, $3, $4) "
            "ON CONFLICT (identifier) DO UPDATE "
            "SET type = EXCLUDED.type, plate = EXCLUDED.plate, is_active = EXCLUDED.is_active "
            "RETURNING id",
            type, identifier, plate, isActive);
    } else {
        r = txn.exec_params(
            "INSERT INTO emergency_vehicles (type, identifier, is_active) "
            "VALUES ($1, $2, $3) "
            "ON CONFLICT (identifier) DO UPDATE "
            "SET type = EXCLUDED.type, is_active = EXCLUDED.is_active "
            "RETURNING id",
            type, identifier, isActive);
    }
    txn.commit();
    return r[0][0].as<int>();
}

std::optional<EmergencyVehicle> EmergencyVehicleRepository::findById(int id) {
    pqxx::work txn(DatabaseConnection::get());
    auto r = txn.exec_params(
        "SELECT id, type, identifier, plate, is_active, created_at::text "
        "FROM emergency_vehicles WHERE id = $1",
        id);
    if (r.empty()) return std::nullopt;
    return rowToVehicle(r[0]);
}

std::vector<EmergencyVehicle> EmergencyVehicleRepository::findAll(int limit) {
    pqxx::work txn(DatabaseConnection::get());
    auto r = txn.exec_params(
        "SELECT id, type, identifier, plate, is_active, created_at::text "
        "FROM emergency_vehicles ORDER BY id LIMIT $1",
        limit);
    std::vector<EmergencyVehicle> list;
    list.reserve(r.size());
    for (const auto& row : r) list.push_back(rowToVehicle(row));
    return list;
}

bool EmergencyVehicleRepository::existsByPlate(const std::string& plate) {
    if (plate.empty()) return false;
    pqxx::work txn(DatabaseConnection::get());
    auto r = txn.exec_params(
        "SELECT 1 FROM emergency_vehicles WHERE plate = $1 AND is_active = TRUE LIMIT 1",
        plate);
    return !r.empty();
}
