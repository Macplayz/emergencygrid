#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include <algorithm>
#include <random>

#include "core/types.hpp"
#include "core/trace.hpp"
#include "dsa/graph.hpp"
#include "dsa/binary_heap.hpp"
#include "simulation/seed_data.hpp"
#include "simulation/incident_service.hpp"

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

// Helper to make an incident
static Incident makeInc(
    const std::string& id,
    IncidentSeverity severity,
    uint64_t arrivalSeq,
    const std::string& loc = "J1",
    IncidentType type = IncidentType::MEDICAL
) {
    Incident inc;
    inc.id = id;
    inc.severity = severity;
    inc.arrivalSequence = arrivalSeq;
    inc.locationId = loc;
    inc.type = type;
    inc.status = IncidentStatus::PENDING;
    inc.computedPriority = static_cast<double>(severity) * 1000.0;
    return inc;
}

// -------------------------------------------------------------
// HEAP CORRECTNESS TESTS (1 - 9)
// -------------------------------------------------------------

// 1. Inserting one incident
void test_insert_one_incident() {
    BinaryHeap heap;
    TEST_ASSERT(heap.isEmpty(), "Heap must initially be empty");
    TEST_ASSERT(heap.size() == 0, "Heap initial size must be 0");

    heap.insert(makeInc("INC-1", IncidentSeverity::HIGH, 1));
    TEST_ASSERT(!heap.isEmpty(), "Heap must not be empty after insert");
    TEST_ASSERT(heap.size() == 1, "Heap size must be 1 after one insert");

    auto top = heap.peek();
    TEST_ASSERT(top.has_value(), "Peek must return value for non-empty heap");
    TEST_ASSERT(top->id == "INC-1", "Peeked item ID must match inserted ID");
}

// 2. Inserting multiple incidents with different severities
void test_insert_multiple_different_severities() {
    BinaryHeap heap;
    heap.insert(makeInc("INC-LOW", IncidentSeverity::LOW, 1));
    heap.insert(makeInc("INC-CRIT", IncidentSeverity::CRITICAL, 2));
    heap.insert(makeInc("INC-MED", IncidentSeverity::MEDIUM, 3));
    heap.insert(makeInc("INC-HIGH", IncidentSeverity::HIGH, 4));

    TEST_ASSERT(heap.size() == 4, "Heap must contain 4 items");

    // Root must be CRITICAL
    auto top = heap.peek();
    TEST_ASSERT(top.has_value(), "Peek should succeed");
    TEST_ASSERT(top->id == "INC-CRIT", "Highest severity must be at root");
}

// 3. Extracting the highest-priority incident
void test_extract_highest_priority() {
    BinaryHeap heap;
    heap.insert(makeInc("INC-MED", IncidentSeverity::MEDIUM, 1));
    heap.insert(makeInc("INC-HIGH", IncidentSeverity::HIGH, 2));
    heap.insert(makeInc("INC-CRIT", IncidentSeverity::CRITICAL, 3));

    auto extracted = heap.extractMax();
    TEST_ASSERT(extracted.has_value(), "ExtractMax must return an item");
    TEST_ASSERT(extracted->id == "INC-CRIT", "Extracted item must be CRITICAL");
    TEST_ASSERT(heap.size() == 2, "Heap size must be 2 after extract");

    auto next = heap.extractMax();
    TEST_ASSERT(next.has_value(), "Second extractMax must return an item");
    TEST_ASSERT(next->id == "INC-HIGH", "Second extracted item must be HIGH");
}

