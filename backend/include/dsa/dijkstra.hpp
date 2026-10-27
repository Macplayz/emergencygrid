#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <limits>
#include "json.hpp"
#include "dsa/graph.hpp"
#include "core/trace.hpp"

namespace emergencygrid {

// RouteResult is defined in core/types.hpp

class DijkstraRouter {
public:
    static RouteResult findShortestPath(
        const RoadGraph& graph,
        const std::string& sourceId,
        const std::string& destinationId,
        TraceCollector* trace = nullptr
    );
};

} // namespace emergencygrid
