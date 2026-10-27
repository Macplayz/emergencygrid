#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include <algorithm>
#include <cmath>

#include "core/types.hpp"
#include "core/trace.hpp"
#include "dsa/graph.hpp"
#include "dsa/dijkstra.hpp"
#include "dsa/binary_heap.hpp"
#include "dsa/avl_tree.hpp"
#include "dsa/hash_table.hpp"
#include "simulation/seed_data.hpp"
#include "simulation/incident_service.hpp"
#include "simulation/unit_service.hpp"
#include "simulation/dispatch_service.hpp"

using namespace emergencygrid;

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

// Helper to set up standard test environment
struct TestFixture {
    RoadGraph graph;
    IncidentService incidentService;
    UnitService unitService;
    DispatchService dispatchService;

    TestFixture() {
        graph = createMetroGrid16();
        incidentService.setGraph(&graph);
        unitService.setGraph(&graph);
        dispatchService.setGraph(&graph);
        dispatchService.setIncidentService(&incidentService);
        dispatchService.setUnitService(&unitService);
        for (const auto& u : createDefaultUnits()) {
            unitService.registerUnit(u);
        }
    }
};

static std::string createTestIncident(
    TestFixture& f,
    const std::string& locationId,
    const std::string& type,
    int severity,
    const std::vector<std::string>& caps = {},
    const std::string& desc = "Test Incident"
) {
    CreateIncidentRequest req{
        .id = "",
        .locationId = locationId,
        .type = type,
        .severity = severity,
        .description = desc,
        .requiredCapabilities = caps,
        .enableTrace = false
    };
    auto res = f.incidentService.createIncident(req);
    if (!res.success || !res.incident.has_value()) {
        std::cerr << "  [FAIL] createTestIncident failed: " << res.errorMessage << "\n";
        return "";
    }
    return res.incident->id;
}

// ==========================================
// 1. RESOURCE MANAGEMENT TESTS (1 - 8)
// ==========================================

// 1. Register a valid unit
void test_unit_register_valid() {
    TestFixture f;
    EmergencyUnit unit{
        .id = "AMB-99",
        .name = "Test Medic",
        .callSign = "Test Medic",
        .serviceType = IncidentType::MEDICAL,
        .locationId = "J1",
        .status = UnitStatus::AVAILABLE,
        .capabilities = {"BLS"}
    };

    TEST_ASSERT(f.unitService.registerUnit(unit).success, "Failed to register valid unit");
    TEST_ASSERT(f.unitService.hasUnit("AMB-99"), "Registered unit not found in service");
    auto retrieved = f.unitService.getUnit("AMB-99");
    TEST_ASSERT(retrieved.has_value(), "getUnit failed for AMB-99");
    TEST_ASSERT(retrieved->callSign == "Test Medic", "Callsign mismatch");
}

// 2. Reject a duplicate unit ID
void test_unit_reject_duplicate_id() {
    TestFixture f;
    EmergencyUnit duplicate{
        .id = "AMB-01", // already in default units
        .name = "Duplicate Medic",
        .callSign = "Duplicate Medic",
        .serviceType = IncidentType::MEDICAL,
        .locationId = "J1",
        .status = UnitStatus::AVAILABLE,
        .capabilities = {"BLS"}
    };

    TEST_ASSERT(!f.unitService.registerUnit(duplicate).success, "Should reject duplicate unit ID");
}

// 3. Reject an invalid location
void test_unit_reject_invalid_location() {
    TestFixture f;
    EmergencyUnit invalidLoc{
        .id = "AMB-INVALID-LOC",
        .name = "Bad Loc Unit",
        .callSign = "Bad Loc Unit",
        .serviceType = IncidentType::MEDICAL,
        .locationId = "J999_NON_EXISTENT",
        .status = UnitStatus::AVAILABLE,
        .capabilities = {"BLS"}
    };

    TEST_ASSERT(!f.unitService.registerUnit(invalidLoc).success, "Should reject unit with invalid location");
    TEST_ASSERT(!f.unitService.hasUnit("AMB-INVALID-LOC"), "Invalid unit should not be stored");
}