// 4. Preserving heap order after repeated insertions and extractions
void test_preserve_heap_order_repeated_ops() {
    BinaryHeap heap;
    std::vector<IncidentSeverity> severities = {
        IncidentSeverity::LOW,
        IncidentSeverity::CRITICAL,
        IncidentSeverity::MEDIUM,
        IncidentSeverity::HIGH,
        IncidentSeverity::CRITICAL,
        IncidentSeverity::LOW
    };

    uint64_t seq = 0;
    for (auto sev : severities) {
        heap.insert(makeInc("INC-" + std::to_string(++seq), sev, seq));
    }

    IncidentSeverity prevSev = IncidentSeverity::CRITICAL;
    while (!heap.isEmpty()) {
        auto top = heap.extractMax();
        TEST_ASSERT(top.has_value(), "Extract must yield element");
        TEST_ASSERT(static_cast<int>(top->severity) <= static_cast<int>(prevSev),
                    "Each extracted severity must be <= previous extracted severity");
        prevSev = top->severity;
    }
}

// 5. Empty-heap behavior
void test_empty_heap_behavior() {
    BinaryHeap heap;
    TEST_ASSERT(heap.isEmpty(), "New heap must be empty");
    TEST_ASSERT(heap.size() == 0, "New heap size must be 0");
    TEST_ASSERT(!heap.peek().has_value(), "Peek on empty heap must return nullopt");
    TEST_ASSERT(!heap.extractMax().has_value(), "ExtractMax on empty heap must return nullopt");
}

// 6. Peek does not remove an entry
void test_peek_does_not_mutate() {
    BinaryHeap heap;
    heap.insert(makeInc("INC-1", IncidentSeverity::CRITICAL, 1));
    heap.insert(makeInc("INC-2", IncidentSeverity::HIGH, 2));

    size_t sizeBefore = heap.size();
    auto peek1 = heap.peek();
    auto peek2 = heap.peek();

    TEST_ASSERT(heap.size() == sizeBefore, "Peek must not alter heap size");
    TEST_ASSERT(peek1.has_value() && peek2.has_value(), "Peek must return value");
    TEST_ASSERT(peek1->id == peek2->id, "Repeated peek must yield identical result");
    TEST_ASSERT(peek1->id == "INC-1", "Peeked item must be the highest priority");
}

// 7. Equal-severity incidents follow arrival order (FIFO)
void test_equal_severity_follows_arrival_order() {
    BinaryHeap heap;
    heap.insert(makeInc("INC-SECOND", IncidentSeverity::HIGH, 20));
    heap.insert(makeInc("INC-FIRST", IncidentSeverity::HIGH, 10));
    heap.insert(makeInc("INC-THIRD", IncidentSeverity::HIGH, 30));

    auto first = heap.extractMax();
    auto second = heap.extractMax();
    auto third = heap.extractMax();

    TEST_ASSERT(first->id == "INC-FIRST", "Earlier arrival 10 must be extracted first");
    TEST_ASSERT(second->id == "INC-SECOND", "Arrival 20 must be extracted second");
    TEST_ASSERT(third->id == "INC-THIRD", "Arrival 30 must be extracted third");
}

// 8. Deterministic tie-breaking
void test_deterministic_tie_breaking() {
    BinaryHeap heap;
    // Identical severity and arrival sequence: lexicographical ID must break the tie
    heap.insert(makeInc("INC-B", IncidentSeverity::CRITICAL, 1));
    heap.insert(makeInc("INC-A", IncidentSeverity::CRITICAL, 1));
    heap.insert(makeInc("INC-C", IncidentSeverity::CRITICAL, 1));

    auto first = heap.extractMax();
    auto second = heap.extractMax();
    auto third = heap.extractMax();

    TEST_ASSERT(first->id == "INC-A", "Lexicographically smallest ID 'INC-A' must come first");
    TEST_ASSERT(second->id == "INC-B", "INC-B must come second");
    TEST_ASSERT(third->id == "INC-C", "INC-C must come third");
}

