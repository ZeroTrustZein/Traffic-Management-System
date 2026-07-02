#include "JunctionController.h"
#include "PlateValidator.h"
#include "Response.h"
#include "../Models.h"
#include <set>

crow::response JunctionController::logVehicle(const crow::request& req, int junctionId) {
    auto body = crow::json::load(req.body);
    if (!body)
        return jsonError(400, "Request body must be valid JSON");

    const auto getString = [&body](const std::string& key) -> std::string {
        if (!body.has(key)) return "";
        if (body[key].t() != crow::json::type::String) return "";
        return std::string(body[key].s());
    };
    const std::string plate = getString("number_plate");
    std::string eventType = body.has("event_type") && body["event_type"].t() == crow::json::type::String
                                ? std::string(body["event_type"].s())
                                : "PASSAGE";
    double speedKmh = 0.0;
    bool hasSpeed = false;
    if (body.has("speed_kmh") && body["speed_kmh"].t() != crow::json::type::Null) {
        if (body["speed_kmh"].t() != crow::json::type::Number) {
            return jsonFieldError(400, "speed_kmh", "speed_kmh must be a number");
        }
        speedKmh = body["speed_kmh"].d();
        hasSpeed = true;
    }

    if (plate.empty())
        return jsonFieldError(400, "number_plate", "number_plate is required");

    if (!PlateValidator::validate(plate))
        return jsonFieldError(400, "number_plate", "Invalid plate format — expected AB12CDE");

    static const std::set<std::string> kValidEvents = {
        "PASSAGE", "SPEEDING", "RED_LIGHT", "PARKING"
    };
    if (kValidEvents.find(eventType) == kValidEvents.end())
        return jsonFieldError(400, "event_type", "event_type must be one of: PASSAGE, SPEEDING, RED_LIGHT, PARKING");

    if (hasSpeed) {
        if (speedKmh < 0.0 || speedKmh > 300.0) {
            return jsonFieldError(400, "speed_kmh", "speed_kmh must be between 0 and 300");
        }
    }
    if (eventType == "SPEEDING" && !hasSpeed) {
        return jsonFieldError(400, "speed_kmh", "speed_kmh is required for SPEEDING events");
    }

    try {
        auto junction = junctionRepo_.findById(junctionId);
        if (!junction)
            return jsonError(404, "Junction " + std::to_string(junctionId) + " not found");

        // Automated speeding detection: if the recorded speed exceeds the
        // junction's posted speed limit and the caller did not already flag
        // it as a non-PASSAGE event, promote the event to SPEEDING.
        bool autoSpeeding = false;
        if (hasSpeed && eventType == "PASSAGE" && speedKmh > junction->speedLimitKmh) {
            eventType    = "SPEEDING";
            autoSpeeding = true;
        }

        auto vehicle = vehicleRepo_.findByPlate(plate);
        if (!vehicle) {
            const int logId = logRepo_.saveUnregistered(junctionId, plate);
            crow::json::wvalue res;
            res["log_id"]       = logId;
            res["number_plate"] = plate;
            res["event_type"]   = eventType;
            res["status"]       = "UNREGISTERED";
            res["warning"]      = "Vehicle not in registry — alert raised";
            return jsonResponse(200, std::move(res));
        }

        const int logId = logRepo_.save(junctionId, vehicle->id, plate,
                                         eventType, "REGISTERED", speedKmh, hasSpeed);

        // W15: Automatic emergency-vehicle detection.
        // A vehicle is an emergency if its vehicleType is EMERGENCY OR if its plate
        // matches a row in the emergency_vehicles table.
        const bool isEmergencyByType  = (vehicle->vehicleType == VehicleType::EMERGENCY);
        const bool isEmergencyByPlate = emVehicleRepo_.existsByPlate(plate);
        const bool isEmergency = isEmergencyByType || isEmergencyByPlate;

        crow::json::wvalue res;
        res["log_id"]       = logId;
        res["number_plate"] = plate;
        res["vehicle_id"]   = vehicle->id;
        res["event_type"]   = eventType;
        res["status"]       = "REGISTERED";
        if (hasSpeed) res["speed_kmh"] = speedKmh;
        res["speed_limit_kmh"] = junction->speedLimitKmh;
        if (autoSpeeding) {
            res["auto_speeding_detected"] = true;
            res["speeding_excess_kmh"]    = speedKmh - junction->speedLimitKmh;
            res["warning"] = "Speed " + std::to_string(static_cast<int>(speedKmh))
                           + " km/h exceeds limit of "
                           + std::to_string(junction->speedLimitKmh) + " km/h";
        }

        if (isEmergency) {
            // Set this junction's signal to GREEN_PRIORITY (transient; no event id)
            signalRepo_.setTransientPriority(junctionId);

            // Write audit notification (no specific owner for a junction-level event)
            notifRepo_.create(
                0,
                "EMERGENCY_DETECTED",
                "Emergency vehicle detected at junction " + std::to_string(junctionId),
                "Emergency vehicle with plate " + plate +
                " detected at junction " + junction->name + ".",
                "SENT"
            );

            res["is_emergency"] = true;
            res["emergency_message"] =
                "Emergency vehicle detected — junction signal set to GREEN_PRIORITY";
        } else {
            res["is_emergency"] = false;
        }

        return jsonResponse(200, std::move(res));

    } catch (const std::exception&) {
        return jsonServerError();
    }
}

