#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <cassert>
#include <iomanip>

#include "core/types.hpp"
#include "core/trace.hpp"
#include "dsa/graph.hpp"
#include "dsa/dijkstra.hpp"
#include "simulation/seed_data.hpp"

using namespace emergencygrid;

// Lightweight test framework
static int g_testsPassed = 0;
static int g_testsFailed = 0;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "  [FAIL] Line " << __LINE__ << ": " << (msg) << "\n"; \
            g_testsFailed++; \
            return; \
        } \
    } while (0)

#define TEST_ASSERT_NEAR(val1, val2, eps, msg) \
    do { \
        if (std::abs((val1) - (val2)) > (eps)) { \
            std::cerr << "  [FAIL] Line " << __LINE__ << ": " << (msg) \
                      << " (expected: " << (val2) << ", got: " << (val1) << ")\n"; \
            g_testsFailed++; \
            return; \
        } \
    } while (0)

#define RUN_TEST(fn) \
    do { \
        std::cout << "[RUNNING] " << #fn << " ...\n"; \
        int failedBefore = g_testsFailed; \
        fn(); \
        if (g_testsFailed == failedBefore) { \
            std::cout << "  [PASS] " << #fn << "\n"; \
            g_testsPassed++; \
        } \
    } while (0)

// 1. Known graph with a known shortest path
void test_known_shortest_path() {
    RoadGraph graph = createMetroGrid16();

    // Route from J1 (Central Plaza) to J4 (East Avenue)
    // Direct road R1_4 has base length 2.0 km, speed 40 km/h -> travel time = 3.0 min
    RouteResult res = DijkstraRouter::findShortestPath(graph, "J1", "J4");

    TEST_ASSERT(res.routeFound, "Route J1 -> J4 must be found");
    TEST_ASSERT_NEAR(res.totalTravelTimeMinutes, 3.0, 1e-4, "Direct route J1 -> J4 travel time should be 3.0 min");
    TEST_ASSERT(res.path.size() == 2, "Path should contain exactly 2 nodes");
    TEST_ASSERT(res.path[0] == "J1" && res.path[1] == "J4", "Path should be J1 -> J4");
    TEST_ASSERT(res.roadIds.size() == 1 && res.roadIds[0] == "R1_4", "Road used should be R1_4");
}

// 2. Longer physical route initially optimal due to higher speed limit
void test_longer_physical_route_faster() {
    RoadGraph graph;
    graph.addLocation(Location{.id = "A", .name = "Point A"});
    graph.addLocation(Location{.id = "B", .name = "Point B"});
    graph.addLocation(Location{.id = "C", .name = "Point C"});

    // Direct path A -> C: 5 km at 20 km/h -> 15.0 minutes
    graph.addRoad(RoadSegment{.id = "R_AC", .sourceId = "A", .targetId = "C", .baseLengthKm = 5.0, .speedLimitKmh = 20.0, .congestionFactor = 1.0, .isClosed = false, .isBidirectional = true});

    // Indirect path A -> B -> C: Total 12 km (longer distance), but Highway speed 120 km/h:
    // A -> B: 6 km at 120 km/h -> 3.0 minutes
    // B -> C: 6 km at 120 km/h -> 3.0 minutes
    // Total indirect = 6.0 minutes!
    graph.addRoad(RoadSegment{.id = "R_AB", .sourceId = "A", .targetId = "B", .baseLengthKm = 6.0, .speedLimitKmh = 120.0, .congestionFactor = 1.0, .isClosed = false, .isBidirectional = true});
    graph.addRoad(RoadSegment{.id = "R_BC", .sourceId = "B", .targetId = "C", .baseLengthKm = 6.0, .speedLimitKmh = 120.0, .congestionFactor = 1.0, .isClosed = false, .isBidirectional = true});

    RouteResult res = DijkstraRouter::findShortestPath(graph, "A", "C");

    TEST_ASSERT(res.routeFound, "Route A -> C must be found");
    TEST_ASSERT_NEAR(res.totalTravelTimeMinutes, 6.0, 1e-4, "Indirect highway route must be chosen with 6.0 minutes");
    TEST_ASSERT(res.path.size() == 3, "Optimal route must traverse A -> B -> C");
    TEST_ASSERT(res.path[0] == "A" && res.path[1] == "B" && res.path[2] == "C", "Route nodes must match A, B, C");
}

