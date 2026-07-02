#include "RoutePlanner.h"
#include "TrafficRules.h"
#include <algorithm>
#include <limits>
#include <queue>
#include <set>

RoutePlanner::RoutePlanner(const std::vector<RoadSegment>& segments,
                           std::unordered_map<int, std::string> congestionLevels)
    : congestionLevels_(std::move(congestionLevels)) {
    std::set<int> uniqueNodes;
    for (const auto& segment : segments) {
        graph_[segment.fromJunctionId].push_back({
            segment.toJunctionId,
            segment.distanceKm,
            segment.speedLimitKmh,
            segment.name
        });
        uniqueNodes.insert(segment.fromJunctionId);
        uniqueNodes.insert(segment.toJunctionId);
    }
    nodes_.assign(uniqueNodes.begin(), uniqueNodes.end());
}

bool RoutePlanner::hasOutgoingRoads(int junctionId) const {
    return graph_.find(junctionId) != graph_.end();
}

std::optional<RoutePlannerPath> RoutePlanner::shortestPath(int fromId, int toId) const {
    const auto result = runDijkstra(fromId);
    auto path = reconstructPath(fromId, toId, result.prev, result.distMinutes);
    if (!path) return std::nullopt;

    RoutePlannerPath plan;
    plan.junctionIds = *path;
    plan.totalMinutes = result.distMinutes.at(toId);

    for (size_t i = 0; i + 1 < plan.junctionIds.size(); ++i) {
        const int from = plan.junctionIds[i];
        const int to   = plan.junctionIds[i + 1];
        const auto* edge = findEdge(from, to);
        if (!edge) continue;

        RoutePlannerStep step;
        step.fromId = from;
        step.toId = to;
        step.roadName = edge->name;
        step.distanceKm = edge->distanceKm;
        step.speedLimitKmh = edge->speedLimitKmh;
        step.congestionLevel = congestionLevelFor(to);
        step.estimatedMinutes = weightedMinutes(*edge);
        plan.steps.push_back(step);
        plan.totalDistanceKm += edge->distanceKm;
    }

    return plan;
}

std::vector<RoutePlannerAlternative> RoutePlanner::topReachableRoutes(int fromId, int maxRoutes) const {
    std::vector<RoutePlannerAlternative> routes;
    if (maxRoutes <= 0) return routes;

    const auto result = runDijkstra(fromId);

    for (int node : nodes_) {
        if (node == fromId) continue;
        const auto it = result.distMinutes.find(node);
        if (it == result.distMinutes.end() || it->second == std::numeric_limits<double>::infinity()) {
            continue;
        }

        auto path = reconstructPath(fromId, node, result.prev, result.distMinutes);
        if (!path) continue;

        routes.push_back({
            node,
            *path,
            result.distKm.count(node) ? result.distKm.at(node) : 0.0,
            it->second
        });
    }

    std::sort(routes.begin(), routes.end(),
              [](const RoutePlannerAlternative& a, const RoutePlannerAlternative& b) {
                  return a.totalMinutes < b.totalMinutes;
              });

    if (static_cast<int>(routes.size()) > maxRoutes) {
        routes.resize(maxRoutes);
    }
    return routes;
}

std::optional<RoutePlannerTour> RoutePlanner::nearestNeighborTour(const std::vector<int>& stops, int startId) const {
    RoutePlannerTour tour;
    tour.visitOrder.push_back(startId);

    std::set<int> unvisited(stops.begin(), stops.end());
    unvisited.erase(startId);

    int current = startId;
    while (!unvisited.empty()) {
        const auto result = runDijkstra(current);

        int bestNext = -1;
        double bestCost = std::numeric_limits<double>::infinity();
        for (int candidate : unvisited) {
            const auto it = result.distMinutes.find(candidate);
            if (it != result.distMinutes.end() && it->second < bestCost) {
                bestCost = it->second;
                bestNext = candidate;
            }
        }

        if (bestNext < 0 || bestCost == std::numeric_limits<double>::infinity()) {
            return std::nullopt;
        }

        tour.legs.push_back({current, bestNext, bestCost});
        tour.totalMinutes += bestCost;
        tour.visitOrder.push_back(bestNext);
        current = bestNext;
        unvisited.erase(bestNext);
    }

    return tour;
}

const RoutePlanner::EdgeView* RoutePlanner::findEdge(int fromId, int toId) const {
    const auto it = graph_.find(fromId);
    if (it == graph_.end()) return nullptr;
    for (const auto& edge : it->second) {
        if (edge.toId == toId) return &edge;
    }
    return nullptr;
}

std::string RoutePlanner::congestionLevelFor(int junctionId) const {
    const auto it = congestionLevels_.find(junctionId);
    return it == congestionLevels_.end() ? "LOW" : it->second;
}

double RoutePlanner::weightedMinutes(const EdgeView& edge) const {
    const double baseMinutes = (edge.distanceKm / std::max(1, edge.speedLimitKmh)) * 60.0;
    return baseMinutes * TrafficRules::congestionPenaltyMultiplier(congestionLevelFor(edge.toId));
}

RoutePlanner::DijkstraResult RoutePlanner::runDijkstra(int sourceId) const {
    DijkstraResult result;
    for (int node : nodes_) {
        result.distMinutes[node] = std::numeric_limits<double>::infinity();
        result.distKm[node] = 0.0;
    }

    if (!hasOutgoingRoads(sourceId)) {
        return result;
    }

    result.distMinutes[sourceId] = 0.0;

    using QItem = std::pair<double, int>;
    std::priority_queue<QItem, std::vector<QItem>, std::greater<>> queue;
    queue.push({0.0, sourceId});

    while (!queue.empty()) {
        auto [cost, fromId] = queue.top();
        queue.pop();
        if (cost > result.distMinutes[fromId]) continue;

        const auto graphIt = graph_.find(fromId);
        if (graphIt == graph_.end()) continue;

        for (const auto& edge : graphIt->second) {
            const double nextCost = cost + weightedMinutes(edge);
            if (nextCost < result.distMinutes[edge.toId]) {
                result.distMinutes[edge.toId] = nextCost;
                result.prev[edge.toId] = fromId;
                result.distKm[edge.toId] = result.distKm[fromId] + edge.distanceKm;
                queue.push({nextCost, edge.toId});
            }
        }
    }

    return result;
}

std::optional<std::vector<int>> RoutePlanner::reconstructPath(
    int fromId,
    int toId,
    const std::unordered_map<int, int>& prev,
    const std::unordered_map<int, double>& distMinutes) const {
    const auto distIt = distMinutes.find(toId);
    if (distIt == distMinutes.end() || distIt->second == std::numeric_limits<double>::infinity()) {
        return std::nullopt;
    }
    if (fromId == toId) {
        return std::vector<int>{fromId};
    }

    std::vector<int> path;
    int current = toId;
    while (current != fromId) {
        path.push_back(current);
        const auto prevIt = prev.find(current);
        if (prevIt == prev.end()) {
            return std::nullopt;
        }
        current = prevIt->second;
    }
    path.push_back(fromId);
    std::reverse(path.begin(), path.end());
    return path;
}