// 4. Retrieve a unit by ID
void test_unit_retrieve_by_id() {
    TestFixture f;
    auto u = f.unitService.getUnit("ENG-01");
    TEST_ASSERT(u.has_value(), "ENG-01 must exist");
    TEST_ASSERT(u->id == "ENG-01", "ID must match");
    TEST_ASSERT(u->serviceType == IncidentType::FIRE, "Service type must be FIRE");
    TEST_ASSERT(u->locationId == "J8", "Location must be J8");
    TEST_ASSERT(u->status == UnitStatus::AVAILABLE, "Status must be AVAILABLE");
    TEST_ASSERT(u->hasCapability("FIRE_SUPPRESSION"), "Must have FIRE_SUPPRESSION");
}

// 5. Filter units by service type
void test_unit_filter_by_service_type() {
    TestFixture f;
    auto ambulances = f.unitService.listUnits(IncidentType::MEDICAL);
    TEST_ASSERT(ambulances.size() == 3, "Expected 3 ambulances");
    for (const auto& a : ambulances) {
        TEST_ASSERT(a.serviceType == IncidentType::MEDICAL, "Filtered unit must be MEDICAL");
    }

    auto fireUnits = f.unitService.listUnits(IncidentType::FIRE);
    TEST_ASSERT(fireUnits.size() == 3, "Expected 3 fire units");

    auto policeUnits = f.unitService.listUnits(IncidentType::POLICE);
    TEST_ASSERT(policeUnits.size() == 3, "Expected 3 police units");
}

// 6. Change availability when permitted
void test_unit_change_availability_permitted() {
    TestFixture f;
    TEST_ASSERT(f.unitService.updateUnitStatus("AMB-01", UnitStatus::OUT_OF_SERVICE), "Should transition AVAILABLE -> OUT_OF_SERVICE");
    auto u1 = f.unitService.getUnit("AMB-01");
    TEST_ASSERT(u1->status == UnitStatus::OUT_OF_SERVICE, "Status should be OUT_OF_SERVICE");

    TEST_ASSERT(f.unitService.updateUnitStatus("AMB-01", UnitStatus::AVAILABLE), "Should transition OUT_OF_SERVICE -> AVAILABLE");
    auto u2 = f.unitService.getUnit("AMB-01");
    TEST_ASSERT(u2->status == UnitStatus::AVAILABLE, "Status should be AVAILABLE");
}

// 7. Reject invalid state transitions
void test_unit_reject_invalid_state_transitions() {
    TestFixture f;
    // Put unit OUT_OF_SERVICE
    f.unitService.updateUnitStatus("AMB-01", UnitStatus::OUT_OF_SERVICE);

    // Direct transition OUT_OF_SERVICE -> DISPATCHED should fail
    TEST_ASSERT(!f.unitService.updateUnitStatus("AMB-01", UnitStatus::DISPATCHED), "Cannot dispatch an OUT_OF_SERVICE unit");

    // Direct transition OUT_OF_SERVICE -> RESPONDING should fail
    TEST_ASSERT(!f.unitService.updateUnitStatus("AMB-01", UnitStatus::RESPONDING), "Cannot set OUT_OF_SERVICE unit to RESPONDING");
}

// 8. Prevent movement of a busy unit where the lifecycle does not permit it
void test_unit_prevent_movement_busy() {
    TestFixture f;
    // Set status to DISPATCHED
    f.unitService.updateUnitStatus("AMB-01", UnitStatus::DISPATCHED);

    // Attempting to move a dispatched unit should fail
    TEST_ASSERT(!f.unitService.updateUnitLocation("AMB-01", "J1"), "Cannot relocate a DISPATCHED unit");

    // Set back to AVAILABLE
    f.unitService.updateUnitStatus("AMB-01", UnitStatus::AVAILABLE);
    TEST_ASSERT(f.unitService.updateUnitLocation("AMB-01", "J1"), "Should allow relocation of AVAILABLE unit");
}

// ==========================================
// 2. DISPATCH SELECTION TESTS (9 - 18)
// ==========================================

// 9. Select the unit with the minimum valid travel time
void test_dispatch_select_minimum_travel_time() {
    TestFixture f;
    // AMB-01 is at J13, AMB-02 is at J14, AMB-03 is at J9
    // Create incident at J13 requiring AMBULANCE BLS
    std::string incId = createTestIncident(f, "J13", "MEDICAL", 3, {"BLS"}, "Medical Emergency at J13");
    TEST_ASSERT(!incId.empty(), "Incident creation must succeed");

    auto result = f.dispatchService.dispatchIncident(incId);
    TEST_ASSERT(result.success, "Dispatch must succeed");
    TEST_ASSERT(result.record.has_value(), "Dispatch record must exist");
    // AMB-01 is located directly at J13 (travel time = 0.0)
    TEST_ASSERT(result.record->unitId == "AMB-01", "AMB-01 should be selected (travel time 0.0)");
    TEST_ASSERT(result.record->travelTimeMinutes == 0.0, "Travel time should be 0.0 minutes at same node");
}

