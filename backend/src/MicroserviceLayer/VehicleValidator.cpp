#include "VehicleValidator.h"
#include "PlateValidator.h"
#include <regex>
#include <set>

ValidationResult VehicleValidator::validatePlate(const std::string& plate) {
    if (!PlateValidator::validate(plate))
        return ValidationResult::fail("plate", "Plate must match format AB12CDE");
    return ValidationResult::ok();
}

ValidationResult VehicleValidator::validateOwnerName(const std::string& name) {
    if (name.size() < 2)
        return ValidationResult::fail("owner_name", "Owner name must be at least 2 characters");
    static const std::regex kPattern(R"(^[a-zA-Z\s\-']+$)");
    if (!std::regex_match(name, kPattern))
        return ValidationResult::fail("owner_name", "Owner name contains invalid characters");
    return ValidationResult::ok();
}

ValidationResult VehicleValidator::validateOwnerEmail(const std::string& email) {
    static const std::regex kPattern(R"(^[^@\s]+@[^@\s]+\.[^@\s]+$)");
    if (!std::regex_match(email, kPattern))
        return ValidationResult::fail("owner_email", "owner_email must be a valid email address");
    return ValidationResult::ok();
}

ValidationResult VehicleValidator::validateOwnerPhone(const std::string& phone) {
    if (phone.empty()) return ValidationResult::ok();
    static const std::regex kPattern(R"(^[0-9+\-\s()]{6,20}$)");
    if (!std::regex_match(phone, kPattern))
        return ValidationResult::fail("owner_phone", "owner_phone contains invalid characters");
    return ValidationResult::ok();
}

ValidationResult VehicleValidator::validateVehicleType(const std::string& type) {
    static const std::set<std::string> kValid = {
        "CAR", "TRUCK", "MOTORCYCLE", "BUS", "EMERGENCY"
    };
    if (kValid.find(type) == kValid.end())
        return ValidationResult::fail("vehicle_type",
            "vehicle_type must be one of: CAR, TRUCK, MOTORCYCLE, BUS, EMERGENCY");
    return ValidationResult::ok();
}

std::vector<ValidationResult> VehicleValidator::validateAll(const std::string& plate,
                                                              const std::string& ownerName,
                                                              const std::string& ownerEmail,
                                                              const std::string& ownerPhone,
                                                              const std::string& vehicleType) {
    std::vector<ValidationResult> errors;
    auto r1 = validatePlate(plate);
    auto r2 = validateOwnerName(ownerName);
    auto r3 = validateOwnerEmail(ownerEmail);
    auto r4 = validateOwnerPhone(ownerPhone);
    auto r5 = validateVehicleType(vehicleType);
    if (!r1.isValid) errors.push_back(r1);
    if (!r2.isValid) errors.push_back(r2);
    if (!r3.isValid) errors.push_back(r3);
    if (!r4.isValid) errors.push_back(r4);
    if (!r5.isValid) errors.push_back(r5);
    return errors;
}
