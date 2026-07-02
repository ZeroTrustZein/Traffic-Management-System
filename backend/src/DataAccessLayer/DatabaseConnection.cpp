#include "DatabaseConnection.h"
#include <stdexcept>

std::string DatabaseConnection::s_connStr;

void DatabaseConnection::initialize(const std::string& connStr) {
    s_connStr = connStr;
}

pqxx::connection& DatabaseConnection::get() {
    if (s_connStr.empty())
        throw std::runtime_error("DatabaseConnection::initialize() was not called");
    thread_local pqxx::connection conn(s_connStr);
    return conn;
}
