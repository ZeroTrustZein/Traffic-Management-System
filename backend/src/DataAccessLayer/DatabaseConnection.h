#pragma once
#include <string>
#include <pqxx/pqxx>

// Thread-safe: each OS thread gets its own pqxx::connection via thread_local,
// so concurrent requests never share a connection.
class DatabaseConnection {
public:
    static void initialize(const std::string& connStr);
    static pqxx::connection& get();

private:
    static std::string s_connStr;
};