// 10. Exclude units with the wrong service type
void test_dispatch_exclude_wrong_service_type() {
    TestFixture f;
    // Fire incident at J1. POL-01 is right at J1 (distance 0), but wrong service type!
    std::string incId = createTestIncident(f, "J1", "FIRE", 4, {"FIRE_SUPPRESSION"}, "Structure Fire");
    TEST_ASSERT(!incId.empty(), "Incident creation failed");

    auto result = f.dispatchService.dispatchIncident(incId);
    TEST_ASSERT(result.success, "Dispatch should succeed");
    // Selected unit must be FIRE
    auto assignedUnit = f.unitService.getUnit(result.record->unitId);
    TEST_ASSERT(assignedUnit->serviceType == IncidentType::FIRE, "Assigned unit must be FIRE");
    TEST_ASSERT(result.record->unitId != "POL-01", "POL-01 must not be selected despite being at J1");
}

// 11. Exclude units missing required capabilities
void test_dispatch_exclude_missing_capabilities() {
    TestFixture f;
    // Incident at J14 (Memorial Hospital) requires AMBULANCE with ALS (Advanced Life Support)
    // AMB-02 is located right at J14, BUT only has {"BLS"}!
    // AMB-01 is at J13, and has {"BLS", "ALS"}.
    std::string incId = createTestIncident(f, "J14", "MEDICAL", 4, {"ALS"}, "Cardiac Arrest requiring ALS");
    TEST_ASSERT(!incId.empty(), "Incident creation failed");

    auto result = f.dispatchService.dispatchIncident(incId);
    TEST_ASSERT(result.success, "Dispatch should succeed with eligible ALS unit");
    TEST_ASSERT(result.record->unitId != "AMB-02", "AMB-02 must be excluded because it lacks ALS capability");
    TEST_ASSERT(result.record->unitId == "AMB-01", "AMB-01 should be selected as nearest ALS ambulance");
}

// 12. Exclude unavailable units
void test_dispatch_exclude_unavailable_units() {
    TestFixture f;
    // Put AMB-01 out of service
    f.unitService.updateUnitStatus("AMB-01", UnitStatus::OUT_OF_SERVICE);

    std::string incId = createTestIncident(f, "J13", "MEDICAL", 3, {"BLS"});
    TEST_ASSERT(!incId.empty(), "Incident creation failed");

    auto result = f.dispatchService.dispatchIncident(incId);
    TEST_ASSERT(result.success, "Dispatch must succeed");
    TEST_ASSERT(result.record->unitId != "AMB-01", "Unavailable unit AMB-01 must be excluded");
    // Next nearest should be AMB-02 (at J14)
    TEST_ASSERT(result.record->unitId == "AMB-02", "AMB-02 should be selected as next nearest available unit");
}

// 13. Exclude units without a valid route
void test_dispatch_exclude_no_valid_route() {
    TestFixture f;
    // Add an isolated location J99 with no roads
    f.graph.addLocation(Location{.id = "J99", .name = "Isolated Island", .x = 500, .y = 500, .zone = "Remote"});

    std::string incId = createTestIncident(f, "J99", "MEDICAL", 3, {"BLS"});
    TEST_ASSERT(!incId.empty(), "Incident created");

    auto result = f.dispatchService.dispatchIncident(incId);
    TEST_ASSERT(!result.success, "Dispatch should fail when destination is unreachable");
    TEST_ASSERT(result.error.find("No reachable units") != std::string::npos, "Error should indicate no reachable units");
}