// 3. Road closure that changes the shortest path
void test_road_closure_rerouting() {
    RoadGraph graph = createMetroGrid16();

    // Baseline route from J1 to J5:
    // J1 -> J4 -> J5: (3.0 + 3.0 = 6.0 min)
    RouteResult baseline = DijkstraRouter::findShortestPath(graph, "J1", "J5");
    TEST_ASSERT(baseline.routeFound, "Baseline route must exist");
    TEST_ASSERT_NEAR(baseline.totalTravelTimeMinutes, 6.0, 1e-4, "Baseline travel time should be 6.0 min");
    TEST_ASSERT(baseline.path.size() == 3 && baseline.path[1] == "J4", "Baseline path goes through J4");

    // Close road R1_4 (between J1 and J4)
    bool closedOk = graph.setRoadClosed("R1_4", true);
    TEST_ASSERT(closedOk, "Closing road R1_4 must succeed");

    // Recalculate route J1 to J5
    RouteResult rerouted = DijkstraRouter::findShortestPath(graph, "J1", "J5");
    TEST_ASSERT(rerouted.routeFound, "Rerouted path must exist");
    TEST_ASSERT(rerouted.totalTravelTimeMinutes > 6.0, "Alternative path must take longer than 6.0 min");

    // Verify R1_4 is NOT used
    for (const auto& roadId : rerouted.roadIds) {
        TEST_ASSERT(roadId != "R1_4", "Closed road R1_4 must never appear in rerouted route");
    }

    // Reopening road restores optimal route
    graph.setRoadClosed("R1_4", false);
    RouteResult restored = DijkstraRouter::findShortestPath(graph, "J1", "J5");
    TEST_ASSERT_NEAR(restored.totalTravelTimeMinutes, 6.0, 1e-4, "Reopened road must restore 6.0 min optimal route");
}

// 4. Travel-time update (congestion) that changes the route
void test_congestion_update_rerouting() {
    RoadGraph graph = createMetroGrid16();

    // Route J1 to J5 initially takes J1 -> J4 -> J5 (6.0 min)
    // Alternate route: J1 -> J13 -> J3 -> J5 (2.0 + 3.0 + 4.0 = 9.0 min)
    RouteResult initial = DijkstraRouter::findShortestPath(graph, "J1", "J5");
    TEST_ASSERT_NEAR(initial.totalTravelTimeMinutes, 6.0, 1e-4, "Initial travel time should be 6.0 min");

    // Increase congestion on R1_4 by 3.5x:
    // Base 3.0 min * 3.5 = 10.5 min.
    // Total via J4 becomes 10.5 + 3.0 = 13.5 min!
    bool updated = graph.setRoadCongestion("R1_4", 3.5);
    TEST_ASSERT(updated, "Setting congestion on R1_4 must succeed");

    RouteResult congested = DijkstraRouter::findShortestPath(graph, "J1", "J5");
    TEST_ASSERT(congested.routeFound, "Route must still be found");
    TEST_ASSERT_NEAR(congested.totalTravelTimeMinutes, 9.0, 1e-4, "Route should switch to J1 -> J13 -> J3 -> J5 (9.0 min)");
    TEST_ASSERT(congested.path[1] != "J4", "Congested road J1-J4 should be bypassed");
}

