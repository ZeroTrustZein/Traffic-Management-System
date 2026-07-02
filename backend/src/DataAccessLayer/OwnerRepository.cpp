#include "OwnerRepository.h"

int OwnerRepository::save(const std::string& fullName,
                           const std::string& email,
                           const std::string& phone) {
    pqxx::work txn(DatabaseConnection::get());
    pqxx::result r;
    if (phone.empty()) {
        r = txn.exec_params(
            "INSERT INTO owners (full_name, email) VALUES ($1, $2) RETURNING id",
            fullName, email);
    } else {
        r = txn.exec_params(
            "INSERT INTO owners (full_name, email, phone) VALUES ($1, $2, $3) RETURNING id",
            fullName, email, phone);
    }
    txn.commit();
    return r[0][0].as<int>();
}

std::optional<Owner> OwnerRepository::findByEmail(const std::string& email) {
    pqxx::work txn(DatabaseConnection::get());
    auto r = txn.exec_params(
        "SELECT id, full_name, email, COALESCE(phone,''), created_at::text "
        "FROM owners WHERE email = $1",
        email);
    if (r.empty()) return std::nullopt;
    Owner o;
    o.id        = r[0][0].as<int>();
    o.fullName  = r[0][1].as<std::string>();
    o.email     = r[0][2].as<std::string>();
    o.phone     = r[0][3].as<std::string>();
    o.createdAt = r[0][4].as<std::string>();
    return o;
}

std::optional<Owner> OwnerRepository::findById(int id) {
    pqxx::work txn(DatabaseConnection::get());
    auto r = txn.exec_params(
        "SELECT id, full_name, email, COALESCE(phone,''), created_at::text "
        "FROM owners WHERE id = $1",
        id);
    if (r.empty()) return std::nullopt;
    Owner o;
    o.id        = r[0][0].as<int>();
    o.fullName  = r[0][1].as<std::string>();
    o.email     = r[0][2].as<std::string>();
    o.phone     = r[0][3].as<std::string>();
    o.createdAt = r[0][4].as<std::string>();
    return o;
}

int OwnerRepository::findOrCreate(const std::string& fullName,
                                   const std::string& email,
                                   const std::string& phone) {
    auto existing = findByEmail(email);
    if (existing) return existing->id;

    pqxx::work txn(DatabaseConnection::get());
    pqxx::result r;
    if (phone.empty()) {
        r = txn.exec_params(
            "INSERT INTO owners (full_name, email) VALUES ($1, $2) RETURNING id",
            fullName, email);
    } else {
        r = txn.exec_params(
            "INSERT INTO owners (full_name, email, phone) VALUES ($1, $2, $3) RETURNING id",
            fullName, email, phone);
    }
    txn.commit();
    return r[0][0].as<int>();
}
