#include "FineRepository.h"

namespace {
Fine rowToFine(const pqxx::row& row) {
    Fine f;
    f.id          = row[0].as<int>();
    f.violationId = row[1].as<int>();
    f.amount      = row[2].as<double>();
    f.status      = fineStatusFrom(row[3].as<std::string>());
    f.issuedAt    = row[4].as<std::string>();
    f.dueDate     = row[5].is_null() ? "" : row[5].as<std::string>();
    f.paidAt      = row[6].is_null() ? "" : row[6].as<std::string>();
    return f;
}
const char* kSelect =
    "SELECT id, violation_id, amount, status, issued_at::text, "
    "       due_date::text, paid_at::text FROM fines ";
}

int FineRepository::save(int violationId, double amount) {
    pqxx::work txn(DatabaseConnection::get());
    auto r = txn.exec_params(
        "INSERT INTO fines (violation_id, amount, due_date) "
        "VALUES ($1, $2, NOW() + INTERVAL '30 days') RETURNING id",
        violationId, amount);
    txn.commit();
    return r[0][0].as<int>();
}

std::optional<Fine> FineRepository::findById(int id) {
    pqxx::work txn(DatabaseConnection::get());
    auto r = txn.exec_params(std::string(kSelect) + "WHERE id = $1", id);
    if (r.empty()) return std::nullopt;
    return rowToFine(r[0]);
}

std::optional<Fine> FineRepository::findByViolationId(int violationId) {
    pqxx::work txn(DatabaseConnection::get());
    auto r = txn.exec_params(std::string(kSelect) + "WHERE violation_id = $1", violationId);
    if (r.empty()) return std::nullopt;
    return rowToFine(r[0]);
}

std::vector<Fine> FineRepository::findAll(int limit) {
    pqxx::work txn(DatabaseConnection::get());
    auto r = txn.exec_params(
        std::string(kSelect) + "ORDER BY issued_at DESC LIMIT $1", limit);
    std::vector<Fine> list;
    list.reserve(r.size());
    for (const auto& row : r) list.push_back(rowToFine(row));
    return list;
}

bool FineRepository::updateStatus(int id, const std::string& status) {
    pqxx::work txn(DatabaseConnection::get());
    pqxx::result r;
    if (status == "PAID") {
        r = txn.exec_params(
            "UPDATE fines SET status=$1, paid_at=NOW() WHERE id=$2", status, id);
    } else {
        r = txn.exec_params(
            "UPDATE fines SET status=$1 WHERE id=$2", status, id);
    }
    txn.commit();
    return r.affected_rows() > 0;
}