// 5. Destination that is unreachable
void test_unreachable_destination() {
    RoadGraph graph = createMetroGrid16();

    // Isolate J5 by closing all incoming roads: R4_5, R3_5, R5_6
    graph.setRoadClosed("R4_5", true);
    graph.setRoadClosed("R3_5", true);
    graph.setRoadClosed("R5_6", true);

    TraceCollector trace(true);
    RouteResult res = DijkstraRouter::findShortestPath(graph, "J1", "J5", &trace);

    TEST_ASSERT(!res.routeFound, "Route to fully isolated destination must report routeFound = false");
    TEST_ASSERT(res.path.empty(), "Unreachable route path must be empty");
    TEST_ASSERT_NEAR(res.totalTravelTimeMinutes, 0.0, 1e-6, "Travel time for unreachable route should be 0.0");

    // Verify trace event has no_route_found
    bool foundNoRouteEvent = false;
    for (const auto& ev : trace.getEvents()) {
        if (ev.operation == "no_route_found") {
            foundNoRouteEvent = true;
            break;
        }
    }
    TEST_ASSERT(foundNoRouteEvent, "Trace must include 'no_route_found' event when search is exhausted");
}

// 6. Source equals destination
void test_source_equals_destination() {
    RoadGraph graph = createMetroGrid16();

    TraceCollector trace(true);
    RouteResult res = DijkstraRouter::findShortestPath(graph, "J3", "J3", &trace);

    TEST_ASSERT(res.routeFound, "Source equals destination must be valid");
    TEST_ASSERT_NEAR(res.totalTravelTimeMinutes, 0.0, 1e-6, "Travel time must be 0.0");
    TEST_ASSERT(res.path.size() == 1 && res.path[0] == "J3", "Path should contain only source node");
    TEST_ASSERT(res.roadIds.empty(), "Road list should be empty");
    TEST_ASSERT(trace.size() >= 3, "Trace should log initialization, destination reached, and reconstruction");
}

// 7. Invalid source and destination IDs
void test_invalid_location_ids() {
    RoadGraph graph = createMetroGrid16();

    // Unknown source
    RouteResult res1 = DijkstraRouter::findShortestPath(graph, "NON_EXISTENT", "J1");
    TEST_ASSERT(!res1.routeFound, "Unknown source must fail route lookup");

    // Unknown destination
    RouteResult res2 = DijkstraRouter::findShortestPath(graph, "J1", "INVALID_TARGET");
    TEST_ASSERT(!res2.routeFound, "Unknown destination must fail route lookup");

    // Both unknown
    RouteResult res3 = DijkstraRouter::findShortestPath(graph, "FOO", "BAR");
    TEST_ASSERT(!res3.routeFound, "Both unknown must fail route lookup");
}

// 8. Rejection of invalid travel-time weights and parameters
void test_invalid_weights_and_parameters() {
    RoadGraph graph;
    graph.addLocation(Location{.id = "N1", .name = "Node 1"});
    graph.addLocation(Location{.id = "N2", .name = "Node 2"});

    // Reject negative baseLength
    bool r1 = graph.addRoad(RoadSegment{.id = "BAD1", .sourceId = "N1", .targetId = "N2", .baseLengthKm = -1.0, .speedLimitKmh = 50.0});
    TEST_ASSERT(!r1, "Negative length road must be rejected");

    // Reject zero speedLimit
    bool r2 = graph.addRoad(RoadSegment{.id = "BAD2", .sourceId = "N1", .targetId = "N2", .baseLengthKm = 2.0, .speedLimitKmh = 0.0});
    TEST_ASSERT(!r2, "Zero speed limit road must be rejected");

    // Reject negative congestionFactor
    bool r3 = graph.addRoad(RoadSegment{.id = "BAD3", .sourceId = "N1", .targetId = "N2", .baseLengthKm = 2.0, .speedLimitKmh = 50.0, .congestionFactor = -0.5});
    TEST_ASSERT(!r3, "Negative congestion road must be rejected");

    // Add valid road
    bool rValid = graph.addRoad(RoadSegment{.id = "GOOD", .sourceId = "N1", .targetId = "N2", .baseLengthKm = 2.0, .speedLimitKmh = 50.0, .congestionFactor = 1.0});
    TEST_ASSERT(rValid, "Valid road must be accepted");

    // Reject negative congestion update
    bool up1 = graph.setRoadCongestion("GOOD", -2.0);
    TEST_ASSERT(!up1, "Negative congestion update must be rejected");

    // Reject zero travel time update
    bool up2 = graph.updateRoadTravelTime("GOOD", 0.0);
    TEST_ASSERT(!up2, "Zero travel time update must be rejected");

    // Reject update on non-existent road
    bool up3 = graph.setRoadClosed("GHOST_ROAD", true);
    TEST_ASSERT(!up3, "Update on non-existent road must be rejected");
}