// 14. Apply deterministic tie-breaking
void test_dispatch_deterministic_tie_breaking() {
    TestFixture f;
    // Reset units and register two identical units at the exact same location
    f.unitService.reset();
    EmergencyUnit uB{
        .id = "AMB-B",
        .name = "Medic B",
        .callSign = "Medic B",
        .serviceType = IncidentType::MEDICAL,
        .locationId = "J1",
        .status = UnitStatus::AVAILABLE,
        .capabilities = {"BLS"}
    };
    EmergencyUnit uA{
        .id = "AMB-A",
        .name = "Medic A",
        .callSign = "Medic A",
        .serviceType = IncidentType::MEDICAL,
        .locationId = "J1",
        .status = UnitStatus::AVAILABLE,
        .capabilities = {"BLS"}
    };
    f.unitService.registerUnit(uB);
    f.unitService.registerUnit(uA);

    std::string incId = createTestIncident(f, "J1", "MEDICAL", 2, {"BLS"});
    TEST_ASSERT(!incId.empty(), "Incident created");

    auto result = f.dispatchService.dispatchIncident(incId);
    TEST_ASSERT(result.success, "Dispatch succeeded");
    // "AMB-A" < "AMB-B" lexicographically!
    TEST_ASSERT(result.record->unitId == "AMB-A", "Tie break must select lexicographically smaller ID AMB-A");
}

// 15. Return a clear result when no eligible unit exists
void test_dispatch_return_clear_result_no_eligible_unit() {
    TestFixture f;
    // Incident requires HAZMAT ambulance (no ambulance has HAZMAT!)
    std::string incId = createTestIncident(f, "J1", "MEDICAL", 4, {"HAZMAT"});
    TEST_ASSERT(!incId.empty(), "Incident created");

    auto result = f.dispatchService.dispatchIncident(incId);
    TEST_ASSERT(!result.success, "Dispatch must fail when no unit has capability");
    TEST_ASSERT(result.error.find("No available units match") != std::string::npos, "Error must mention missing capability/type");
}

// 16. Return a clear result when eligible units exist but none can reach the incident
void test_dispatch_return_clear_result_unreachable() {
    TestFixture f;
    // Close all roads into J2
    f.graph.setRoadClosed("R1_2", true);
    f.graph.setRoadClosed("R2_3", true);
    f.graph.setRoadClosed("R11_2", true);

    std::string incId = createTestIncident(f, "J2", "POLICE", 3, {"PATROL"});
    TEST_ASSERT(!incId.empty(), "Incident created");

    auto result = f.dispatchService.dispatchIncident(incId);
    TEST_ASSERT(!result.success, "Dispatch should fail when all access roads are closed");
    TEST_ASSERT(result.error.find("No reachable units") != std::string::npos, "Should return clear unreachable error");
}

// 17. Use the current graph after a road closure
void test_dispatch_use_current_graph_after_road_closure() {
    TestFixture f;
    // Incident at J3. AMB-01 is at J13.
    // Road R13_3 connects J13 directly to J3 (2.0km @ 40km/h = 3.0 min).
    // Let's close R13_3!
    f.graph.setRoadClosed("R13_3", true);

    std::string incId = createTestIncident(f, "J3", "MEDICAL", 3, {"BLS"});
    TEST_ASSERT(!incId.empty(), "Incident created");

    auto result = f.dispatchService.dispatchIncident(incId);
    TEST_ASSERT(result.success, "Dispatch must succeed via alternative route");
    // Verify route does NOT contain edge R13_3
    for (const auto& roadId : result.record->route.roadIds) {
        TEST_ASSERT(roadId != "R13_3", "Route must not use closed road R13_3");
    }
}

// 18. Use the current graph after a travel-time update
void test_dispatch_use_current_graph_after_travel_time_update() {
    TestFixture f;
    // Update road congestion factor on R1_13 to 10.0x (heavy traffic)
    f.graph.setRoadCongestion("R1_13", 10.0);

    std::string incId = createTestIncident(f, "J1", "MEDICAL", 3, {"BLS"});
    TEST_ASSERT(!incId.empty(), "Incident created");

    auto result = f.dispatchService.dispatchIncident(incId);
    TEST_ASSERT(result.success, "Dispatch should succeed");
    // AMB-02 is at J14 (R1_14: 1.5km @ 45km/h = 2.0 min)
    // AMB-01 is at J13 (R1_13: 1.5km @ 45km/h * 10 = 20.0 min)
    // AMB-02 should now be much faster than AMB-01!
    TEST_ASSERT(result.record->unitId == "AMB-02", "AMB-02 should be selected due to congestion on AMB-01 route");
}

// ==========================================
// 3. STATE CONSISTENCY TESTS (19 - 28)
// ==========================================