crow::response JunctionController::getLogs(const crow::request&, int junctionId) {
    try {
        auto logs = logRepo_.findByJunctionId(junctionId);

        std::vector<crow::json::wvalue> logList;
        logList.reserve(logs.size());
        for (const auto& log : logs) {
            crow::json::wvalue entry;
            entry["id"]           = log.id;
            entry["vehicle_id"]   = log.vehicleId;
            entry["number_plate"] = log.numberPlate;
            entry["event_type"]   = toString(log.eventType);
            entry["status"]       = toString(log.status);
            entry["detected_at"]  = log.detectedAt;
            if (log.hasSpeed) entry["speed_kmh"] = log.speedKmh;
            logList.push_back(std::move(entry));
        }

        crow::json::wvalue res;
        res["junction_id"] = junctionId;
        res["count"]       = static_cast<int>(logs.size());
        res["logs"]        = std::move(logList);
        return jsonResponse(200, std::move(res));

    } catch (const std::exception&) {
        return jsonServerError();
    }
}

crow::response JunctionController::getAllLogs(const crow::request&) {
    try {
        auto logs = logRepo_.findAll(200);

        std::vector<crow::json::wvalue> logList;
        logList.reserve(logs.size());
        for (const auto& log : logs) {
            crow::json::wvalue entry;
            entry["id"]           = log.id;
            entry["junction_id"]  = log.junctionId;
            entry["vehicle_id"]   = log.vehicleId;
            entry["number_plate"] = log.numberPlate;
            entry["event_type"]   = toString(log.eventType);
            entry["status"]       = toString(log.status);
            entry["detected_at"]  = log.detectedAt;
            if (log.hasSpeed) entry["speed_kmh"] = log.speedKmh;
            logList.push_back(std::move(entry));
        }

        crow::json::wvalue res;
        res["count"] = static_cast<int>(logs.size());
        res["logs"]  = std::move(logList);
        return jsonResponse(200, std::move(res));

    } catch (const std::exception&) {
        return jsonServerError();
    }
}

crow::response JunctionController::listJunctions(const crow::request&) {
    try {
        auto junctions = junctionRepo_.findAll();

        std::vector<crow::json::wvalue> list;
        list.reserve(junctions.size());
        for (const auto& j : junctions) {
            crow::json::wvalue entry;
            entry["id"]               = j.id;
            entry["name"]             = j.name;
            entry["location"]         = j.location;
            entry["speed_limit_kmh"]  = j.speedLimitKmh;
            entry["is_active"]        = j.isActive;
            entry["latitude"]         = j.latitude;
            entry["longitude"]        = j.longitude;
            list.push_back(std::move(entry));
        }

        crow::json::wvalue res;
        res["count"]     = static_cast<int>(junctions.size());
        res["junctions"] = std::move(list);
        return jsonResponse(200, std::move(res));

    } catch (const std::exception&) {
        return jsonServerError();
    }
}

crow::response JunctionController::createJunction(const crow::request& req) {
    auto body = crow::json::load(req.body);
    if (!body)
        return jsonError(400, "Request body must be valid JSON");

    const auto getString = [&body](const std::string& key) -> std::string {
        if (!body.has(key)) return "";
        if (body[key].t() != crow::json::type::String) return "";
        return std::string(body[key].s());
    };
    const std::string name     = getString("name");
    const std::string location = getString("location");

    // Numeric fields must be JSON numbers. Reject strings/booleans up front so
    // malformed input returns a clean 400 instead of an uncaught .i()/.d() throw.
    if (body.has("speed_limit_kmh") && body["speed_limit_kmh"].t() != crow::json::type::Number)
        return jsonFieldError(400, "speed_limit_kmh", "speed_limit_kmh must be a number");
    if (body.has("latitude") && body["latitude"].t() != crow::json::type::Null
        && body["latitude"].t() != crow::json::type::Number)
        return jsonFieldError(400, "latitude", "latitude must be a number");
    if (body.has("longitude") && body["longitude"].t() != crow::json::type::Null
        && body["longitude"].t() != crow::json::type::Number)
        return jsonFieldError(400, "longitude", "longitude must be a number");

    const int speedLimit       = body.has("speed_limit_kmh") ? body["speed_limit_kmh"].i() : 60;
    const double latitude      = body.has("latitude")  ? body["latitude"].d()  : 0.0;
    const double longitude     = body.has("longitude") ? body["longitude"].d() : 0.0;

    if (name.empty())
        return jsonFieldError(400, "name", "name is required");
    if (speedLimit < 10 || speedLimit > 130)
        return jsonFieldError(400, "speed_limit_kmh", "speed_limit_kmh must be between 10 and 130");
    if ((body.has("latitude") && body["latitude"].t() != crow::json::type::Null) && (latitude < -90.0 || latitude > 90.0))
        return jsonFieldError(400, "latitude", "latitude must be between -90 and 90");
    if ((body.has("longitude") && body["longitude"].t() != crow::json::type::Null) && (longitude < -180.0 || longitude > 180.0))
        return jsonFieldError(400, "longitude", "longitude must be between -180 and 180");

    try {
        const int id = junctionRepo_.save(name, location, speedLimit, latitude, longitude);
        crow::json::wvalue res;
        res["id"]               = id;
        res["name"]             = name;
        res["location"]         = location;
        res["speed_limit_kmh"]  = speedLimit;
        res["latitude"]         = latitude;
        res["longitude"]        = longitude;
        return jsonResponse(201, std::move(res));
    } catch (const std::exception&) {
        return jsonServerError();
    }
}