// 9. Correct path reconstruction
void test_path_reconstruction_integrity() {
    RoadGraph graph = createMetroGrid16();

    RouteResult res = DijkstraRouter::findShortestPath(graph, "J11", "J6");
    TEST_ASSERT(res.routeFound, "Route J11 -> J6 must exist");
    TEST_ASSERT(res.path.front() == "J11", "First node in path must be source");
    TEST_ASSERT(res.path.back() == "J6", "Last node in path must be destination");
    TEST_ASSERT(res.roadIds.size() == res.path.size() - 1, "Number of roads must be nodes - 1");

    // Verify adjacency along every step of the returned path
    for (size_t i = 0; i < res.roadIds.size(); ++i) {
        const auto* road = graph.getRoad(res.roadIds[i]);
        TEST_ASSERT(road != nullptr, "Road in path must exist in graph");
        std::string u = res.path[i];
        std::string v = res.path[i + 1];

        bool connected = (road->sourceId == u && road->targetId == v) ||
                         (road->isBidirectional && road->sourceId == v && road->targetId == u);
        TEST_ASSERT(connected, "Road must connect consecutive nodes in reconstructed path");
    }
}

// 10. Deterministic results for equal-cost alternatives
void test_deterministic_equal_cost_tie_breaking() {
    RoadGraph graph;
    graph.addLocation(Location{.id = "S", .name = "Source"});
    graph.addLocation(Location{.id = "A", .name = "Path A"});
    graph.addLocation(Location{.id = "B", .name = "Path B"});
    graph.addLocation(Location{.id = "D", .name = "Destination"});

    // Path 1: S -> A -> D (3.0 + 3.0 = 6.0 min)
    graph.addRoad(RoadSegment{.id = "R_SA", .sourceId = "S", .targetId = "A", .baseLengthKm = 2.5, .speedLimitKmh = 50.0, .isBidirectional = true});
    graph.addRoad(RoadSegment{.id = "R_AD", .sourceId = "A", .targetId = "D", .baseLengthKm = 2.5, .speedLimitKmh = 50.0, .isBidirectional = true});

    // Path 2: S -> B -> D (3.0 + 3.0 = 6.0 min, exactly equal cost)
    graph.addRoad(RoadSegment{.id = "R_SB", .sourceId = "S", .targetId = "B", .baseLengthKm = 2.5, .speedLimitKmh = 50.0, .isBidirectional = true});
    graph.addRoad(RoadSegment{.id = "R_BD", .sourceId = "B", .targetId = "D", .baseLengthKm = 2.5, .speedLimitKmh = 50.0, .isBidirectional = true});

    // Run multiple times
    RouteResult run1 = DijkstraRouter::findShortestPath(graph, "S", "D");
    RouteResult run2 = DijkstraRouter::findShortestPath(graph, "S", "D");
    RouteResult run3 = DijkstraRouter::findShortestPath(graph, "S", "D");

    TEST_ASSERT(run1.routeFound && run2.routeFound && run3.routeFound, "All runs must find route");
    TEST_ASSERT(run1.path == run2.path && run2.path == run3.path, "Tie-breaking must produce identical paths across repeated runs");
    TEST_ASSERT(run1.roadIds == run2.roadIds && run2.roadIds == run3.roadIds, "Tie-breaking must produce identical road sequences");
    // Lexicographically, 'A' comes before 'B'
    TEST_ASSERT(run1.path[1] == "A", "Tie-breaker should consistently select lexicographically smaller node A");
}