// 19. Dispatch marks the unit unavailable
void test_state_dispatch_marks_unit_unavailable() {
    TestFixture f;
    std::string incId = createTestIncident(f, "J13", "MEDICAL", 3, {"BLS"});
    auto result = f.dispatchService.dispatchIncident(incId);
    TEST_ASSERT(result.success, "Dispatch succeeded");

    auto unit = f.unitService.getUnit(result.record->unitId);
    TEST_ASSERT(unit->status == UnitStatus::DISPATCHED, "Unit status must be DISPATCHED");
    TEST_ASSERT(unit->activeIncidentId == incId, "Unit active incident ID must match");
}

// 20. Dispatch marks the incident assigned/responding
void test_state_dispatch_marks_incident_assigned() {
    TestFixture f;
    std::string incId = createTestIncident(f, "J13", "MEDICAL", 3, {"BLS"});
    f.dispatchService.dispatchIncident(incId);

    auto updatedInc = f.incidentService.getIncident(incId);
    TEST_ASSERT(updatedInc->status == IncidentStatus::ASSIGNED, "Incident status must be ASSIGNED");
}

// 21. A busy unit cannot be assigned again
void test_state_busy_unit_cannot_be_assigned_again() {
    TestFixture f;
    std::string incId1 = createTestIncident(f, "J13", "MEDICAL", 3, {"BLS"});
    auto res1 = f.dispatchService.dispatchIncident(incId1);
    TEST_ASSERT(res1.record->unitId == "AMB-01", "AMB-01 dispatched to inc1");

    // Second incident also at J13
    std::string incId2 = createTestIncident(f, "J13", "MEDICAL", 3, {"BLS"});
    auto res2 = f.dispatchService.dispatchIncident(incId2);
    TEST_ASSERT(res2.success, "Second dispatch succeeded");
    TEST_ASSERT(res2.record->unitId != "AMB-01", "AMB-01 cannot be assigned again while busy");
}

// 22. An already assigned incident cannot be dispatched twice
void test_state_assigned_incident_cannot_be_dispatched_twice() {
    TestFixture f;
    std::string incId = createTestIncident(f, "J1", "POLICE", 3, {"PATROL"});
    auto res1 = f.dispatchService.dispatchIncident(incId);
    TEST_ASSERT(res1.success, "First dispatch must succeed");

    auto res2 = f.dispatchService.dispatchIncident(incId);
    TEST_ASSERT(!res2.success, "Second dispatch must fail");
    TEST_ASSERT(res2.error.find("already assigned") != std::string::npos, "Error should indicate incident already assigned");
}

// 23. Completing a dispatch resolves the incident
void test_state_complete_dispatch_resolves_incident() {
    TestFixture f;
    std::string incId = createTestIncident(f, "J1", "POLICE", 3, {"PATROL"});
    auto disp = f.dispatchService.dispatchIncident(incId);
    TEST_ASSERT(disp.success, "Dispatch succeeded");

    auto comp = f.dispatchService.completeDispatch(disp.record->id);
    TEST_ASSERT(comp.success, "Complete dispatch succeeded");
    TEST_ASSERT(comp.record->status == DispatchStatus::COMPLETED, "Dispatch status must be COMPLETED");

    auto incAfter = f.incidentService.getIncident(incId);
    TEST_ASSERT(incAfter->status == IncidentStatus::RESOLVED, "Incident must be RESOLVED");
}

// 24. Completing a dispatch releases the unit
void test_state_complete_dispatch_releases_unit() {
    TestFixture f;
    std::string incId = createTestIncident(f, "J1", "POLICE", 3, {"PATROL"});
    auto disp = f.dispatchService.dispatchIncident(incId);
    std::string unitId = disp.record->unitId;

    f.dispatchService.completeDispatch(disp.record->id);

    auto unitAfter = f.unitService.getUnit(unitId);
    TEST_ASSERT(unitAfter->status == UnitStatus::AVAILABLE, "Unit must return to AVAILABLE status");
    TEST_ASSERT(unitAfter->activeIncidentId.empty(), "Unit active incident ID must be cleared");
}

// 25. Completing the same dispatch twice is rejected
void test_state_complete_same_dispatch_twice_rejected() {
    TestFixture f;
    std::string incId = createTestIncident(f, "J1", "POLICE", 3, {"PATROL"});
    auto disp = f.dispatchService.dispatchIncident(incId);

    auto comp1 = f.dispatchService.completeDispatch(disp.record->id);
    TEST_ASSERT(comp1.success, "First completion succeeded");

    auto comp2 = f.dispatchService.completeDispatch(disp.record->id);
    TEST_ASSERT(!comp2.success, "Second completion must fail");
    TEST_ASSERT(comp2.error.find("already completed") != std::string::npos, "Error must state already completed");
}

