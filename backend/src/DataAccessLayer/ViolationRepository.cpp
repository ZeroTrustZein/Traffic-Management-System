#include "ViolationRepository.h"

namespace {
Violation rowToViolation(const pqxx::row& row) {
    Violation v;
    v.id              = row[0].as<int>();
    v.vehicleId       = row[1].as<int>();
    v.junctionId      = row[2].is_null() ? 0 : row[2].as<int>();
    v.violationTypeId = row[3].as<int>();
    v.plateLogId      = row[4].is_null() ? 0 : row[4].as<int>();
    v.detectedAt      = row[5].as<std::string>();
    v.severity        = severityFrom(row[6].as<std::string>());
    v.evidenceNote    = row[7].is_null() ? "" : row[7].as<std::string>();
    v.status          = row[8].as<std::string>();
    return v;
}
const char* kSelect =
    "SELECT id, vehicle_id, junction_id, violation_type_id, plate_log_id, "
    "       detected_at::text, severity, evidence_note, status "
    "FROM violations ";
}

int ViolationRepository::save(int vehicleId, int junctionId, int violationTypeId,
                                int plateLogId, const std::string& severity,
                                const std::string& evidenceNote,
                                const std::string& status) {
    pqxx::work txn(DatabaseConnection::get());
    pqxx::result r;
    if (junctionId > 0 && plateLogId > 0) {
        r = txn.exec_params(
            "INSERT INTO violations "
            "(vehicle_id, junction_id, violation_type_id, plate_log_id, severity, evidence_note, status) "
            "VALUES ($1,$2,$3,$4,$5,$6,$7) RETURNING id",
            vehicleId, junctionId, violationTypeId, plateLogId, severity, evidenceNote, status);
    } else {
        r = txn.exec_params(
            "INSERT INTO violations "
            "(vehicle_id, violation_type_id, severity, evidence_note, status) "
            "VALUES ($1,$2,$3,$4,$5) RETURNING id",
            vehicleId, violationTypeId, severity, evidenceNote, status);
    }
    txn.commit();
    return r[0][0].as<int>();
}

std::optional<Violation> ViolationRepository::findById(int id) {
    pqxx::work txn(DatabaseConnection::get());
    auto r = txn.exec_params(std::string(kSelect) + "WHERE id = $1", id);
    if (r.empty()) return std::nullopt;
    return rowToViolation(r[0]);
}

std::vector<Violation> ViolationRepository::findAll(int limit) {
    pqxx::work txn(DatabaseConnection::get());
    auto r = txn.exec_params(
        std::string(kSelect) + "ORDER BY detected_at DESC LIMIT $1", limit);
    std::vector<Violation> list;
    list.reserve(r.size());
    for (const auto& row : r) list.push_back(rowToViolation(row));
    return list;
}

std::vector<Violation> ViolationRepository::findByVehicleId(int vehicleId) {
    pqxx::work txn(DatabaseConnection::get());
    auto r = txn.exec_params(
        std::string(kSelect) + "WHERE vehicle_id = $1 ORDER BY detected_at DESC",
        vehicleId);
    std::vector<Violation> list;
    list.reserve(r.size());
    for (const auto& row : r) list.push_back(rowToViolation(row));
    return list;
}

bool ViolationRepository::updateStatus(int id, const std::string& status) {
    pqxx::work txn(DatabaseConnection::get());
    auto r = txn.exec_params(
        "UPDATE violations SET status = $1 WHERE id = $2", status, id);
    txn.commit();
    return r.affected_rows() > 0;
}

int ViolationRepository::countByVehicle(int vehicleId) {
    pqxx::work txn(DatabaseConnection::get());
    auto r = txn.exec_params(
        "SELECT COUNT(*) FROM violations WHERE vehicle_id = $1",
        vehicleId);
    return r[0][0].as<int>();
}