// 11. Trace events correspond to actual algorithm operations
void test_trace_events_fidelity() {
    RoadGraph graph = createMetroGrid16();

    TraceCollector trace(true);
    RouteResult res = DijkstraRouter::findShortestPath(graph, "J1", "J3", &trace);

    TEST_ASSERT(res.routeFound, "Route J1 -> J3 must exist");
    TEST_ASSERT(trace.size() > 0, "Trace must collect events when enabled");

    const auto& events = trace.getEvents();
    TEST_ASSERT(events.front().operation == "initialize_distances", "First event must be initialize_distances");
    TEST_ASSERT(events.back().operation == "route_reconstructed", "Last event on success must be route_reconstructed");

    // Verify sequential step numbers
    for (size_t i = 0; i < events.size(); ++i) {
        TEST_ASSERT(events[i].step == static_cast<uint32_t>(i + 1), "Step numbers must be strictly sequential 1..N");
        TEST_ASSERT(!events[i].explanation.empty(), "Explanation must be present in every event");
    }

    // Verify operations present
    bool hasExtractMin = false;
    bool hasExamineEdge = false;
    bool hasRelaxEdge = false;
    bool hasFinalizeNode = false;

    for (const auto& ev : events) {
        if (ev.operation == "extract_min") hasExtractMin = true;
        if (ev.operation == "examine_edge") hasExamineEdge = true;
        if (ev.operation == "relax_edge") hasRelaxEdge = true;
        if (ev.operation == "finalize_node") hasFinalizeNode = true;
    }

    TEST_ASSERT(hasExtractMin, "Trace must contain extract_min");
    TEST_ASSERT(hasExamineEdge, "Trace must contain examine_edge");
    TEST_ASSERT(hasRelaxEdge, "Trace must contain relax_edge");
    TEST_ASSERT(hasFinalizeNode, "Trace must contain finalize_node");
}

// 12. Tracing enabled and disabled return identical route results
void test_tracing_equivalence() {
    RoadGraph graph = createMetroGrid16();

    TraceCollector trace(true);
    RouteResult withTrace = DijkstraRouter::findShortestPath(graph, "J10", "J5", &trace);
    RouteResult withoutTrace = DijkstraRouter::findShortestPath(graph, "J10", "J5", nullptr);

    TEST_ASSERT(withTrace.routeFound == withoutTrace.routeFound, "Found status must be identical");
    TEST_ASSERT_NEAR(withTrace.totalTravelTimeMinutes, withoutTrace.totalTravelTimeMinutes, 1e-6, "Travel times must be identical");
    TEST_ASSERT_NEAR(withTrace.totalDistanceKm, withoutTrace.totalDistanceKm, 1e-6, "Distances must be identical");
    TEST_ASSERT(withTrace.path == withoutTrace.path, "Paths must be identical");
    TEST_ASSERT(withTrace.roadIds == withoutTrace.roadIds, "Road sequences must be identical");
    TEST_ASSERT(withTrace.nodesSettled == withoutTrace.nodesSettled, "Settled node count must match");
    TEST_ASSERT(withTrace.edgesExamined == withoutTrace.edgesExamined, "Examined edge count must match");
    TEST_ASSERT(withTrace.relaxations == withoutTrace.relaxations, "Relaxations count must match");
}

