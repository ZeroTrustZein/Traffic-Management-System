#include "VehicleController.h"
#include "VehicleValidator.h"
#include "Response.h"
#include "../Models.h"

crow::response VehicleController::registerVehicle(const crow::request& req) {
    auto body = crow::json::load(req.body);
    if (!body)
        return jsonError(400, "Request body must be valid JSON");

    const auto getString = [&body](const std::string& key) -> std::string {
        if (!body.has(key)) return "";
        if (body[key].t() != crow::json::type::String) return "";
        return std::string(body[key].s());
    };
    const std::string plate       = getString("number_plate");
    const std::string ownerName   = getString("owner_name");
    const std::string ownerEmail  = getString("owner_email");
    const std::string ownerPhone  = getString("owner_phone");
    const std::string vehicleType = body.has("vehicle_type") && body["vehicle_type"].t() == crow::json::type::String
                                        ? std::string(body["vehicle_type"].s())
                                        : "CAR";

    if (plate.empty())
        return jsonFieldError(400, "number_plate", "number_plate is required");
    if (ownerName.empty())
        return jsonFieldError(400, "owner_name", "owner_name is required");
    if (ownerEmail.empty())
        return jsonFieldError(400, "owner_email", "owner_email is required");

    auto errors = VehicleValidator::validateAll(plate, ownerName, ownerEmail, ownerPhone, vehicleType);
    if (!errors.empty()) {
        return jsonValidationError(400, errors);
    }

    try {
        if (vehicleRepo_.existsByPlate(plate))
            return jsonError(409, "A vehicle with plate " + plate + " is already registered");

        const int ownerId   = ownerRepo_.findOrCreate(ownerName, ownerEmail, ownerPhone);
        const int vehicleId = vehicleRepo_.save(plate, vehicleType, ownerId);

        auto owner = ownerRepo_.findById(ownerId);

        crow::json::wvalue ownerJson;
        ownerJson["id"]        = ownerId;
        ownerJson["full_name"] = owner ? owner->fullName : ownerName;
        ownerJson["email"]     = owner ? owner->email    : ownerEmail;

        crow::json::wvalue res;
        res["vehicle_id"]    = vehicleId;
        res["number_plate"]  = plate;
        res["vehicle_type"]  = vehicleType;
        res["owner"]         = std::move(ownerJson);
        return jsonResponse(201, std::move(res));

    } catch (const std::exception&) {
        return jsonServerError();
    }
}

crow::response VehicleController::getVehicle(const crow::request&, const std::string& plate) {
    try {
        auto v = vehicleRepo_.findByPlate(plate);
        if (!v) return jsonError(404, "Vehicle not found: " + plate);

        crow::json::wvalue res;
        res["id"]           = v->id;
        res["number_plate"] = v->numberPlate;
        res["vehicle_type"] = toString(v->vehicleType);
        res["is_active"]    = v->isActive;
        res["created_at"]   = v->createdAt;

        if (v->ownerId > 0) {
            auto o = ownerRepo_.findById(v->ownerId);
            if (o) {
                crow::json::wvalue ownerJson;
                ownerJson["id"]        = o->id;
                ownerJson["full_name"] = o->fullName;
                ownerJson["email"]     = o->email;
                ownerJson["phone"]     = o->phone;
                res["owner"] = std::move(ownerJson);
            }
        }
        return jsonResponse(200, std::move(res));

    } catch (const std::exception&) {
        return jsonServerError();
    }
}

crow::response VehicleController::listVehicles(const crow::request&) {
    try {
        auto vehicles = vehicleRepo_.findAll(200);

        std::vector<crow::json::wvalue> list;
        list.reserve(vehicles.size());
        for (const auto& v : vehicles) {
            crow::json::wvalue entry;
            entry["id"]           = v.id;
            entry["number_plate"] = v.numberPlate;
            entry["vehicle_type"] = toString(v.vehicleType);
            entry["is_active"]    = v.isActive;
            entry["created_at"]   = v.createdAt;
            if (v.ownerId > 0) {
                auto o = ownerRepo_.findById(v.ownerId);
                if (o) {
                    crow::json::wvalue ownerJson;
                    ownerJson["id"]        = o->id;
                    ownerJson["full_name"] = o->fullName;
                    ownerJson["email"]     = o->email;
                    entry["owner"] = std::move(ownerJson);
                }
            }
            list.push_back(std::move(entry));
        }

        crow::json::wvalue res;
        res["count"]    = static_cast<int>(vehicles.size());
        res["vehicles"] = std::move(list);
        return jsonResponse(200, std::move(res));

    } catch (const std::exception&) {
        return jsonServerError();
    }
}

