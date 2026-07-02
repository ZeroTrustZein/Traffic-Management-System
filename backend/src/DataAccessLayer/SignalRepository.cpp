#include "SignalRepository.h"

static JunctionSignal rowToSignal(const pqxx::row& row) {
    JunctionSignal s;
    s.junctionId       = row[0].as<int>();
    s.state            = row[1].as<std::string>();
    s.emergencyEventId = row[2].is_null() ? 0 : row[2].as<int>();
    s.updatedAt        = row[3].as<std::string>();
    return s;
}

std::vector<JunctionSignal> SignalRepository::findAll() {
    pqxx::work txn(DatabaseConnection::get());
    auto r = txn.exec(
        "SELECT junction_id, state, emergency_event_id, updated_at::text "
        "FROM junction_signals ORDER BY junction_id");
    std::vector<JunctionSignal> list;
    list.reserve(r.size());
    for (const auto& row : r) list.push_back(rowToSignal(row));
    return list;
}

void SignalRepository::setPriorityForRoute(const std::vector<int>& junctionIds, int eventId) {
    if (junctionIds.empty()) return;
    pqxx::work txn(DatabaseConnection::get());
    for (int jid : junctionIds) {
        txn.exec_params(
            "UPDATE junction_signals "
            "SET state = 'GREEN_PRIORITY', emergency_event_id = $2, updated_at = NOW() "
            "WHERE junction_id = $1",
            jid, eventId);
    }
    txn.commit();
}

void SignalRepository::releaseByEvent(int eventId) {
    pqxx::work txn(DatabaseConnection::get());
    txn.exec_params(
        "UPDATE junction_signals "
        "SET state = 'NORMAL', emergency_event_id = NULL, updated_at = NOW() "
        "WHERE emergency_event_id = $1",
        eventId);
    txn.commit();
}

void SignalRepository::setTransientPriority(int junctionId) {
    pqxx::work txn(DatabaseConnection::get());
    txn.exec_params(
        "UPDATE junction_signals "
        "SET state = 'GREEN_PRIORITY', emergency_event_id = NULL, updated_at = NOW() "
        "WHERE junction_id = $1",
        junctionId);
    txn.commit();
}
