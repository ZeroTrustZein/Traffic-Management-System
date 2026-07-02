#include "TrafficController.h"
#include "RoutePlanner.h"
#include "Response.h"
#include "TrafficRules.h"
#include "../Models.h"
#include "../DataAccessLayer/DatabaseConnection.h"
#include <map>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <limits>
#include <queue>
#include <set>
#include <unordered_map>
#include <vector>

std::string TrafficController::classifyLevel(int count) {
    return TrafficRules::classifyCongestionLevel(count);
}

static std::string nowIso() {
    std::time_t t = std::time(nullptr);
    std::tm tm   = *std::gmtime(&t);
    std::ostringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

static std::string offsetIso(int secondsBack) {
    std::time_t t = std::time(nullptr) - secondsBack;
    std::tm tm   = *std::gmtime(&t);
    std::ostringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

static int currentHourUtc() {
    std::time_t t = std::time(nullptr);
    std::tm tm   = *std::gmtime(&t);
    return tm.tm_hour;
}

crow::response TrafficController::analyzeTraffic(const crow::request& req) {
    auto body = crow::json::load(req.body);
    if (!body)
        return jsonError(400, "Request body must be valid JSON");

    if (body.has("junction_id") && body["junction_id"].t() != crow::json::type::Number)
        return jsonFieldError(400, "junction_id", "junction_id must be a number");
    if (body.has("window_hours") && body["window_hours"].t() != crow::json::type::Number)
        return jsonFieldError(400, "window_hours", "window_hours must be a number");

    const int junctionId   = body.has("junction_id")   ? body["junction_id"].i()   : 0;
    const int windowHours  = body.has("window_hours")  ? body["window_hours"].i()  : 1;

    if (junctionId <= 0)
        return jsonFieldError(400, "junction_id", "junction_id is required");
    if (windowHours < 1 || windowHours > 24)
        return jsonFieldError(400, "window_hours", "window_hours must be between 1 and 24");

    try {
        if (!junctionRepo_.existsById(junctionId))
            return jsonError(404, "Junction not found");

        const int secondsBack  = windowHours * 3600;
        const int vehicleCount = congestionRepo_.countLogsInWindow(junctionId, secondsBack);
        const std::string level = classifyLevel(vehicleCount);

        const std::string windowEnd   = nowIso();
        const std::string windowStart = offsetIso(secondsBack);

        const int recordId = congestionRepo_.save(junctionId, windowStart, windowEnd,
                                                   vehicleCount, level);

        // W3: write notification log for HIGH or SEVERE congestion
        if (level == "HIGH" || level == "SEVERE") {
            notifRepo_.create(
                0,
                "CONGESTION_ALERT",
                "Congestion alert: " + level + " at junction " + std::to_string(junctionId),
                "Junction " + std::to_string(junctionId) + " has " + level +
                " congestion (" + std::to_string(vehicleCount) + " vehicles in " +
                std::to_string(windowHours) + "h window).",
                "SENT"
            );
        }

        crow::json::wvalue res;
        res["record_id"]        = recordId;
        res["junction_id"]      = junctionId;
        res["vehicle_count"]    = vehicleCount;
        res["congestion_level"] = level;
        res["window_start"]     = windowStart;
        res["window_end"]       = windowEnd;
        return jsonResponse(200, std::move(res));

    } catch (const std::exception&) {
        return jsonServerError();
    }
}

crow::response TrafficController::getCongestionRecords(const crow::request&) {
    try {
        auto records = congestionRepo_.findAll(200);

        std::vector<crow::json::wvalue> list;
        list.reserve(records.size());
        for (const auto& cr : records) {
            crow::json::wvalue entry;
            entry["id"]                = cr.id;
            entry["junction_id"]       = cr.junctionId;
            entry["time_window_start"] = cr.timeWindowStart;
            entry["time_window_end"]   = cr.timeWindowEnd;
            entry["vehicle_count"]     = cr.vehicleCount;
            entry["congestion_level"]  = cr.congestionLevel;
            entry["recorded_at"]       = cr.recordedAt;
            list.push_back(std::move(entry));
        }

        crow::json::wvalue res;
        res["count"]   = static_cast<int>(records.size());
        res["records"] = std::move(list);
        return jsonResponse(200, std::move(res));

    } catch (const std::exception&) {
        return jsonServerError();
    }
}

crow::response TrafficController::getFlowData(const crow::request&) {
    try {
        auto records = congestionRepo_.findAll(500);

        // Group by junction: collect time_window_start labels and counts
        std::map<int, std::vector<crow::json::wvalue>> byJunction;
        for (const auto& cr : records) {
            crow::json::wvalue pt;
            pt["time"]  = cr.timeWindowStart;
            pt["count"] = cr.vehicleCount;
            pt["level"] = cr.congestionLevel;
            byJunction[cr.junctionId].push_back(std::move(pt));
        }

        std::vector<crow::json::wvalue> junctionData;
        for (auto& [jId, points] : byJunction) {
            crow::json::wvalue jEntry;
            jEntry["junction_id"] = jId;
            jEntry["data"]        = std::move(points);
            junctionData.push_back(std::move(jEntry));
        }

        crow::json::wvalue res;
        res["junctions"] = std::move(junctionData);
        return jsonResponse(200, std::move(res));

    } catch (const std::exception&) {
        return jsonServerError();
    }
}

crow::response TrafficController::getHourlyFlow(const crow::request&) {
    try {
        auto profile = congestionRepo_.hourlyProfile();

        std::vector<crow::json::wvalue> junctions;
        junctions.reserve(profile.size());
        for (auto& hj : profile) {
            std::vector<crow::json::wvalue> hours;
            hours.reserve(hj.hours.size());
            for (auto& b : hj.hours) {
                crow::json::wvalue h;
                h["hour"]      = b.hour;
                h["avg_count"] = b.avgCount;
                hours.push_back(std::move(h));
            }
            crow::json::wvalue entry;
            entry["junction_id"] = hj.junctionId;
            entry["name"]        = hj.name;
            entry["hours"]       = std::move(hours);
            junctions.push_back(std::move(entry));
        }

        crow::json::wvalue res;
        res["junctions"] = std::move(junctions);
        return jsonResponse(200, std::move(res));

    } catch (const std::exception&) {
        return jsonServerError();
    }
}

crow::response TrafficController::getCongestionProne(const crow::request&) {
    try {
        auto summaries = congestionRepo_.congestionProneSummary();

        std::vector<crow::json::wvalue> list;
        list.reserve(summaries.size());
        for (const auto& s : summaries) {
            crow::json::wvalue entry;
            entry["junction_id"]      = s.junctionId;
            entry["name"]             = s.name;
            entry["avg_vehicle_count"]= s.avgCount;
            entry["worst_level"]      = s.worstLevel;
            list.push_back(std::move(entry));
        }

        crow::json::wvalue res;
        res["count"]     = static_cast<int>(summaries.size());
        res["junctions"] = std::move(list);
        return jsonResponse(200, std::move(res));

    } catch (const std::exception&) {
        return jsonServerError();
    }
}

// W8: smarter congestion prediction using time-of-day history blended with
// recent trend. The next hour is predicted by averaging:
//   - hourAvg: mean vehicle_count for the same hour-of-day from all history
//   - recentAvg: 3-sample moving average of most recent windows
// If both are available: predicted = 0.6 * hourAvg + 0.4 * recentAvg
// If only one is available: use that one.
crow::response TrafficController::predictCongestion(const crow::request&, int junctionId) {
    try {
        if (!junctionRepo_.existsById(junctionId))
            return jsonError(404, "Junction not found");

        const int nextHour = (currentHourUtc() + 1) % 24;
        const double hourAvg   = congestionRepo_.averageCountForHour(junctionId, nextHour);
        const double recentAvg = congestionRepo_.averageRecentCount(junctionId, 3);

        double predictedCount;
        std::string basis;

        // hourAvg returns -1 when no history exists for that hour
        const bool hasHourHistory = (hourAvg >= 0.0);

        if (hasHourHistory && recentAvg > 0.0) {
            predictedCount = 0.6 * hourAvg + 0.4 * recentAvg;
            basis = "60% time-of-day historical average for hour " + std::to_string(nextHour) +
                    " + 40% recent 3-window average";
        } else if (hasHourHistory) {
            predictedCount = hourAvg;
            basis = "time-of-day historical average for hour " + std::to_string(nextHour) +
                    " (no recent data)";
        } else {
            predictedCount = recentAvg;
            basis = "3-sample moving average of recent windows (no hour-of-day history)";
        }

        const std::string predictedLevel = classifyLevel(static_cast<int>(predictedCount));

        crow::json::wvalue res;
        res["junction_id"]        = junctionId;
        res["target_hour"]        = nextHour;
        res["predicted_count"]    = predictedCount;
        res["predicted_level"]    = predictedLevel;
        res["basis"]              = basis;
        res["recommend_diversion"]= (predictedLevel == "HIGH" || predictedLevel == "SEVERE");
        return jsonResponse(200, std::move(res));

    } catch (const std::exception&) {
        return jsonServerError();
    }
}

// =============================================================================
// Route guidance — congestion-aware shortest path
// =============================================================================

namespace {

std::string levelForCount(int count) {
    return TrafficRules::classifyCongestionLevel(count);
}

// Pre-compute congestion levels for every junction in one query so the path
// solver does not issue a query per edge.
std::unordered_map<int, std::string> loadCongestionLevels() {
    std::unordered_map<int, std::string> levels;
    pqxx::work txn(DatabaseConnection::get());
    auto r = txn.exec(
        "SELECT junction_id, "
        "       COALESCE(AVG(vehicle_count), 0) AS avg_count "
        "FROM ("
        "  SELECT junction_id, vehicle_count, "
        "         ROW_NUMBER() OVER (PARTITION BY junction_id "
        "                            ORDER BY time_window_start DESC) AS rn "
        "  FROM congestion_records"
        ") sub "
        "WHERE rn <= 3 "
        "GROUP BY junction_id");
    for (const auto& row : r) {
        levels[row[0].as<int>()] = levelForCount(static_cast<int>(row[1].as<double>()));
    }
    return levels;
}

}  // namespace

crow::response TrafficController::recommendRoute(const crow::request& req) {
    auto body = crow::json::load(req.body);
    if (!body) return jsonError(400, "Request body must be valid JSON");

    if (body.has("from_junction_id") && body["from_junction_id"].t() != crow::json::type::Number)
        return jsonFieldError(400, "from_junction_id", "from_junction_id must be a number");
    if (body.has("to_junction_id") && body["to_junction_id"].t() != crow::json::type::Number)
        return jsonFieldError(400, "to_junction_id", "to_junction_id must be a number");

    const int fromId = body.has("from_junction_id") ? body["from_junction_id"].i() : 0;
    const int toId   = body.has("to_junction_id")   ? body["to_junction_id"].i()   : 0;

    if (fromId <= 0 || toId <= 0)
        return jsonError(400, "from_junction_id and to_junction_id are required");
    if (fromId == toId)
        return jsonError(400, "from and to must be different junctions");

    try {
        auto fromJ = junctionRepo_.findById(fromId);
        auto toJ   = junctionRepo_.findById(toId);
        if (!fromJ) return jsonError(404, "from junction not found");
        if (!toJ)   return jsonError(404, "to junction not found");

        auto segments = roadRepo_.findAll();
        auto levels = loadCongestionLevels();
        RoutePlanner planner(segments, std::move(levels));
        if (!planner.hasOutgoingRoads(fromId))
            return jsonError(404, "No outgoing roads from the starting junction");

        auto route = planner.shortestPath(fromId, toId);
        if (!route)
            return jsonError(404, "No route exists between the two junctions");

        // Build step-by-step instructions
        std::vector<crow::json::wvalue> steps;
        steps.reserve(route->steps.size());
        for (const auto& routeStep : route->steps) {
            auto fromJunc = junctionRepo_.findById(routeStep.fromId);
            auto toJunc   = junctionRepo_.findById(routeStep.toId);

            crow::json::wvalue step;
            step["from_junction_id"]   = routeStep.fromId;
            step["from_junction_name"] = fromJunc ? fromJunc->name : "";
            step["to_junction_id"]     = routeStep.toId;
            step["to_junction_name"]   = toJunc ? toJunc->name : "";
            step["via"]                = routeStep.roadName;
            step["distance_km"]        = routeStep.distanceKm;
            step["speed_limit_kmh"]    = routeStep.speedLimitKmh;
            step["congestion_level"]   = routeStep.congestionLevel;
            step["estimated_minutes"]  = routeStep.estimatedMinutes;
            step["instruction"]        = "Continue on " + routeStep.roadName
                                       + " to " + (toJunc ? toJunc->name : "next junction");
            steps.push_back(std::move(step));
        }

        crow::json::wvalue res;
        res["from_junction_id"]         = fromId;
        res["from_junction_name"]       = fromJ->name;
        res["to_junction_id"]           = toId;
        res["to_junction_name"]         = toJ->name;
        res["total_distance_km"]        = route->totalDistanceKm;
        res["estimated_total_minutes"]  = route->totalMinutes;
        res["hops"]                     = static_cast<int>(steps.size());
        res["algorithm"]                = "Dijkstra (congestion-weighted edge cost)";
        res["steps"]                    = std::move(steps);
        return jsonResponse(200, std::move(res));

    } catch (const std::exception&) {
        return jsonServerError();
    }
}

crow::response TrafficController::planTour(const crow::request& req) {
    auto body = crow::json::load(req.body);
    if (!body) return jsonError(400, "Request body must be valid JSON");
    if (!body.has("junction_ids"))
        return jsonFieldError(400, "junction_ids", "junction_ids (array) is required");
    if (body["junction_ids"].t() != crow::json::type::List)
        return jsonFieldError(400, "junction_ids", "junction_ids must be an array of integers");

    std::vector<int> stops;
    for (const auto& v : body["junction_ids"]) {
        if (v.t() != crow::json::type::Number)
            return jsonFieldError(400, "junction_ids", "junction_ids must be an array of integers");
        const int id = v.i();
        if (id <= 0)
            return jsonFieldError(400, "junction_ids", "junction_ids values must be positive");
        stops.push_back(id);
    }
    if (stops.size() < 2)
        return jsonError(400, "Need at least two junction_ids to plan a tour");

    // W5: validate start_junction_id type before calling .i()
    if (body.has("start_junction_id") && body["start_junction_id"].t() != crow::json::type::Number)
        return jsonFieldError(400, "start_junction_id", "start_junction_id must be a number");

    const int startId = body.has("start_junction_id")
                           ? body["start_junction_id"].i()
                           : stops.front();
    if (startId <= 0)
        return jsonFieldError(400, "start_junction_id", "start_junction_id must be positive");

    try {
        // Verify all stops exist
        for (int id : stops) {
            if (!junctionRepo_.existsById(id))
                return jsonError(404, "Junction " + std::to_string(id) + " not found");
        }

        auto segments = roadRepo_.findAll();
        auto levels = loadCongestionLevels();
        RoutePlanner planner(segments, std::move(levels));
        auto tour = planner.nearestNeighborTour(stops, startId);
        if (!tour) {
            return jsonError(404, "No reachable route between all requested junctions");
        }

        std::vector<crow::json::wvalue> legs;
        legs.reserve(tour->legs.size());
        for (const auto& legDef : tour->legs) {
            auto curJ  = junctionRepo_.findById(legDef.fromId);
            auto nxtJ  = junctionRepo_.findById(legDef.toId);
            crow::json::wvalue leg;
            leg["from_junction_id"]   = legDef.fromId;
            leg["from_junction_name"] = curJ ? curJ->name : "";
            leg["to_junction_id"]     = legDef.toId;
            leg["to_junction_name"]   = nxtJ ? nxtJ->name : "";
            leg["estimated_minutes"]  = legDef.totalMinutes;
            legs.push_back(std::move(leg));
        }

        std::vector<crow::json::wvalue> orderJson;
        orderJson.reserve(tour->visitOrder.size());
        for (int id : tour->visitOrder) orderJson.push_back(crow::json::wvalue(id));

        crow::json::wvalue res;
        res["algorithm"]                  = "Nearest-neighbour TSP heuristic over congestion-weighted graph";
        res["start_junction_id"]          = startId;
        res["stop_count"]                 = static_cast<int>(stops.size());
        res["total_estimated_minutes"]    = tour->totalMinutes;
        res["visit_order"]                = std::move(orderJson);
        res["legs"]                       = std::move(legs);
        return jsonResponse(200, std::move(res));

    } catch (const std::exception&) {
        return jsonServerError();
    }
}

crow::response TrafficController::getRoadNetwork(const crow::request&) {
    try {
        auto junctions = junctionRepo_.findAll();
        auto segments  = roadRepo_.findAll();
        auto levels    = loadCongestionLevels();

        std::vector<crow::json::wvalue> jList;
        jList.reserve(junctions.size());
        for (const auto& j : junctions) {
            crow::json::wvalue entry;
            entry["id"]               = j.id;
            entry["name"]             = j.name;
            entry["location"]         = j.location;
            entry["speed_limit_kmh"]  = j.speedLimitKmh;
            entry["latitude"]         = j.latitude;
            entry["longitude"]        = j.longitude;
            entry["congestion_level"] = levels.count(j.id) ? levels[j.id] : "LOW";
            jList.push_back(std::move(entry));
        }

        std::vector<crow::json::wvalue> sList;
        sList.reserve(segments.size());
        for (const auto& s : segments) {
            crow::json::wvalue entry;
            entry["id"]               = s.id;
            entry["from_junction_id"] = s.fromJunctionId;
            entry["to_junction_id"]   = s.toJunctionId;
            entry["name"]             = s.name;
            entry["distance_km"]      = s.distanceKm;
            entry["speed_limit_kmh"]  = s.speedLimitKmh;
            sList.push_back(std::move(entry));
        }

        crow::json::wvalue res;
        res["junctions"] = std::move(jList);
        res["roads"]     = std::move(sList);
        return jsonResponse(200, std::move(res));

    } catch (const std::exception&) {
        return jsonServerError();
    }
}

// =============================================================================
// W6: Computed alternative routes — replaces static route_alternatives query.
// Algorithm: from junction J (potentially congested), run Dijkstra to find all
// reachable destinations. Return the 3 nearest distinct destinations using
// congestion-weighted edge costs. Each result includes the ordered junction
// path, distance_km, and est_minutes. The congested junction itself is never
// returned as a destination.
// =============================================================================
crow::response TrafficController::getAlternativeRoutes(const crow::request&, int junctionId) {
    try {
        if (!junctionRepo_.existsById(junctionId))
            return jsonError(404, "Junction not found");

        auto segments = roadRepo_.findAll();
        auto levels   = loadCongestionLevels();
        RoutePlanner planner(segments, std::move(levels));

        if (!planner.hasOutgoingRoads(junctionId)) {
            crow::json::wvalue res;
            res["from_junction_id"] = junctionId;
            res["count"]            = 0;
            res["routes"]           = std::vector<crow::json::wvalue>{};
            return jsonResponse(200, std::move(res));
        }

        const auto candidates = planner.topReachableRoutes(junctionId, 3);

        std::vector<crow::json::wvalue> routes;
        routes.reserve(candidates.size());
        for (size_t ri = 0; ri < candidates.size(); ++ri) {
            const auto& cand = candidates[ri];
            std::vector<crow::json::wvalue> pathJson;
            pathJson.reserve(cand.junctionIds.size());
            for (int jid : cand.junctionIds) pathJson.push_back(crow::json::wvalue(jid));

            auto destJ = junctionRepo_.findById(cand.destId);

            // Build a via_description string from path junctions
            std::string viaDesc = "Via";
            for (size_t pi = 1; pi + 1 < cand.junctionIds.size(); ++pi) {
                auto midJ = junctionRepo_.findById(cand.junctionIds[pi]);
                viaDesc += " " + (midJ ? midJ->name : std::to_string(cand.junctionIds[pi]));
                if (pi + 2 < cand.junctionIds.size()) viaDesc += ",";
            }
            if (cand.junctionIds.size() <= 2) viaDesc = "Direct";

            crow::json::wvalue route;
            route["route_id"]               = static_cast<int>(ri) + 1;
            route["to_junction_id"]         = cand.destId;
            route["to_junction_name"]       = destJ ? destJ->name : "";
            route["junction_path"]          = std::move(pathJson);
            route["distance_km"]            = cand.totalDistanceKm;
            route["est_minutes"]            = cand.totalMinutes;
            route["via_description"]        = viaDesc;
            routes.push_back(std::move(route));
        }

        crow::json::wvalue res;
        res["from_junction_id"] = junctionId;
        res["count"]            = static_cast<int>(routes.size());
        res["algorithm"]        = "Dijkstra (congestion-weighted, live graph)";
        res["routes"]           = std::move(routes);
        return jsonResponse(200, std::move(res));

    } catch (const std::exception&) {
        return jsonServerError();
    }
}

