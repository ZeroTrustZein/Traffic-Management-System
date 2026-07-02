#pragma once
#include <vector>
#include <optional>
#include "DatabaseConnection.h"
#include "../Models.h"

class JunctionRepository {
public:
    bool                     existsById(int junctionId);
    std::optional<Junction>  findById(int junctionId);
    std::vector<Junction>    findAll();
    int                      save(const std::string& name,
                                  const std::string& location,
                                  int speedLimitKmh,
                                  double latitude = 0.0,
                                  double longitude = 0.0);
};
