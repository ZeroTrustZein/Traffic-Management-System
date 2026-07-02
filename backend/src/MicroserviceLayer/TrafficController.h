#pragma once
#include <crow.h>
#include "../DataAccessLayer/CongestionRepository.h"
#include "../DataAccessLayer/JunctionRepository.h"
#include "../DataAccessLayer/RoadSegmentRepository.h"
#include "../DataAccessLayer/NotificationRepository.h"

class TrafficController {
public:
    crow::response analyzeTraffic(const crow::request& req);
    crow::response getCongestionRecords(const crow::request& req);
    crow::response getFlowData(const crow::request& req);
    crow::response getHourlyFlow(const crow::request& req);
    crow::response getCongestionProne(const crow::request& req);
    crow::response predictCongestion(const crow::request& req, int junctionId);
    crow::response getAlternativeRoutes(const crow::request& req, int junctionId);

    // Congestion-aware shortest path between two junctions (Dijkstra).
    // Body: { "from_junction_id": int, "to_junction_id": int }
    crow::response recommendRoute(const crow::request& req);

    // Nearest-neighbour TSP heuristic over a list of junctions.
    // Body: { "junction_ids": [int, ...], "start_junction_id": int (optional) }
    crow::response planTour(const crow::request& req);

    // Returns the full road graph (junctions + edges) for the map UI.
    crow::response getRoadNetwork(const crow::request& req);

private:
    CongestionRepository    congestionRepo_;
    JunctionRepository      junctionRepo_;
    RoadSegmentRepository   roadRepo_;
    NotificationRepository  notifRepo_;  // W3: congestion alert notifications

    static std::string classifyLevel(int count);
};
