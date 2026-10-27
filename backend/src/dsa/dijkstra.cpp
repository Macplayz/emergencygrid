#include "dsa/dijkstra.hpp"
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cmath>

namespace emergencygrid {

namespace {

struct PQCandidate {
    double distance{0.0};
    std::string nodeId;

    bool operator>(const PQCandidate& other) const noexcept {
        if (std::abs(distance - other.distance) > 1e-6) {
            return distance > other.distance;
        }
        return nodeId > other.nodeId; // Deterministic tie-breaker
    }
};

struct PredecessorInfo {
    std::string fromNodeId;
    std::string roadId;
    double edgeTravelTime{0.0};
    double edgeLengthKm{0.0};
};

} // namespace

RouteResult DijkstraRouter::findShortestPath(
    const RoadGraph& graph,
    const std::string& sourceId,
    const std::string& destinationId,
    TraceCollector* trace
) {
    RouteResult result;
    result.sourceId = sourceId;
    result.destinationId = destinationId;
    result.algorithm = "Dijkstra";

    // 1. Edge Case: Unknown source ID
    if (!graph.hasLocation(sourceId)) {
        if (trace && trace->isEnabled()) {
            TraceEvent ev;
            ev.algorithm = "Dijkstra";
            ev.operation = "no_route_found";
            ev.entityId = sourceId;
            ev.from = sourceId;
            ev.to = destinationId;
            ev.explanation = "Unknown source location ID '" + sourceId + "'. Search aborted.";
            trace->addEvent(std::move(ev));
        }
        return result;
    }

    // 2. Edge Case: Unknown destination ID
    if (!graph.hasLocation(destinationId)) {
        if (trace && trace->isEnabled()) {
            TraceEvent ev;
            ev.algorithm = "Dijkstra";
            ev.operation = "no_route_found";
            ev.entityId = destinationId;
            ev.from = sourceId;
            ev.to = destinationId;
            ev.explanation = "Unknown destination location ID '" + destinationId + "'. Search aborted.";
            trace->addEvent(std::move(ev));
        }
        return result;
    }

    // 3. Edge Case: Source equals Destination
    if (sourceId == destinationId) {
        result.routeFound = true;
        result.path = {sourceId};
        result.totalTravelTimeMinutes = 0.0;
        result.totalDistanceKm = 0.0;
        result.nodesSettled = 1;
        result.nodesExplored = 1;

        if (trace && trace->isEnabled()) {
            TraceEvent initEv;
            initEv.algorithm = "Dijkstra";
            initEv.operation = "initialize_distances";
            initEv.entityId = sourceId;
            initEv.cost = 0.0;
            initEv.explanation = "Source equals destination (" + sourceId + "). Route has 0 travel time.";
            trace->addEvent(std::move(initEv));

            TraceEvent reachedEv;
            reachedEv.algorithm = "Dijkstra";
            reachedEv.operation = "destination_reached";
            reachedEv.entityId = sourceId;
            reachedEv.cost = 0.0;
            reachedEv.explanation = "Already at destination " + sourceId + ".";
            trace->addEvent(std::move(reachedEv));

            TraceEvent reconEv;
            reconEv.algorithm = "Dijkstra";
            reconEv.operation = "route_reconstructed";
            reconEv.entityId = sourceId;
            reconEv.metadata = {
                {"path", {sourceId}},
                {"roadIds", nlohmann::json::array()},
                {"shortestPathTreeRoadIds", nlohmann::json::array()},
                {"shortestPathTree", nlohmann::json::array()}
            };
            reconEv.explanation = "Route is trivial: [" + sourceId + "] with travel time 0.00 min.";
            trace->addEvent(std::move(reconEv));
        }
        return result;
    }

    // 4. Standard Dijkstra Initialization
    std::priority_queue<PQCandidate, std::vector<PQCandidate>, std::greater<PQCandidate>> pq;
    std::unordered_map<std::string, double> distances;
    std::unordered_map<std::string, PredecessorInfo> predecessors;
    std::unordered_set<std::string> settled;

    const double inf = std::numeric_limits<double>::infinity();
    for (const auto& locId : graph.getLocationIds()) {
        distances[locId] = inf;
    }

    distances[sourceId] = 0.0;
    pq.push(PQCandidate{.distance = 0.0, .nodeId = sourceId});

    if (trace && trace->isEnabled()) {
        TraceEvent initEv;
        initEv.algorithm = "Dijkstra";
        initEv.operation = "initialize_distances";
        initEv.entityId = sourceId;
        initEv.from = sourceId;
        initEv.cost = 0.0;
        initEv.explanation = "Initialized all tentative distances to infinity. Set source " + sourceId + " to 0.0 min.";
        trace->addEvent(std::move(initEv));
    }

    bool destReached = false;

    // 5. Exploration Loop
    while (!pq.empty()) {
        auto current = pq.top();
        pq.pop();

        if (trace && trace->isEnabled()) {
            TraceEvent extractEv;
            extractEv.algorithm = "Dijkstra";
            extractEv.operation = "extract_min";
            extractEv.entityId = current.nodeId;
            extractEv.cost = current.distance;
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2);
            oss << "Extracted closest unvisited candidate " << current.nodeId 
                << " with tentative travel time " << current.distance << " min.";
            extractEv.explanation = oss.str();
            trace->addEvent(std::move(extractEv));
        }

        // Check for stale entry: already settled or higher distance than best recorded
        if (settled.find(current.nodeId) != settled.end() || current.distance > distances[current.nodeId] + 1e-6) {
            if (trace && trace->isEnabled()) {
                TraceEvent staleEv;
                staleEv.algorithm = "Dijkstra";
                staleEv.operation = "skip_stale_entry";
                staleEv.entityId = current.nodeId;
                staleEv.cost = current.distance;
                std::ostringstream oss;
                oss << std::fixed << std::setprecision(2);
                oss << "Skipping stale priority queue entry for " << current.nodeId 
                    << " (candidate cost: " << current.distance << "m vs known: " << distances[current.nodeId] << "m).";
                staleEv.explanation = oss.str();
                trace->addEvent(std::move(staleEv));
            }
            continue;
        }

        // Finalize node
        settled.insert(current.nodeId);
        result.nodesSettled++;

        if (trace && trace->isEnabled()) {
            TraceEvent finalizeEv;
            finalizeEv.algorithm = "Dijkstra";
            finalizeEv.operation = "finalize_node";
            finalizeEv.entityId = current.nodeId;
            finalizeEv.cost = current.distance;
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2);
            oss << "Finalized shortest travel time to " << current.nodeId << " as " << current.distance << " min.";
            finalizeEv.explanation = oss.str();
            trace->addEvent(std::move(finalizeEv));
        }