// 9. Correct behavior after a large number of insertions
void test_large_number_of_insertions() {
    BinaryHeap heap;
    const int count = 200;

    for (int i = 0; i < count; ++i) {
        IncidentSeverity sev = static_cast<IncidentSeverity>((i % 4) + 1);
        heap.insert(makeInc("INC-" + std::to_string(i), sev, static_cast<uint64_t>(i)));
    }

    TEST_ASSERT(heap.size() == static_cast<size_t>(count), "Heap size must equal count");

    // Extracted items must be monotonically non-increasing in priority
    Incident prev = heap.extractMax().value();
    while (!heap.isEmpty()) {
        Incident cur = heap.extractMax().value();
        bool correctOrder = (static_cast<int>(prev.severity) > static_cast<int>(cur.severity)) ||
                            (prev.severity == cur.severity && prev.arrivalSequence <= cur.arrivalSequence);
        TEST_ASSERT(correctOrder, "Heap invariant violated during large extraction sequence");
        prev = cur;
    }
}

// -------------------------------------------------------------
// INCIDENT SERVICE CORRECTNESS TESTS (10 - 18)
// -------------------------------------------------------------

// 10. Rejecting duplicate incident IDs
void test_reject_duplicate_ids() {
    RoadGraph graph = createMetroGrid16();
    IncidentService service(&graph);

    CreateIncidentRequest req1{.id = "INC-DUPE", .locationId = "J1", .type = "FIRE", .severity = 3};
    auto res1 = service.createIncident(req1);
    TEST_ASSERT(res1.success, "First creation must succeed");

    CreateIncidentRequest req2{.id = "INC-DUPE", .locationId = "J2", .type = "MEDICAL", .severity = 4};
    auto res2 = service.createIncident(req2);
    TEST_ASSERT(!res2.success, "Duplicate ID creation must fail");
    TEST_ASSERT(res2.errorCode == "DUPLICATE_INCIDENT_ID", "Error code must be DUPLICATE_INCIDENT_ID");
}

// 11. Rejecting unknown location IDs
void test_reject_unknown_location_id() {
    RoadGraph graph = createMetroGrid16();
    IncidentService service(&graph);

    CreateIncidentRequest req{.id = "INC-1", .locationId = "UNKNOWN_NODE_999", .type = "POLICE", .severity = 2};
    auto res = service.createIncident(req);
    TEST_ASSERT(!res.success, "Unknown location ID must be rejected");
    TEST_ASSERT(res.errorCode == "LOCATION_NOT_FOUND", "Error code must be LOCATION_NOT_FOUND");
}

// 12. Returning the correct next pending incident
void test_return_correct_next_pending() {
    RoadGraph graph = createMetroGrid16();
    IncidentService service(&graph);

    service.createIncident({.id = "INC-LOW", .locationId = "J1", .type = "MEDICAL", .severity = 1});
    service.createIncident({.id = "INC-CRIT", .locationId = "J2", .type = "FIRE", .severity = 4});
    service.createIncident({.id = "INC-MED", .locationId = "J3", .type = "POLICE", .severity = 2});

    auto next = service.peekNext();
    TEST_ASSERT(next.has_value(), "PeekNext must return next incident");
    TEST_ASSERT(next->id == "INC-CRIT", "Highest priority incident INC-CRIT must be next");
}

// 13. Preventing cancelled incidents from being selected
void test_cancelled_incident_not_selected() {
    RoadGraph graph = createMetroGrid16();
    IncidentService service(&graph);

    service.createIncident({.id = "INC-CRIT", .locationId = "J1", .type = "FIRE", .severity = 4});
    service.createIncident({.id = "INC-HIGH", .locationId = "J2", .type = "MEDICAL", .severity = 3});

    // Cancel INC-CRIT
    auto cancelRes = service.cancelIncident("INC-CRIT");
    TEST_ASSERT(cancelRes.success, "Cancellation must succeed");

    // Next selected should skip cancelled INC-CRIT and return INC-HIGH
    auto selectRes = service.selectNext();
    TEST_ASSERT(selectRes.success, "Select next must succeed");
    TEST_ASSERT(selectRes.incident->id == "INC-HIGH", "Selected incident must be INC-HIGH, skipping cancelled INC-CRIT");
}

