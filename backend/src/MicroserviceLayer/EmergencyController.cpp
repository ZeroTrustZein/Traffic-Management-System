#include "EmergencyController.h"
#include "RoutePlanner.h"
#include "Response.h"
#include "../Models.h"
#include <unordered_map>
#include <set>

namespace {

static crow::json::wvalue toJsonRoute(const std::vector<int>& route) {
    std::vector<crow::json::wvalue> out;
    out.reserve(route.size());
    for (int id : route) out.push_back(crow::json::wvalue(id));
    crow::json::wvalue w;
    w = std::move(out);
    return w;
}

// Valid emergency vehicle types
static const std::set<std::string> kValidEmVehicleTypes = {
    "AMBULANCE", "FIRE", "POLICE"
};

}  // namespace

crow::response EmergencyController::upsertEmergencyVehicle(const crow::request& req) {
    auto body = crow::json::load(req.body);
    if (!body) return jsonError(400, "Request body must be valid JSON");

    const auto getString = [&body](const std::string& key) -> std::string {
        if (!body.has(key)) return "";
        if (body[key].t() != crow::json::type::String) return "";
        return std::string(body[key].s());
    };
    const std::string type = getString("type");
    const std::string identifier = getString("identifier");
    const std::string plate = getString("plate");
    bool isActive = true;
    if (body.has("is_active") && body["is_active"].t() != crow::json::type::Null) {
        if (body["is_active"].t() != crow::json::type::True && body["is_active"].t() != crow::json::type::False)
            return jsonFieldError(400, "is_active", "is_active must be a boolean");
        isActive = body["is_active"].b();
    }

    if (type.empty()) return jsonFieldError(400, "type", "type is required");
    if (identifier.empty()) return jsonFieldError(400, "identifier", "identifier is required");

    // W5: validate that type is one of the accepted values
    if (kValidEmVehicleTypes.find(type) == kValidEmVehicleTypes.end())
        return jsonFieldError(400, "type", "type must be one of: AMBULANCE, FIRE, POLICE");

    try {
        const int id = vehicleRepo_.upsert(type, identifier, isActive, plate);
        crow::json::wvalue res;
        res["id"]         = id;
        res["type"]       = type;
        res["identifier"] = identifier;
        res["plate"]      = plate;
        res["is_active"]  = isActive;
        return jsonResponse(201, std::move(res));
    } catch (const std::exception&) {
        return jsonServerError();
    }
}

crow::response EmergencyController::listEmergencyVehicles(const crow::request&) {
    try {
        auto list = vehicleRepo_.findAll(200);
        std::vector<crow::json::wvalue> items;
        items.reserve(list.size());
        for (const auto& v : list) {
            crow::json::wvalue e;
            e["id"]         = v.id;
            e["type"]       = v.type;
            e["identifier"] = v.identifier;
            e["plate"]      = v.plate;
            e["is_active"]  = v.isActive;
            items.push_back(std::move(e));
        }
        crow::json::wvalue res;
        res["count"]    = static_cast<int>(items.size());
        res["vehicles"] = std::move(items);
        return jsonResponse(200, std::move(res));
    } catch (const std::exception&) {
        return jsonServerError();
    }
}

crow::response EmergencyController::createEmergencyEvent(const crow::request& req) {
    auto body = crow::json::load(req.body);
    if (!body) return jsonError(400, "Request body must be valid JSON");

    if (body.has("emergency_vehicle_id") && body["emergency_vehicle_id"].t() != crow::json::type::Number)
        return jsonFieldError(400, "emergency_vehicle_id", "emergency_vehicle_id must be a number");
    if (body.has("start_junction_id") && body["start_junction_id"].t() != crow::json::type::Number)
        return jsonFieldError(400, "start_junction_id", "start_junction_id must be a number");
    if (body.has("target_junction_id") && body["target_junction_id"].t() != crow::json::type::Number)
        return jsonFieldError(400, "target_junction_id", "target_junction_id must be a number");

    const int vehicleId = body.has("emergency_vehicle_id") ? body["emergency_vehicle_id"].i() : 0;
    const int startId   = body.has("start_junction_id")    ? body["start_junction_id"].i()    : 0;
    const int targetId  = body.has("target_junction_id")   ? body["target_junction_id"].i()   : 0;

    if (vehicleId <= 0) return jsonFieldError(400, "emergency_vehicle_id", "emergency_vehicle_id is required");
    if (startId <= 0) return jsonFieldError(400, "start_junction_id", "start_junction_id is required");
    if (targetId <= 0) return jsonFieldError(400, "target_junction_id", "target_junction_id is required");
    if (startId == targetId) return jsonError(400, "start and target must be different");

    try {
        auto ev = vehicleRepo_.findById(vehicleId);
        if (!ev) return jsonError(404, "Emergency vehicle not found");
        if (!junctionRepo_.existsById(startId)) return jsonError(404, "Start junction not found");
        if (!junctionRepo_.existsById(targetId)) return jsonError(404, "Target junction not found");

        auto segments = roadRepo_.findAll();
        RoutePlanner planner(segments);
        auto route = planner.shortestPath(startId, targetId);
        if (!route) return jsonError(404, "No route exists between the two junctions");

        const int eventId = eventRepo_.create(vehicleId, startId, targetId);
        eventRepo_.setRoute(eventId, route->junctionIds, route->totalMinutes);

        // W15: set green corridor — every junction on the route gets GREEN_PRIORITY
        signalRepo_.setPriorityForRoute(route->junctionIds, eventId);

        // Build signal_plan array for the response
        std::vector<crow::json::wvalue> signalPlan;
        signalPlan.reserve(route->junctionIds.size());
        for (int jid : route->junctionIds) {
            crow::json::wvalue sp;
            sp["junction_id"] = jid;
            sp["state"]       = "GREEN_PRIORITY";
            signalPlan.push_back(std::move(sp));
        }

        crow::json::wvalue res;
        res["event_id"]             = eventId;
        res["status"]               = "ACTIVE";
        res["emergency_vehicle_id"] = vehicleId;
        res["start_junction_id"]    = startId;
        res["target_junction_id"]   = targetId;
        res["estimated_minutes"]    = route->totalMinutes;
        res["route"]                = toJsonRoute(route->junctionIds);
        res["signal_plan"]          = std::move(signalPlan);
        return jsonResponse(201, std::move(res));
    } catch (const std::exception&) {
        return jsonServerError();
    }
}

