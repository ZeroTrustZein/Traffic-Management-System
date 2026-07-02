#include "ViolationTypeRepository.h"

namespace {
ViolationType rowToType(const pqxx::row& row) {
    ViolationType vt;
    vt.id            = row[0].as<int>();
    vt.name          = row[1].as<std::string>();
    vt.code          = row[2].as<std::string>();
    vt.baseFine      = row[3].as<double>();
    vt.description   = row[4].is_null() ? "" : row[4].as<std::string>();
    vt.isActive      = row[5].as<bool>();
    vt.multLow       = row[6].as<double>();
    vt.multMedium    = row[7].as<double>();
    vt.multHigh      = row[8].as<double>();
    vt.multCritical  = row[9].as<double>();
    vt.autoNotify    = row[10].as<bool>();   // W13
    vt.requiresReview = row[11].as<bool>();  // W13
    return vt;
}
const char* kSelect =
    "SELECT id, name, code, base_fine, description, is_active, "
    "       multiplier_low, multiplier_medium, multiplier_high, multiplier_critical, "
    "       auto_notify, requires_review "
    "FROM violation_types ";
}

std::optional<ViolationType> ViolationTypeRepository::findByCode(const std::string& code) {
    pqxx::work txn(DatabaseConnection::get());
    auto r = txn.exec_params(std::string(kSelect) + "WHERE code = $1", code);
    if (r.empty()) return std::nullopt;
    return rowToType(r[0]);
}

std::optional<ViolationType> ViolationTypeRepository::findById(int id) {
    pqxx::work txn(DatabaseConnection::get());
    auto r = txn.exec_params(std::string(kSelect) + "WHERE id = $1", id);
    if (r.empty()) return std::nullopt;
    return rowToType(r[0]);
}

std::vector<ViolationType> ViolationTypeRepository::findAll() {
    pqxx::work txn(DatabaseConnection::get());
    auto r = txn.exec(std::string(kSelect) + "ORDER BY id");
    std::vector<ViolationType> list;
    list.reserve(r.size());
    for (const auto& row : r) list.push_back(rowToType(row));
    return list;
}
