#include "NotificationRepository.h"

static NotificationLog rowToNotif(const pqxx::row& row) {
    NotificationLog n;
    n.id       = row[0].as<int>();
    n.ownerId  = row[1].is_null() ? 0 : row[1].as<int>();
    n.category = row[2].as<std::string>();
    n.subject  = row[3].is_null() ? "" : row[3].as<std::string>();
    n.message  = row[4].as<std::string>();
    n.sentAt   = row[5].as<std::string>();
    n.status   = row[6].as<std::string>();
    return n;
}

int NotificationRepository::create(int ownerId,
                                    const std::string& category,
                                    const std::string& subject,
                                    const std::string& message,
                                    const std::string& status) {
    pqxx::work txn(DatabaseConnection::get());
    pqxx::result r;
    if (ownerId > 0) {
        r = txn.exec_params(
            "INSERT INTO notification_logs (owner_id, category, subject, message, status) "
            "VALUES ($1, $2, $3, $4, $5) RETURNING id",
            ownerId, category, subject, message, status);
    } else {
        r = txn.exec_params(
            "INSERT INTO notification_logs (owner_id, category, subject, message, status) "
            "VALUES (NULL, $1, $2, $3, $4) RETURNING id",
            category, subject, message, status);
    }
    txn.commit();
    return r[0][0].as<int>();
}

std::vector<NotificationLog> NotificationRepository::findAll(int limit) {
    pqxx::work txn(DatabaseConnection::get());
    auto r = txn.exec_params(
        "SELECT id, owner_id, category, subject, message, sent_at::text, status "
        "FROM notification_logs ORDER BY sent_at DESC LIMIT $1",
        limit);
    std::vector<NotificationLog> list;
    list.reserve(r.size());
    for (const auto& row : r) list.push_back(rowToNotif(row));
    return list;
}