        // Early stopping if destination settled
        if (current.nodeId == destinationId) {
            destReached = true;
            if (trace && trace->isEnabled()) {
                TraceEvent destEv;
                destEv.algorithm = "Dijkstra";
                destEv.operation = "destination_reached";
                destEv.entityId = destinationId;
                destEv.cost = current.distance;
                std::ostringstream oss;
                oss << std::fixed << std::setprecision(2);
                oss << "Target destination " << destinationId << " settled with optimal travel time " << current.distance << " min.";
                destEv.explanation = oss.str();
                trace->addEvent(std::move(destEv));
            }
            break;
        }

        // Examine outgoing edges
        auto edges = graph.getOutgoingEdges(current.nodeId);
        for (const auto& edge : edges) {
            result.edgesExamined++;

            if (trace && trace->isEnabled()) {
                TraceEvent examineEv;
                examineEv.algorithm = "Dijkstra";
                examineEv.operation = "examine_edge";
                examineEv.entityId = edge.roadId;
                examineEv.from = current.nodeId;
                examineEv.to = edge.targetLocationId;
                examineEv.metadata = {
                    {"roadId", edge.roadId},
                    {"travelTimeMinutes", edge.travelTimeMinutes},
                    {"baseLengthKm", edge.baseLengthKm},
                    {"isClosed", edge.isClosed}
                };
                std::ostringstream oss;
                oss << std::fixed << std::setprecision(2);
                oss << "Examining road " << edge.roadId << " from " << current.nodeId 
                    << " to " << edge.targetLocationId << " (weight: " << edge.travelTimeMinutes << " min).";
                examineEv.explanation = oss.str();
                trace->addEvent(std::move(examineEv));
            }

            // Skip closed roads
            if (edge.isClosed) {
                if (trace && trace->isEnabled()) {
                    TraceEvent skipClosedEv;
                    skipClosedEv.algorithm = "Dijkstra";
                    skipClosedEv.operation = "skip_closed_road";
                    skipClosedEv.entityId = edge.roadId;
                    skipClosedEv.from = current.nodeId;
                    skipClosedEv.to = edge.targetLocationId;
                    skipClosedEv.explanation = "Road " + edge.roadId + " is CLOSED. Cannot traverse.";
                    trace->addEvent(std::move(skipClosedEv));
                }
                continue;
            }

            // Skip already settled neighbors
            if (settled.find(edge.targetLocationId) != settled.end()) {
                continue;
            }

            // Relaxation check
            double tentativeDist = current.distance + edge.travelTimeMinutes;
            double currentDist = distances[edge.targetLocationId];

            bool shouldRelax = false;
            if (tentativeDist < currentDist - 1e-6) {
                shouldRelax = true;
            } else if (std::abs(tentativeDist - currentDist) <= 1e-6 && predecessors.find(edge.targetLocationId) != predecessors.end()) {
                // Deterministic tie-breaking for equal-cost paths:
                // Compare predecessor node IDs, then road IDs
                const auto& existingPred = predecessors[edge.targetLocationId];
                if (current.nodeId < existingPred.fromNodeId || 
                    (current.nodeId == existingPred.fromNodeId && edge.roadId < existingPred.roadId)) {
                    shouldRelax = true;
                }
            }

            if (shouldRelax) {
                result.relaxations++;
                distances[edge.targetLocationId] = tentativeDist;
                predecessors[edge.targetLocationId] = PredecessorInfo{
                    .fromNodeId = current.nodeId,
                    .roadId = edge.roadId,
                    .edgeTravelTime = edge.travelTimeMinutes,
                    .edgeLengthKm = edge.baseLengthKm
                };

                pq.push(PQCandidate{.distance = tentativeDist, .nodeId = edge.targetLocationId});

                if (trace && trace->isEnabled()) {
                    TraceEvent relaxEv;
                    relaxEv.algorithm = "Dijkstra";
                    relaxEv.operation = "relax_edge";
                    relaxEv.entityId = edge.targetLocationId;
                    relaxEv.from = current.nodeId;
                    relaxEv.to = edge.targetLocationId;
                    if (currentDist < inf) {
                        relaxEv.previousValue = currentDist;
                    }
                    relaxEv.newValue = tentativeDist;
                    relaxEv.metadata = {
                        {"roadId", edge.roadId},
                        {"fromNodeId", current.nodeId},
                        {"toNodeId", edge.targetLocationId},
                        {"edgeWeight", edge.travelTimeMinutes},
                        {"predecessorUpdated", true}
                    };
                    std::ostringstream oss;
                    oss << std::fixed << std::setprecision(2);
                    oss << "Relaxed edge via " << current.nodeId << " to " << edge.targetLocationId 
                        << ": travel time improved from " << (currentDist < inf ? std::to_string(currentDist) : "INF") 
                        << " to " << tentativeDist << " min.";
                    relaxEv.explanation = oss.str();
                    trace->addEvent(std::move(relaxEv));
                }
            }
        }
    }

    result.nodesExplored = static_cast<uint32_t>(settled.size());

    // Collect confirmed Shortest-Path Tree (SPT) edges from predecessors
    for (const auto& [nodeId, pred] : predecessors) {
        if (distances.find(nodeId) != distances.end() && distances[nodeId] < inf) {
            result.shortestPathTreeRoadIds.push_back(pred.roadId);
            result.shortestPathTree.push_back({
                {"from", pred.fromNodeId},
                {"to", nodeId},
                {"roadId", pred.roadId},
                {"travelTimeMinutes", pred.edgeTravelTime},
                {"lengthKm", pred.edgeLengthKm}
            });
        }
    }
    std::sort(result.shortestPathTreeRoadIds.begin(), result.shortestPathTreeRoadIds.end());

    // 6. Path Reconstruction
    if (destReached || (distances.find(destinationId) != distances.end() && distances[destinationId] < inf && predecessors.find(destinationId) != predecessors.end())) {
        result.routeFound = true;
        result.totalTravelTimeMinutes = distances[destinationId];

        std::vector<std::string> revPath;
        std::vector<std::string> revRoads;
        double sumDistanceKm = 0.0;

        std::string curr = destinationId;
        revPath.push_back(curr);

        while (curr != sourceId) {
            auto it = predecessors.find(curr);
            if (it == predecessors.end()) {
                // Inconsistency safety guard
                result.routeFound = false;
                result.path.clear();
                result.roadIds.clear();
                return result;
            }
            revRoads.push_back(it->second.roadId);
            sumDistanceKm += it->second.edgeLengthKm;
            curr = it->second.fromNodeId;
            revPath.push_back(curr);
        }

        std::reverse(revPath.begin(), revPath.end());
        std::reverse(revRoads.begin(), revRoads.end());

        result.path = std::move(revPath);
        result.roadIds = std::move(revRoads);
        result.totalDistanceKm = sumDistanceKm;

        if (trace && trace->isEnabled()) {
            TraceEvent reconEv;
            reconEv.algorithm = "Dijkstra";
            reconEv.operation = "route_reconstructed";
            reconEv.from = sourceId;
            reconEv.to = destinationId;
            reconEv.cost = result.totalTravelTimeMinutes;
            reconEv.metadata = {
                {"path", result.path},
                {"roadIds", result.roadIds},
                {"shortestPathTreeRoadIds", result.shortestPathTreeRoadIds},
                {"shortestPathTree", result.shortestPathTree},
                {"totalTravelTimeMinutes", result.totalTravelTimeMinutes},
                {"totalDistanceKm", result.totalDistanceKm}
            };
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2);
            oss << "Successfully reconstructed shortest route from " << sourceId << " to " << destinationId 
                << " (" << result.path.size() << " nodes, " << result.totalTravelTimeMinutes << " min).";
            reconEv.explanation = oss.str();
            trace->addEvent(std::move(reconEv));
        }
    } else {
        // Destination unreachable
        result.routeFound = false;
        result.totalTravelTimeMinutes = 0.0;
        result.totalDistanceKm = 0.0;

        if (trace && trace->isEnabled()) {
            TraceEvent noRouteEv;
            noRouteEv.algorithm = "Dijkstra";
            noRouteEv.operation = "no_route_found";
            noRouteEv.from = sourceId;
            noRouteEv.to = destinationId;
            noRouteEv.metadata = {
                {"shortestPathTreeRoadIds", result.shortestPathTreeRoadIds},
                {"shortestPathTree", result.shortestPathTree}
            };
            noRouteEv.explanation = "Search exhausted all reachable nodes. Destination " + destinationId 
                + " is unreachable from " + sourceId + " (disconnected graph or road closures).";
            trace->addEvent(std::move(noRouteEv));
        }
    }

    return result;
}

} // namespace emergencygrid
