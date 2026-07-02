#include "VehicleRepository.h"

int VehicleRepository::save(const std::string& numberPlate,
                              const std::string& vehicleType,
                              int ownerId) {
    pqxx::work txn(DatabaseConnection::get());
    auto r = txn.exec_params(
        "INSERT INTO vehicles (number_plate, vehicle_type, owner_id) "
        "VALUES ($1, $2, $3) RETURNING id",
        numberPlate, vehicleType, ownerId);
    txn.commit();
    return r[0][0].as<int>();
}

bool VehicleRepository::existsByPlate(const std::string& plate) {
    pqxx::work txn(DatabaseConnection::get());
    auto r = txn.exec_params(
        "SELECT 1 FROM vehicles WHERE number_plate = $1", plate);
    return !r.empty();
}

std::optional<Vehicle> VehicleRepository::findByPlate(const std::string& plate) {
    pqxx::work txn(DatabaseConnection::get());
    auto r = txn.exec_params(
        "SELECT id, number_plate, vehicle_type, COALESCE(owner_id,0), is_active, created_at::text "
        "FROM vehicles WHERE number_plate = $1",
        plate);
    if (r.empty()) return std::nullopt;
    Vehicle v;
    v.id          = r[0][0].as<int>();
    v.numberPlate = r[0][1].as<std::string>();
    v.vehicleType = vehicleTypeFrom(r[0][2].as<std::string>());
    v.ownerId     = r[0][3].as<int>();
    v.isActive    = r[0][4].as<bool>();
    v.createdAt   = r[0][5].as<std::string>();
    return v;
}

std::optional<Vehicle> VehicleRepository::findById(int id) {
    pqxx::work txn(DatabaseConnection::get());
    auto r = txn.exec_params(
        "SELECT id, number_plate, vehicle_type, COALESCE(owner_id,0), is_active, created_at::text "
        "FROM vehicles WHERE id = $1",
        id);
    if (r.empty()) return std::nullopt;
    Vehicle v;
    v.id          = r[0][0].as<int>();
    v.numberPlate = r[0][1].as<std::string>();
    v.vehicleType = vehicleTypeFrom(r[0][2].as<std::string>());
    v.ownerId     = r[0][3].as<int>();
    v.isActive    = r[0][4].as<bool>();
    v.createdAt   = r[0][5].as<std::string>();
    return v;
}

std::vector<Vehicle> VehicleRepository::findAll(int limit) {
    pqxx::work txn(DatabaseConnection::get());
    auto r = txn.exec_params(
        "SELECT id, number_plate, vehicle_type, COALESCE(owner_id,0), is_active, created_at::text "
        "FROM vehicles ORDER BY id DESC LIMIT $1",
        limit);
    std::vector<Vehicle> list;
    list.reserve(r.size());
    for (const auto& row : r) {
        Vehicle v;
        v.id          = row[0].as<int>();
        v.numberPlate = row[1].as<std::string>();
        v.vehicleType = vehicleTypeFrom(row[2].as<std::string>());
        v.ownerId     = row[3].as<int>();
        v.isActive    = row[4].as<bool>();
        v.createdAt   = row[5].as<std::string>();
        list.push_back(std::move(v));
    }
    return list;
}