crow::response EmergencyController::getEmergencyEvent(const crow::request&, int eventId) {
    if (eventId <= 0) return jsonError(400, "eventId must be positive");
    try {
        auto ev = eventRepo_.findById(eventId);
        if (!ev) return jsonError(404, "Emergency event not found");

        auto junctions = junctionRepo_.findAll();
        std::unordered_map<int, Junction> byId;
        for (const auto& j : junctions) byId[j.id] = j;

        crow::json::wvalue res;
        res["id"]                  = ev->id;
        res["status"]              = ev->status;
        res["emergency_vehicle_id"]= ev->emergencyVehicleId;
        res["start_junction_id"]   = ev->startJunctionId;
        res["target_junction_id"]  = ev->targetJunctionId;
        res["started_at"]          = ev->startedAt;
        res["estimated_minutes"]   = ev->estimatedMinutes;
        res["route"]               = toJsonRoute(ev->lastRoute);
        res["start_junction_name"] = byId.count(ev->startJunctionId) ? byId[ev->startJunctionId].name : "";
        res["target_junction_name"]= byId.count(ev->targetJunctionId) ? byId[ev->targetJunctionId].name : "";
        return jsonResponse(200, std::move(res));
    } catch (const std::exception&) {
        return jsonServerError();
    }
}

crow::response EmergencyController::listAffectedDrivers(const crow::request&, int eventId) {
    if (eventId <= 0) return jsonError(400, "eventId must be positive");
    try {
        auto ev = eventRepo_.findById(eventId);
        if (!ev) return jsonError(404, "Emergency event not found");

        auto profiles = profileRepo_.findOptedInByRouteOverlap(ev->lastRoute, 200);
        std::vector<crow::json::wvalue> drivers;
        drivers.reserve(profiles.size());
        for (const auto& p : profiles) {
            auto owner = ownerRepo_.findById(p.ownerId);
            if (!owner) continue;

            // W3: write notification log for each affected driver
            notifRepo_.create(
                owner->id,
                "EMERGENCY_ALERT",
                "Emergency vehicle approaching your route",
                "An emergency vehicle (event #" + std::to_string(eventId) +
                ") is active on your route. Please give way.",
                "SENT"
            );

            crow::json::wvalue d;
            d["owner_id"]  = owner->id;
            d["full_name"] = owner->fullName;
            d["email"]     = owner->email;
            drivers.push_back(std::move(d));
        }

        crow::json::wvalue res;
        res["event_id"] = eventId;
        res["count"]    = static_cast<int>(drivers.size());
        res["drivers"]  = std::move(drivers);
        return jsonResponse(200, std::move(res));
    } catch (const std::exception&) {
        return jsonServerError();
    }
}

// W15: resolve event and release its green corridor
crow::response EmergencyController::resolveEmergencyEvent(const crow::request&, int eventId) {
    if (eventId <= 0) return jsonError(400, "eventId must be positive");
    try {
        auto ev = eventRepo_.findById(eventId);
        if (!ev) return jsonError(404, "Emergency event not found");
        if (ev->status == "RESOLVED")
            return jsonError(409, "Emergency event is already resolved");

        eventRepo_.resolve(eventId);
        signalRepo_.releaseByEvent(eventId);

        // Re-fetch to return updated state
        auto updated = eventRepo_.findById(eventId);
        crow::json::wvalue res;
        res["id"]                   = eventId;
        res["status"]               = "RESOLVED";
        res["emergency_vehicle_id"] = ev->emergencyVehicleId;
        res["start_junction_id"]    = ev->startJunctionId;
        res["target_junction_id"]   = ev->targetJunctionId;
        res["estimated_minutes"]    = ev->estimatedMinutes;
        res["route"]                = toJsonRoute(ev->lastRoute);
        res["message"]              = "Event resolved; signal corridor released to NORMAL";
        return jsonResponse(200, std::move(res));
    } catch (const std::exception&) {
        return jsonServerError();
    }
}

// W15: list all junction signal states
crow::response EmergencyController::listSignals(const crow::request&) {
    try {
        auto signals = signalRepo_.findAll();
        std::vector<crow::json::wvalue> items;
        items.reserve(signals.size());
        for (const auto& s : signals) {
            crow::json::wvalue e;
            e["junction_id"]        = s.junctionId;
            e["state"]              = s.state;
            if (s.emergencyEventId > 0)
                e["emergency_event_id"] = s.emergencyEventId;
            else
                e["emergency_event_id"] = nullptr;
            e["updated_at"]         = s.updatedAt;
            items.push_back(std::move(e));
        }
        crow::json::wvalue res;
        res["count"]   = static_cast<int>(items.size());
        res["signals"] = std::move(items);
        return jsonResponse(200, std::move(res));
    } catch (const std::exception&) {
        return jsonServerError();
    }
}

