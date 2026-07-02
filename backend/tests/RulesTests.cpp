#include <cassert>
#include <unordered_map>
#include "../src/MicroserviceLayer/VehicleValidator.h"
#include "../src/MicroserviceLayer/PlateValidator.h"
#include "../src/MicroserviceLayer/RoutePlanner.h"
#include "../src/MicroserviceLayer/ViolationRules.h"
#include "../src/MicroserviceLayer/TrafficRules.h"
#include "../src/Models.h"

int main() {
    {
        auto r = VehicleValidator::validatePlate("AB12CDE");
        assert(r.isValid);
        r = VehicleValidator::validatePlate("ABC-1234");
        assert(!r.isValid);
    }

    {
        assert(PlateValidator::validate("TT55ABC"));
        assert(!PlateValidator::validate("tt55abc"));
    }

    {
        auto errs = VehicleValidator::validateAll("AB12CDE", "John Doe", "john@example.com", "+49 123 4567", "CAR");
        assert(errs.empty());
        errs = VehicleValidator::validateAll("AB12CDE", "John Doe", "bad-email", "+49 123 4567", "CAR");
        assert(!errs.empty());
    }

    {
        assert(ViolationRules::severityForSpeed(91.0, 60) == "CRITICAL");
        assert(ViolationRules::severityForSpeed(82.0, 60) == "HIGH");
        assert(ViolationRules::severityForSpeed(72.0, 60) == "MEDIUM");
        assert(ViolationRules::severityForSpeed(65.0, 60) == "LOW");
        assert(ViolationRules::severityForEvent("RED_LIGHT", 0.0, false, 60) == "CRITICAL");
        assert(ViolationRules::severityForEvent("PARKING", 0.0, false, 60) == "MEDIUM");
        assert(!ViolationRules::isRepeatOffender(2));
        assert(ViolationRules::isRepeatOffender(3));
        assert(ViolationRules::repeatOffenderMultiplier() == 1.5);
    }

    {
        assert(TrafficRules::classifyCongestionLevel(0) == "LOW");
        assert(TrafficRules::classifyCongestionLevel(10) == "MODERATE");
        assert(TrafficRules::classifyCongestionLevel(20) == "HIGH");
        assert(TrafficRules::classifyCongestionLevel(30) == "SEVERE");
        assert(TrafficRules::congestionPenaltyMultiplier("LOW") == 1.0);
        assert(TrafficRules::congestionPenaltyMultiplier("MODERATE") > 1.0);
        assert(TrafficRules::congestionPenaltyMultiplier("SEVERE") > TrafficRules::congestionPenaltyMultiplier("HIGH"));
    }

    {
        std::vector<RoadSegment> segments = {
            {1, 1, 2, 1.0, 60, "A"},
            {2, 2, 3, 1.0, 60, "B"},
            {3, 1, 3, 5.0, 60, "C"},
            {4, 3, 4, 1.0, 60, "D"}
        };

        RoutePlanner plainPlanner(segments);
        auto plainRoute = plainPlanner.shortestPath(1, 3);
        assert(plainRoute);
        assert(plainRoute->junctionIds.size() == 3);
        assert(plainRoute->junctionIds[0] == 1);
        assert(plainRoute->junctionIds[1] == 2);
        assert(plainRoute->junctionIds[2] == 3);
        assert(plainRoute->steps.size() == 2);
        assert(plainRoute->totalDistanceKm == 2.0);

        RoutePlanner weightedPlanner(segments, {{2, "SEVERE"}, {3, "LOW"}, {4, "LOW"}});
        auto weightedRoute = weightedPlanner.shortestPath(1, 3);
        assert(weightedRoute);
        assert(weightedRoute->junctionIds.size() == 2);
        assert(weightedRoute->junctionIds[0] == 1);
        assert(weightedRoute->junctionIds[1] == 3);

        auto alternatives = weightedPlanner.topReachableRoutes(1, 2);
        assert(!alternatives.empty());
        assert(alternatives[0].destId == 3);

        auto tour = weightedPlanner.nearestNeighborTour({1, 3, 4}, 1);
        assert(tour);
        assert(tour->visitOrder.size() == 3);
        assert(tour->visitOrder[0] == 1);
    }

    return 0;
}