// 14. Preventing the same incident from being selected twice
void test_cannot_select_same_incident_twice() {
    RoadGraph graph = createMetroGrid16();
    IncidentService service(&graph);

    service.createIncident({.id = "INC-1", .locationId = "J1", .type = "FIRE", .severity = 4});

    auto sel1 = service.selectNext();
    TEST_ASSERT(sel1.success, "First selection must succeed");
    TEST_ASSERT(sel1.incident->id == "INC-1", "Selected INC-1");

    // Second selection attempt must see queue is empty (INC-1 is now ASSIGNED)
    auto sel2 = service.selectNext();
    TEST_ASSERT(!sel2.success, "Second selection must fail because queue is exhausted");
    TEST_ASSERT(sel2.errorCode == "EMPTY_QUEUE", "Error code must be EMPTY_QUEUE");
}

// 15. Rejecting invalid lifecycle transitions
void test_invalid_lifecycle_transitions() {
    RoadGraph graph = createMetroGrid16();
    IncidentService service(&graph);

    service.createIncident({.id = "INC-1", .locationId = "J1", .type = "FIRE", .severity = 4});

    // Cancel once
    auto c1 = service.cancelIncident("INC-1");
    TEST_ASSERT(c1.success, "First cancel succeeds");

    // Cancel twice -> must fail
    auto c2 = service.cancelIncident("INC-1");
    TEST_ASSERT(!c2.success, "Second cancel must fail");
    TEST_ASSERT(c2.errorCode == "ALREADY_CANCELLED", "Error code must be ALREADY_CANCELLED");

    // Resolve a cancelled incident -> must fail
    auto r1 = service.resolveIncident("INC-1");
    TEST_ASSERT(!r1.success, "Cannot resolve cancelled incident");

    // Select on empty queue
    auto s1 = service.selectNext();
    TEST_ASSERT(!s1.success, "Select on empty queue must fail");
}

// 16. Correct handling of an empty pending queue
void test_empty_pending_queue_handling() {
    RoadGraph graph = createMetroGrid16();
    IncidentService service(&graph);

    TEST_ASSERT(service.pendingCount() == 0, "Pending count must be 0");
    auto peek = service.peekNext();
    TEST_ASSERT(!peek.has_value(), "PeekNext on empty queue must return nullopt");

    auto select = service.selectNext();
    TEST_ASSERT(!select.success, "SelectNext on empty queue must fail cleanly");
    TEST_ASSERT(select.errorCode == "EMPTY_QUEUE", "Error code must be EMPTY_QUEUE");
}

// 17. Correct behavior when cancelled entries are buried inside the heap
void test_buried_cancelled_entries_lazy_invalidation() {
    RoadGraph graph = createMetroGrid16();
    IncidentService service(&graph);

    // Insert 5 incidents
    service.createIncident({.id = "INC-1", .locationId = "J1", .type = "MEDICAL", .severity = 4}); // Highest
    service.createIncident({.id = "INC-2", .locationId = "J2", .type = "FIRE", .severity = 3});
    service.createIncident({.id = "INC-3", .locationId = "J3", .type = "POLICE", .severity = 3});
    service.createIncident({.id = "INC-4", .locationId = "J4", .type = "MEDICAL", .severity = 2});
    service.createIncident({.id = "INC-5", .locationId = "J5", .type = "FIRE", .severity = 1}); // Lowest

    // Cancel INC-2 and INC-3 which are buried in the middle
    service.cancelIncident("INC-2");
    service.cancelIncident("INC-3");

    // Extract sequence must be: INC-1, then INC-4, then INC-5
    auto s1 = service.selectNext();
    TEST_ASSERT(s1.success && s1.incident->id == "INC-1", "INC-1 selected first");

    auto s2 = service.selectNext();
    TEST_ASSERT(s2.success && s2.incident->id == "INC-4", "INC-4 selected second (INC-2 and INC-3 were skipped)");

    auto s3 = service.selectNext();
    TEST_ASSERT(s3.success && s3.incident->id == "INC-5", "INC-5 selected third");

    auto s4 = service.selectNext();
    TEST_ASSERT(!s4.success, "Queue should now be empty");
}