// 13. A closed road is never used in the returned route
void test_closed_road_never_used() {
    RoadGraph graph = createMetroGrid16();

    // Close multiple roads
    graph.setRoadClosed("R1_2", true);
    graph.setRoadClosed("R1_4", true);
    graph.setRoadClosed("R1_8", true);

    RouteResult res = DijkstraRouter::findShortestPath(graph, "J1", "J7");
    TEST_ASSERT(res.routeFound, "Alternative path avoiding closed roads should exist");

    for (const auto& rId : res.roadIds) {
        const auto* r = graph.getRoad(rId);
        TEST_ASSERT(r != nullptr && !r->isClosed, "Returned route must NEVER contain a closed road segment");
    }
}

// 14. Repeated operations persist updated graph state
void test_state_persistence_across_operations() {
    RoadGraph graph = createMetroGrid16();

    // Query 1: Initial state
    RouteResult r1 = DijkstraRouter::findShortestPath(graph, "J1", "J2");
    TEST_ASSERT_NEAR(r1.totalTravelTimeMinutes, 3.0, 1e-4, "Initial travel time J1-J2 should be 3.0 min");

    // Mutation: Increase congestion factor to 2.0
    graph.setRoadCongestion("R1_2", 2.0);

    // Query 2: Must reflect updated congestion
    RouteResult r2 = DijkstraRouter::findShortestPath(graph, "J1", "J2");
    TEST_ASSERT_NEAR(r2.totalTravelTimeMinutes, 6.0, 1e-4, "Travel time must reflect congestion update (6.0 min)");

    // Mutation: Close road
    graph.setRoadClosed("R1_2", true);

    // Query 3: Must avoid closed road
    RouteResult r3 = DijkstraRouter::findShortestPath(graph, "J1", "J2");
    TEST_ASSERT(r3.routeFound, "Detour route must be found");
    TEST_ASSERT(r3.path[1] != "J2", "Direct road cannot be used when closed");
    TEST_ASSERT(r3.totalTravelTimeMinutes > 6.0, "Detour route must take longer");
}