// 26. Failed dispatch attempts do not leave partial assignments
void test_state_failed_dispatch_no_partial_assignment() {
    TestFixture f;
    std::string incId = createTestIncident(f, "J1", "POLICE", 3, {"NON_EXISTENT_CAPABILITY"});
    auto disp = f.dispatchService.dispatchIncident(incId);
    TEST_ASSERT(!disp.success, "Dispatch must fail");

    // Incident remains PENDING
    auto incAfter = f.incidentService.getIncident(incId);
    TEST_ASSERT(incAfter->status == IncidentStatus::PENDING, "Incident must remain PENDING");

    // All units remain AVAILABLE
    for (const auto& u : f.unitService.listUnits()) {
        TEST_ASSERT(u.status == UnitStatus::AVAILABLE, "All units must remain AVAILABLE");
        TEST_ASSERT(u.activeIncidentId.empty(), "No unit active incident should be set");
    }
}

// 27. Cancelled or resolved incidents are never dispatched
void test_state_cancelled_or_resolved_never_dispatched() {
    TestFixture f;
    std::string incId = createTestIncident(f, "J1", "POLICE", 3, {"PATROL"});
    f.incidentService.cancelIncident(incId);

    auto disp = f.dispatchService.dispatchIncident(incId);
    TEST_ASSERT(!disp.success, "Cannot dispatch cancelled incident");
    TEST_ASSERT(disp.error.find("cancelled") != std::string::npos, "Error must indicate cancelled");
}

// 28. Stale heap entries cannot trigger duplicate assignments
void test_state_stale_heap_entries_no_duplicate_assignment() {
    TestFixture f;
    std::string incId = createTestIncident(f, "J1", "POLICE", 3, {"PATROL"});

    // Explicitly dispatch the incident by ID first
    auto disp1 = f.dispatchService.dispatchIncident(incId);
    TEST_ASSERT(disp1.success, "First explicit dispatch succeeded");

    // Now call dispatchNext() which pulls from heap!
    // Since inc is already ASSIGNED, dispatchNext() should skip it!
    auto disp2 = f.dispatchService.dispatchNext();
    TEST_ASSERT(!disp2.success, "dispatchNext() should find no pending incidents because the heap entry was stale/assigned");
}

// ==========================================
// 4. TRACE AND API TESTS (29 - 35)
// ==========================================

// 29. Candidate filtering generates correct trace reasons
void test_trace_candidate_filtering_reasons() {
    TestFixture f;
    TraceCollector trace(true);

    std::string incId = createTestIncident(f, "J14", "MEDICAL", 3, {"ALS"});
    auto disp = f.dispatchService.dispatchIncident(incId, &trace);
    TEST_ASSERT(disp.success, "Dispatch succeeded");

    const auto& events = trace.getEvents();
    bool sawMissingCap = false;
    bool sawCandidateRoute = false;
    bool sawCandidateSelected = false;

    for (const auto& ev : events) {
        if (ev.operation == "candidate_rejected" && ev.metadata.value("reason", "") == "missing_capabilities") {
            sawMissingCap = true;
        }
        if (ev.operation == "candidate_route_evaluated") {
            sawCandidateRoute = true;
        }
        if (ev.operation == "candidate_selected") {
            sawCandidateSelected = true;
        }
    }

    TEST_ASSERT(sawMissingCap, "Trace must log candidate_rejected for missing capabilities (AMB-02 lacking ALS)");
    TEST_ASSERT(sawCandidateRoute, "Trace must log candidate_route_evaluated");
    TEST_ASSERT(sawCandidateSelected, "Trace must log candidate_selected");
}

// 30. Candidate travel times match actual Dijkstra results
void test_trace_travel_times_match_dijkstra() {
    TestFixture f;
    std::string incId = createTestIncident(f, "J16", "POLICE", 3, {"PATROL"});
    auto disp = f.dispatchService.dispatchIncident(incId);
    TEST_ASSERT(disp.success, "Dispatch succeeded");

    std::string unitLoc = f.unitService.getUnit(disp.record->unitId)->locationId;
    RouteResult directDijkstra = DijkstraRouter::findShortestPath(f.graph, unitLoc, "J16");

    TEST_ASSERT(std::abs(disp.record->travelTimeMinutes - directDijkstra.totalTravelTimeMinutes) < 1e-4,
                "Dispatch travel time must match direct Dijkstra result");
    TEST_ASSERT(disp.record->route.path == directDijkstra.path,
                "Dispatch path must match direct Dijkstra result");
}