// 18. Confirming that resolving or cancelling prevents later dispatch selection
void test_resolving_prevents_dispatch() {
    RoadGraph graph = createMetroGrid16();
    IncidentService service(&graph);

    service.createIncident({.id = "INC-1", .locationId = "J1", .type = "FIRE", .severity = 4});
    service.resolveIncident("INC-1");

    auto next = service.peekNext();
    TEST_ASSERT(!next.has_value(), "Resolved incident must not appear in peekNext");

    auto sel = service.selectNext();
    TEST_ASSERT(!sel.success, "Resolved incident cannot be selected");
}

// -------------------------------------------------------------
// TRACE CORRECTNESS TESTS (19 - 23)
// -------------------------------------------------------------

// 19. Insertion produces real trace events when tracing is enabled
void test_insertion_trace_events() {
    BinaryHeap heap;
    TraceCollector trace(true);

    heap.insert(makeInc("INC-1", IncidentSeverity::HIGH, 1), &trace);
    TEST_ASSERT(trace.size() >= 1, "Trace events must be recorded");

    const auto& events = trace.getEvents();
    TEST_ASSERT(events[0].algorithm == "BinaryHeap", "Algorithm must be BinaryHeap");
    TEST_ASSERT(events[0].operation == "insert", "First operation must be insert");
    TEST_ASSERT(events[0].entityId == "INC-1", "EntityId must be INC-1");
}

// 20. Swaps and comparisons reflect actual heap operations
void test_swaps_and_comparisons_trace() {
    BinaryHeap heap;
    TraceCollector trace(true);

    // Insert lower priority, then higher priority -> causes swim up
    heap.insert(makeInc("INC-LOW", IncidentSeverity::LOW, 1), &trace);
    trace.clear();

    heap.insert(makeInc("INC-CRIT", IncidentSeverity::CRITICAL, 2), &trace);

    const auto& events = trace.getEvents();
    bool foundCompare = false;
    bool foundSwap = false;
    for (const auto& ev : events) {
        if (ev.operation == "compare") foundCompare = true;
        if (ev.operation == "swap") foundSwap = true;
    }

    TEST_ASSERT(foundCompare, "Must have recorded compare event during heapifyUp");
    TEST_ASSERT(foundSwap, "Must have recorded swap event during heapifyUp");
}

// 21. Extraction produces the correct operation sequence
void test_extraction_trace_sequence() {
    BinaryHeap heap;
    heap.insert(makeInc("INC-1", IncidentSeverity::HIGH, 1));
    heap.insert(makeInc("INC-2", IncidentSeverity::CRITICAL, 2));

    TraceCollector trace(true);
    heap.extractMax(&trace);

    const auto& events = trace.getEvents();
    TEST_ASSERT(events.size() >= 2, "Extraction should emit swap and extract_max events");

    bool foundExtract = false;
    for (const auto& ev : events) {
        if (ev.operation == "extract_max") foundExtract = true;
    }
    TEST_ASSERT(foundExtract, "Trace must include extract_max operation");
}

// 22. Enabling or disabling tracing produces identical logical results
void test_tracing_equivalence() {
    BinaryHeap heapTrace;
    BinaryHeap heapNoTrace;

    TraceCollector trace(true);

    std::vector<IncidentSeverity> sevs = {
        IncidentSeverity::LOW, IncidentSeverity::CRITICAL,
        IncidentSeverity::HIGH, IncidentSeverity::MEDIUM
    };

    uint64_t seq = 0;
    for (auto sev : sevs) {
        uint64_t currentSeq = ++seq;
        heapTrace.insert(makeInc("INC-" + std::to_string(currentSeq), sev, currentSeq), &trace);
        heapNoTrace.insert(makeInc("INC-" + std::to_string(currentSeq), sev, currentSeq), nullptr);
    }

    TEST_ASSERT(heapTrace.size() == heapNoTrace.size(), "Sizes must match");

    while (!heapTrace.isEmpty()) {
        auto t1 = heapTrace.extractMax(&trace);
        auto t2 = heapNoTrace.extractMax(nullptr);
        TEST_ASSERT(t1->id == t2->id, "Extracted IDs must be completely identical");
        TEST_ASSERT(t1->severity == t2->severity, "Extracted severities must be completely identical");
    }
}

