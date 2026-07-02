#pragma once
#include <string>
#include <optional>
#include "DatabaseConnection.h"
#include "../Models.h"

class OwnerRepository {
public:
    int                  save(const std::string& fullName,
                              const std::string& email,
                              const std::string& phone);
    std::optional<Owner> findByEmail(const std::string& email);
    std::optional<Owner> findById(int id);
    int                  findOrCreate(const std::string& fullName,
                                      const std::string& email,
                                      const std::string& phone);
};
