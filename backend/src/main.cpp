#include <crow.h>
#include "DataAccessLayer/DatabaseConnection.h"
#include "MicroserviceLayer/VehicleController.h"
#include "MicroserviceLayer/JunctionController.h"
#include "MicroserviceLayer/ViolationController.h"
#include "MicroserviceLayer/FineController.h"
#include "MicroserviceLayer/TrafficController.h"
#include "MicroserviceLayer/EmergencyController.h"
#include "MicroserviceLayer/NotificationController.h"
#include <cstdlib>
#include <iostream>
#include <stdexcept>

int main() {
    const char* dbUrl = std::getenv("DATABASE_URL");
    if (!dbUrl)
        throw std::runtime_error(
            "DATABASE_URL is not set.\n"
            "Example: host=localhost dbname=traffic_db user=postgres password=secret");

    DatabaseConnection::initialize(dbUrl);
    std::cout << "[DB] PostgreSQL connection string loaded\n";

    VehicleController      vehicleCtrl;
    JunctionController     junctionCtrl;
    ViolationController    violationCtrl;
    FineController         fineCtrl;
    TrafficController      trafficCtrl;
    EmergencyController    emergencyCtrl;
    NotificationController notifCtrl;

    crow::SimpleApp app;

    // ── Vehicles ─────────────────────────────────────────────────────────────
    CROW_ROUTE(app, "/api/vehicles")
        .methods("POST"_method)
        ([&vehicleCtrl](const crow::request& req) {
            return vehicleCtrl.registerVehicle(req);
        });

    CROW_ROUTE(app, "/api/vehicles")
        .methods("GET"_method)
        ([&vehicleCtrl](const crow::request& req) {
            return vehicleCtrl.listVehicles(req);
        });

    CROW_ROUTE(app, "/api/vehicles/<string>")
        .methods("GET"_method)
        ([&vehicleCtrl](const crow::request& req, const std::string& plate) {
            return vehicleCtrl.getVehicle(req, plate);
        });

    // ── Junctions ─────────────────────────────────────────────────────────────
    CROW_ROUTE(app, "/api/junctions")
        .methods("GET"_method)
        ([&junctionCtrl](const crow::request& req) {
            return junctionCtrl.listJunctions(req);
        });

    CROW_ROUTE(app, "/api/junctions")
        .methods("POST"_method)
        ([&junctionCtrl](const crow::request& req) {
            return junctionCtrl.createJunction(req);
        });

    CROW_ROUTE(app, "/api/junctions/<int>/log")
        .methods("POST"_method)
        ([&junctionCtrl](const crow::request& req, int junctionId) {
            return junctionCtrl.logVehicle(req, junctionId);
        });

    CROW_ROUTE(app, "/api/junctions/<int>/logs")
        .methods("GET"_method)
        ([&junctionCtrl](const crow::request& req, int junctionId) {
            return junctionCtrl.getLogs(req, junctionId);
        });

    CROW_ROUTE(app, "/api/plate-logs")
        .methods("GET"_method)
        ([&junctionCtrl](const crow::request& req) {
            return junctionCtrl.getAllLogs(req);
        });

    // ── Violations ─────────────────────────────────────────────────────────────
    CROW_ROUTE(app, "/api/violation-types")
        .methods("GET"_method)
        ([&violationCtrl](const crow::request& req) {
            return violationCtrl.listViolationTypes(req);
        });

    CROW_ROUTE(app, "/api/violations")
        .methods("GET"_method)
        ([&violationCtrl](const crow::request& req) {
            return violationCtrl.listViolations(req);
        });

    CROW_ROUTE(app, "/api/violations/detect")
        .methods("POST"_method)
        ([&violationCtrl](const crow::request& req) {
            return violationCtrl.detectViolations(req);
        });

    CROW_ROUTE(app, "/api/violations/<int>")
        .methods("GET"_method)
        ([&violationCtrl](const crow::request& req, int id) {
            return violationCtrl.getViolation(req, id);
        });

    CROW_ROUTE(app, "/api/violations/<int>/fine")
        .methods("POST"_method)
        ([&violationCtrl](const crow::request& req, int id) {
            return violationCtrl.issueFine(req, id);
        });

    // ── Fines ─────────────────────────────────────────────────────────────────
    CROW_ROUTE(app, "/api/fines")
        .methods("GET"_method)
        ([&fineCtrl](const crow::request& req) {
            return fineCtrl.listFines(req);
        });

    CROW_ROUTE(app, "/api/fines/<int>")
        .methods("GET"_method)
        ([&fineCtrl](const crow::request& req, int id) {
            return fineCtrl.getFine(req, id);
        });

    CROW_ROUTE(app, "/api/fines/<int>/pay")
        .methods("POST"_method)
        ([&fineCtrl](const crow::request& req, int id) {
            return fineCtrl.payFine(req, id);
        });

    CROW_ROUTE(app, "/api/fines/<int>/cancel")
        .methods("POST"_method)
        ([&fineCtrl](const crow::request& req, int id) {
            return fineCtrl.cancelFine(req, id);
        });

    // ── Traffic / Congestion ─────────────────────────────────────────────────
    CROW_ROUTE(app, "/api/traffic/analyze")
        .methods("POST"_method)
        ([&trafficCtrl](const crow::request& req) {
            return trafficCtrl.analyzeTraffic(req);
        });

    CROW_ROUTE(app, "/api/traffic/congestion")
        .methods("GET"_method)
        ([&trafficCtrl](const crow::request& req) {
            return trafficCtrl.getCongestionRecords(req);
        });

    CROW_ROUTE(app, "/api/traffic/flow")
        .methods("GET"_method)
        ([&trafficCtrl](const crow::request& req) {
            return trafficCtrl.getFlowData(req);
        });

    CROW_ROUTE(app, "/api/traffic/flow/hourly")
        .methods("GET"_method)
        ([&trafficCtrl](const crow::request& req) {
            return trafficCtrl.getHourlyFlow(req);
        });

    CROW_ROUTE(app, "/api/traffic/congestion-prone")
        .methods("GET"_method)
        ([&trafficCtrl](const crow::request& req) {
            return trafficCtrl.getCongestionProne(req);
        });

    CROW_ROUTE(app, "/api/traffic/predict/<int>")
        .methods("GET"_method)
        ([&trafficCtrl](const crow::request& req, int junctionId) {
            return trafficCtrl.predictCongestion(req, junctionId);
        });

    CROW_ROUTE(app, "/api/traffic/routes/<int>")
        .methods("GET"_method)
        ([&trafficCtrl](const crow::request& req, int junctionId) {
            return trafficCtrl.getAlternativeRoutes(req, junctionId);
        });

    // ── Route guidance & map ─────────────────────────────────────────────────
    CROW_ROUTE(app, "/api/traffic/route")
        .methods("POST"_method)
        ([&trafficCtrl](const crow::request& req) {
            return trafficCtrl.recommendRoute(req);
        });

    CROW_ROUTE(app, "/api/traffic/tour")
        .methods("POST"_method)
        ([&trafficCtrl](const crow::request& req) {
            return trafficCtrl.planTour(req);
        });

    CROW_ROUTE(app, "/api/traffic/network")
        .methods("GET"_method)
        ([&trafficCtrl](const crow::request& req) {
            return trafficCtrl.getRoadNetwork(req);
        });

    // ── Emergency ────────────────────────────────────────────────────────────
    CROW_ROUTE(app, "/api/emergency/vehicles")
        .methods("GET"_method)
        ([&emergencyCtrl](const crow::request& req) {
            return emergencyCtrl.listEmergencyVehicles(req);
        });

    CROW_ROUTE(app, "/api/emergency/vehicles")
        .methods("POST"_method)
        ([&emergencyCtrl](const crow::request& req) {
            return emergencyCtrl.upsertEmergencyVehicle(req);
        });

    CROW_ROUTE(app, "/api/emergency/events")
        .methods("POST"_method)
        ([&emergencyCtrl](const crow::request& req) {
            return emergencyCtrl.createEmergencyEvent(req);
        });

    CROW_ROUTE(app, "/api/emergency/events/<int>")
        .methods("GET"_method)
        ([&emergencyCtrl](const crow::request& req, int eventId) {
            return emergencyCtrl.getEmergencyEvent(req, eventId);
        });

    CROW_ROUTE(app, "/api/emergency/events/<int>/affected-drivers")
        .methods("GET"_method)
        ([&emergencyCtrl](const crow::request& req, int eventId) {
            return emergencyCtrl.listAffectedDrivers(req, eventId);
        });

    // W15: resolve event + release signal corridor
    CROW_ROUTE(app, "/api/emergency/events/<int>/resolve")
        .methods("POST"_method)
        ([&emergencyCtrl](const crow::request& req, int eventId) {
            return emergencyCtrl.resolveEmergencyEvent(req, eventId);
        });

    // W15: list all junction signal states
    CROW_ROUTE(app, "/api/signals")
        .methods("GET"_method)
        ([&emergencyCtrl](const crow::request& req) {
            return emergencyCtrl.listSignals(req);
        });

    // W13/W14/W15: notification audit trail
    CROW_ROUTE(app, "/api/notifications")
        .methods("GET"_method)
        ([&notifCtrl](const crow::request& req) {
            return notifCtrl.listNotifications(req);
        });

    // Keep the API contract 100% JSON: unknown paths / wrong methods return a
    // JSON 404 instead of Crow's default HTML/plain-text error page.
    CROW_CATCHALL_ROUTE(app)
    ([](const crow::request&, crow::response& res) {
        res.code = 404;
        res.add_header("Content-Type", "application/json");
        res.body = R"({"error":"Route not found"})";
        res.end();
    });

    std::cout << "[API] Traffic Management Crow backend running on http://localhost:8080\n";
    app.port(8080).multithreaded().run();
    return 0;
}
