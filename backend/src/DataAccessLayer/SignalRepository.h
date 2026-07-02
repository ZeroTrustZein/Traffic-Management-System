#pragma once
#include <vector>
#include "DatabaseConnection.h"
#include "../Models.h"

// W15: manages the junction_signals table — controls traffic-signal state
// per junction for emergency green-corridor creation.
class SignalRepository {
public:
    // Return all junction signal states.
    std::vector<JunctionSignal> findAll();

    // Set every junction in junctionIds to GREEN_PRIORITY, tagged with eventId.
    void setPriorityForRoute(const std::vector<int>& junctionIds, int eventId);

    // Release all junctions tagged with eventId back to NORMAL.
    void releaseByEvent(int eventId);

    // Set a single junction to GREEN_PRIORITY with no event tag
    // (used for transient emergency-vehicle detection).
    void setTransientPriority(int junctionId);
};