// 15. Regression: J1 -> J16 route reconstruction and edge highlighting verification
void test_j1_to_j16_regression_route_reconstruction() {
    RoadGraph graph = createMetroGrid16();

    // Verify weights on the path
    const auto* r1_4 = graph.getRoad("R1_4");
    TEST_ASSERT(r1_4 != nullptr, "Road R1_4 must exist in MetroGrid-16");
    TEST_ASSERT_NEAR(r1_4->travelTimeMinutes(), 3.0, 1e-4, "R1_4 travel time must be 3.0 min (2.0km @ 40km/h)");

    const auto* r4_16 = graph.getRoad("R4_16");
    TEST_ASSERT(r4_16 != nullptr, "Road R4_16 must exist in MetroGrid-16");
    TEST_ASSERT_NEAR(r4_16->travelTimeMinutes(), 2.4, 1e-4, "R4_16 travel time must be 2.4 min (2.0km @ 50km/h)");

    TraceCollector trace;
    trace.setEnabled(true);

    RouteResult res = DijkstraRouter::findShortestPath(graph, "J1", "J16", &trace);

    // 1. Route result verification
    TEST_ASSERT(res.routeFound, "Route J1 -> J16 must be found");
    TEST_ASSERT(res.path.size() == 3, "Path must have exactly 3 nodes: [J1, J4, J16]");
    TEST_ASSERT(res.path[0] == "J1", "Source node must be J1");
    TEST_ASSERT(res.path[1] == "J4", "Intermediate node must be J4");
    TEST_ASSERT(res.path[2] == "J16", "Destination node must be J16");

    TEST_ASSERT(res.roadIds.size() == 2, "Path must use exactly 2 road segments: [R1_4, R4_16]");
    TEST_ASSERT(res.roadIds[0] == "R1_4", "First road segment must be R1_4");
    TEST_ASSERT(res.roadIds[1] == "R4_16", "Second road segment must be R4_16");

    TEST_ASSERT_NEAR(res.totalTravelTimeMinutes, 5.40, 1e-4, "Total travel time must be 5.40 min (3.0 + 2.4)");
    TEST_ASSERT_NEAR(res.totalDistanceKm, 4.0, 1e-4, "Total distance must be 4.0 km (2.0 + 2.0)");
    TEST_ASSERT(res.nodesSettled == 14, "Nodes settled must be 14 for J16 reaching condition in MetroGrid-16");

    // 2. Trace verification: final event must be route_reconstructed
    const auto& events = trace.getEvents();
    TEST_ASSERT(!events.empty(), "Trace events must not be empty");
    const auto& lastEv = events.back();
    TEST_ASSERT(lastEv.operation == "route_reconstructed", "Final trace event operation must be 'route_reconstructed'");
    TEST_ASSERT(lastEv.from == "J1" && lastEv.to == "J16", "Final trace event endpoints must be J1 -> J16");
    TEST_ASSERT(lastEv.cost.has_value(), "Final trace event cost must have a value");
    TEST_ASSERT_NEAR(lastEv.cost.value(), 5.40, 1e-4, "Final trace event cost must match 5.40 min");
    TEST_ASSERT(lastEv.metadata.contains("path") && lastEv.metadata["path"].size() == 3, "Trace metadata must contain 3-node path");
    TEST_ASSERT(lastEv.metadata.contains("roadIds") && lastEv.metadata["roadIds"].size() == 2, "Trace metadata must contain 2 roadIds");

    // 3. Serialization verification for frontend compatibility
    nlohmann::json rJson = r1_4->toJson();
    TEST_ASSERT(rJson.contains("source") && rJson.contains("target"), "RoadSegment::toJson must contain 'source' and 'target'");
    TEST_ASSERT(rJson.contains("sourceId") && rJson.contains("targetId"), "RoadSegment::toJson must contain 'sourceId' and 'targetId'");
    TEST_ASSERT(rJson["sourceId"] == "J1" && rJson["targetId"] == "J4", "R1_4 endpoints must match J1 -> J4");

    nlohmann::json resJson = res.toJson();
    TEST_ASSERT(resJson.contains("path") && resJson["path"].size() == 3, "RouteResult::toJson must serialize path");
    TEST_ASSERT(resJson.contains("roadIds") && resJson["roadIds"].size() == 2, "RouteResult::toJson must serialize roadIds");
    TEST_ASSERT(resJson.contains("shortestPathTreeRoadIds"), "RouteResult::toJson must serialize shortestPathTreeRoadIds");
}

int main() {
    std::cout << "======================================================\n";
    std::cout << " EmergencyGrid: C++ Graph & Dijkstra Test Suite       \n";
    std::cout << "======================================================\n\n";

    RUN_TEST(test_known_shortest_path);
    RUN_TEST(test_longer_physical_route_faster);
    RUN_TEST(test_road_closure_rerouting);
    RUN_TEST(test_congestion_update_rerouting);
    RUN_TEST(test_unreachable_destination);
    RUN_TEST(test_source_equals_destination);
    RUN_TEST(test_invalid_location_ids);
    RUN_TEST(test_invalid_weights_and_parameters);
    RUN_TEST(test_path_reconstruction_integrity);
    RUN_TEST(test_deterministic_equal_cost_tie_breaking);
    RUN_TEST(test_trace_events_fidelity);
    RUN_TEST(test_tracing_equivalence);
    RUN_TEST(test_closed_road_never_used);
    RUN_TEST(test_state_persistence_across_operations);
    RUN_TEST(test_j1_to_j16_regression_route_reconstruction);

    std::cout << "\n======================================================\n";
    std::cout << " Test Summary: " << g_testsPassed << " passed, " << g_testsFailed << " failed.\n";
    std::cout << "======================================================\n";

    return (g_testsFailed == 0) ? 0 : 1;
}
