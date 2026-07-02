#pragma once
#include <string>
#include <vector>
#include "../ValidationResult.h"

class VehicleValidator {
public:
    static ValidationResult validatePlate(const std::string& plate);
    static ValidationResult validateOwnerName(const std::string& name);
    static ValidationResult validateOwnerEmail(const std::string& email);
    static ValidationResult validateOwnerPhone(const std::string& phone);
    static ValidationResult validateVehicleType(const std::string& type);

    static std::vector<ValidationResult> validateAll(const std::string& plate,
                                                      const std::string& ownerName,
                                                      const std::string& ownerEmail,
                                                      const std::string& ownerPhone,
                                                      const std::string& vehicleType);
};