// 23. Stale-entry removal is represented correctly
void test_stale_entry_removal_trace() {
    RoadGraph graph = createMetroGrid16();
    IncidentService service(&graph);

    service.createIncident({.id = "INC-1", .locationId = "J1", .type = "FIRE", .severity = 4});
    service.createIncident({.id = "INC-2", .locationId = "J2", .type = "MEDICAL", .severity = 3});
    service.cancelIncident("INC-1");

    TraceCollector trace(true);
    auto res = service.selectNext(&trace);

    TEST_ASSERT(res.success && res.incident->id == "INC-2", "INC-2 selected");

    const auto& events = trace.getEvents();
    bool foundSkip = false;
    for (const auto& ev : events) {
        if (ev.operation == "skip_stale_entry" && ev.entityId == "INC-1") {
            foundSkip = true;
            TEST_ASSERT(ev.metadata.contains("staleStatus"), "Metadata must report stale status");
        }
    }
    TEST_ASSERT(foundSkip, "Trace must include skip_stale_entry event for INC-1");
}

// -------------------------------------------------------------
// SERVICE & API INTEGRATION TESTS (24 - 28)
// -------------------------------------------------------------

// 24. Valid incident creation
void test_valid_incident_creation() {
    RoadGraph graph = createMetroGrid16();
    IncidentService service(&graph);

    CreateIncidentRequest req{
        .id = "INC-201",
        .locationId = "J5",
        .type = "FIRE",
        .severity = 3,
        .description = "Small chemical spill"
    };

    auto res = service.createIncident(req);
    TEST_ASSERT(res.success, "Incident creation must succeed");
    TEST_ASSERT(res.incident.has_value(), "Incident must be present");
    TEST_ASSERT(res.incident->id == "INC-201", "ID must match");
    TEST_ASSERT(res.incident->severity == IncidentSeverity::HIGH, "Severity must match");
    TEST_ASSERT(res.incident->status == IncidentStatus::PENDING, "Status must be PENDING");
    TEST_ASSERT(service.pendingCount() == 1, "Pending count must be 1");
}

// 25. Invalid request handling
void test_invalid_request_handling() {
    RoadGraph graph = createMetroGrid16();
    IncidentService service(&graph);

    // Invalid severity
    auto res1 = service.createIncident({.locationId = "J1", .type = "FIRE", .severity = 99});
    TEST_ASSERT(!res1.success, "Invalid severity must fail");
    TEST_ASSERT(res1.errorCode == "INVALID_SEVERITY", "Error code must be INVALID_SEVERITY");

    // Invalid type
    auto res2 = service.createIncident({.locationId = "J1", .type = "ALIEN_INVASION", .severity = 2});
    TEST_ASSERT(!res2.success, "Invalid type must fail");
    TEST_ASSERT(res2.errorCode == "INVALID_TYPE", "Error code must be INVALID_TYPE");
}

// 26. Retrieving an incident by ID
void test_retrieve_incident_by_id() {
    RoadGraph graph = createMetroGrid16();
    IncidentService service(&graph);

    service.createIncident({.id = "INC-301", .locationId = "J4", .type = "POLICE", .severity = 2, .description = "Traffic accident"});

    auto incOpt = service.getIncident("INC-301");
    TEST_ASSERT(incOpt.has_value(), "Must find incident");
    TEST_ASSERT(incOpt->description == "Traffic accident", "Description must match");

    auto missing = service.getIncident("INC-NONEXISTENT");
    TEST_ASSERT(!missing.has_value(), "Non-existent ID must return nullopt");
}

