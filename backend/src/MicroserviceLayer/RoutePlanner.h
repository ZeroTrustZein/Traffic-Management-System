#pragma once
#include "../Models.h"
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

struct RoutePlannerStep {
    int         fromId = 0;
    int         toId = 0;
    std::string roadName;
    double      distanceKm = 0.0;
    int         speedLimitKmh = 0;
    std::string congestionLevel = "LOW";
    double      estimatedMinutes = 0.0;
};

struct RoutePlannerPath {
    std::vector<int>              junctionIds;
    std::vector<RoutePlannerStep> steps;
    double                        totalDistanceKm = 0.0;
    double                        totalMinutes = 0.0;
};

struct RoutePlannerAlternative {
    int              destId = 0;
    std::vector<int> junctionIds;
    double           totalDistanceKm = 0.0;
    double           totalMinutes = 0.0;
};

struct RoutePlannerTourLeg {
    int    fromId = 0;
    int    toId = 0;
    double totalMinutes = 0.0;
};

struct RoutePlannerTour {
    std::vector<int>              visitOrder;
    std::vector<RoutePlannerTourLeg> legs;
    double                        totalMinutes = 0.0;
};

class RoutePlanner {
public:
    RoutePlanner(const std::vector<RoadSegment>& segments,
                 std::unordered_map<int, std::string> congestionLevels = {});

    bool hasOutgoingRoads(int junctionId) const;

    std::optional<RoutePlannerPath> shortestPath(int fromId, int toId) const;
    std::vector<RoutePlannerAlternative> topReachableRoutes(int fromId, int maxRoutes) const;
    std::optional<RoutePlannerTour> nearestNeighborTour(const std::vector<int>& stops, int startId) const;

private:
    struct EdgeView {
        int         toId = 0;
        double      distanceKm = 0.0;
        int         speedLimitKmh = 0;
        std::string name;
    };

    struct DijkstraResult {
        std::unordered_map<int, double> distMinutes;
        std::unordered_map<int, int>    prev;
        std::unordered_map<int, double> distKm;
    };

    std::unordered_map<int, std::vector<EdgeView>> graph_;
    std::vector<int> nodes_;
    std::unordered_map<int, std::string> congestionLevels_;

    const EdgeView* findEdge(int fromId, int toId) const;
    std::string congestionLevelFor(int junctionId) const;
    double weightedMinutes(const EdgeView& edge) const;
    DijkstraResult runDijkstra(int sourceId) const;
    std::optional<std::vector<int>> reconstructPath(
        int fromId,
        int toId,
        const std::unordered_map<int, int>& prev,
        const std::unordered_map<int, double>& distMinutes) const;
};
