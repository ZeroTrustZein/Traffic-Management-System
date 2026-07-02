#pragma once
#include <vector>
#include "DatabaseConnection.h"
#include "../Models.h"

// Reads the directed road graph (junction-to-junction edges) used by the
// route-guidance algorithm. Inserts are not currently exposed via API — the
// graph is bootstrapped from seed.sql.
class RoadSegmentRepository {
public:
    std::vector<RoadSegment> findAll();
    std::vector<RoadSegment> findOutgoing(int fromJunctionId);
};