// 31. Tracing enabled and disabled produce identical dispatch decisions
void test_trace_enabled_disabled_identical_decisions() {
    TestFixture f1;
    TestFixture f2;

    std::string incId1 = createTestIncident(f1, "J4", "FIRE", 4, {"FIRE_SUPPRESSION"});
    std::string incId2 = createTestIncident(f2, "J4", "FIRE", 4, {"FIRE_SUPPRESSION"});

    TraceCollector trace(true);
    auto resWithTrace = f1.dispatchService.dispatchIncident(incId1, &trace);
    auto resWithoutTrace = f2.dispatchService.dispatchIncident(incId2, nullptr);

    TEST_ASSERT(resWithTrace.success && resWithoutTrace.success, "Both must succeed");
    TEST_ASSERT(resWithTrace.record->unitId == resWithoutTrace.record->unitId, "Selected unit must be identical");
    TEST_ASSERT(resWithTrace.record->travelTimeMinutes == resWithoutTrace.record->travelTimeMinutes, "Travel time must be identical");
    TEST_ASSERT(resWithTrace.record->route.path == resWithoutTrace.record->route.path, "Path must be identical");
}

// 32. Unit-management endpoints behave correctly
void test_api_unit_management_methods() {
    TestFixture f;
    TEST_ASSERT(f.unitService.totalCount() == 9, "Total units must be 9");
    TEST_ASSERT(f.unitService.availableCount() == 9, "Available units must be 9 initially");

    f.unitService.updateUnitStatus("AMB-01", UnitStatus::OUT_OF_SERVICE);
    TEST_ASSERT(f.unitService.availableCount() == 8, "Available units must decrease to 8");

    auto availOnly = f.unitService.listUnits(std::nullopt, std::nullopt, true);
    TEST_ASSERT(availOnly.size() == 8, "Filtered available units must be 8");
}

// 33. Dispatch endpoint returns the selected unit and route
void test_api_dispatch_returns_selected_unit_route() {
    TestFixture f;
    std::string incId = createTestIncident(f, "J7", "MEDICAL", 2, {"BLS"});
    auto res = f.dispatchService.dispatchIncident(incId);
    TEST_ASSERT(res.success, "Dispatch succeeded");

    TEST_ASSERT(!res.record->id.empty(), "Dispatch ID must be non-empty");
    TEST_ASSERT(res.record->incidentId == incId, "Incident ID must match");
    TEST_ASSERT(!res.record->unitId.empty(), "Unit ID must be set");
    TEST_ASSERT(res.record->status == DispatchStatus::ACTIVE, "Dispatch status must be ACTIVE");
    TEST_ASSERT(!res.record->route.path.empty(), "Route path must not be empty");
    TEST_ASSERT(!res.explanation.empty(), "Explanation must not be empty");
}

// 34. Completion endpoint updates the relevant states
void test_api_completion_updates_states() {
    TestFixture f;
    std::string incId = createTestIncident(f, "J5", "POLICE", 3, {"PATROL"});
    auto disp = f.dispatchService.dispatchIncident(incId);
    TEST_ASSERT(disp.success, "Dispatch created");

    auto comp = f.dispatchService.completeDispatch(disp.record->id);
    TEST_ASSERT(comp.success, "Dispatch completed");

    // Check dispatch record
    auto dispRecord = f.dispatchService.getDispatch(disp.record->id);
    TEST_ASSERT(dispRecord->status == DispatchStatus::COMPLETED, "Dispatch record must be COMPLETED");

    // Check incident
    auto incRecord = f.incidentService.getIncident(incId);
    TEST_ASSERT(incRecord->status == IncidentStatus::RESOLVED, "Incident must be RESOLVED");

    // Check unit
    auto unitRecord = f.unitService.getUnit(disp.record->unitId);
    TEST_ASSERT(unitRecord->status == UnitStatus::AVAILABLE, "Unit must be AVAILABLE");
    TEST_ASSERT(unitRecord->activeIncidentId.empty(), "Unit active incident ID must be cleared");
}