// 27. Selecting the next eligible incident
void test_selecting_next_eligible_incident() {
    RoadGraph graph = createMetroGrid16();
    IncidentService service(&graph);

    service.createIncident({.id = "INC-A", .locationId = "J1", .type = "FIRE", .severity = 3});
    service.createIncident({.id = "INC-B", .locationId = "J2", .type = "MEDICAL", .severity = 4});

    auto sel = service.selectNext();
    TEST_ASSERT(sel.success, "Selection must succeed");
    TEST_ASSERT(sel.incident->id == "INC-B", "INC-B must be selected first");
    TEST_ASSERT(sel.incident->status == IncidentStatus::ASSIGNED, "Status must be ASSIGNED");
    TEST_ASSERT(service.assignedCount() == 1, "Assigned count must be 1");
    TEST_ASSERT(service.pendingCount() == 1, "Pending count must be 1 remaining");
}

// 28. Cancellation behavior through service / API
void test_cancellation_behavior() {
    RoadGraph graph = createMetroGrid16();
    IncidentService service(&graph);

    service.createIncident({.id = "INC-CANC", .locationId = "J3", .type = "FIRE", .severity = 4});
    TEST_ASSERT(service.pendingCount() == 1, "1 pending incident");

    auto canRes = service.cancelIncident("INC-CANC");
    TEST_ASSERT(canRes.success, "Cancel must succeed");
    TEST_ASSERT(canRes.incident->status == IncidentStatus::CANCELLED, "Status must be CANCELLED");
    TEST_ASSERT(service.cancelledCount() == 1, "1 cancelled incident");
    TEST_ASSERT(service.pendingCount() == 0, "0 pending incidents");
}

// -------------------------------------------------------------
// MAIN RUNNER
// -------------------------------------------------------------

int main() {
    std::cout << "======================================================\n";
    std::cout << " EmergencyGrid: C++ Binary Max-Heap & Incident Tests \n";
    std::cout << " Advanced Data Structures & Algorithms Capstone Project\n";
    std::cout << "======================================================\n\n";

    std::cout << "--- Section A: Heap Correctness Tests ---\n";
    RUN_TEST(test_insert_one_incident);
    RUN_TEST(test_insert_multiple_different_severities);
    RUN_TEST(test_extract_highest_priority);
    RUN_TEST(test_preserve_heap_order_repeated_ops);
    RUN_TEST(test_empty_heap_behavior);
    RUN_TEST(test_peek_does_not_mutate);
    RUN_TEST(test_equal_severity_follows_arrival_order);
    RUN_TEST(test_deterministic_tie_breaking);
    RUN_TEST(test_large_number_of_insertions);

    std::cout << "\n--- Section B: Incident Service Correctness Tests ---\n";
    RUN_TEST(test_reject_duplicate_ids);
    RUN_TEST(test_reject_unknown_location_id);
    RUN_TEST(test_return_correct_next_pending);
    RUN_TEST(test_cancelled_incident_not_selected);
    RUN_TEST(test_cannot_select_same_incident_twice);
    RUN_TEST(test_invalid_lifecycle_transitions);
    RUN_TEST(test_empty_pending_queue_handling);
    RUN_TEST(test_buried_cancelled_entries_lazy_invalidation);
    RUN_TEST(test_resolving_prevents_dispatch);

    std::cout << "\n--- Section C: Heap Tracing Tests ---\n";
    RUN_TEST(test_insertion_trace_events);
    RUN_TEST(test_swaps_and_comparisons_trace);
    RUN_TEST(test_extraction_trace_sequence);
    RUN_TEST(test_tracing_equivalence);
    RUN_TEST(test_stale_entry_removal_trace);

    std::cout << "\n--- Section D: API & Integration Tests ---\n";
    RUN_TEST(test_valid_incident_creation);
    RUN_TEST(test_invalid_request_handling);
    RUN_TEST(test_retrieve_incident_by_id);
    RUN_TEST(test_selecting_next_eligible_incident);
    RUN_TEST(test_cancellation_behavior);

    std::cout << "\n======================================================\n";
    std::cout << " Test Summary: " << g_testsPassed << " passed, " << g_testsFailed << " failed.\n";
    std::cout << "======================================================\n";

    return (g_testsFailed == 0) ? 0 : 1;
}