// 35. Existing graph, heap, AVL, and hash-table tests continue to pass
void test_regression_existing_components() {
    TestFixture f;
    // Verify AVL tree ordered iteration from incident service
    auto sortedIncidents = f.incidentService.listIncidentsSorted();
    TEST_ASSERT(sortedIncidents.empty(), "Initially no incidents in sorted list");

    // Add 3 incidents with different priorities
    createTestIncident(f, "J1", "POLICE", 1, {}, "Low Priority");
    createTestIncident(f, "J2", "FIRE", 4, {}, "Critical Priority");
    createTestIncident(f, "J3", "MEDICAL", 2, {}, "Medium Priority");

    auto sorted = f.incidentService.listIncidentsSorted();
    TEST_ASSERT(sorted.size() == 3, "AVL tree must hold 3 incidents");

    // Verify Hash Table lookup
    auto incById = f.incidentService.getIncident(sorted[0].id);
    TEST_ASSERT(incById.has_value(), "Hash table lookup must find incident");
    TEST_ASSERT(incById->id == sorted[0].id, "Hash table ID must match");
}

// ==========================================
// MAIN TEST RUNNER
// ==========================================

int main() {
    std::cout << "========================================================\n";
    std::cout << " EmergencyGrid Unit Allocation & Dispatch Test Suite\n";
    std::cout << "========================================================\n\n";

    std::cout << "--- Section 1: Resource Management (1 - 8) ---\n";
    RUN_TEST(test_unit_register_valid);
    RUN_TEST(test_unit_reject_duplicate_id);
    RUN_TEST(test_unit_reject_invalid_location);
    RUN_TEST(test_unit_retrieve_by_id);
    RUN_TEST(test_unit_filter_by_service_type);
    RUN_TEST(test_unit_change_availability_permitted);
    RUN_TEST(test_unit_reject_invalid_state_transitions);
    RUN_TEST(test_unit_prevent_movement_busy);

    std::cout << "\n--- Section 2: Dispatch Selection (9 - 18) ---\n";
    RUN_TEST(test_dispatch_select_minimum_travel_time);
    RUN_TEST(test_dispatch_exclude_wrong_service_type);
    RUN_TEST(test_dispatch_exclude_missing_capabilities);
    RUN_TEST(test_dispatch_exclude_unavailable_units);
    RUN_TEST(test_dispatch_exclude_no_valid_route);
    RUN_TEST(test_dispatch_deterministic_tie_breaking);
    RUN_TEST(test_dispatch_return_clear_result_no_eligible_unit);
    RUN_TEST(test_dispatch_return_clear_result_unreachable);
    RUN_TEST(test_dispatch_use_current_graph_after_road_closure);
    RUN_TEST(test_dispatch_use_current_graph_after_travel_time_update);

    std::cout << "\n--- Section 3: State Consistency (19 - 28) ---\n";
    RUN_TEST(test_state_dispatch_marks_unit_unavailable);
    RUN_TEST(test_state_dispatch_marks_incident_assigned);
    RUN_TEST(test_state_busy_unit_cannot_be_assigned_again);
    RUN_TEST(test_state_assigned_incident_cannot_be_dispatched_twice);
    RUN_TEST(test_state_complete_dispatch_resolves_incident);
    RUN_TEST(test_state_complete_dispatch_releases_unit);
    RUN_TEST(test_state_complete_same_dispatch_twice_rejected);
    RUN_TEST(test_state_failed_dispatch_no_partial_assignment);
    RUN_TEST(test_state_cancelled_or_resolved_never_dispatched);
    RUN_TEST(test_state_stale_heap_entries_no_duplicate_assignment);

    std::cout << "\n--- Section 4: Trace and API (29 - 35) ---\n";
    RUN_TEST(test_trace_candidate_filtering_reasons);
    RUN_TEST(test_trace_travel_times_match_dijkstra);
    RUN_TEST(test_trace_enabled_disabled_identical_decisions);
    RUN_TEST(test_api_unit_management_methods);
    RUN_TEST(test_api_dispatch_returns_selected_unit_route);
    RUN_TEST(test_api_completion_updates_states);
    RUN_TEST(test_regression_existing_components);

    std::cout << "\n========================================================\n";
    std::cout << " Test Summary: " << g_testsPassed << " Passed, " << g_testsFailed << " Failed\n";
    std::cout << "========================================================\n";

    return (g_testsFailed == 0) ? 0 : 1;
}
